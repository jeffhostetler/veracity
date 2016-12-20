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

#define sg_VHASH_NUM_BUILTIN (8)

typedef struct _sg_hashitem
{
	const char* key;            // points into the strpool
    SG_uint32 hash32;           // so we can just mask on rehash
    SG_variant* pVariant;       // points into the varpool
	struct _sg_hashitem* pNext; // for chaining within a bucket
} sg_hashitem;

typedef struct _sg_hashbucket
{
    sg_hashitem* head;
} sg_hashbucket;

struct _SG_vhash
{
	SG_uint32 count;

	SG_strpool *pStrPool;
	SG_bool strpool_is_mine;

	SG_varpool *pVarPool;
	SG_bool varpool_is_mine;

	SG_uint32 space;

	sg_hashitem* aItems;
	sg_hashbucket* aBuckets;
    SG_uint32 bucket_mask;

    sg_hashbucket builtin_buckets[sg_VHASH_NUM_BUILTIN];
    sg_hashitem builtin_items[sg_VHASH_NUM_BUILTIN];
};

void sg_vhash_variant__freecontents(SG_context * pCtx, SG_variant* pv)
{
    if (!pv)
    {
        return;
    }

	switch (pv->type)
	{
	case SG_VARIANT_TYPE_VARRAY:
		SG_VARRAY_NULLFREE(pCtx, pv->v.val_varray);
		break;
	case SG_VARIANT_TYPE_VHASH:
		SG_VHASH_NULLFREE(pCtx, pv->v.val_vhash);
		break;
	}
}

SG_uint8 sg_vhash__calc_bits_for_guess(SG_uint32 guess)
{
	SG_uint8 log = 1;
	while (guess > 1)
	{
		guess = guess >> 1;
		log++;
	}

	if (log < 3)
	{
		log = 3;
	}
	else if (log > 18)
	{
		log = 16;
	}

	return log;
}

void SG_vhash__alloc__shared(
        SG_context* pCtx, 
        SG_vhash** ppResult, 
        SG_uint32 guess, 
        SG_vhash* pvhShared
        )
{
	if (pvhShared)
	{
		SG_ERR_CHECK_RETURN(  SG_vhash__alloc__params(pCtx, ppResult, guess, pvhShared->pStrPool, pvhShared->pVarPool)  );	// do not use SG_VHASH__ALLOC__PARAMS
	}
	else
	{
		SG_ERR_CHECK_RETURN(  SG_vhash__alloc__params(pCtx, ppResult, guess, NULL, NULL)  );	// do not use SG_VHASH__ALLOC__PARAMS
	}
}

void SG_vhash__addcopy__variant(
	SG_context* pCtx,
	SG_vhash* pHash,
	const char* putf8Key,
	const SG_variant* pv
	)
{
	SG_vhash*  pvh_sub = NULL;
	SG_varray* pva_sub = NULL;

	SG_NULLARGCHECK(pv);

	switch (pv->type)
	{
	case SG_VARIANT_TYPE_DOUBLE:
		SG_ERR_CHECK(  SG_vhash__add__double(pCtx, pHash, putf8Key, pv->v.val_double)  );
		break;

	case SG_VARIANT_TYPE_INT64:
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pHash, putf8Key, pv->v.val_int64)  );
		break;

	case SG_VARIANT_TYPE_BOOL:
		SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pHash, putf8Key, pv->v.val_bool)  );
		break;

	case SG_VARIANT_TYPE_NULL:
		SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pHash, putf8Key)  );
		break;

	case SG_VARIANT_TYPE_SZ:
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pHash, putf8Key, pv->v.val_sz)  );
		break;

	case SG_VARIANT_TYPE_VHASH:
		SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pHash, putf8Key, &pvh_sub)  );
		SG_ERR_CHECK(  SG_vhash__copy_items(pCtx, pv->v.val_vhash, pvh_sub)  );
		break;

	case SG_VARIANT_TYPE_VARRAY:
		SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pHash, putf8Key, &pva_sub)  );
		SG_ERR_CHECK(  SG_varray__copy_items(pCtx, pv->v.val_varray, pva_sub)  );
		break;
	}

fail:
	return;
}

void SG_vhash__copy_items(
    SG_context* pCtx,
    const SG_vhash* pvh_from,
    SG_vhash* pvh_to
	)
{
    SG_uint32 i = 0;

    for (i=0; i<pvh_from->count; i++)
    {
        const char* psz_key = NULL;
        const SG_variant* pv = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_from, i, &psz_key, &pv)  );
        SG_ERR_CHECK(  SG_vhash__addcopy__variant(pCtx, pvh_to, psz_key, pv)  );
    }

fail:
    ;
}

void SG_vhash__copy_some_items(
    SG_context* pCtx,
    const SG_vhash* pvh_from,
    SG_vhash* pvh_to,
    ...
	)
{
    va_list ap;

    va_start(ap, pvh_to);
    do
    {
        const char* psz_key = va_arg(ap, const char*);
        SG_variant* pv = NULL;

        if (!psz_key)
        {
            break;
        }

		SG_ERR_CHECK(  SG_vhash__check__variant(pCtx, pvh_from, psz_key, &pv)  );
        if (pv)
        {
            SG_ERR_CHECK(  SG_vhash__addcopy__variant(pCtx, pvh_to, psz_key, pv)  );
        }
    } while (1);

fail:
    ;
}

void SG_vhash__alloc__copy(
    SG_context* pCtx,
	SG_vhash** ppResult,
	const SG_vhash* pvhOther
	)
{
	SG_vhash* pvh = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc__params(pCtx, &pvh, pvhOther->count, NULL, NULL)  );
    SG_ERR_CHECK(  SG_vhash__copy_items(pCtx, pvhOther, pvh)  );

	*ppResult = pvh;
    pvh = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_vhash__steal_the_pools(
    SG_context* pCtx,
    SG_vhash* pvh
	)
{
    SG_NULLARGCHECK_RETURN(pvh);

    pvh->strpool_is_mine = SG_TRUE;
    pvh->varpool_is_mine = SG_TRUE;
}

void SG_vhash__alloc__params(
        SG_context* pCtx, 
        SG_vhash** ppResult, 
        SG_uint32 guess,
        SG_strpool* pStrPool, 
        SG_varpool* pVarPool
        )
{
	SG_vhash * pvh = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pvh)  );

	if (pStrPool)
	{
		pvh->strpool_is_mine = SG_FALSE;
		pvh->pStrPool = pStrPool;
	}
	else
	{
		pvh->strpool_is_mine = SG_TRUE;
		SG_ERR_CHECK(  SG_STRPOOL__ALLOC(pCtx, &pvh->pStrPool, (guess ? guess : 8) * 200)  );
	}

	if (pVarPool)
	{
		pvh->varpool_is_mine = SG_FALSE;
		pvh->pVarPool = pVarPool;
	}
	else
	{
		pvh->varpool_is_mine = SG_TRUE;
		SG_ERR_CHECK(  SG_VARPOOL__ALLOC(pCtx, &pvh->pVarPool, (guess ? guess : 8))  );
	}

    if (0 == guess)
    {
        pvh->space = sg_VHASH_NUM_BUILTIN;
        pvh->aBuckets = pvh->builtin_buckets;
        pvh->aItems = pvh->builtin_items;
    }
    else
    {
        pvh->space = (1 << sg_vhash__calc_bits_for_guess(guess));

        SG_ERR_CHECK(  SG_alloc(pCtx, pvh->space, sizeof(sg_hashbucket), &pvh->aBuckets)  );
        SG_ERR_CHECK(  SG_alloc(pCtx, pvh->space, sizeof(sg_hashitem), &pvh->aItems)  );
    }
    pvh->bucket_mask = pvh->space - 1;

	*ppResult = pvh;
    pvh = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_vhash__alloc(SG_context* pCtx, SG_vhash** ppResult)
{
	SG_vhash__alloc__params(pCtx, ppResult, 0, NULL, NULL);	// do not use SG_VHASH__ALLOC__PARAMS
}

