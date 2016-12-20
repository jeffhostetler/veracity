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
 * @file sg_bitvector_prototypes.h
 *
 * @details A growable bit-vector.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_BITVECTOR_PROTOTYPES_H
#define H_SG_BITVECTOR_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_bitvector__alloc(SG_context* pCtx,
						 SG_bitvector ** ppBitVectorNew,
						 SG_uint32 suggestedInitialSize);

void SG_bitvector__alloc__copy(SG_context* pCtx,
							   SG_bitvector ** ppBitVectorNew,
							   const SG_bitvector * pBitVectorSrc);

#if defined(DEBUG)
#define SG_BITVECTOR__ALLOC(pCtx,ppBitVector,s)							SG_STATEMENT(  SG_bitvector * _p = NULL;                                      \
																					   SG_bitvector__alloc(pCtx,&_p,s);                               \
																					   _sg_mem__set_caller_data(_p,__FILE__,__LINE__,"SG_bitvector"); \
																					   *(ppBitVector) = _p;                                           )

#define SG_BITVECTOR__ALLOC__COPY(pCtx,ppBitVectorNew,pBitVectorSrc)	SG_STATEMENT(  SG_bitvector * _p = NULL;                                      \
																					   SG_bitvector__alloc__copy(pCtx,&_p,(pBitVectorSrc));           \
																					   _sg_mem__set_caller_data(_p,__FILE__,__LINE__,"SG_bitvector"); \
																					   *(ppBitVectorNew) = _p;                                        )

#else
#define SG_BITVECTOR__ALLOC(pCtx,ppBitVector,s)                 		SG_bitvector__alloc(pCtx,ppBitVector,s)

#define SG_BITVECTOR__ALLOC__COPY(pCtx,ppBitVectorNew,pBitVectorSrc)	SG_bitvector__alloc__copy(pCtx,ppBitVectorNew,(pBitVectorSrc))
#endif


void SG_bitvector__free(SG_context * pCtx, SG_bitvector * pBitVector);

void SG_bitvector__zero(SG_context* pCtx,
						SG_bitvector * pBitVector);

void SG_bitvector__length(SG_context* pCtx,
						  const SG_bitvector * pBitVector,
						  SG_uint32 * pLength);

void SG_bitvector__get_bit(SG_context* pCtx,
						   const SG_bitvector * pBitVector,
						   SG_uint32 bit_nr,
						   SG_bool bValueIfUndefined,
						   SG_bool * pbValue);

void SG_bitvector__set_bit(SG_context* pCtx,
						   SG_bitvector * pBitVector,
						   SG_uint32 bit_nr,
						   SG_bool bValue);

void SG_bitvector__append_bit(SG_context * pCtx,
							  SG_bitvector * pBitVector,
							  SG_bool bValue,
							  SG_uint32 * pIndexReturned);

void SG_bitvector__count_set_bits(SG_context * pCtx,
								  const SG_bitvector * pBitVector,
								  SG_uint32 * pCount);

void SG_bitvector_debug__to_sz(SG_context * pCtx,
							   const SG_bitvector * pBitVector,
							   char ** ppsz);

void SG_bitvector__assign__bv__eq__bv(SG_context * pCtx,
									  SG_bitvector * pBitVector_LValue,
									  const SG_bitvector * pBitVector_RValue);

void SG_bitvector__assign__bv__or_eq__bv(SG_context * pCtx,
										 SG_bitvector * pBitVector_LValue,
										 const SG_bitvector * pBitVector_RValue);

void SG_bitvector__assign__bv__and_eq__not__bv(SG_context * pCtx,
											   SG_bitvector * pBitVector_LValue,
											   const SG_bitvector * pBitVector_RValue);

void SG_bitvector__operator__bv__eq_eq__bv(SG_context * pCtx,
										   const SG_bitvector * pBitVector_1,
										   const SG_bitvector * pBitVector_2,
										   SG_bool * pbEqual);

void SG_bitvector__operator__bv__eq_eq__zero(SG_context * pCtx,
											 const SG_bitvector * pBitVector,
											 SG_bool * pbEqualZero);

void SG_bitvector__operator__bv__and__bv__eq_eq__zero(SG_context * pCtx,
													  const SG_bitvector * pBitVector_1,
													  const SG_bitvector * pBitVector_2,
													  SG_bool * pbEqualZero);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_BITVECTOR_PROTOTYPES_H
