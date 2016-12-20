/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <sg.h>

struct rollback_item
{
    SG_uint32 offset;
    SG_uint16 len;
    SG_byte* pdata;
    struct rollback_item* next;
};

struct _sg_hdb
{
    SG_file* pFile;
    SG_mmap* pmap;
    SG_byte* pMem;
    SG_uint32 mem_size;
    SG_uint32 cur_end;
    SG_uint32 starting_size;
    SG_bool guess;
    SG_strpool* pool;
    SG_vector* pvec_rollback;
};

/*

    Header (8 bytes)

        1 byte  -- version of the file format
        1 byte  -- key length in bytes
        1 byte  -- num bits to hash on
        1 byte  -- reserved
        2 bytes -- data size for each key
        2 bytes -- reserved

    Buckets

        Number of buckets is 1 << numbits

        Each bucket is

        4 bytes -- offset to the first item in this bucket

    Each bucket item is

        N bytes -- the key
        4 bytes -- offset to the next item in this bucket
        M bytes -- the data

    All offsets are relative to the beginning of the file.

*/

#define SG_HDB__C_HEADER_BYTES      (8)
#define SG_HDB__C_SIZEOF_OFFSET     (4)
#define SG_HDB__C_SIZEOF_BUCKET     (SG_HDB__C_SIZEOF_OFFSET)

#define SG_HDB__GETBYTE(i,s) ((SG_byte) ( ((i) >> s) & 0xff))

#define SG_HDB__PACK_16(p,i) SG_STATEMENT((p)[0]=(SG_HDB__GETBYTE(i,8));(p)[1]=(SG_HDB__GETBYTE(i,0));)

#define SG_HDB__PACK_32(ptr,i32) SG_STATEMENT(SG_byte*q=(SG_byte*)ptr;SG_uint32 i=(SG_uint32)i32;(q)[0]=(SG_HDB__GETBYTE(i,24));(q)[1]=(SG_HDB__GETBYTE(i,16));(q)[2]=(SG_HDB__GETBYTE(i,8));(q)[3]=(SG_HDB__GETBYTE(i,0));)

#define SG_HDB__UNPACK_16(p) ( (SG_uint16) ( ((p)[0] << 8) | ((p)[1]) ) )

#define SG_HDB__LEFT32(p,n,s) (((SG_uint32) ((p)[n])) << (s)) 

#define SG_HDB__UNPACK_32(i32,ptr) SG_STATEMENT(SG_byte*q=(SG_byte*)ptr;i32=\
                                 SG_HDB__LEFT32(q,0,24)  \
                               | SG_HDB__LEFT32(q,1,16)  \
                               | SG_HDB__LEFT32(q,2,8)  \
                               | SG_HDB__LEFT32(q,3,0);  \
                             )

#define SG_HDB__P_NUM_BUCKETS(num_bits)    (1 << num_bits)
#define SG_HDB__P_SIZEOF_TABLE(num_bits)   (SG_HDB__P_NUM_BUCKETS(num_bits) * SG_HDB__C_SIZEOF_BUCKET)

#define SG_HDB__H_VERSION(pMem)      ((pMem)[0])
#define SG_HDB__H_KEY_LENGTH(pMem)   ((pMem)[1])
#define SG_HDB__H_NUM_BITS(pMem)     ((pMem)[2])
#define SG_HDB__H_DATA_LENGTH(pMem)  (SG_HDB__UNPACK_16((pMem) + 4))
#define SG_HDB__H_SIZEOF_ITEM(pMem)  ((SG_HDB__H_KEY_LENGTH(pMem)) + (SG_HDB__H_DATA_LENGTH(pMem)) + SG_HDB__C_SIZEOF_OFFSET)

