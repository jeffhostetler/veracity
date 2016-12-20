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
 * @file sg_wc8api__private_prototypes.h
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC8API__PRIVATE_PROTOTYPES_H
#define H_SG_WC8API__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * The wc8api layer is defined as a convenience wrapper around
 * the full tx-based wc7txapi layer.
 *
 * This wrapper template is used by routines that optionally
 * alter the working directory and optionally return a journal.
 * (Routines that want to present a --test and/or --verbose option.)
 *
 */
#define SG_WC__WRAPPER_TEMPLATE__tj(pPathWc, bTest, ppvaJournal, code)	\
	do																	\
	{																	\
		SG_wc_tx * pWcTx = NULL;										\
		SG_varray * pvaAllocated = NULL;								\
																		\
		SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pPathWc, SG_FALSE)  ); \
																		\
		code;															\
																		\
		if ((ppvaJournal))												\
			SG_ERR_CHECK(  SG_WC_TX__JOURNAL__ALLOC_COPY(pCtx, pWcTx,	\
														 &pvaAllocated)  ); \
																		\
		if ((bTest))													\
			SG_ERR_CHECK(  SG_wc_tx__cancel(pCtx, pWcTx)  );			\
		else															\
			SG_ERR_CHECK(  SG_wc_tx__apply(pCtx, pWcTx)  );				\
																		\
		SG_ERR_CHECK(  SG_wc_tx__free(pCtx, pWcTx)  );					\
																		\
		if ((ppvaJournal))												\
			*ppvaJournal = pvaAllocated;								\
																		\
		return;															\
																		\
	fail:																\
		SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );				\
		SG_WC_TX__NULLFREE(pCtx, pWcTx);								\
		SG_VARRAY_NULLFREE(pCtx, pvaAllocated);							\
	}																	\
	while (0)
	

//////////////////////////////////////////////////////////////////

/**
 * The wc8api layer is defined as a convenience wrapper around
 * the full tx-based wc7txapi layer.
 *
 * This wrapper template is used by READ-ONLY routines that BEGIN
 * a TX, execute a single chunk of code, and CANCEL the TX.
 *
 */
#define SG_WC__WRAPPER_TEMPLATE__ro(pPathWc, code)						\
	do																	\
	{																	\
		SG_wc_tx * pWcTx = NULL;										\
																		\
		SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, ((pPathWc)), SG_TRUE)  ); \
																		\
		code;															\
																		\
		SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );				\
		SG_WC_TX__NULLFREE(pCtx, pWcTx);								\
																		\
		return;															\
																		\
	fail:																\
		SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );				\
		SG_WC_TX__NULLFREE(pCtx, pWcTx);								\
	}																	\
	while (0)

//////////////////////////////////////////////////////////////////

/**
 * The wc8api layer is defined as a convenience wrapper around
 * the full tx-based wc7txapi layer.
 *
 * This wrapper template is used by READ-WRITE routines that BEGIN
 * a TX, execute a single chunk of code, and APPLY the TX.
 *
 */
#define SG_WC__WRAPPER_TEMPLATE__rw(pPathWc, code)						\
	do																	\
	{																	\
		SG_wc_tx * pWcTx = NULL;										\
																		\
		SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pPathWc, SG_FALSE)  ); \
																		\
		code;															\
																		\
		SG_ERR_IGNORE(  SG_wc_tx__apply(pCtx, pWcTx)  );				\
		SG_WC_TX__NULLFREE(pCtx, pWcTx);								\
																		\
		return;															\
																		\
	fail:																\
		SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );				\
		SG_WC_TX__NULLFREE(pCtx, pWcTx);								\
	}																	\
	while (0)

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC8API__PRIVATE_PROTOTYPES_H