void SG_vhash__free(SG_context * pCtx, SG_vhash* pvh)
{
	if (!pvh)
		return;

    //printf("vhash_count\t%06d\n", pvh->count);

	if (pvh->aItems)
	{
        SG_uint32 i;

        for (i=0; i<pvh->space; i++)
        {
            sg_vhash_variant__freecontents(pCtx, pvh->aItems[i].pVariant);
        }

        if (pvh->aItems != pvh->builtin_items)
        {
            SG_NULLFREE(pCtx, pvh->aItems);
        }
	}

    if (pvh->aBuckets != pvh->builtin_buckets)
    {
        SG_NULLFREE(pCtx, pvh->aBuckets);
    }

	if (pvh->strpool_is_mine)
	{
		SG_STRPOOL_NULLFREE(pCtx, pvh->pStrPool);
	}

	if (pvh->varpool_is_mine)
	{
		SG_VARPOOL_NULLFREE(pCtx, pvh->pVarPool);
	}

	SG_NULLFREE(pCtx, pvh);
}

void SG_vhash__count(SG_context* pCtx, const SG_vhash* pvh, SG_uint32* piResult)
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

SG_uint32 sg_vhash__hashlittle( const SG_uint8 *k, size_t length)
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

void sg_vhash__rehash__same_buckets(
        SG_context* pCtx, 
        SG_vhash* pvh
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
    // TODO what should we do here?  do we need to leave the vhash
    // in a usable state?  can we?
    ;
}

void sg_vhash__rehash__new_buckets(
        SG_context* pCtx, 
        SG_vhash* pvh
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
    // TODO what should we do here?  do we need to leave the vhash
    // in a usable state?  can we?
    ;
}

void sg_vhash__grow(
        SG_context* pCtx, 
        SG_vhash* pvh
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

    SG_ERR_CHECK_RETURN(  sg_vhash__rehash__new_buckets(pCtx, pvh)  );
}

static void sg_vhash__add(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        SG_variant* pv
        )
{
	SG_int32 iBucket = -1;
    sg_hashitem* phit = NULL;

	SG_NULLARGCHECK_RETURN(psz_key);

	if ((pvh->count + 1) > (pvh->space * 3 / 4))
    {
        SG_ERR_CHECK(  sg_vhash__grow(pCtx, pvh)  );
    }

    phit = &(pvh->aItems[pvh->count]);
	SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pvh->pStrPool, psz_key, &phit->key)  );
    phit->hash32 = sg_vhash__hashlittle((SG_uint8*)phit->key, strlen(phit->key));
    phit->pVariant = pv;
    phit->pNext = NULL;

	iBucket = phit->hash32 & (pvh->bucket_mask);
    SG_ERR_CHECK(  sg_hashbucket__add__hashitem(pCtx, &pvh->aBuckets[iBucket], phit)  );
    pvh->count++;

fail:
    ;
}

/* Note that if the key is not found, this routine returns SG_ERR_OK and the resulting hashitem is set to NULL */
void sg_vhash__find(
        SG_UNUSED_PARAM(SG_context* pCtx), 
        const SG_vhash* pvh, 
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
        SG_uint32 iBucket = sg_vhash__hashlittle((SG_uint8*)psz_key, strlen(psz_key)) & (pvh->bucket_mask);
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

void SG_vhash__update__string__sz(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        const char* psz_value
        )
{
	SG_ERR_CHECK_RETURN(  SG_vhash__update__string__buflen(pCtx, pvh, psz_key, psz_value, 0)  );
}

void SG_vhash__update__string__buflen(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        const char* psz_value, 
        SG_uint32 len
        )
{
	if (psz_value)
	{
        sg_hashitem* phit = NULL;

		SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );
		if (!phit)
		{
			SG_ERR_CHECK_RETURN(  SG_vhash__add__string__sz(pCtx, pvh, psz_key, psz_value)  );
			return;
		}

		sg_vhash_variant__freecontents(pCtx, phit->pVariant);

		phit->pVariant->type = SG_VARIANT_TYPE_SZ;

		SG_ERR_CHECK(  SG_strpool__add__buflen__sz(pCtx, pvh->pStrPool, psz_value, len, &(phit->pVariant->v.val_sz))  );
	}
	else
	{
		SG_ERR_CHECK_RETURN(  SG_vhash__update__null(pCtx, pvh, psz_key)  );
	}

fail:
    ;
}

void SG_vhash__update__int64(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        SG_int64 ival
        )
{
    sg_hashitem* phit = NULL;

	SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );
	if (!phit)
	{
		SG_ERR_CHECK_RETURN(  SG_vhash__add__int64(pCtx, pvh, psz_key, ival)  );
		return;
	}

    sg_vhash_variant__freecontents(pCtx, phit->pVariant);

	phit->pVariant->type = SG_VARIANT_TYPE_INT64;
	phit->pVariant->v.val_int64 = ival;
}

void SG_vhash__update__double(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        double val
        )
{
    sg_hashitem* phit = NULL;

	SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );
	if (!phit)
	{
		SG_ERR_CHECK_RETURN(  SG_vhash__add__double(pCtx, pvh, psz_key, val)  );
		return;
	}

    sg_vhash_variant__freecontents(pCtx, phit->pVariant);

	phit->pVariant->type = SG_VARIANT_TYPE_DOUBLE;
	phit->pVariant->v.val_double = val;
}

void SG_vhash__update__bool(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        SG_bool val
        )
{
    sg_hashitem* phit = NULL;

	SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );
	if (!phit)
	{
		SG_ERR_CHECK_RETURN(  SG_vhash__add__bool(pCtx, pvh, psz_key, val)  );
		return;
	}

    sg_vhash_variant__freecontents(pCtx, phit->pVariant);

	phit->pVariant->type = SG_VARIANT_TYPE_BOOL;
	phit->pVariant->v.val_bool = val;
}

void SG_vhash__update__vhash(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        SG_vhash** ppvh_val
        )
{
	if (ppvh_val && *ppvh_val)
	{
        sg_hashitem* phit = NULL;

		SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );
		if (!phit)
		{
			SG_ERR_CHECK_RETURN(  SG_vhash__add__vhash(pCtx, pvh, psz_key, ppvh_val)  );
			return;
		}

        sg_vhash_variant__freecontents(pCtx, phit->pVariant);

		phit->pVariant->type = SG_VARIANT_TYPE_VHASH;
		phit->pVariant->v.val_vhash = *ppvh_val;
        *ppvh_val = NULL;
	}
	else
	{
		SG_ERR_CHECK_RETURN(  SG_vhash__update__null(pCtx, pvh, psz_key)  );
	}
}

void SG_vhash__update__varray(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        SG_varray** ppva_val
        )
{
	if (ppva_val && *ppva_val)
	{
        sg_hashitem* phit = NULL;
		SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );
		if (!phit)
		{
			SG_ERR_CHECK_RETURN(  SG_vhash__add__varray(pCtx, pvh, psz_key, ppva_val)  );
			return;
		}

        sg_vhash_variant__freecontents(pCtx, phit->pVariant);

		phit->pVariant->type = SG_VARIANT_TYPE_VARRAY;
		phit->pVariant->v.val_varray = *ppva_val;
		*ppva_val = NULL;
	}
	else
	{
		SG_ERR_CHECK_RETURN(  SG_vhash__update__null(pCtx, pvh, psz_key)  );
	}
}

void SG_vhash__update__null(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key
        )
{
    sg_hashitem* phit = NULL;

	SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );
	if (!phit)
	{
		SG_ERR_CHECK_RETURN(  SG_vhash__add__null(pCtx, pvh, psz_key)  );
		return;
	}

    sg_vhash_variant__freecontents(pCtx, phit->pVariant);

	phit->pVariant->type = SG_VARIANT_TYPE_NULL;
}

void SG_vhash__add__string__sz(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        const char* psz_value
        )
{
	SG_ERR_CHECK_RETURN(  SG_vhash__add__string__buflen(pCtx, pvh, psz_key, psz_value, 0)  );
}

