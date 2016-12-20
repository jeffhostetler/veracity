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

// 8 buckets is enough for audit, stamp, tag, comment or sup

#define sg_IHASH_NUM_BUILTIN (8)

typedef struct _sg_hashitem
{
	const char* key;            // points into the strpool
    SG_uint32 hash32;           // so we can just mask on rehash
    SG_int64 v;
	struct _sg_hashitem* pNext; // for chaining within a bucket
} sg_hashitem;

typedef struct _sg_hashbucket
{
    sg_hashitem* head;
} sg_hashbucket;

struct _SG_ihash
{
	SG_uint32 count;

	SG_strpool *pStrPool;

	SG_uint32 space;

	sg_hashitem* aItems;
	sg_hashbucket* aBuckets;
    SG_uint32 bucket_mask;

    sg_hashbucket builtin_buckets[sg_IHASH_NUM_BUILTIN];
    sg_hashitem builtin_items[sg_IHASH_NUM_BUILTIN];
};

void SG_ihash__alloc(
        SG_context* pCtx, 
        SG_ihash** ppResult
        )
{
	SG_ihash * pvh = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pvh)  );

    SG_ERR_CHECK(  SG_STRPOOL__ALLOC(pCtx, &pvh->pStrPool, 8 * 200)  );

    pvh->space = sg_IHASH_NUM_BUILTIN;
    pvh->aBuckets = pvh->builtin_buckets;
    pvh->aItems = pvh->builtin_items;

    pvh->bucket_mask = pvh->space - 1;

	*ppResult = pvh;
    pvh = NULL;

fail:
    SG_IHASH_NULLFREE(pCtx, pvh);
}

void SG_ihash__free(SG_context * pCtx, SG_ihash* pvh)
{
	if (!pvh)
		return;

    //printf("ihash_count\t%06d\n", pvh->count);

	if (pvh->aItems)
	{
        if (pvh->aItems != pvh->builtin_items)
        {
            SG_NULLFREE(pCtx, pvh->aItems);
        }
	}

    if (pvh->aBuckets != pvh->builtin_buckets)
    {
        SG_NULLFREE(pCtx, pvh->aBuckets);
    }

    SG_STRPOOL_NULLFREE(pCtx, pvh->pStrPool);

	SG_NULLFREE(pCtx, pvh);
}

void SG_ihash__count(SG_context* pCtx, const SG_ihash* pvh, SG_uint32* piResult)
{
	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(piResult);

	*piResult = pvh->count;
}

/*
-------------------------------------------------------------------------------
lookup3.c, by Bob Jenkins, May 2006, Public Domain.
*/

#define hashsize(n) ((SG_uint32)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

/*
-------------------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.

This is reversible, so any information in (a,b,c) before mix() is
still in (a,b,c) after mix().

If four pairs of (a,b,c) inputs are run through mix(), or through
mix() in reverse, there are at least 32 bits of the output that
are sometimes the same for one pair and different for another pair.
This was tested for:
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or
  all zero plus a counter that starts at zero.

Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
satisfy this are
    4  6  8 16 19  4
    9 15  3 18 27 15
   14  9  3  7 17  3
Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
for "differ" defined as + with a one-bit base and a two-bit delta.  I
used http://burtleburtle.net/bob/hash/avalanche.html to choose
the operations, constants, and arrangements of the variables.

This does not achieve avalanche.  There are input bits of (a,b,c)
that fail to affect some output bits of (a,b,c), especially of a.  The
most thoroughly mixed value is c, but it doesn't really even achieve
avalanche in c.

This allows some parallelism.  Read-after-writes are good at doubling
the number of bits affected, so the goal of mixing pulls in the opposite
direction as the goal of parallelism.  I did what I could.  Rotates
seem to cost as much as shifts on every machine I could lay my hands
on, and rotates are much kinder to the top and bottom bits, so I used
rotates.
-------------------------------------------------------------------------------
*/
#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

/*
-------------------------------------------------------------------------------
final -- final mixing of 3 32-bit values (a,b,c) into c

Pairs of (a,b,c) values differing in only a few bits will usually
produce values of c that look totally different.  This was tested for
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or
  all zero plus a counter that starts at zero.

These constants passed:
 14 11 25 16 4 14 24
 12 14 25 16 4 14 24
and these came close:
  4  8 15 26 3 22 24
 10  8 15 26 3 22 24
 11  8 15 26 3 22 24
-------------------------------------------------------------------------------
*/
#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