#define SG_HDB__M_VERSION(pdb)      (SG_HDB__H_VERSION(pdb->pMem))
#define SG_HDB__M_KEY_LENGTH(pdb)   (SG_HDB__H_KEY_LENGTH(pdb->pMem))
#define SG_HDB__M_NUM_BITS(pdb)     (SG_HDB__H_NUM_BITS(pdb->pMem))
#define SG_HDB__M_DATA_LENGTH(pdb)  (SG_HDB__H_DATA_LENGTH(pdb->pMem))

#define SG_HDB__M_NUM_BUCKETS(pdb)  ((SG_uint32) SG_HDB__P_NUM_BUCKETS(SG_HDB__M_NUM_BITS(pdb)))

#define SG_HDB__M_SIZEOF_TABLE(pdb) (SG_HDB__M_NUM_BUCKETS(pdb) * SG_HDB__C_SIZEOF_BUCKET)

#define SG_HDB__M_SIZEOF_ITEM(pdb)  (SG_HDB__H_SIZEOF_ITEM(pdb->pMem))

#define SG_HDB__P_BUCKET_OFFSET(i) (SG_HDB__C_HEADER_BYTES + ((i) * SG_HDB__C_SIZEOF_BUCKET))
#define SG_HDB__GET_DATA_OFFSET_FOR_ITEM(pdb,item_offset) ((item_offset) + SG_HDB__M_KEY_LENGTH(pdb) + SG_HDB__C_SIZEOF_OFFSET)
#define SG_HDB__GET_NEXT_OFFSET_FOR_ITEM(pdb,item_offset) ((item_offset) + SG_HDB__M_KEY_LENGTH(pdb) )
#define SG_HDB__GET_NEXT_FOR_ITEM(i32,pdb,item_offset) SG_HDB__UNPACK_32(i32, pdb->pMem + SG_HDB__GET_NEXT_OFFSET_FOR_ITEM(pdb,item_offset))
#define SG_HDB__SET_NEXT_FOR_ITEM(pdb,item_offset,next) SG_HDB__PACK_32(pdb->pMem + SG_HDB__GET_NEXT_OFFSET_FOR_ITEM(pdb,item_offset),next)


void SG_hdb__open(
    SG_context* pCtx, 
    SG_pathname* pPath,
    SG_uint32 guess,
    SG_bool allow_rollback,
    SG_hdb** ppResult
    )
{
	SG_hdb * pdb = NULL;
    SG_file_flags acc = 0;
    SG_byte hdr[SG_HDB__C_HEADER_BYTES];
    SG_uint64 i64 = 0;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pdb)  );
    pdb->guess = guess;

    if (guess)
    {
        acc = SG_FILE_RDWR;
    }
    else
    {
        acc = SG_FILE_RDONLY;
    }

    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_LOCK | acc | SG_FILE_OPEN_EXISTING, 0644, &pdb->pFile)  );
    SG_ERR_CHECK(  SG_file__read(pCtx, pdb->pFile, SG_HDB__C_HEADER_BYTES, hdr, NULL)  );

    SG_ERR_CHECK(  SG_file__seek_end(pCtx, pdb->pFile, &i64)  );
    pdb->starting_size = (SG_uint32) i64;
    if (guess)
    {
        SG_uint32 new_fsize = 0;

        new_fsize = pdb->starting_size + (guess * (SG_HDB__H_SIZEOF_ITEM(hdr)) );
        SG_ERR_CHECK(  SG_file__seek(pCtx, pdb->pFile, new_fsize-1)  );
        SG_ERR_CHECK(  SG_file__write(pCtx, pdb->pFile, 1, (SG_byte*) "e", NULL)  );
        pdb->mem_size = new_fsize;

        if (allow_rollback)
        {
            SG_ERR_CHECK(  SG_strpool__alloc(pCtx, &pdb->pool, guess * (SG_HDB__H_SIZEOF_ITEM(hdr)))  );
            SG_ERR_CHECK(  SG_vector__alloc(pCtx, &pdb->pvec_rollback, guess * 2)  );
        }
    }
    else
    {
        pdb->mem_size = pdb->starting_size;
    }
    pdb->cur_end = pdb->starting_size;

    SG_ERR_CHECK(  SG_file__mmap(pCtx, pdb->pFile, 0, pdb->mem_size, acc, &pdb->pmap)  );
    SG_ERR_CHECK(  SG_mmap__get_ptr(pCtx, pdb->pmap, &pdb->pMem)  );

    *ppResult = pdb;
    pdb = NULL;