void SG_vhash__add__string__buflen(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        const char* psz_value, 
        SG_uint32 len
        )
{
    SG_variant* pv = NULL;

	SG_NULLARGCHECK_RETURN(psz_key);
	SG_NULLARGCHECK_RETURN(psz_value);
//	SG_ARGCHECK_RETURN(*psz_key != 0, psz_key);

    SG_ERR_CHECK(  SG_varpool__add(pCtx, pvh->pVarPool, &pv)  );
    pv->type = SG_VARIANT_TYPE_SZ;
    SG_ERR_CHECK(  SG_strpool__add__buflen__sz(pCtx, pvh->pStrPool, psz_value, len, &(pv->v.val_sz))  );

    SG_ERR_CHECK(  sg_vhash__add(pCtx, pvh, psz_key, pv)  );

fail:
    ;
}


#ifndef SG_IOS
void SG_vhash__add__string__jsstring(
        SG_context* pCtx,
        SG_vhash* pvh,
        const char* psz_key,
        JSContext *cx,
        JSString *str
        )
{
    SG_variant* pv = NULL;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(psz_key);
	SG_NULLARGCHECK_RETURN(cx);
	SG_NULLARGCHECK_RETURN(str);

    SG_ERR_CHECK(  SG_varpool__add(pCtx, pvh->pVarPool, &pv)  );
    pv->type = SG_VARIANT_TYPE_SZ;
    SG_ERR_CHECK(  SG_strpool__add__jsstring(pCtx, pvh->pStrPool, cx, str, &(pv->v.val_sz))  );

    SG_ERR_CHECK(  sg_vhash__add(pCtx, pvh, psz_key, pv)  );

fail:
    ;
}

#endif

void SG_vhash__addtoval__int64(
    SG_context* pCtx,
	SG_vhash* pvh,
	const char* psz_key,
    SG_int64 addend
	)
{
    sg_hashitem* phit = NULL;

	SG_ERR_CHECK(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );
	if (!phit)
	{
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, psz_key, addend)  );
		return;
	}

    if (SG_VARIANT_TYPE_INT64 == phit->pVariant->type)
    {
        phit->pVariant->v.val_int64 += addend;
    }
    else
    {
        SG_ERR_THROW(  SG_ERR_VARIANT_INVALIDTYPE  );
    }

fail:
    ;
}

void SG_vhash__add__int64(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        SG_int64 ival
        )
{
    SG_variant* pv = NULL;

    SG_ERR_CHECK(  SG_varpool__add(pCtx, pvh->pVarPool, &pv)  );
	pv->type = SG_VARIANT_TYPE_INT64;
	pv->v.val_int64 = ival;

	SG_ERR_CHECK(  sg_vhash__add(pCtx, pvh, psz_key, pv)  );

fail:
    ;
}

void SG_vhash__add__double(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        double fv
        )
{
    SG_variant* pv = NULL;

    SG_ERR_CHECK(  SG_varpool__add(pCtx, pvh->pVarPool, &pv)  );
	pv->type = SG_VARIANT_TYPE_DOUBLE;
	pv->v.val_double = fv;

	SG_ERR_CHECK(  sg_vhash__add(pCtx, pvh, psz_key, pv)  );

fail:
    ;
}

void SG_vhash__add__bool(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        SG_bool b
        )
{
    SG_variant* pv = NULL;

    SG_ERR_CHECK(  SG_varpool__add(pCtx, pvh->pVarPool, &pv)  );
	pv->type = SG_VARIANT_TYPE_BOOL;
	pv->v.val_bool = b;

	SG_ERR_CHECK(  sg_vhash__add(pCtx, pvh, psz_key, pv)  );

fail:
    ;
}

void SG_vhash__addnew__varray(
    SG_context* pCtx,
	SG_vhash* pvh,
	const char* psz_key,
	SG_varray** pResult
	)
{
    SG_varray* pva_sub = NULL;
    SG_varray* pva_sub_ref = NULL;

    SG_ERR_CHECK(  SG_varray__alloc__params(pCtx, &pva_sub, 4, pvh->pStrPool, pvh->pVarPool)  );
    pva_sub_ref = pva_sub;
    SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh, psz_key, &pva_sub)  );
    *pResult = pva_sub_ref;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_sub);
}

void SG_vhash__addcopy__vhash(
    SG_context* pCtx,
	SG_vhash* pvh,
	const char* psz_key,
	const SG_vhash* pvh_orig
	)
{
    SG_vhash* pvh_copy = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__addnew__vhash(pCtx, pvh, psz_key, &pvh_copy)  );
	SG_ERR_CHECK_RETURN(  SG_vhash__copy_items(pCtx, pvh_orig, pvh_copy)  );
}

void SG_vhash__addcopy__varray(
    SG_context* pCtx,
	SG_vhash* pvh,
	const char* psz_key,
	SG_varray* pva_orig
	)
{
    SG_varray* pva_copy = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__addnew__varray(pCtx, pvh, psz_key, &pva_copy)  );
	SG_ERR_CHECK_RETURN(  SG_varray__copy_items(pCtx, pva_orig, pva_copy)  );
}

void SG_vhash__addnew__vhash(
    SG_context* pCtx,
	SG_vhash* pvh,
	const char* psz_key,
	SG_vhash** pResult
	)
{
    SG_vhash* pvh_sub = NULL;
    SG_vhash* pvh_sub_ref = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc__params(pCtx, &pvh_sub, 0, pvh->pStrPool, pvh->pVarPool)  );
    pvh_sub_ref = pvh_sub;
    SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh, psz_key, &pvh_sub)  );
    if (pResult)
    {
        *pResult = pvh_sub_ref;
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_sub);
}

void SG_vhash__updatenew__vhash(
    SG_context* pCtx,
	SG_vhash* pvh,
	const char* psz_key,
	SG_vhash** pResult
	)
{
    SG_vhash* pvh_sub = NULL;
    SG_vhash* pvh_sub_ref = NULL;

    SG_ERR_CHECK(  SG_vhash__alloc__params(pCtx, &pvh_sub, 0, pvh->pStrPool, pvh->pVarPool)  );
    pvh_sub_ref = pvh_sub;
    SG_ERR_CHECK(  SG_vhash__update__vhash(pCtx, pvh, psz_key, &pvh_sub)  );
    if (pResult)
    {
        *pResult = pvh_sub_ref;
    }

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_sub);
}

void SG_vhash__add__vhash(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        SG_vhash** ppvh_val
        )
{
    SG_variant* pv = NULL;

    SG_NULLARGCHECK_RETURN(ppvh_val);
    SG_NULLARGCHECK_RETURN(*ppvh_val);

    SG_ERR_CHECK(  SG_varpool__add(pCtx, pvh->pVarPool, &pv)  );
    pv->type = SG_VARIANT_TYPE_VHASH;
    pv->v.val_vhash = *ppvh_val;
    *ppvh_val = NULL;

    SG_ERR_CHECK(  sg_vhash__add(pCtx, pvh, psz_key, pv)  );

fail:
    ;
}

void SG_vhash__add__varray(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        SG_varray** ppva_val
        )
{
    SG_variant* pv = NULL;

    SG_NULLARGCHECK_RETURN(ppva_val);
    SG_NULLARGCHECK_RETURN(*ppva_val);

    SG_ERR_CHECK(  SG_varpool__add(pCtx, pvh->pVarPool, &pv)  );
    pv->type = SG_VARIANT_TYPE_VARRAY;
    pv->v.val_varray = *ppva_val;
    *ppva_val = NULL;

    SG_ERR_CHECK(  sg_vhash__add(pCtx, pvh, psz_key, pv)  );

fail:
    ;
}

void SG_vhash__add__null(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key
        )
{
    SG_variant* pv = NULL;

    SG_ERR_CHECK(  SG_varpool__add(pCtx, pvh->pVarPool, &pv)  );
	pv->type = SG_VARIANT_TYPE_NULL;

	SG_ERR_CHECK(  sg_vhash__add(pCtx, pvh, psz_key, pv)  );

fail:
    ;
}