/*
-------------------------------------------------------------------------------
hashlittle() -- hash a variable-length key into a 32-bit value
  k       : the key (the unaligned variable-length array of bytes)
  length  : the length of the key, counting by bytes
  initval : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Two keys differing by one or two bits will have
totally different hash values.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
  h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (SG_uint8 **)k, do it like this:
  for (i=0, h=0; i<n; ++i) h = hashlittle( k[i], len[i], h);

By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.  It's free.

Use for hash table lookup, or anything where one collision in 2^^32 is
acceptable.  Do NOT use for cryptographic purposes.
-------------------------------------------------------------------------------
*/

SG_uint32 sg_ihash__hashlittle( const SG_uint8 *k, size_t length)
{
	SG_uint32 a,b,c;                                          /* internal state */

#if 1
    /*
     * Basically, this code says:  If the length of
     * the string happens to match the length of one of our
     * hash algorithms output, assume the string is a hash
     * and just take the first 8 hex chars as our result.  If
     * we find anything that is not hex, then our assumption
     * must have wrong, so we bail out and just run the usual
     * algorithm.
     *
     * All our hash output is lower-case hex, so we don't even
     * check for A-F.
     *
     * This code will cause horrible performance if you insert
     * a whole bunch of keys which are all exactly 40/64/128
     * chars long and which all start with the same 8 characters
     * that all happen to be hex.
     */
    if (
            (40 == length)          // SHA1/160
            || (64 == length)       // SHA2/256 or Skein/256
            || (128 == length)      // 512
       )
    {
        SG_uint32 i = 0;
        SG_uint32 v = 0;
        const SG_uint8* p = k;

        for (i=0; i<8; i++)
        {
            SG_uint8 c = *p++;
            SG_uint8 n;
            if ((c >= '0') && (c <= '9'))
            {
                n = c - '0';
            }
            else if ((c >= 'a') && (c <= 'f'))
            {
                n = c - 'a' + 10;
            }
            else
            {
                goto not_hex;
            }

            v = (v << 4) | (SG_uint32) n;
        }
        return v;
    }
    else if (
            (65 == length)          // a GID, starts with a g
            && ('g' == k[0])
            )
    {
        SG_uint32 i = 0;
        SG_uint32 v = 0;
        const SG_uint8* p = k + 1;  // add 1 to skip the g

        for (i=0; i<8; i++)
        {
            SG_uint8 c = *p++;
            SG_uint8 n;
            if ((c >= '0') && (c <= '9'))
            {
                n = c - '0';
            }
            else if ((c >= 'a') && (c <= 'f'))
            {
                n = c - 'a' + 10;
            }
            else
            {
                goto not_hex;
            }

            v = (v << 4) | (SG_uint32) n;
        }
        return v;
    }

not_hex:
#endif

	/* Set up the internal state */
	a = b = c = 0xdeadbeef + ((SG_uint32)length);

	/*--------------- all but the last block: affect some 32 bits of (a,b,c) */
	while (length > 12)
	{
		a += k[0];
		a += ((SG_uint32)k[1])<<8;
		a += ((SG_uint32)k[2])<<16;
		a += ((SG_uint32)k[3])<<24;
		b += k[4];
		b += ((SG_uint32)k[5])<<8;
		b += ((SG_uint32)k[6])<<16;
		b += ((SG_uint32)k[7])<<24;
		c += k[8];
		c += ((SG_uint32)k[9])<<8;
		c += ((SG_uint32)k[10])<<16;
		c += ((SG_uint32)k[11])<<24;
		mix(a,b,c);
		length -= 12;
		k += 12;
	}

	/*-------------------------------- last block: affect all 32 bits of (c) */
	switch(length)                   /* all the case statements fall through */
	{
	case 12: c+=((SG_uint32)k[11])<<24;
	case 11: c+=((SG_uint32)k[10])<<16;
	case 10: c+=((SG_uint32)k[9])<<8;
	case 9 : c+=k[8];
	case 8 : b+=((SG_uint32)k[7])<<24;
	case 7 : b+=((SG_uint32)k[6])<<16;
	case 6 : b+=((SG_uint32)k[5])<<8;
	case 5 : b+=k[4];
	case 4 : a+=((SG_uint32)k[3])<<24;
	case 3 : a+=((SG_uint32)k[2])<<16;
	case 2 : a+=((SG_uint32)k[1])<<8;
	case 1 : a+=k[0];
		break;
	case 0 : return c;
	}

	final(a,b,c);
	return c;
}

