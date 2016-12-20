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
 * @file sg_vector_prototypes.h
 *
 * @details
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VECTOR_PROTOTYPES_H
#define H_SG_VECTOR_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Allocate a new vector.  This is a variable-length/growable array of pointers.
 *
 * The suggested-initial-size is a guess at how much space to initially allocate
 * in the internal array; this (along with SG_vector__setChunkSize()) allow you
 * to control how often we have to realloc() internally.  This only controls the
 * alloc() -- it does not affect the in-use length of the vector.
 *
 * Regardless of the allocated size, the vector initially has an in-use length of 0.
 */
void SG_vector__alloc(SG_context* pCtx, SG_vector ** ppVector, SG_uint32 suggestedInitialSize);

/**
 * Allocates a new vector that is a copy of another vector.
 * Individual items in the vector will be copied using the given copy callback.
 * If the copy callback is NULL, then just the item pointers will be copied, resulting in a shallow copy.
 * The free callback is only used to free copied items in the event of an error.
 * If the free callback is NULL and the copy callback returns newly allocated memory, then memory leaks may occur if there's an error.
 */
void SG_vector__alloc__copy(SG_context* pCtx, const SG_vector* pInputVector, SG_copy_callback fCopyCallback, SG_free_callback fFreeCallback, SG_vector** ppOutputVector);

#if defined(DEBUG)
#define __SG_VECTOR__ALLOC__(ppOutput, pVarName, AllocExpression)            \
	SG_STATEMENT(                                                            \
		SG_vector* pVarName = NULL;                                          \
		AllocExpression;                                                     \
		_sg_mem__set_caller_data(pVarName, __FILE__, __LINE__, "SG_vector"); \
		*(ppOutput) = pVarName;                                              \
	)
#define SG_VECTOR__ALLOC(pCtx, ppVector, uInitialSize)               __SG_VECTOR__ALLOC__(ppVector, pAlloc, SG_vector__alloc(pCtx, &pAlloc, uInitialSize))
#define SG_VECTOR__ALLOC__COPY(pCtx, pInput, fCopy, fFree, ppOutput) __SG_VECTOR__ALLOC__(ppOutput, pAlloc, SG_vector__alloc__copy(pCtx, pInput, fCopy, fFree, &pAlloc))
#else
#define SG_VECTOR__ALLOC(pCtx, ppVector, uInitialSize)               SG_vector__alloc(pCtx,ppVector,uInitialSize)
#define SG_VECTOR__ALLOC__COPY(pCtx, pInput, fCopy, fFree, ppOutput) SG_vector__alloc__copy(pCtx, pInput, fCopy, fFree, ppOutput)
#endif

/**
 * Free the given vector.  Since the vector does not own the pointers in the vector,
 * it only frees the vector itself and our internal array.
 */
void SG_vector__free(SG_context * pCtx, SG_vector * pVector);

/**
 * Free the given vector.  But first call the given callback function for each
 * element in the vector.
 */
void SG_vector__free__with_assoc(SG_context * pCtx, SG_vector * pVector, SG_free_callback * pcbFreeValue);

/**
 * Extend the in-use length of the vector and store the given pointer in the new cell.
 * Return the index of the new cell.
 *
 * We store the pointer but we do not take ownership of it.
 */
void SG_vector__append(SG_context* pCtx, SG_vector * pVector, void * pValue, SG_uint32 * pIndexReturned);

/**
 * Remove the last pointer in the list.
 * Return it in the optional ppValue output parameter.
 */
void SG_vector__pop_back(SG_context * pCtx, SG_vector * pVector, void ** ppValue);

/**
 * Set the in-use length to a known size.  This is like a traditional
 * dimension statement on a regular array.  Space is allocated and the
 * in-use length is set.  Newly created cells are initialized to NULL.
 *
 * Use this with __get() and __set() if this makes more sense than calling
 * __append() in your application.
 *
 * This *CANNOT* be used to shorten the in-use length of a vector.
 */
void SG_vector__reserve(SG_context* pCtx, SG_vector * pVector, SG_uint32 size);

/**
 * Fetch the pointer value in cell k in the vector.
 * This returns an error if k is >= the in-use length.
 */
void SG_vector__get(SG_context* pCtx, const SG_vector * pVector, SG_uint32 k, void ** ppValue);

/**
 * Set the pointer value in call k in the vector.
 * This returns an error if k is >= the in-use-length.
 */
void SG_vector__set(SG_context* pCtx, SG_vector * pVector, SG_uint32 k, void * pValue);

/**
 * Set the in-use length to 0.  This does not realloc() down.
 */
void SG_vector__clear(SG_context* pCtx, SG_vector * pVector);

/**
 * As clear, but first calls the given callback function for each element.
 * This allows the caller to free the individual elements in the vector, if they were previously allocated.
 */
void SG_vector__clear__with_assoc(SG_context * pCtx, SG_vector * pVector, SG_free_callback * pcbFreeValue);

/**
 * Returns the in-use length.
 */
void SG_vector__length(SG_context* pCtx, const SG_vector * pVector, SG_uint32 * pLength);