#define SG_VHASH_GET_PRELUDE													\
    sg_hashitem* phit = NULL;                                                   \
																				\
	SG_NULLARGCHECK_RETURN(pvh);												\
	SG_NULLARGCHECK_RETURN(psz_key);											\
	SG_ERR_CHECK_RETURN(sg_vhash__find(pCtx, pvh, psz_key, &phit));	            \
	if (!phit)																	\
	{																			\
        SG_ERR_THROW2_RETURN(SG_ERR_VHASH_KEYNOTFOUND, (pCtx, "%s", psz_key));  \
	}


void SG_vhash__get__sz(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        const char** ppsz_value
        )
{
	SG_VHASH_GET_PRELUDE;

	SG_ERR_CHECK_RETURN(  SG_variant__get__sz(pCtx, phit->pVariant, ppsz_value)  );
}

void SG_vhash__get__int64(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_int64* pResult
        )
{
	SG_VHASH_GET_PRELUDE;

	SG_ERR_CHECK_RETURN(  SG_variant__get__int64(pCtx, phit->pVariant, pResult)  );
}

void SG_vhash__get__uint64(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_uint64* pResult
        )
{
	SG_VHASH_GET_PRELUDE;

	SG_ERR_CHECK_RETURN(  SG_variant__get__uint64(pCtx, phit->pVariant, pResult)  );
}

void SG_vhash__get__int64_or_double(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_int64* pResult
        )
{
	SG_VHASH_GET_PRELUDE;

	SG_ERR_CHECK_RETURN(  SG_variant__get__int64_or_double(pCtx, phit->pVariant, pResult)  );
}

void SG_vhash__get__uint32(
    SG_context* pCtx,
	const SG_vhash* pvh,
	const char* psz_key,
	SG_uint32* pResult
	)
{
    SG_int64 i64 = 0;
    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, psz_key, &i64)  );
    if (SG_int64__fits_in_uint32(i64))
    {
        *pResult = (SG_uint32) i64;
        return;
    }
    else
    {
        SG_ERR_THROW_RETURN(  SG_ERR_INTEGER_OVERFLOW  );
    }

fail:
    return;
}

void SG_vhash__get__int32(
    SG_context* pCtx,
	const SG_vhash* pvh,
	const char* psz_key,
	SG_int32* pResult
	)
{
    SG_int64 i64 = 0;
    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh, psz_key, &i64)  );
    if (SG_int64__fits_in_int32(i64))
    {
        *pResult = (SG_int32) i64;
        return;
    }
    else
    {
        SG_ERR_THROW_RETURN(  SG_ERR_INTEGER_OVERFLOW  );
    }

fail:
    return;
}

void SG_vhash__get__double(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        double* pResult
        )
{
	SG_VHASH_GET_PRELUDE;

	SG_ERR_CHECK_RETURN(  SG_variant__get__double(pCtx, phit->pVariant, pResult)  );
}

void SG_vhash__get__bool(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_bool* pResult
        )
{
	SG_VHASH_GET_PRELUDE;

	SG_ERR_CHECK_RETURN(  SG_variant__get__bool(pCtx, phit->pVariant, pResult)  );
}

void SG_vhash__get__vhash(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_vhash** ppResult /* should be const? */
        )
{
	SG_VHASH_GET_PRELUDE;

	SG_ERR_CHECK_RETURN(  SG_variant__get__vhash(pCtx, phit->pVariant, ppResult)  );
}

void SG_vhash__ensure__varray(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        SG_varray** ppResult
        )
{
    SG_bool b_has = SG_FALSE;

    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh, psz_key, &b_has)  );
    if (b_has)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__get__varray(pCtx, pvh, psz_key, ppResult)  );
    }
    else
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__addnew__varray(pCtx, pvh, psz_key, ppResult)  );
    }
}

void SG_vhash__ensure__vhash(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key, 
        SG_vhash** ppResult
        )
{
    SG_bool b_has = SG_FALSE;

    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, pvh, psz_key, &b_has)  );
    if (b_has)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__get__vhash(pCtx, pvh, psz_key, ppResult)  );
    }
    else
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__addnew__vhash(pCtx, pvh, psz_key, ppResult)  );
    }
}

void SG_vhash__get__varray(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_varray** ppResult
        )
{
	SG_VHASH_GET_PRELUDE;

	SG_ERR_CHECK_RETURN(  SG_variant__get__varray(pCtx, phit->pVariant, ppResult)  );
}

void SG_vhash__has(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_bool* pbResult
        )
{
    sg_hashitem* phit = NULL;

	SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );

	*pbResult = (phit != NULL);
}

void SG_vhash__check__uint64(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_uint64* pResult
        )
{
    sg_hashitem* phit = NULL;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(psz_key);
	SG_ERR_CHECK_RETURN(sg_vhash__find(pCtx, pvh, psz_key, &phit));
	if (phit)													
	{														
        SG_int64 i64 = 0;
        SG_ERR_CHECK_RETURN(  SG_variant__get__int64(pCtx, phit->pVariant, &i64)  );
        if (SG_int64__fits_in_uint64(i64))
        {
            *pResult = (SG_uint64) i64;
            return;
        }
        else
        {
            SG_ERR_THROW_RETURN(  SG_ERR_INTEGER_OVERFLOW  );
        }
    }
}

void SG_vhash__check__bool(
	SG_context* pCtx, 
	const SG_vhash* pvh, 
	const char* psz_key, 
	SG_bool* pResult
	)
{
	sg_hashitem* phit = NULL;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(psz_key);
	SG_ERR_CHECK_RETURN(sg_vhash__find(pCtx, pvh, psz_key, &phit));
	if (phit)													
	{														
		SG_ERR_CHECK_RETURN(  SG_variant__get__bool(pCtx, phit->pVariant, pResult)  );
	}
}

void SG_vhash__check__int64(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_int64* pResult
        )
{
    sg_hashitem* phit = NULL;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(psz_key);
	SG_ERR_CHECK_RETURN(sg_vhash__find(pCtx, pvh, psz_key, &phit));
	if (phit)													
	{														
        SG_ERR_CHECK_RETURN(  SG_variant__get__int64(pCtx, phit->pVariant, pResult)  );
    }
}

void SG_vhash__check__int32(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_int32* pResult
        )
{
    sg_hashitem* phit = NULL;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(psz_key);
	SG_ERR_CHECK_RETURN(sg_vhash__find(pCtx, pvh, psz_key, &phit));
	if (phit)													
	{														
        SG_int64 i64 = 0;
        SG_ERR_CHECK_RETURN(  SG_variant__get__int64(pCtx, phit->pVariant, &i64)  );
        if (SG_int64__fits_in_int32(i64))
        {
            *pResult = (SG_int32) i64;
            return;
        }
        else
        {
            SG_ERR_THROW_RETURN(  SG_ERR_INTEGER_OVERFLOW  );
        }
    }
}

void SG_vhash__check__sz(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        const char** ppsz_value
        )
{
    sg_hashitem* phit = NULL;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(psz_key);
	SG_ERR_CHECK_RETURN(sg_vhash__find(pCtx, pvh, psz_key, &phit));
	if (!phit)													
	{														
        *ppsz_value = NULL;
	}
    else
    {
        SG_ERR_CHECK_RETURN(  SG_variant__get__sz(pCtx, phit->pVariant, ppsz_value)  );
    }
}

void SG_vhash__check__vhash(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_vhash** ppvh
        )
{
    sg_hashitem* phit = NULL;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(psz_key);
	SG_ERR_CHECK_RETURN(sg_vhash__find(pCtx, pvh, psz_key, &phit));
	if (!phit)													
	{														
        *ppvh = NULL;
	}
    else
    {
        SG_ERR_CHECK_RETURN(  SG_variant__get__vhash(pCtx, phit->pVariant, ppvh)  );
    }
}

