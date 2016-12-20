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

/**
 *
 * @file sg_bitvector.c
 *
 * @details A growable bit-vector.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

//////////////////////////////////////////////////////////////////

struct _SG_bitvector
{
	SG_vector_i64 *		pVec64;
	SG_uint32			nrBitsInUse;
};

//////////////////////////////////////////////////////////////////

void SG_bitvector__alloc(SG_context* pCtx,
						 SG_bitvector ** ppBitVectorNew,
						 SG_uint32 suggestedInitialSize)
{
	SG_bitvector * pBitVectorNew = NULL;

	SG_NULLARGCHECK_RETURN(ppBitVectorNew);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pBitVectorNew)  );
	SG_ERR_CHECK(  SG_VECTOR_I64__ALLOC(pCtx, &pBitVectorNew->pVec64, ((suggestedInitialSize+63)/64))  );
	pBitVectorNew->nrBitsInUse = 0;

	*ppBitVectorNew = pBitVectorNew;

	return;

fail:
	SG_BITVECTOR_NULLFREE(pCtx, pBitVectorNew);
}

void SG_bitvector__alloc__copy(SG_context* pCtx,
							   SG_bitvector ** ppBitVectorNew,
							   const SG_bitvector * pBitVectorSrc)
{
	SG_bitvector * pBitVectorNew = NULL;

	SG_NULLARGCHECK_RETURN(ppBitVectorNew);
	SG_NULLARGCHECK_RETURN(pBitVectorSrc);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pBitVectorNew)  );
	SG_ERR_CHECK(  SG_VECTOR_I64__ALLOC__COPY(pCtx,
											  &pBitVectorNew->pVec64,
											  pBitVectorSrc->pVec64)  );
	pBitVectorNew->nrBitsInUse = pBitVectorSrc->nrBitsInUse;

	*ppBitVectorNew = pBitVectorNew;

	return;

fail:
	SG_BITVECTOR_NULLFREE(pCtx, pBitVectorNew);
}

void SG_bitvector__free(SG_context * pCtx, SG_bitvector * pBitVector)
{
	if (!pBitVector)
		return;

	SG_VECTOR_I64_NULLFREE(pCtx, pBitVector->pVec64);
	pBitVector->nrBitsInUse = 0;
	SG_NULLFREE(pCtx, pBitVector);
}

//////////////////////////////////////////////////////////////////

void SG_bitvector__zero(SG_context* pCtx,
						SG_bitvector * pBitVector)
{
	SG_NULLARGCHECK_RETURN(pBitVector);

	// TODO 2011/03/17 Should this change the defined length of the bitvector
	// TODO            or just set the defined bits to 0 ?

	SG_ERR_CHECK_RETURN(  SG_vector_i64__clear(pCtx, pBitVector->pVec64)  );
	pBitVector->nrBitsInUse = 0;
}

//////////////////////////////////////////////////////////////////

void SG_bitvector__length(SG_context* pCtx,
						  const SG_bitvector * pBitVector,
						  SG_uint32 * pLength)
{
	SG_NULLARGCHECK_RETURN(pBitVector);
	SG_NULLARGCHECK_RETURN(pLength);

	*pLength = pBitVector->nrBitsInUse;
}

//////////////////////////////////////////////////////////////////

static void _sg_bitvector__bit_nr__to__parts(SG_uint32 bit_nr,
											 SG_uint32 * p_ndx_word,
											 SG_uint64 * p_bit_mask_in_word)
{
	SG_uint32 ndx_word = bit_nr / 64;
	SG_uint32 ndx_bit  = bit_nr % 64;
	SG_uint64 bit_mask_in_word = 0x8000000000000000ULL >> ndx_bit;		// use MSB numbering

	*p_ndx_word = ndx_word;
	*p_bit_mask_in_word = bit_mask_in_word;
}

//////////////////////////////////////////////////////////////////

void SG_bitvector__get_bit(SG_context* pCtx,
						   const SG_bitvector * pBitVector,
						   SG_uint32 bit_nr,
						   SG_bool bValueIfUndefined,
						   SG_bool * pbValue)
{
	SG_NULLARGCHECK_RETURN(pBitVector);
	SG_NULLARGCHECK_RETURN(pbValue);

	if (bit_nr < pBitVector->nrBitsInUse)
	{
		SG_uint32 ndx_word;
		SG_uint64 bit_mask_in_word;
		SG_uint64 i64value;

		SG_ERR_CHECK_RETURN(  _sg_bitvector__bit_nr__to__parts(bit_nr,
															   &ndx_word, &bit_mask_in_word)  );
		SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector->pVec64, ndx_word, (SG_int64 *)&i64value)  );
		*pbValue = ((i64value & bit_mask_in_word) != 0);
	}
	else
	{
		*pbValue = bValueIfUndefined;
	}
}

void SG_bitvector__set_bit(SG_context* pCtx,
						   SG_bitvector * pBitVector,
						   SG_uint32 bit_nr,
						   SG_bool bValue)
{
	SG_uint32 ndx_word;
	SG_uint64 bit_mask_in_word;
	SG_uint64 i64value;

	SG_NULLARGCHECK_RETURN(pBitVector);

	SG_ERR_CHECK_RETURN(  _sg_bitvector__bit_nr__to__parts(bit_nr,
														   &ndx_word, &bit_mask_in_word)  );

	// see if the bit we are about to set will be in a new int64
	// and extend the underlying vector_i64.

	SG_ERR_CHECK_RETURN(  SG_vector_i64__reserve(pCtx, pBitVector->pVec64, ndx_word+1)  );
	SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector->pVec64, ndx_word, (SG_int64 *)&i64value)  );
	if (bValue)
		i64value |= bit_mask_in_word;
	else
		i64value &= ~bit_mask_in_word;
	SG_ERR_CHECK_RETURN(  SG_vector_i64__set(pCtx, pBitVector->pVec64, ndx_word, i64value)  );

	pBitVector->nrBitsInUse = SG_MAX(pBitVector->nrBitsInUse, (bit_nr + 1));
}

//////////////////////////////////////////////////////////////////

void SG_bitvector__append_bit(SG_context * pCtx,
							  SG_bitvector * pBitVector,
							  SG_bool bValue,
							  SG_uint32 * pIndexReturned)
{
	SG_uint32 bit_nr = pBitVector->nrBitsInUse;

	SG_ERR_CHECK_RETURN(  SG_bitvector__set_bit(pCtx,
												pBitVector,
												bit_nr,
												bValue)  );

	if (pIndexReturned)
		*pIndexReturned = bit_nr;
}

//////////////////////////////////////////////////////////////////

void SG_bitvector_debug__to_sz(SG_context * pCtx,
							   const SG_bitvector * pBitVector,
							   char ** ppsz)
{
	char * psz = NULL;
	SG_uint32 k;

	SG_NULLARGCHECK_RETURN(pBitVector);
	SG_NULLARGCHECK_RETURN(ppsz);

	// yes, there are better/faster ways to do this, but
	// this way lets me debug things a little.

	SG_ERR_CHECK(  SG_allocN(pCtx, (pBitVector->nrBitsInUse+1), psz)  );
	for (k=0; k<pBitVector->nrBitsInUse; k++)
	{
		SG_bool b_k;

		SG_ERR_CHECK(  SG_bitvector__get_bit(pCtx, pBitVector, k, SG_FALSE, &b_k)  );
		psz[k] = ((b_k) ? '1' : '0');
	}

	*ppsz = psz;
	return;

fail:
	SG_NULLFREE(pCtx, psz);
}

//////////////////////////////////////////////////////////////////

void SG_bitvector__assign__bv__eq__bv(SG_context * pCtx,
									  SG_bitvector * pBitVector_LValue,
									  const SG_bitvector * pBitVector_RValue)
{
	// lvalue = rvalue
	//
	// straight assignment of bits.

	SG_uint32 k;

	SG_NULLARGCHECK_RETURN( pBitVector_LValue );
	SG_NULLARGCHECK_RETURN( pBitVector_RValue );

	// truncate lvalue first.

	SG_ERR_CHECK_RETURN(  SG_vector_i64__clear(pCtx, pBitVector_LValue->pVec64)  );

	for (k=0; k<pBitVector_RValue->nrBitsInUse; k++)
	{
		SG_bool b_k;

		SG_ERR_CHECK_RETURN(  SG_bitvector__get_bit(pCtx, pBitVector_RValue, k, SG_FALSE, &b_k)  );
		SG_ERR_CHECK_RETURN(  SG_bitvector__set_bit(pCtx, pBitVector_LValue, k, b_k)  );
	}
	
// TODO 2010/03/10 consider using something like the following instead:
//	SG_uint32 nr_words_in_rvalue;
//	SG_ERR_CHECK_RETURN(  SG_vector_i64__length(pCtx, pBitVector_RValue->pVec64, &nr_words_in_rvalue)  );
//	SG_ERR_CHECK_RETURN(  SG_vector_i64__reserve(pCtx, pBitVector_LValue->pVec64, nr_words_in_rvalue)  );
//	
//	for (k=0; k<nr_words_in_rvalue; k++)
//	{
//		SG_uint64 rv_k;
//
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector_RValue->pVec64, k, &rv_k)  );
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__set(pCtx, pBitVector_LValue->pVec64, k, rv_k)  );
//	}
//
//	pBitVector_LValue->nrBitsInUse = pBitVector_RValue->nrBitsInUse;
}

void SG_bitvector__assign__bv__or_eq__bv(SG_context * pCtx,
										 SG_bitvector * pBitVector_LValue,
										 const SG_bitvector * pBitVector_RValue)
{
	// lvalue |= rvalue
	//
	// if the vectors have different length, assume zeros.

	SG_uint32 k;
	
	SG_NULLARGCHECK_RETURN( pBitVector_LValue );
	SG_NULLARGCHECK_RETURN( pBitVector_RValue );

	for (k=0; k<pBitVector_RValue->nrBitsInUse; k++)
	{
		SG_bool bl_k, br_k;

		SG_ERR_CHECK_RETURN(  SG_bitvector__get_bit(pCtx, pBitVector_LValue, k, SG_FALSE, &bl_k)  );
		SG_ERR_CHECK_RETURN(  SG_bitvector__get_bit(pCtx, pBitVector_RValue, k, SG_FALSE, &br_k)  );
		
		SG_ERR_CHECK_RETURN(  SG_bitvector__set_bit(pCtx, pBitVector_LValue, k, (bl_k | br_k))  );
	}
	
// TODO 2010/03/10 consider using something like the following instead:
//	SG_uint32 nr_words_in_lvalue;
//	SG_uint32 nr_words_in_rvalue;
//
//	SG_ERR_CHECK_RETURN(  SG_vector_i64__length(pCtx, pBitVector_LValue->pVec64, &nr_words_in_lvalue)  );
//	SG_ERR_CHECK_RETURN(  SG_vector_i64__length(pCtx, pBitVector_RValue->pVec64, &nr_words_in_rvalue)  );
//
//	// zero-extend lvalue if it is shorter than rvalue
//
//	if (nr_words_in_lvalue < nr_words_in_rvalue)
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__reserve(pCtx, pBitVector_LValue->pVec64, nr_words_in_rvalue)  );
//	
//	for (k=0; k<nr_words_in_rvalue; k++)
//	{
//		SG_uint64 lv_k, rv_k;
//
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector_LValue->pVec64, k, &lv_k)  );
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector_RValue->pVec64, k, &rv_k)  );
//
//		lv_k |= rv_k;
//		
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__set(pCtx, pBitVector_LValue->pVec64, k, lv_k)  );
//	}
//
//	pBitVector_LValue->nrBitsInUse = SG_MAX( pBitVector_LValue->nrBitsInUse,
//											 pBitVector_RValue->nrBitsInUse );
}

void SG_bitvector__assign__bv__and_eq__not__bv(SG_context * pCtx,
											   SG_bitvector * pBitVector_LValue,
											   const SG_bitvector * pBitVector_RValue)
{
	// lvalue &= ~rvalue
	//
	// if vectors have different length, assume zeros.

	SG_uint32 k;

	SG_NULLARGCHECK_RETURN( pBitVector_LValue );
	SG_NULLARGCHECK_RETURN( pBitVector_RValue );

	for (k=0; k<pBitVector_RValue->nrBitsInUse; k++)
	{
		SG_bool bl_k, br_k;

		SG_ERR_CHECK_RETURN(  SG_bitvector__get_bit(pCtx, pBitVector_LValue, k, SG_FALSE, &bl_k)  );
		SG_ERR_CHECK_RETURN(  SG_bitvector__get_bit(pCtx, pBitVector_RValue, k, SG_FALSE, &br_k)  );

		SG_ERR_CHECK_RETURN(  SG_bitvector__set_bit(pCtx, pBitVector_LValue, k, (bl_k & ~br_k))  );
	}
	
// TODO 2010/03/10 consider using something like the following instead:
//	SG_uint32 nr_words_in_lvalue;
//	SG_uint32 nr_words_in_rvalue;
//
//	SG_ERR_CHECK_RETURN(  SG_vector_i64__length(pCtx, pBitVector_LValue->pVec64, &nr_words_in_lvalue)  );
//	SG_ERR_CHECK_RETURN(  SG_vector_i64__length(pCtx, pBitVector_RValue->pVec64, &nr_words_in_rvalue)  );
//
//	// zero-extend lvalue if it is shorter than rvalue
//
//	if (nr_words_in_lvalue < nr_words_in_rvalue)
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__reserve(pCtx, pBitVector_LValue->pVec64, nr_words_in_rvalue)  );
//	
//	for (k=0; k<nr_words_in_rvalue; k++)
//	{
//		SG_uint64 lv_k, rv_k;
//
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector_LValue->pVec64, k, &lv_k)  );
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector_RValue->pVec64, k, &rv_k)  );
//
//		lv_k &= ~rv_k;
//
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__set(pCtx, pBitVector_LValue->pVec64, k, lv_k)  );
//	}
//
//	pBitVector_LValue->nrBitsInUse = SG_MAX( pBitVector_LValue->nrBitsInUse,
//											 pBitVector_RValue->nrBitsInUse );
}

//////////////////////////////////////////////////////////////////

void SG_bitvector__operator__bv__eq_eq__bv(SG_context * pCtx,
										   const SG_bitvector * pBitVector_1,
										   const SG_bitvector * pBitVector_2,
										   SG_bool * pbEqual)
{
	// return (bv1 == bv2);
	//
	// since we allow zero-extension we shouldn't short-cut compare nrBitsInUse

	SG_uint32 nrBitsMax;
	SG_uint32 k;

	SG_NULLARGCHECK_RETURN( pBitVector_1 );
	SG_NULLARGCHECK_RETURN( pBitVector_2 );
	SG_NULLARGCHECK_RETURN( pbEqual );

	nrBitsMax = SG_MAX( pBitVector_1->nrBitsInUse,
						pBitVector_2->nrBitsInUse );
	for (k=0; k<nrBitsMax; k++)
	{
		SG_bool bl_k, br_k;

		SG_ERR_CHECK_RETURN(  SG_bitvector__get_bit(pCtx, pBitVector_1, k, SG_FALSE, &bl_k)  );
		SG_ERR_CHECK_RETURN(  SG_bitvector__get_bit(pCtx, pBitVector_2, k, SG_FALSE, &br_k)  );

		if (bl_k != br_k)
		{
			*pbEqual = SG_FALSE;
			return;
		}
	}

	*pbEqual = SG_TRUE;

// TODO 2010/03/10 consider using something like the following instead:
//	SG_uint32 nr_words_in_1;
//	SG_uint32 nr_words_in_2;
//	SG_uint32 nr_words_min;
//	SG_uint32 nr_words_max;
//	SG_uint32 k;
//	const SG_bitvector * pBitVector_longer = NULL;
//
//	SG_NULLARGCHECK_RETURN( pBitVector_1 );
//	SG_NULLARGCHECK_RETURN( pBitVector_2 );
//
//	// because we don't have a method to shorten a
//	// bitvector (other than __zero) and we always
//	// zero-init when a new word is added to pVec64,
//	// we can do whole word compares here knowning
//	// that any bits in the word beyond the nrBitsInUse
//	// will be zero.
//
//	SG_ERR_CHECK_RETURN(  SG_vector_i64__length(pCtx, pBitVector_1->pVec64, &nr_words_in_1)  );
//	SG_ERR_CHECK_RETURN(  SG_vector_i64__length(pCtx, pBitVector_2->pVec64, &nr_words_in_2)  );
//	if (nr_words_in_1 > nr_words_in_2)
//	{
//		pBitVector_longer = pBitVector_1;
//		nr_words_max = nr_words_in_1;
//		nr_words_min = nr_words_in_2;
//	}
//	else if (nr_words_in_2 > nr_words_in_1)
//	{
//		pBitVector_longer = pBitVector_2;
//		nr_words_max = nr_words_in_2;
//		nr_words_min = nr_words_in_1;
//	}
//
//	for (k=0; k<nr_words_min; k++)
//	{
//		SG_uint64 v1_k, v2_k;
//
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector_1->pVec64, k, &v1_k)  );
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector_2->pVec64, k, &v2_k)  );
//
//		if (v1_k != v2_k)
//		{
//			*pbEqual = SG_FALSE;
//			return;
//		}
//	}
//
//	if (pBitVector_longer)
//	{
//		for (k=nr_words_min; k<nr_words_max; k++)
//		{
//			SG_uint64 v_k;
//
//			SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector_longer->pVec64, k, &v_k)  );
//
//			if (v_k != 0)
//			{
//				*pbEqual = SG_FALSE;
//				return;
//			}
//		}
//	}
//	
//	*pbEqual = SG_TRUE;
}

//////////////////////////////////////////////////////////////////

void SG_bitvector__operator__bv__eq_eq__zero(SG_context * pCtx,
											 const SG_bitvector * pBitVector,
											 SG_bool * pbEqualZero)
{
	// return (bv == 0);

	SG_uint32 k;

	SG_NULLARGCHECK_RETURN( pBitVector );

	for (k=0; k<pBitVector->nrBitsInUse; k++)
	{
		SG_bool b_k;

		SG_ERR_CHECK_RETURN(  SG_bitvector__get_bit(pCtx, pBitVector, k, SG_FALSE, &b_k)  );

		if (b_k != SG_FALSE)
		{
			*pbEqualZero = SG_FALSE;
			return;
		}
	}

	*pbEqualZero = SG_TRUE;
	
// TODO 2010/03/10 consider using something like the following instead:
//	SG_uint32 nr_words;
//	// because we don't have a method to shorten a
//	// bitvector (other than __zero) and we always
//	// zero-init when a new word is added to pVec64,
//	// we can do whole word compares here knowning
//	// that any bits in the word beyond the nrBitsInUse
//	// will be zero.
//
//	SG_ERR_CHECK_RETURN(  SG_vector_i64__length(pCtx, pBitVector->pVec64, &nr_words)  );
//	for (k=0; k<nr_words; k++)
//	{
//		SG_uint64 v_k;
//
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector->pVec64, k, &v_k)  );
//
//		if (v_k != 0)
//		{
//			*pbEqualZero = SG_FALSE;
//			return;
//		}
//	}
//
//	*pbEqualZero = SG_TRUE;
}

//////////////////////////////////////////////////////////////////

void SG_bitvector__operator__bv__and__bv__eq_eq__zero(SG_context * pCtx,
													  const SG_bitvector * pBitVector_1,
													  const SG_bitvector * pBitVector_2,
													  SG_bool * pbEqualZero)
{
	// return ((bv1 & bv2) == 0)

	SG_uint32 nrBitsMax;
	SG_uint32 k;

	SG_NULLARGCHECK_RETURN( pBitVector_1 );
	SG_NULLARGCHECK_RETURN( pBitVector_2 );
	SG_NULLARGCHECK_RETURN( pbEqualZero );

	nrBitsMax = SG_MAX( pBitVector_1->nrBitsInUse,
						pBitVector_2->nrBitsInUse );
	for (k=0; k<nrBitsMax; k++)
	{
		SG_bool bl_k, br_k;

		SG_ERR_CHECK_RETURN(  SG_bitvector__get_bit(pCtx, pBitVector_1, k, SG_FALSE, &bl_k)  );
		SG_ERR_CHECK_RETURN(  SG_bitvector__get_bit(pCtx, pBitVector_2, k, SG_FALSE, &br_k)  );

		if ((bl_k & br_k) != SG_FALSE)
		{
			*pbEqualZero = SG_FALSE;
			return;
		}
	}

	*pbEqualZero = SG_TRUE;


//
//	SG_uint32 nr_words_in_1;
//	SG_uint32 nr_words_in_2;
//	SG_uint32 nr_words_min;
//	SG_uint32 nr_words_max;
//	const SG_bitvector * pBitVector_longer = NULL;
//
//
//	// because we don't have a method to shorten a
//	// bitvector (other than __zero) and we always
//	// zero-init when a new word is added to pVec64,
//	// we can do whole word compares here knowning
//	// that any bits in the word beyond the nrBitsInUse
//	// will be zero.
//
//	SG_ERR_CHECK_RETURN(  SG_vector_i64__length(pCtx, pBitVector_1->pVec64, &nr_words_in_1)  );
//	SG_ERR_CHECK_RETURN(  SG_vector_i64__length(pCtx, pBitVector_2->pVec64, &nr_words_in_2)  );
//	if (nr_words_in_1 > nr_words_in_2)
//	{
//		pBitVector_longer = pBitVector_1;
//		nr_words_max = nr_words_in_1;
//		nr_words_min = nr_words_in_2;
//	}
//	else if (nr_words_in_2 > nr_words_in_1)
//	{
//		pBitVector_longer = pBitVector_2;
//		nr_words_max = nr_words_in_2;
//		nr_words_min = nr_words_in_1;
//	}
//
//	for (k=0; k<nr_words_min; k++)
//	{
//		SG_uint64 v1_k, v2_k;
//
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector_1->pVec64, k, &v1_k)  );
//		SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector_2->pVec64, k, &v2_k)  );
//
//		if ((v1_k & v2_k) != 0)
//		{
//			*pbEqual = SG_FALSE;
//			return;
//		}
//	}
//
//	if (pBitVector_longer)
//	{
//		for (k=nr_words_min; k<nr_words_max; k++)
//		{
//			SG_uint64 v_k;
//
//			SG_ERR_CHECK_RETURN(  SG_vector_i64__get(pCtx, pBitVector_longer->pVec64, k, &v_k)  );
//
//			if (v_k != 0)
//			{
//				*pbEqual = SG_FALSE;
//				return;
//			}
//		}
//	}
//	
//	*pbEqual = SG_TRUE;
}

void SG_bitvector__count_set_bits(SG_context * pCtx,
								  const SG_bitvector * pBitVector,
								  SG_uint32 * pCount)
{
	SG_uint32 sum = 0;
	SG_uint32 k;

	for (k=0; k<pBitVector->nrBitsInUse; k++)
	{
		SG_bool b_k;

		SG_ERR_CHECK_RETURN(  SG_bitvector__get_bit(pCtx, pBitVector, k, SG_FALSE, &b_k)  );
		if (b_k)
			sum++;
	}

	*pCount = sum;
}