/**
 * Sets the chunk-size for our internal calls to realloc().
 */
void SG_vector__setChunkSize(SG_context* pCtx, SG_vector * pVector, SG_uint32 size);

/**
 * Call the given function with each item in the vector.
 */
void SG_vector__foreach(SG_context * pCtx,
						SG_vector * pVector,
						SG_vector_foreach_callback * pfn,
						void * pVoidCallerData);

/**
 * Finds the first item in the vector that matches a predicate.
 */
void SG_vector__find__first(
	SG_context*          pCtx,        //< [in] [out] Error and context info.
	SG_vector*           pVector,     //< [in] The vector to search.
	SG_vector_predicate* fPredicate,  //< [in] The predicate to match items against.
	void*                pUserData,   //< [in] User data to pass to the predicate.
	SG_uint32*           pFoundIndex, //< [out] The index of the first item that matched the predicate.
	                                  //<       The size of the vector if no items matched.
	void**               ppFoundData  //< [out] The data of the first item that matched the predicate.
	                                  //<       NULL if no items matched.
	);

/**
 * Finds the last item in the vector that matches a predicate.
 */
void SG_vector__find__last(
	SG_context*          pCtx,        //< [in] [out] Error and context info.
	SG_vector*           pVector,     //< [in] The vector to search.
	SG_vector_predicate* fPredicate,  //< [in] The predicate to match items against.
	void*                pUserData,   //< [in] User data to pass to the predicate.
	SG_uint32*           pFoundIndex, //< [out] The index of the last item that matched the predicate.
	                                  //<       The size of the vector if no items matched.
	void**               ppFoundData  //< [out] The data of the last item that matched the predicate.
	                                  //<       NULL if no items matched.
	);

/**
 * Finds all items in the vector that match a predicate.
 */
void SG_vector__find__all(
	SG_context*                 pCtx,           //< [in] [out] Error and context info.
	SG_vector*                  pVector,        //< [in] The vector to search.
	SG_vector_predicate*        fPredicate,     //< [in] The predicate to match items against.
	void*                       pPredicateData, //< [in] User data to pass to the predicate.
	SG_vector_foreach_callback* fCallback,      //< [in] The callback to invoke for matching items.
	void*                       pCallbackData   //< [in] User data to pass to the callback.
	);

/**
 * Checks if the vector contains a given piece of data.
 */
void SG_vector__contains(
	SG_context* pCtx,     //< [in] [out] Error and context info.
	SG_vector*  pVector,  //< [in] The vector to search.
	void*       pData,    //< [in] The data to search for.
	SG_bool*    pContains //< [out] Whether or not the vector contains the given data.
	);

/**
 * Removes the item at a given index from a vector.
 */
void SG_vector__remove__index(
	SG_context*       pCtx,       //< [in] [out] Error and context info.
	SG_vector*        pVector,    //< [in] The vector to remove an item from.
	SG_uint32         uIndex,     //< [in] The index of the item to remove.
	SG_free_callback* fDestructor //< [in] Function to call to free the removed item.
	                              //<      May be NULL if unnecessary.
	);

/**
 * Removes all occurances of an item from a vector.
 * Note that this does simple pointer comparisons.
 */
void SG_vector__remove__value(
	SG_context*       pCtx,       //< [in] [out] Error and context info.
	SG_vector*        pVector,    //< [in] The vector to remove an item from.
	void*             pValue,     //< [in] The index of the item to remove.
	SG_uint32*        pCount,     //< [out] The number of items removed.
	SG_free_callback* fDestructor //< [in] Function to call to free the removed items.
	                              //<      May be NULL if unnecessary.
	);

/**
 * Removes any items from a vector that match a predicate.
 */
void SG_vector__remove__if(
	SG_context*          pCtx,       //< [in] [out] Error and context info.
	SG_vector*           pVector,    //< [in] The vector to remove items from.
	SG_vector_predicate* fPredicate, //< [in] The predicate to match items against.
	void*                pUserData,  //< [in] User data to pass to the predicate.
	SG_uint32*           pCount,     //< [out] The number of items removed.
	SG_free_callback*    fDestructor //< [in] Function to call to free the removed items.
	                                 //<      May be NULL if unnecessary.
	);

//////////////////////////////////////////////////////////////////

/**
 * An SG_vector_predicate that simply checks to see if the value equals a given value.
 * It just does simple pointer comparison.
 */
void SG_vector__predicate__match_value(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	SG_uint32   uIndex,    //< [in] Index of the current item.
	void*       pValue,    //< [in] Value of the current item.
	void*       pUserData, //< [in] Value to match against.
	SG_bool*    pMatch     //< [out] Whether or nto the current item matches.
	);

/**
 * An SG_vector_callback that appends each value to another vector.
 */
void SG_vector__callback__append(
	SG_context* pCtx,     //< [in] [out] Error and context info.
	SG_uint32   uIndex,   //< [in] Index of the current item.
	void*       pValue,   //< [in] Value of the current item.
	void*       pUserData //< [in] An SG_vector to add the current item to.
	);

END_EXTERN_C;

#endif//H_SG_VECTOR_PROTOTYPES_H