void SG_vhash__check__varray(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_varray** ppva
        )
{
    sg_hashitem* phit = NULL;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(psz_key);
	SG_ERR_CHECK_RETURN(sg_vhash__find(pCtx, pvh, psz_key, &phit));
	if (!phit)													
	{														
        *ppva = NULL;
	}
    else
    {
        SG_ERR_CHECK_RETURN(  SG_variant__get__varray(pCtx, phit->pVariant, ppva)  );
    }
}

void SG_vhash__check__variant(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_variant** ppv
        )
{
    sg_hashitem* phit = NULL;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(psz_key);
	SG_ERR_CHECK_RETURN(sg_vhash__find(pCtx, pvh, psz_key, &phit));
	if (!phit)													
	{														
        *ppv = NULL;
	}
    else
    {
        *ppv = phit->pVariant;
    }
}

void SG_vhash__key(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        const char** pp
        )
{
    sg_hashitem* phit = NULL;

	SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );

    if (phit)
    {
        *pp = phit->key;
    }
    else
    {
        *pp = NULL;
    }
}

void SG_vhash__indexof(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key,
        SG_int32* pi
        )
{
	sg_hashitem *phit = NULL;
	SG_uint32 key_ndx = 0;

	SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );

	if (!phit)
	{
        *pi = -1;
        return;
	}

    key_ndx = (SG_uint32) (phit - pvh->aItems);

    *pi = (SG_int32) key_ndx;
}

void SG_vhash__remove_if_present(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key,
        SG_bool* pb_was_removed
        )
{
	sg_hashitem *phit = NULL;

	SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );

	if (!phit)
	{
        *pb_was_removed = SG_FALSE;
	}
    else
    {
        SG_uint32 key_ndx = (SG_uint32) (phit - pvh->aItems);

        sg_vhash_variant__freecontents(pCtx, pvh->aItems[key_ndx].pVariant);

        memmove((char**)&(pvh->aItems[key_ndx]), &(pvh->aItems[key_ndx+1]), (pvh->space - key_ndx - 1) * sizeof(sg_hashitem));

        pvh->count--;

        SG_ERR_CHECK_RETURN(  sg_vhash__rehash__same_buckets(pCtx, pvh)  );

        *pb_was_removed = SG_TRUE;
    }
}

void SG_vhash__remove(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_key
        )
{
    SG_bool b = SG_FALSE;

    SG_ERR_CHECK_RETURN(  SG_vhash__remove_if_present(pCtx, pvh, psz_key, &b)  );
	if (!b)
	{
		SG_ERR_THROW2_RETURN(  SG_ERR_VHASH_KEYNOTFOUND, (pCtx, "%s", psz_key)  );
	}
}

void SG_vhash__typeof(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        SG_uint16* pResult
        )
{
    sg_hashitem* phit = NULL;

	SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pvh, psz_key, &phit)  );

	if (!phit)
	{
		*pResult = 0;
		SG_ERR_THROW2_RETURN(  SG_ERR_VHASH_KEYNOTFOUND, (pCtx, "%s", psz_key)  );
	}

	*pResult = phit->pVariant->type;
}

void SG_vhash__alloc__from_json__buflen(
        SG_context* pCtx, 
        SG_vhash** pResult, 
        const char* pszJson,
        SG_uint32 len
        )
{
    SG_vhash* pvh = NULL;
    SG_varray* pva = NULL;

    SG_ERR_CHECK(  SG_veither__parse_json__buflen(pCtx, pszJson, len, &pvh, &pva)  );
    if (pva)
    {
        SG_ERR_THROW(  SG_ERR_JSON_WRONG_TOP_TYPE  );
    }

	*pResult = pvh;
    pvh = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_vhash__alloc__from_json__buflen__utf8_fix(
        SG_context* pCtx, 
        SG_vhash** pResult, 
        const char* pszJson,
        SG_uint32 len
        )
{
    SG_vhash* pvh = NULL;
    SG_varray* pva = NULL;

    SG_ERR_CHECK(  SG_veither__parse_json__buflen__utf8_fix(pCtx, pszJson, len, &pvh, &pva)  );
    if (pva)
    {
        SG_ERR_THROW(  SG_ERR_JSON_WRONG_TOP_TYPE  );
    }

	*pResult = pvh;
    pvh = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_vhash__alloc__from_json__sz(
        SG_context* pCtx, 
        SG_vhash** pResult, 
        const char* pszJson
        )
{
    SG_ERR_CHECK_RETURN(  SG_vhash__alloc__from_json__buflen(pCtx, pResult, pszJson, SG_STRLEN(pszJson))  );
}

void SG_vhash__alloc__from_json__string(
        SG_context* pCtx, 
        SG_vhash** pResult, 
        SG_string* pJson
        )
{
	SG_NULLARGCHECK_RETURN(pJson);
	SG_ERR_CHECK_RETURN(  SG_vhash__alloc__from_json__buflen(pCtx, pResult, SG_string__sz(pJson), SG_string__length_in_bytes(pJson))  );
}

void SG_vhash__foreach(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        SG_vhash_foreach_callback* cb, 
        void* ctx
        )
{
	SG_uint32 i;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(cb);

	for (i=0; i<pvh->count; i++)
	{
		SG_ERR_CHECK_RETURN(  cb(pCtx, ctx, pvh, pvh->aItems[i].key, pvh->aItems[i].pVariant)  );
	}
}

void sg_vhash__write_json_callback(
        SG_context* pCtx, 
        void* ctx, 
        SG_UNUSED_PARAM(const SG_vhash* pvh), 
        const char* psz_key, 
        const SG_variant* pv
        )
{
	SG_jsonwriter* pjson = (SG_jsonwriter*) ctx;

	SG_UNUSED(pvh);

    SG_ERR_CHECK_RETURN(  SG_jsonwriter__write_pair__variant(pCtx, pjson, psz_key, pv)  );
}

void SG_vhash__write_json(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        SG_jsonwriter* pjson
        )
{
	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(pjson);

	SG_ERR_CHECK_RETURN(  SG_jsonwriter__write_start_object(pCtx, pjson)  );

	SG_ERR_CHECK_RETURN(  SG_vhash__foreach(pCtx, pvh, sg_vhash__write_json_callback, pjson)  );

	SG_ERR_CHECK_RETURN(  SG_jsonwriter__write_end_object(pCtx, pjson)  );
}

void SG_vhash__to_json__pretty_print_NOT_for_storage(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        SG_string* pStr
        )
{
	SG_jsonwriter* pjson = NULL;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(pStr);

	SG_ERR_CHECK(  SG_jsonwriter__alloc__pretty_print_NOT_for_storage(pCtx, &pjson, pStr)  );

	SG_ERR_CHECK(  SG_vhash__write_json(pCtx, pvh, pjson)  );

fail:
    SG_JSONWRITER_NULLFREE(pCtx, pjson);
}

void SG_vhash__to_json(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        SG_string* pStr
        )
{
	SG_jsonwriter* pjson = NULL;

	SG_NULLARGCHECK_RETURN(pvh);
	SG_NULLARGCHECK_RETURN(pStr);

	SG_ERR_CHECK(  SG_jsonwriter__alloc(pCtx, &pjson, pStr)  );

	SG_ERR_CHECK(  SG_vhash__write_json(pCtx, pvh, pjson)  );

fail:
    SG_JSONWRITER_NULLFREE(pCtx, pjson);
}

void SG_vhash__get__variant(
        SG_context* pCtx, 
        const SG_vhash* pvh, 
        const char* psz_key, 
        const SG_variant** ppResult
        )
{
	SG_VHASH_GET_PRELUDE;

	*ppResult = phit->pVariant;
}

void SG_vhash__equal(
        SG_context* pCtx, 
        const SG_vhash* pv1, 
        const SG_vhash* pv2, 
        SG_bool* pbResult
        )
{
	SG_uint32 i;

	SG_NULLARGCHECK_RETURN(pv1);
	SG_NULLARGCHECK_RETURN(pv2);

	if (pv1 == pv2)
	{
		*pbResult = SG_TRUE;
		return;
	}

	if (pv1->count != pv2->count)
	{
		*pbResult = SG_FALSE;
		return;
	}

	for (i=0; i<pv1->count; i++)
	{
		SG_variant* v1 = NULL;
		const SG_variant* v2 = NULL;
		SG_bool b = SG_FALSE;

        sg_hashitem* phit1 = NULL;
		SG_ERR_CHECK_RETURN(  sg_vhash__find(pCtx, pv1, pv1->aItems[i].key, &phit1)  );
        v1 = phit1->pVariant;

		SG_vhash__get__variant(pCtx, pv2, pv1->aItems[i].key, &v2);
		if (SG_context__err_equals(pCtx, SG_ERR_VHASH_KEYNOTFOUND))
		{
			SG_context__err_reset(pCtx);
			*pbResult = SG_FALSE;
			return;
		}
		SG_ERR_CHECK_RETURN_CURRENT;

		SG_ERR_CHECK_RETURN(  SG_variant__equal(pCtx, v1, v2, &b)  );

		if (!b)
		{
			*pbResult = SG_FALSE;
			return;
		}
	}

	*pbResult = SG_TRUE;
}

//////////////////////////////////////////////////////////////////
// This hack is needed to make VS2005 happy.  It didn't like
// arbitrary void * data pointers and function pointers mixing.

struct _recursive_sort_foreach_callback_data
{
	SG_qsort_compare_function * pfnCompare;
};

static void _sg_vhash__recursive_sort__foreach_callback(
        SG_context* pCtx,
        void * pVoidData,
        SG_UNUSED_PARAM(const SG_vhash * pvh),
        SG_UNUSED_PARAM(const char * szKey),
        const SG_variant * pvar
        )
{
	// iterate over all values in this vhash and sort the ones that are also vhashes.

	SG_vhash * pvhValue;
	struct _recursive_sort_foreach_callback_data * pData = (struct _recursive_sort_foreach_callback_data *)pVoidData;

	SG_UNUSED(pvh);
	SG_UNUSED(szKey);

	SG_variant__get__vhash(pCtx, pvar,&pvhValue);
	if (SG_CONTEXT__HAS_ERR(pCtx) || !pvhValue)
	{
		SG_context__err_reset(pCtx);
		return;
	}

	SG_ERR_CHECK_RETURN(  SG_vhash__sort(pCtx,pvhValue,SG_TRUE,pData->pfnCompare)  );
}

int SG_vhash_sort_callback__decreasing(
        SG_UNUSED_PARAM(SG_context * pCtx),
        const void * pVoid_item1,
        const void * pVoid_item2,
        SG_UNUSED_PARAM(void * pVoidCallerData)
        )
{
    sg_hashitem* pit1 = ((sg_hashitem*) pVoid_item1);
    sg_hashitem* pit2 = ((sg_hashitem*) pVoid_item2);

	SG_UNUSED(pCtx);
	SG_UNUSED(pVoidCallerData);

	// a simple decreasing, case sensitive ordering
	return -strcmp(pit1->key, pit2->key);
}

int SG_vhash_sort_callback__increasing(
        SG_UNUSED_PARAM(SG_context * pCtx),
        const void * pVoid_item1,
        const void * pVoid_item2,
        SG_UNUSED_PARAM(void * pVoidCallerData)
        )
{
    sg_hashitem* pit1 = ((sg_hashitem*) pVoid_item1);
    sg_hashitem* pit2 = ((sg_hashitem*) pVoid_item2);

	SG_UNUSED(pCtx);
	SG_UNUSED(pVoidCallerData);

	// a simple increasing, case sensitive ordering
	return strcmp(pit1->key, pit2->key);
}

int SG_vhash_sort_callback__increasing_insensitive(
        SG_UNUSED_PARAM(SG_context * pCtx),
        const void * pVoid_item1,
        const void * pVoid_item2,
        SG_UNUSED_PARAM(void * pVoidCallerData)
        )
{
    sg_hashitem* pit1 = ((sg_hashitem*) pVoid_item1);
    sg_hashitem* pit2 = ((sg_hashitem*) pVoid_item2);

	SG_UNUSED(pCtx);
	SG_UNUSED(pVoidCallerData);

	// a simple increasing, case insensitive ordering
	return SG_stricmp(pit1->key, pit2->key);
}

int SG_vhash_sort_callback__vhash_field_int_desc(
        SG_context * pCtx,
        const void * pVoid_item1,
        const void * pVoid_item2,
        void * pVoidCallerData
        )
{
    sg_hashitem* pit1 = ((sg_hashitem*) pVoid_item1);
    sg_hashitem* pit2 = ((sg_hashitem*) pVoid_item2);
    SG_vhash* pvh1 = NULL;
    SG_vhash* pvh2 = NULL;
    const char* psz_field = (const char*) pVoidCallerData;
    SG_int64 v1 = 0;
    SG_int64 v2 = 0;

    SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pit1->pVariant, &pvh1)  );
    SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pit2->pVariant, &pvh2)  );
    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh1, psz_field, &v1)  );
    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh2, psz_field, &v2)  );

    if (v1 == v2)
    {
        return 0;
    }
    else if (v1 < v2)
    {
        return 1;
    }
    else
    {
        return -1;
    }
    