// http://www.burtleburtle.net/bob/hash/doobs.html

static void sg_hashbucket__add__hashitem(
        SG_context* pCtx, 
        sg_hashbucket* pbuck, 
        sg_hashitem* pNew
        )
{
	if (!(pbuck->head))
	{
        pbuck->head = pNew;
	}
	else
	{
		sg_hashitem* pCur = pbuck->head;
		sg_hashitem* pPrev = NULL;

		while (pCur)
		{
			int cmp = strcmp(pNew->key, pCur->key);
			if (cmp == 0)
			{
				SG_ERR_THROW2_RETURN(  SG_ERR_VHASH_DUPLICATEKEY, (pCtx, "%s", pNew->key)  );
			}
			else if (cmp < 0)
			{
				// put pv before this one
				if (pPrev)
				{
					pPrev->pNext = pNew;
				}
				else
				{
					pbuck->head = pNew;
				}

				pNew->pNext = pCur;
				return;
			}
			else
			{
				pPrev = pCur;
				pCur = pCur->pNext;
			}
		}
		pPrev->pNext = pNew;
	}
}

void sg_ihash__rehash__same_buckets(
        SG_context* pCtx, 
        SG_ihash* pvh
        )
{
    SG_uint32 i = 0;

    SG_NULLARGCHECK_RETURN(pvh);

    for (i=0; i<pvh->space; i++)
    {
        pvh->aBuckets[i].head = NULL;
    }

    // rehash everything
    for (i=0; i<pvh->count; i++)
    {
        sg_hashitem* p = &pvh->aItems[i];
        SG_uint32 iBucket = p->hash32 & pvh->bucket_mask;
        p->pNext = NULL;
        SG_ERR_CHECK(  sg_hashbucket__add__hashitem(pCtx, &pvh->aBuckets[iBucket], p)  );
    }

fail:
    // TODO what should we do here?  do we need to leave the ihash
    // in a usable state?  can we?
    ;
}

void sg_ihash__rehash__new_buckets(
        SG_context* pCtx, 
        SG_ihash* pvh
        )
{
    SG_uint32 i = 0;

    SG_NULLARGCHECK_RETURN(pvh);

    if (pvh->aBuckets != pvh->builtin_buckets)
    {
        SG_NULLFREE(pCtx, pvh->aBuckets);
    }
    SG_ERR_CHECK(  SG_alloc(pCtx, pvh->space, sizeof(sg_hashbucket), &pvh->aBuckets)  );

    // rehash everything
    for (i=0; i<pvh->count; i++)
    {
        sg_hashitem* p = &pvh->aItems[i];
        SG_uint32 iBucket = p->hash32 & pvh->bucket_mask;
        p->pNext = NULL;
        SG_ERR_CHECK(  sg_hashbucket__add__hashitem(pCtx, &pvh->aBuckets[iBucket], p)  );
    }

fail:
    // TODO what should we do here?  do we need to leave the ihash
    // in a usable state?  can we?
    ;
}

void sg_ihash__grow(
        SG_context* pCtx, 
        SG_ihash* pvh
        )
{
    SG_uint32 new_space = 4 * pvh->space;
    sg_hashitem* new_aKeys = NULL;

    SG_ERR_CHECK_RETURN(  SG_alloc(pCtx, new_space, sizeof(sg_hashitem), &new_aKeys)  );

    memcpy((char **)new_aKeys, pvh->aItems, pvh->count * sizeof(sg_hashitem));
    if (pvh->aItems != pvh->builtin_items)
    {
        SG_NULLFREE(pCtx, pvh->aItems);
    }
    pvh->aItems = new_aKeys;
    pvh->space = new_space;
    pvh->bucket_mask = pvh->space - 1;

    SG_ERR_CHECK_RETURN(  sg_ihash__rehash__new_buckets(pCtx, pvh)  );
}

static void sg_ihash__add(
        SG_context* pCtx, 
        SG_ihash* pvh, 
        const char* psz_key, 
        SG_int64 v
        )
{
	SG_int32 iBucket = -1;
    sg_hashitem* phit = NULL;

    if ((pvh->count + 1) > (pvh->space * 3 / 4))
    {
        SG_ERR_CHECK(  sg_ihash__grow(pCtx, pvh)  );
    }

    phit = &(pvh->aItems[pvh->count]);
	SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pvh->pStrPool, psz_key, &phit->key)  );
    phit->hash32 = sg_ihash__hashlittle((SG_uint8*)phit->key, strlen(phit->key));
    phit->v = v;
    phit->pNext = NULL;

	iBucket = phit->hash32 & (pvh->bucket_mask);
    SG_ERR_CHECK(  sg_hashbucket__add__hashitem(pCtx, &pvh->aBuckets[iBucket], phit)  );
    pvh->count++;