fail:
    SG_ERR_IGNORE(  SG_hdb__close_free(pCtx, pdb)  );
}

void sg_hdb__grow(
    SG_context* pCtx, 
    SG_hdb* pdb
    )
{
    SG_uint32 new_fsize = 0;
    SG_file_flags acc = 0;
    SG_uint32 item_size = SG_HDB__M_SIZEOF_ITEM(pdb);

    if (pdb->guess)
    {
        acc = SG_FILE_RDWR;
    }
    else
    {
        acc = SG_FILE_RDONLY;
    }

    SG_ERR_IGNORE(  SG_file__munmap(pCtx, &pdb->pmap)  );
    pdb->pMem = NULL;

    new_fsize = pdb->mem_size + (pdb->guess * item_size );
    SG_ERR_CHECK(  SG_file__seek(pCtx, pdb->pFile, new_fsize-1)  );
    SG_ERR_CHECK(  SG_file__write(pCtx, pdb->pFile, 1, (SG_byte*) "e", NULL)  );
    pdb->mem_size = new_fsize;

    SG_ERR_CHECK(  SG_file__mmap(pCtx, pdb->pFile, 0, pdb->mem_size, acc, &pdb->pmap)  );
    SG_ERR_CHECK(  SG_mmap__get_ptr(pCtx, pdb->pmap, &pdb->pMem)  );

fail:
    ;
}

static void sg_hdb__add_rollback(
    SG_context* pCtx, 
    SG_hdb* pdb,
    SG_uint32 offset,
    SG_uint16 len
    )
{
    SG_byte* pblock = NULL;

    if (!pdb->pvec_rollback)
    {
        return;
    }

	SG_ERR_CHECK(  SG_strpool__add__len(
                pCtx, 
                pdb->pool, 
                SG_HDB__C_SIZEOF_OFFSET + sizeof(SG_uint16) + len,
                (const char**) &pblock)  );

    SG_HDB__PACK_32(pblock, offset);
    SG_HDB__PACK_16(pblock + SG_HDB__C_SIZEOF_OFFSET, len);
    memcpy(pblock + SG_HDB__C_SIZEOF_OFFSET + sizeof(SG_uint16), pdb->pMem + offset, len);

    SG_ERR_CHECK(  SG_vector__append(pCtx, pdb->pvec_rollback, pblock, NULL)  );

#if defined( DEBUG )
    {
        SG_uint32 chk = 0;
        SG_HDB__UNPACK_32(chk, pblock);
        SG_ASSERT(chk == offset);
    }
#endif

fail:
    ;
}

static void sg_hdb__do_rollback(
    SG_context* pCtx, 
    SG_hdb* pdb
    )
{

    SG_uint32 count = 0;
    SG_uint32 i = 0;

    if (!pdb->pvec_rollback)
    {
        return;
    }

    SG_ERR_CHECK(  SG_vector__length(pCtx, pdb->pvec_rollback, &count)  );
    for (i=0; i<count; i++)
    {
        SG_uint32 ndx = count - 1 - i;
        SG_byte* p = NULL;
        SG_uint32 offset = 0;
        SG_uint16 len = 0;

        SG_ERR_CHECK(  SG_vector__get(pCtx, pdb->pvec_rollback, ndx, (void**) &p)  );
        SG_HDB__UNPACK_32(offset, p);
        len = SG_HDB__UNPACK_16(p + SG_HDB__C_SIZEOF_OFFSET);
        memcpy(pdb->pMem + offset, p + SG_HDB__C_SIZEOF_OFFSET + sizeof(SG_uint16), len);
    }

fail:
    ;
}