fail:
    return 0;
}

int SG_vhash_sort_callback__vhash_field_sz_asc(
        SG_context * pCtx,
        const void * pVoid_item1,
        const void * pVoid_item2,
        void * pVoidCallerData
        )
{
    sg_hashitem* pit1 = ((sg_hashitem*) pVoid_item1);
    sg_hashitem* pit2 = ((sg_hashitem*) pVoid_item2);
    SG_vhash* pvh1 = NULL;
    SG_vhash* pvh2 = NULL;
    const char* psz_field = (const char*) pVoidCallerData;
    const char* psz1 = NULL;
    const char* psz2 = NULL;

    SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pit1->pVariant, &pvh1)  );
    SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pit2->pVariant, &pvh2)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh1, psz_field, &psz1)  );
    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh2, psz_field, &psz2)  );

    return strcmp(psz1, psz2);
    
fail:
    return 0;
}

int SG_vhash_sort_callback__vhash_field_int_asc(
        SG_context * pCtx,
        const void * pVoid_item1,
        const void * pVoid_item2,
        void * pVoidCallerData
        )
{
    sg_hashitem* pit1 = ((sg_hashitem*) pVoid_item1);
    sg_hashitem* pit2 = ((sg_hashitem*) pVoid_item2);
    SG_vhash* pvh1 = NULL;
    SG_vhash* pvh2 = NULL;
    const char* psz_field = (const char*) pVoidCallerData;
    SG_int64 v1 = 0;
    SG_int64 v2 = 0;

    SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pit1->pVariant, &pvh1)  );
    SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pit2->pVariant, &pvh2)  );
    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh1, psz_field, &v1)  );
    SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh2, psz_field, &v2)  );

    if (v1 == v2)
    {
        return 0;
    }
    else if (v2 < v1)
    {
        return 1;
    }
    else
    {
        return -1;
    }
    
fail:
    return 0;
}

void SG_vhash__sort__vhash_field_int__desc(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_field
        )
{
	SG_NULLARGCHECK_RETURN(pvh);

	SG_ERR_CHECK_RETURN(  SG_qsort(pCtx,
								   ((void *)pvh->aItems),pvh->count,sizeof(sg_hashitem),
								   SG_vhash_sort_callback__vhash_field_int_desc,
								   (void*) psz_field)  );

    SG_ERR_CHECK_RETURN(  sg_vhash__rehash__same_buckets(pCtx, pvh)  );
}

void SG_vhash__sort__vhash_field_sz__asc(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_field
        )
{
	SG_NULLARGCHECK_RETURN(pvh);

	SG_ERR_CHECK_RETURN(  SG_qsort(pCtx,
								   ((void *)pvh->aItems),pvh->count,sizeof(sg_hashitem),
								   SG_vhash_sort_callback__vhash_field_sz_asc,
								   (void*) psz_field)  );

    SG_ERR_CHECK_RETURN(  sg_vhash__rehash__same_buckets(pCtx, pvh)  );
}