fail:
    ;
}

void SG_ihash__copy_items(
    SG_context* pCtx,
    const SG_ihash* pih_from,
    SG_ihash* pih_to,
	SG_bool bOverwriteDuplicates
	)
{
    SG_uint32 i = 0;

    for (i=0; i<pih_from->count; i++)
    {
        const char* psz_key = NULL;
        SG_int64 nV = 0;

        SG_ERR_CHECK(  SG_ihash__get_nth_pair(pCtx, pih_from, i, &psz_key, &nV)  );
		if (bOverwriteDuplicates)
			SG_ERR_CHECK(  SG_ihash__update__int64(pCtx, pih_to, psz_key, nV)  );
		else
			SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pih_to, psz_key, nV)  );
    }

fail:
    ;
}

/* Note that if the key is not found, this routine returns SG_ERR_OK and the resulting hashitem is set to NULL */
void sg_ihash__find(
        SG_UNUSED_PARAM(SG_context* pCtx), 
        const SG_ihash* pvh, 
        const char* psz_key, 
        sg_hashitem** ppResult
        )
{
    if (0 == pvh->count)
    {
        *ppResult = NULL;
        return;
    }
    else
    {
        SG_uint32 iBucket = sg_ihash__hashlittle((SG_uint8*)psz_key, strlen(psz_key)) & (pvh->bucket_mask);
        sg_hashitem* pCur = pvh->aBuckets[iBucket].head;

        SG_UNUSED(pCtx);

        while (pCur)
        {
            int cmp = strcmp(psz_key, pCur->key);
            if (cmp == 0)
            {
                *ppResult = pCur;
                return;
            }
            else if (cmp < 0)
            {
                *ppResult = NULL;
                return;
            }
            else
            {
                pCur = pCur->pNext;
            }
        }
        *ppResult = NULL;
    }
}

void SG_ihash__update__int64(
        SG_context* pCtx, 
        SG_ihash* pvh, 
        const char* psz_key, 
        SG_int64 ival
        )
{
    sg_hashitem* phit = NULL;

	SG_ERR_CHECK_RETURN(  sg_ihash__find(pCtx, pvh, psz_key, &phit)  );
	if (!phit)
	{
		SG_ERR_CHECK_RETURN(  SG_ihash__add__int64(pCtx, pvh, psz_key, ival)  );
		return;
	}

	phit->v = ival;
}

void SG_ihash__addtoval__int64(
    SG_context* pCtx,
	SG_ihash* pvh,
	const char* psz_key,
    SG_int64 addend
	)
{
    sg_hashitem* phit = NULL;

	SG_ERR_CHECK(  sg_ihash__find(pCtx, pvh, psz_key, &phit)  );
	if (!phit)
	{
		SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pvh, psz_key, addend)  );
		return;
	}

    phit->v += addend;

fail:
    ;
}

void SG_ihash__add__int64(
        SG_context* pCtx, 
        SG_ihash* pvh, 
        const char* psz_key, 
        SG_int64 ival
        )
{
	SG_ERR_CHECK(  sg_ihash__add(pCtx, pvh, psz_key, ival)  );

fail:
    ;
}

#define SG_IHASH_GET_PRELUDE													\
    sg_hashitem* phit = NULL;                                                   \
																				\
	SG_NULLARGCHECK_RETURN(pvh);												\
	SG_NULLARGCHECK_RETURN(psz_key);											\
	SG_ERR_CHECK_RETURN(sg_ihash__find(pCtx, pvh, psz_key, &phit));	            \
	if (!phit)																	\
	{																			\
        SG_ERR_THROW2_RETURN(SG_ERR_VHASH_KEYNOTFOUND, (pCtx, "%s", psz_key));  \
	}


void SG_ihash__get__int64(
        SG_context* pCtx, 
        const SG_ihash* pvh, 
        const char* psz_key, 
        SG_int64* pResult
        )
{
	SG_IHASH_GET_PRELUDE;

    *pResult = phit->v;
}