void SG_hdb__rollback_close_free(
    SG_context* pCtx, 
    SG_hdb* pdb
    )
{
    if (!pdb)
    {
        return;
    }

    if (!pdb->pvec_rollback)
    {
        SG_ERR_THROW_RETURN(  SG_ERR_HDB_CANNOT_ROLLBACK  );
    }

    SG_ERR_IGNORE(  sg_hdb__do_rollback(pCtx, pdb)  );

    SG_VECTOR_NULLFREE(pCtx, pdb->pvec_rollback);

    if (pdb->pmap)
    {
        SG_ERR_IGNORE(  SG_file__munmap(pCtx, &pdb->pmap)  );
        pdb->pMem = NULL;
    }

    if (pdb->pFile)
    {
        // truncate the file
        SG_ERR_IGNORE(  SG_file__seek(pCtx, pdb->pFile, pdb->starting_size)  );
        SG_ERR_IGNORE(  SG_file__truncate(pCtx, pdb->pFile)  );

        SG_ERR_IGNORE(  SG_file__close(pCtx, &pdb->pFile)  );
    }

    SG_STRPOOL_NULLFREE(pCtx, pdb->pool);

    SG_NULLFREE(pCtx, pdb);
}

void SG_hdb__close_free(
    SG_context* pCtx, 
    SG_hdb* pdb
    )
{
    if (!pdb)
    {
        return;
    }

    SG_VECTOR_NULLFREE(pCtx, pdb->pvec_rollback);

    if (pdb->pMem)
    {
        SG_ERR_IGNORE(  SG_file__munmap(pCtx, &pdb->pmap)  );
        pdb->pMem = NULL;
    }

    if (pdb->pFile)
    {
        // truncate the file
        SG_ERR_IGNORE(  SG_file__seek(pCtx, pdb->pFile, pdb->cur_end)  );
        SG_ERR_IGNORE(  SG_file__truncate(pCtx, pdb->pFile)  );

        SG_ERR_IGNORE(  SG_file__close(pCtx, &pdb->pFile)  );
    }

    SG_STRPOOL_NULLFREE(pCtx, pdb->pool);

    SG_NULLFREE(pCtx, pdb);
}

static void sg_hdb__find(
    SG_context* pCtx, 
    SG_hdb* pdb,
    SG_byte* pkey,
    SG_uint32 starting_offset,
    SG_bool* pb_found,
    SG_uint32* pi_bucket,
    SG_uint32* pi_cur_offset,
    SG_uint32* pi_prev_offset
    )
{
    SG_bool b_found = SG_FALSE;
    SG_uint32 bucket = 0;
    SG_uint32 cur_offset = 0;
    SG_uint32 prev_offset = 0;
    SG_uint8 key_length = 0;
    SG_uint32 bits;

	SG_NULLARGCHECK_RETURN(pdb);
	SG_NULLARGCHECK_RETURN(pkey);
    
    SG_HDB__UNPACK_32(bits, pkey);
    bucket = bits & ((1 << SG_HDB__M_NUM_BITS(pdb)) - 1);
    key_length = SG_HDB__M_KEY_LENGTH(pdb);

    if (starting_offset)
    {
        cur_offset = starting_offset;
    }
    else
    {
        SG_HDB__UNPACK_32(cur_offset, pdb->pMem + SG_HDB__P_BUCKET_OFFSET(bucket));
    }
    while (cur_offset)
    {
        int cmp = memcmp(pkey, pdb->pMem + cur_offset, key_length);
        if (0 == cmp)
        {
            b_found = SG_TRUE;
            break;
        }
        else if (cmp < 0)
        {
            prev_offset = cur_offset;
            SG_HDB__UNPACK_32(cur_offset, pdb->pMem + SG_HDB__GET_NEXT_OFFSET_FOR_ITEM(pdb, cur_offset));
        }
        else
        {
            b_found = SG_FALSE;
            break;
        }
    }

    *pb_found = b_found;
    *pi_bucket = bucket;
    *pi_cur_offset = cur_offset;
    *pi_prev_offset = prev_offset;
}