void SG_vhash__sort__vhash_field_int__asc(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        const char* psz_field
        )
{
	SG_NULLARGCHECK_RETURN(pvh);

	SG_ERR_CHECK_RETURN(  SG_qsort(pCtx,
								   ((void *)pvh->aItems),pvh->count,sizeof(sg_hashitem),
								   SG_vhash_sort_callback__vhash_field_int_asc,
								   (void*) psz_field)  );

    SG_ERR_CHECK_RETURN(  sg_vhash__rehash__same_buckets(pCtx, pvh)  );
}

void SG_vhash__sort(
        SG_context* pCtx, 
        SG_vhash* pvh, 
        SG_bool bRecursive, 
        SG_qsort_compare_function pfnCompare
        )
{
	// sort keys in the given vhash.
	// optionally, sort the values which are vhashes.

	SG_NULLARGCHECK_RETURN(pvh);
	SG_ARGCHECK_RETURN( (pfnCompare!=NULL), pfnCompare);

	SG_ERR_CHECK_RETURN(  SG_qsort(pCtx,
								   ((void *)pvh->aItems),pvh->count,sizeof(sg_hashitem),
								   pfnCompare,
								   NULL)  );

    SG_ERR_CHECK_RETURN(  sg_vhash__rehash__same_buckets(pCtx, pvh)  );

	if (!bRecursive)
		return;

	{
		// This hack is needed to make VS2005 happy.  It didn't like
		// arbitrary void * data pointers and function pointers mixing.

		struct _recursive_sort_foreach_callback_data data;
		data.pfnCompare = pfnCompare;

		SG_ERR_CHECK_RETURN(  SG_vhash__foreach(pCtx, pvh,
												_sg_vhash__recursive_sort__foreach_callback,
												&data)  );
	}
}

//////////////////////////////////////////////////////////////////

void SG_vhash__get_nth_pair(
	SG_context* pCtx,
	const SG_vhash* pvh,
	SG_uint32 n,
	const char** psz_key,
	const SG_variant** ppResult
	)
{
	SG_NULLARGCHECK_RETURN(pvh);
	if (n >= pvh->count)
	{
		SG_ERR_THROW_RETURN(  SG_ERR_ARGUMENT_OUT_OF_RANGE  );
	}

    if (ppResult)
    {
        *ppResult = pvh->aItems[n].pVariant;
    }

	if (psz_key)
    {
		*psz_key = pvh->aItems[n].key;
    }
}

void SG_vhash__get_nth_pair__uint64(
	SG_context* pCtx,
	const SG_vhash* pvh,
	SG_uint32 n,
	const char** ppsz_key,
    SG_uint64* pi
	)
{
    const char* psz_key = NULL;
    const SG_variant* pv = NULL;
    SG_uint64 i64 = 0;

    SG_ERR_CHECK_RETURN(  SG_vhash__get_nth_pair(pCtx, pvh, n, &psz_key, &pv)  );
    SG_ERR_CHECK_RETURN(  SG_variant__get__uint64(pCtx, pv, &i64)  );

    if (ppsz_key)
    {
        *ppsz_key = psz_key;
    }
    if (pi)
    {
        *pi = i64;
    }
}

void SG_vhash__get_nth_pair__int32(
	SG_context* pCtx,
	const SG_vhash* pvh,
	SG_uint32 n,
	const char** ppsz_key,
	SG_int32* pi
	)
{
	const char* psz_key = NULL;
	const SG_variant* pv = NULL;
	SG_int64 i64 = 0;

	SG_ERR_CHECK_RETURN(  SG_vhash__get_nth_pair(pCtx, pvh, n, &psz_key, &pv)  );
	SG_ERR_CHECK_RETURN(  SG_variant__get__int64(pCtx, pv, &i64)  );
	if (SG_int64__fits_in_int32(i64))
	{
		if (pi)
			*pi = (SG_int32)i64;
	}
	else
	{
		SG_ERR_THROW_RETURN(  SG_ERR_INTEGER_OVERFLOW  );
	}

	if (ppsz_key)
	{
		*ppsz_key = psz_key;
	}
}

void SG_vhash__get_nth_pair__vhash(
	SG_context* pCtx,
	const SG_vhash* pvh,
	SG_uint32 n,
	const char** ppsz_key,
    SG_vhash** ppvh
	)
{
    const char* psz_key = NULL;
    const SG_variant* pv = NULL;
    SG_vhash* pvh_result = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__get_nth_pair(pCtx, pvh, n, &psz_key, &pv)  );
    SG_ERR_CHECK_RETURN(  SG_variant__get__vhash(pCtx, pv, &pvh_result)  );

    if (ppsz_key)
    {
        *ppsz_key = psz_key;
    }
    if (ppvh)
    {
        *ppvh = pvh_result;
    }
}

void SG_vhash__get_nth_pair__varray(
	SG_context* pCtx,
	const SG_vhash* pvh,
	SG_uint32 n,
	const char** ppsz_key,
    SG_varray** ppva
	)
{
    const char* psz_key = NULL;
    const SG_variant* pv = NULL;
    SG_varray* pva_result = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__get_nth_pair(pCtx, pvh, n, &psz_key, &pv)  );
    SG_ERR_CHECK_RETURN(  SG_variant__get__varray(pCtx, pv, &pva_result)  );

    if (ppsz_key)
    {
        *ppsz_key = psz_key;
    }
    if (ppva)
    {
        *ppva = pva_result;
    }
}

void SG_vhash__get_nth_pair__sz(
	SG_context* pCtx,
	const SG_vhash* pvh,
	SG_uint32 n,
	const char** ppsz_key,
    const char** ppsz_val
	)
{
    const char* psz_key = NULL;
    const SG_variant* pv = NULL;
    const char* psz_val = NULL;

    SG_ERR_CHECK_RETURN(  SG_vhash__get_nth_pair(pCtx, pvh, n, &psz_key, &pv)  );
    SG_ERR_CHECK_RETURN(  SG_variant__get__sz(pCtx, pv, &psz_val)  );

    if (ppsz_key)
    {
        *ppsz_key = psz_key;
    }
    if (ppsz_val)
    {
        *ppsz_val = psz_val;
    }
}

void SG_vhash__get_keys_as_rbtree(
	SG_context* pCtx,
	const SG_vhash* pvh,
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

void SG_vhash__get_keys(
	SG_context* pCtx,
	const SG_vhash* pvh,
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

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void SG_vhash_debug__dump(SG_context* pCtx, const SG_vhash * pvh, SG_string * pStrOut)
{
	SG_NULLARGCHECK_RETURN(pStrOut);

	if (pvh)
		SG_vhash__to_json(pCtx,pvh,pStrOut);
	else
		SG_string__append__sz(pCtx, pStrOut, "NULL");
}

/**
 * Dump a vhash to stderr.  You should be able to call this from GDB
 * or from the VS command window.
 */
void SG_vhash_debug__dump_to_console(SG_context* pCtx, const SG_vhash * pvh)
{
	SG_ERR_CHECK_RETURN(  SG_vhash_debug__dump_to_console__named(pCtx, pvh, NULL)  );
}

void SG_vhash_debug__dump_to_console__named(SG_context* pCtx, const SG_vhash * pvh, const char* pszName)
{
	SG_string * pStrOut = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStrOut)  );
	SG_ERR_CHECK(  SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvh, pStrOut)  );
	if (!pszName)
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR,"VHash:\n%s\n",SG_string__sz(pStrOut))  );
	else
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR,"%s:\n%s\n",pszName,SG_string__sz(pStrOut))  );

	SG_ERR_CHECK(  SG_console__flush(pCtx, SG_CS_STDERR)  );

	// fall thru to common cleanup

fail:
	SG_STRING_NULLFREE(pCtx, pStrOut);
}

#endif