void SG_ihash__has(
        SG_context* pCtx, 
        const SG_ihash* pvh, 
        const char* psz_key, 
        SG_bool* pbResult
        )
{
    sg_hashitem* phit = NULL;

	SG_ERR_CHECK_RETURN(  sg_ihash__find(pCtx, pvh, psz_key, &phit)  );

	*pbResult = (phit != NULL);
}

void SG_ihash__check__int64(
        SG_context* pCtx, 
        const SG_ihash* pvh, 
        const char* psz_key, 
        SG_int64* piResult
        )
{
    sg_hashitem* phit = NULL;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(psz_key);
	SG_ERR_CHECK_RETURN(sg_ihash__find(pCtx, pvh, psz_key, &phit));
	if (phit)													
	{														
        *piResult = phit->v;
    }
}

void SG_ihash__key(
        SG_context* pCtx, 
        const SG_ihash* pvh, 
        const char* psz_key, 
        const char** pp
        )
{
    sg_hashitem* phit = NULL;

	SG_ERR_CHECK_RETURN(  sg_ihash__find(pCtx, pvh, psz_key, &phit)  );

    if (phit)
    {
        *pp = phit->key;
    }
    else
    {
        *pp = NULL;
    }
}

void SG_ihash__indexof(
        SG_context* pCtx, 
        SG_ihash* pvh, 
        const char* psz_key,
        SG_int32* pi
        )
{
	sg_hashitem *phit = NULL;
	SG_uint32 key_ndx = 0;

	SG_ERR_CHECK_RETURN(  sg_ihash__find(pCtx, pvh, psz_key, &phit)  );

	if (!phit)
	{
        *pi = -1;
        return;
	}

    key_ndx = (SG_uint32) (phit - pvh->aItems);

    *pi = (SG_int32) key_ndx;
}

void SG_ihash__remove_if_present(
        SG_context* pCtx, 
        SG_ihash* pvh, 
        const char* psz_key,
        SG_bool* pb_was_removed
        )
{
	sg_hashitem *phit = NULL;

	SG_ERR_CHECK_RETURN(  sg_ihash__find(pCtx, pvh, psz_key, &phit)  );

	if (!phit)
	{
        *pb_was_removed = SG_FALSE;
	}
    else
    {
        SG_uint32 key_ndx = (SG_uint32) (phit - pvh->aItems);

        memmove((char**)&(pvh->aItems[key_ndx]), &(pvh->aItems[key_ndx+1]), (pvh->space - key_ndx - 1) * sizeof(sg_hashitem));

        pvh->count--;

        SG_ERR_CHECK_RETURN(  sg_ihash__rehash__same_buckets(pCtx, pvh)  );

        *pb_was_removed = SG_TRUE;
    }
}

void SG_ihash__remove(
        SG_context* pCtx, 
        SG_ihash* pvh, 
        const char* psz_key
        )
{
    SG_bool b = SG_FALSE;

    SG_ERR_CHECK_RETURN(  SG_ihash__remove_if_present(pCtx, pvh, psz_key, &b)  );
	if (!b)
	{
		SG_ERR_THROW_RETURN(  SG_ERR_VHASH_KEYNOTFOUND  );
	}
}

void SG_ihash__get_nth_pair(
	SG_context* pCtx,
	const SG_ihash* pvh,
	SG_uint32 n,
	const char** psz_key,
    SG_int64* pi
	)
{
	SG_NULLARGCHECK_RETURN(pvh);
	if (n >= pvh->count)
	{
		SG_ERR_THROW_RETURN(  SG_ERR_ARGUMENT_OUT_OF_RANGE  );
	}

    if (pi)
    {
        *pi = pvh->aItems[n].v;
    }

	if (psz_key)
    {
		*psz_key = pvh->aItems[n].key;
    }
}

void SG_ihash__get_keys_as_rbtree(
	SG_context* pCtx,
	const SG_ihash* pvh,
    SG_rbtree** pprb
	)
{
	SG_rbtree* prb = NULL;
	SG_uint32 i;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb)  );

	for (i=0; i<pvh->count; i++)
	{
		SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb, pvh->aItems[i].key)  );
	}

	*pprb = prb;
    prb = NULL;

	return;

fail:
    SG_RBTREE_NULLFREE(pCtx, prb);
}

void SG_ihash__get_keys(
	SG_context* pCtx,
	const SG_ihash* pvh,
	SG_varray** ppResult
	)
{
	SG_varray* pva = NULL;
	SG_uint32 i;

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );

	for (i=0; i<pvh->count; i++)
	{
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, pvh->aItems[i].key)  );
	}

	*ppResult = pva;

	return;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
}