void SG_hdb__put(
    SG_context* pCtx, 
    SG_hdb* pdb,
    SG_byte* pkey,
    SG_byte* pdata,
    SG_uint8 on_collision
    )
{
    SG_bool b_found = SG_FALSE;
    SG_uint32 bucket = 0;
    SG_uint32 cur_offset = 0;
    SG_uint32 prev_offset = 0;
    SG_uint16 data_length = 0;
    SG_uint32 key_length = 0;

    data_length = SG_HDB__M_DATA_LENGTH(pdb);
    key_length = SG_HDB__M_KEY_LENGTH(pdb);

    if ((pdb->cur_end + SG_HDB__M_SIZEOF_ITEM(pdb)) > pdb->mem_size)
    {
        SG_ERR_CHECK(  sg_hdb__grow(pCtx, pdb)  );
    }

    SG_ERR_CHECK(  sg_hdb__find(pCtx, pdb, pkey, 0, &b_found, &bucket, &cur_offset, &prev_offset)  );
    if (b_found)
    {
        if (SG_HDB__ON_COLLISION__OVERWRITE == on_collision)
        {
            SG_uint32 data_offset = SG_HDB__GET_DATA_OFFSET_FOR_ITEM(pdb, cur_offset);
            SG_ERR_CHECK(  sg_hdb__add_rollback(
                        pCtx, 
                        pdb, 
                        data_offset,
                        data_length
                        )  );
            memcpy(pdb->pMem + data_offset, pdata, data_length);
            goto done;
        }
        else if (SG_HDB__ON_COLLISION__ERROR == on_collision)
        {
            SG_ERR_THROW_RETURN(  SG_ERR_HDB_DUPLICATE_KEY  );
        }
        else if (SG_HDB__ON_COLLISION__IGNORE == on_collision)
        {
            goto done;
        }
        else if (SG_HDB__ON_COLLISION__MULTIPLE == on_collision)
        {
            goto insert_new;
        }
    }
    else
    {
        goto insert_new;
    }

    SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );

insert_new:
    { 
        SG_uint32 new_item_offset = pdb->cur_end;
        SG_uint32 new_data_offset = SG_HDB__GET_DATA_OFFSET_FOR_ITEM(pdb, new_item_offset);
        SG_uint32 new_next = 0;

        // The following two writes occur after pdb->starting_size,
        // so we don't need to save the in the rollback log.  The
        // truncate will get rid of them.
        memcpy(pdb->pMem + new_item_offset, pkey, key_length);
        memcpy(pdb->pMem + new_data_offset, pdata, data_length);

        if (prev_offset)
        {
            SG_HDB__UNPACK_32(new_next, pdb->pMem + SG_HDB__GET_NEXT_OFFSET_FOR_ITEM(pdb, prev_offset));

            SG_ERR_CHECK(  sg_hdb__add_rollback(
                        pCtx, 
                        pdb, 
                        SG_HDB__GET_NEXT_OFFSET_FOR_ITEM(pdb, prev_offset),
                        SG_HDB__C_SIZEOF_OFFSET
                        )  );
            SG_HDB__SET_NEXT_FOR_ITEM(pdb,prev_offset,new_item_offset);
        }
        else
        {
            SG_HDB__UNPACK_32(new_next, pdb->pMem + SG_HDB__P_BUCKET_OFFSET(bucket));
            SG_ERR_CHECK(  sg_hdb__add_rollback(
                        pCtx, 
                        pdb, 
                        SG_HDB__P_BUCKET_OFFSET(bucket),
                        SG_HDB__C_SIZEOF_OFFSET
                        )  );
            SG_HDB__PACK_32(pdb->pMem + SG_HDB__P_BUCKET_OFFSET(bucket), new_item_offset);
        }

        // The following write occurs after pdb->starting_size,
        // so we don't need to save the in the rollback log.  The
        // truncate will get rid of them.
        SG_ERR_CHECK(  sg_hdb__add_rollback(
                    pCtx, 
                    pdb, 
                    SG_HDB__GET_NEXT_OFFSET_FOR_ITEM(pdb, new_item_offset),
                    SG_HDB__C_SIZEOF_OFFSET
                    )  );
        SG_HDB__SET_NEXT_FOR_ITEM(pdb,new_item_offset,new_next);

        pdb->cur_end += SG_HDB__M_SIZEOF_ITEM(pdb);
    }