void SG_vhash__path__get__vhash(
    SG_context* pCtx,
	const SG_vhash* pvh,
    SG_vhash** ppvh_result,
    SG_bool* pb,
    ...
    )
{
    SG_vhash* pvh_cur = NULL;
    va_list ap;

    va_start(ap, pb);

    pvh_cur = (SG_vhash*) pvh;
    do
    {
        const char* psz_key = va_arg(ap, const char*);
        SG_vhash* pvh_temp = NULL;
        SG_bool b = SG_FALSE;

        if (!psz_key)
        {
            break;
        }

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_cur, psz_key, &b)  );
        if (!b)
        {
            if (pb)
            {
                pvh_cur = NULL;
                break;
            }
            else
            {
                SG_ERR_THROW(  SG_ERR_NOT_FOUND  );
            }
        }

        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cur, psz_key, &pvh_temp)  );
        SG_ASSERT(pvh_temp);
        pvh_cur = pvh_temp;
    } while (1);

    if (pb)
    {
        *pb = pvh_cur ? SG_TRUE : SG_FALSE;
    }
    *ppvh_result = (SG_vhash*) pvh_cur;

    return;

fail:
    return;
}

static void SG_vhash__slashpath__parse(
    SG_context* pCtx,
	const char* psz_path,
    SG_varray** ppva
    )
{
    SG_varray* pva = NULL;
    const char* pcur = NULL;
    const char* pbegin = NULL;

    SG_NULLARGCHECK_RETURN(psz_path);

    SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva)  );

    pcur = psz_path;
    while (*pcur)
    {
        SG_uint32 len = 0;

        pbegin = pcur;
        while (*pcur && ('/' != *pcur))
        {
            pcur++;
        }

        len = (SG_uint32)(pcur - pbegin);
        if (len)
        {
            SG_ERR_CHECK(  SG_varray__append__string__buflen(pCtx, pva, pbegin, len)  );
        }

        if ('/' == *pcur)
        {
            pcur++;
        }
    }

    *ppva = pva;
    pva = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
}

static void SG_vhash__slashpath__walk(
    SG_context* pCtx,
	const SG_vhash* pvh,
	const char* psz_path,
    SG_bool b_create,
    SG_vhash** ppvh,
    const char** ppsz_leaf
    )
{
    SG_varray* pva = NULL;
    SG_vhash* pvh_cur = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    const char* psz_leaf = NULL;
    SG_uint32 len = 0;

    SG_NULLARGCHECK_RETURN(pvh);
	SG_NONEMPTYCHECK_RETURN(psz_path);
    SG_ARGCHECK_RETURN('/' != psz_path[0], psz_path);

    len = SG_STRLEN(psz_path);
    SG_ARGCHECK_RETURN('/' != psz_path[len-1], psz_path);

    psz_leaf = strrchr(psz_path, '/');
    if (psz_leaf)
    {
        psz_leaf++;
    }
    else
    {
        psz_leaf = psz_path;
    }

    SG_ERR_CHECK(  SG_vhash__slashpath__parse(pCtx, psz_path, &pva)  );
    pvh_cur = (SG_vhash*) pvh;
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    for (i=0; i<count; i++)
    {
        const char* psz = NULL;
        SG_vhash* pvh_next = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva, i, &psz)  );
        if (i == (count-1))
        {
            SG_ASSERT(0 == strcmp(psz_leaf, psz));
        }
        else
        {
            SG_bool b_has = SG_FALSE;

            SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_cur, psz, &b_has)  );
            if (b_has)
            {
                SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_cur, psz, &pvh_next)  );
            }
            else
            {
                if (b_create)
                {
                    SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvh_cur, psz, &pvh_next)  );
                }
            }
        }
        if (pvh_next)
        {
            pvh_cur = pvh_next;
        }
        else
        {
            break;
        }
    }

    *ppvh = pvh_cur;
    *ppsz_leaf = psz_leaf;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
}

void SG_vhash__slashpath__get__variant(
    SG_context* pCtx,
	const SG_vhash* pvh,
	const char* psz_path,
	const SG_variant** ppv
    )
{
    SG_vhash* pvh_container = NULL;
    const char* psz_leaf = NULL;
    const SG_variant* pv = NULL;

    SG_NULLARGCHECK_RETURN(ppv);

    SG_ERR_CHECK(  SG_vhash__slashpath__walk(
                pCtx,
                pvh,
                psz_path,
                SG_FALSE,
                &pvh_container,
                &psz_leaf
                )  );

    if (pvh_container && psz_leaf)
    {
        SG_bool b_has = SG_FALSE;

        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_container, psz_leaf, &b_has)  );
        if (b_has)
        {
            SG_ERR_CHECK(  SG_vhash__get__variant(pCtx, pvh_container, psz_leaf, &pv)  );
        }
    }

    *ppv = pv;

fail:
    ;
}

void SG_vhash__slashpath__update__string__sz(
	SG_context* pCtx,
	SG_vhash* pvh,
	const char* psz_path,
	const char* psz_val
	)
{
    SG_vhash* pvh_container = NULL;
    const char* psz_leaf = NULL;

    SG_NULLARGCHECK_RETURN(psz_val);

    SG_ERR_CHECK(  SG_vhash__slashpath__walk(
                pCtx,
                pvh,
                psz_path,
                SG_TRUE,
                &pvh_container,
                &psz_leaf
                )  );

    SG_ASSERT (pvh_container && psz_leaf);
    SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvh_container, psz_leaf, psz_val)  );

fail:
    ;
}

void SG_vhash__slashpath__update__varray(
	SG_context* pCtx,
	SG_vhash* pvh,
	const char* psz_path,
    SG_varray** ppva
	)
{
    SG_vhash* pvh_container = NULL;
    const char* psz_leaf = NULL;

    SG_NULLARGCHECK_RETURN(ppva);
    SG_NULLARGCHECK_RETURN(*ppva);

    SG_ERR_CHECK(  SG_vhash__slashpath__walk(
                pCtx,
                pvh,
                psz_path,
                SG_TRUE,
                &pvh_container,
                &psz_leaf
                )  );

    SG_ASSERT (pvh_container && psz_leaf);
    SG_ERR_CHECK(  SG_vhash__update__varray(pCtx, pvh_container, psz_leaf, ppva)  );

fail:
    ;
}

void SG_vhash__slashpath__update__vhash(
	SG_context* pCtx,
	SG_vhash* pvh,
	const char* psz_path,
    SG_vhash** ppvh
	)
{
    SG_vhash* pvh_container = NULL;
    const char* psz_leaf = NULL;

    SG_NULLARGCHECK_RETURN(ppvh);
    SG_NULLARGCHECK_RETURN(*ppvh);

    SG_ERR_CHECK(  SG_vhash__slashpath__walk(
                pCtx,
                pvh,
                psz_path,
                SG_TRUE,
                &pvh_container,
                &psz_leaf
                )  );

    SG_ASSERT (pvh_container && psz_leaf);
    SG_ERR_CHECK(  SG_vhash__update__vhash(pCtx, pvh_container, psz_leaf, ppvh)  );

fail:
    ;
}

//////////////////////////////////////////////////////////////////

static SG_vhash_foreach_callback _to_stringarray__keys_only__cb;
static void _to_stringarray__keys_only__cb(SG_context * pCtx,
										   void * pVoidData,
										   const SG_vhash * pvh,
										   const char * pszKey,
										   const SG_variant * pVariant)
{
	SG_stringarray * psa = (SG_stringarray *)pVoidData;

	SG_UNUSED( pvh );
	SG_UNUSED( pVariant );

	SG_ERR_CHECK_RETURN(  SG_stringarray__add(pCtx, psa, pszKey)  );
}
										   
void SG_vhash__to_stringarray__keys_only(SG_context * pCtx, const SG_vhash * pvh, SG_stringarray ** ppsa)
{
	SG_stringarray * psa = NULL;
	SG_uint32 count;

	SG_NULLARGCHECK_RETURN( pvh );
	SG_NULLARGCHECK_RETURN( ppsa );

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &count)  );
	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psa, count)  );
	SG_ERR_CHECK(  SG_vhash__foreach(pCtx, pvh, _to_stringarray__keys_only__cb, (void *)psa)  );

	*ppsa = psa;
	return;

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psa);
}