done:

fail:
    ;
}

void SG_hdb__find(
    SG_context* pCtx, 
    SG_hdb* pdb,
    SG_byte* pkey,
    SG_uint32 starting_offset,
    SG_byte** ppdata,
    SG_uint32* pi_next_offset
    )
{
    SG_bool b_found = SG_FALSE;
    SG_uint32 bucket = 0;
    SG_uint32 cur_offset = 0;
    SG_uint32 prev_offset = 0;

    SG_ERR_CHECK_RETURN(  sg_hdb__find(pCtx, pdb, pkey, starting_offset, &b_found, &bucket, &cur_offset, &prev_offset)  );

    if (b_found)
    {
        SG_uint32 data_offset = SG_HDB__GET_DATA_OFFSET_FOR_ITEM(pdb, cur_offset);
        *ppdata = pdb->pMem + data_offset;
        if (pi_next_offset)
        {
            SG_uint32 off;
            SG_HDB__UNPACK_32(off, pdb->pMem + SG_HDB__GET_NEXT_OFFSET_FOR_ITEM(pdb, cur_offset));
            *pi_next_offset = off;
        }
    }
    else
    {
        *ppdata = NULL;
    }
}

void SG_hdb__create(
    SG_context* pCtx, 
    SG_pathname* pPath,
    SG_uint8 key_length,
    SG_uint8 num_bits,
    SG_uint16 data_size
    )
{
    SG_file* pFile = NULL;
    SG_mmap* pmap = NULL;
    SG_byte* pMem = NULL;
    SG_uint32 len = 0;

    SG_NULLARGCHECK(pPath);
	SG_ARGCHECK_RETURN( (key_length>0), key_length);
	SG_ARGCHECK_RETURN( (data_size>0), data_size);
	SG_ARGCHECK_RETURN( (key_length*8>=num_bits), num_bits);

    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_LOCK | SG_FILE_RDWR | SG_FILE_CREATE_NEW, 0644, &pFile)  );
    len = SG_HDB__C_HEADER_BYTES + SG_HDB__P_SIZEOF_TABLE(num_bits);
	SG_ERR_CHECK(  SG_file__seek(pCtx, pFile, len-1)  );
    SG_ERR_CHECK(  SG_file__write(pCtx, pFile, 1, (SG_byte*) "e", NULL)  );

    SG_ERR_CHECK(  SG_file__mmap(pCtx, pFile, 0, len, 0, &pmap)  );
    SG_ERR_CHECK(  SG_mmap__get_ptr(pCtx, pmap, &pMem)  );

    memset(pMem, 0, SG_HDB__C_HEADER_BYTES);
    pMem[0] = 1;
    pMem[1] = key_length;
    pMem[2] = num_bits;
    SG_HDB__PACK_16(pMem+4, data_size);
    memset(pMem + SG_HDB__C_HEADER_BYTES, 0, SG_HDB__P_SIZEOF_TABLE(num_bits));

    SG_ERR_CHECK(  SG_file__munmap(pCtx, &pmap)  );
    pMem = NULL;

    SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

fail:
    if (pMem)
    {
        SG_ERR_CHECK(  SG_file__munmap(pCtx, &pmap)  );
        pMem = NULL;
    }
    if (pFile)
    {
        SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile)  );
    }
}

void SG_hdb__to_vhash(
    SG_context* pCtx, 
    SG_pathname* pPath,
    SG_uint8 flags,
    SG_vhash** ppvh
    )
{
    SG_hdb* pdb = NULL;
    SG_vhash* pvh = NULL;
    SG_vhash* pvh_hdr = NULL;
    SG_vhash* pvh_buckets = NULL;
    SG_vhash* pvh_pairs = NULL;
    SG_vhash* pvh_stats = NULL;
    SG_uint32 i;
    SG_uint32 count_buckets_0 = 0;
    SG_uint32 count_buckets_1 = 0;
    SG_uint32 count_buckets_more = 0;
    SG_uint32 count_items = 0;
    SG_uint64 file_length = 0;
    SG_string* pstr_key = NULL;
    SG_string* pstr_data = NULL;

    SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath, &file_length, NULL)  );

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );
    SG_ERR_CHECK(  SG_hdb__open(pCtx, pPath, 0, SG_FALSE, &pdb)  );

    if (flags & SG_HDB_TO_VHASH__HEADER)
    {
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, "header", &pvh_hdr)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_hdr, "version", (SG_int64) SG_HDB__M_VERSION(pdb))  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_hdr, "key_length", (SG_int64) SG_HDB__M_KEY_LENGTH(pdb))  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_hdr, "num_bits", (SG_int64) SG_HDB__M_NUM_BITS(pdb))  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_hdr, "data_length", (SG_int64) SG_HDB__M_DATA_LENGTH(pdb))  );
    }

    if (flags & SG_HDB_TO_VHASH__BUCKETS)
    {
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, "buckets", &pvh_buckets)  );
    }
    if (flags & SG_HDB_TO_VHASH__PAIRS)
    {
        // TODO this code needs to be fixed to handle cases where an hdb has
        // more than one value for a given key.
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, "pairs", &pvh_pairs)  );
    }
    for (i=0; i<SG_HDB__M_NUM_BUCKETS(pdb); i++)
    {
        SG_uint32 count_in_this_bucket = 0;
        SG_uint32 item_offset;
       
        SG_HDB__UNPACK_32(item_offset, pdb->pMem + SG_HDB__P_BUCKET_OFFSET(i));
        while (item_offset)
        {
            if (flags & SG_HDB_TO_VHASH__PAIRS)
            {
                SG_ERR_CHECK(  SG_string__alloc__base64(pCtx, &pstr_key, pdb->pMem + item_offset, SG_HDB__M_KEY_LENGTH(pdb))  );
                SG_ERR_CHECK(  SG_string__alloc__base64(pCtx, &pstr_data, pdb->pMem + SG_HDB__GET_DATA_OFFSET_FOR_ITEM(pdb, item_offset), SG_HDB__M_DATA_LENGTH(pdb))  );
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_pairs, SG_string__sz(pstr_key), SG_string__sz(pstr_data))  );
                SG_STRING_NULLFREE(pCtx, pstr_key);
                SG_STRING_NULLFREE(pCtx, pstr_data);
            }
            SG_HDB__GET_NEXT_FOR_ITEM(item_offset, pdb, item_offset);
            count_in_this_bucket++;
        }

        if (flags & SG_HDB_TO_VHASH__BUCKETS)
        {
            char buf[32];
            SG_hex__format_uint32(buf, i);
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_buckets, buf, count_in_this_bucket)  );
        }

        if (0 == count_in_this_bucket)
        {
            count_buckets_0++;
        }
        else if (1 == count_in_this_bucket)
        {
            count_buckets_1++;
        }
        else
        {
            count_buckets_more++;
        }

        count_items += count_in_this_bucket;
    }

    if (flags & SG_HDB_TO_VHASH__PAIRS)
    {
        SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh_pairs, SG_FALSE, SG_vhash_sort_callback__increasing)  );
    }

    if (flags & SG_HDB_TO_VHASH__STATS)
    {
        SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh, "stats", &pvh_stats)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_stats, "empty_buckets", count_buckets_0)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_stats, "full_buckets", count_buckets_1)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_stats, "overfull_buckets", count_buckets_more)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_stats, "num_buckets", (SG_int64) SG_HDB__M_NUM_BUCKETS(pdb))  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_stats, "percent_overfull", (count_buckets_more * 100 / SG_HDB__M_NUM_BUCKETS(pdb)))  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_stats, "total_items", count_items)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_stats, "file_length", (SG_int64) file_length)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_stats, "min_bytes_per_item", (SG_int64) ((SG_HDB__M_KEY_LENGTH(pdb)) + (SG_HDB__M_DATA_LENGTH(pdb))))  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_stats, "actual_bytes_per_item", (SG_int64) file_length / count_items)  );
    }

    SG_ASSERT(file_length == (count_items * SG_HDB__M_SIZEOF_ITEM(pdb) + SG_HDB__M_SIZEOF_TABLE(pdb) + SG_HDB__C_HEADER_BYTES));
    SG_ASSERT(SG_HDB__M_NUM_BUCKETS(pdb) == (count_buckets_0 + count_buckets_1 + count_buckets_more));

    SG_ERR_CHECK(  SG_hdb__close_free(pCtx, pdb)  );
    pdb = NULL;

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr_key);
    SG_STRING_NULLFREE(pCtx, pstr_data);
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_hdb__rehash(
    SG_context* pCtx, 
    SG_pathname* pPath_old,
    SG_uint8 num_bits_new,
    SG_pathname* pPath_new
    )
{
    SG_hdb* pdb_old = NULL;
    SG_hdb* pdb_new = NULL;
    SG_uint32 i;
    SG_uint64 file_length_old = 0;
    SG_uint32 count_items_old = 0;

    SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath_old, &file_length_old, NULL)  );

    SG_ERR_CHECK(  SG_hdb__open(pCtx, pPath_old, 0, SG_FALSE, &pdb_old)  );

    count_items_old = (SG_uint32) ((file_length_old - (SG_HDB__M_SIZEOF_TABLE(pdb_old) + SG_HDB__C_HEADER_BYTES)) / SG_HDB__M_SIZEOF_ITEM(pdb_old));

    SG_ERR_CHECK(  SG_hdb__create(pCtx, pPath_new, SG_HDB__M_KEY_LENGTH(pdb_old), num_bits_new, SG_HDB__M_DATA_LENGTH(pdb_old))  );

    SG_ERR_CHECK(  SG_hdb__open(pCtx, pPath_new, count_items_old, SG_FALSE, &pdb_new)  );

    for (i=0; i<SG_HDB__M_NUM_BUCKETS(pdb_old); i++)
    {
        SG_uint32 item_offset;
        SG_HDB__UNPACK_32(item_offset, pdb_old->pMem + SG_HDB__P_BUCKET_OFFSET(i));
        while (item_offset)
        {
            SG_ERR_CHECK(  SG_hdb__put(
                        pCtx, 
                        pdb_new,
                        pdb_old->pMem + item_offset,
                        pdb_old->pMem + SG_HDB__GET_DATA_OFFSET_FOR_ITEM(pdb_old, item_offset),
                        SG_HDB__ON_COLLISION__ERROR
                        )  );

            SG_HDB__GET_NEXT_FOR_ITEM(item_offset, pdb_old, item_offset);
        }
    }

fail:
    SG_HDB_NULLFREE(pCtx, pdb_old);
    SG_HDB_NULLFREE(pCtx, pdb_new);
}

// TODO don't need to save rollbacks after starting_size

