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
 * @file sg_vector_typedefs.h
 *
 * @details
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VECTOR_TYPEDEFS_H
#define H_SG_VECTOR_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

typedef struct _SG_vector SG_vector;

typedef void (SG_vector_foreach_callback)(SG_context * pCtx,
										  SG_uint32 ndx,
										  void * pVoidAssocData,
										  void * pVoidCallerData);

/**
 * A predicate used to check if an item in a vector matches some condition.
 */
typedef void SG_vector_predicate(
	SG_context* pCtx,      //< [in] [out] Error and context info.
	SG_uint32   uIndex,    //< [in] The index of the item being matched.
	void*       pData,     //< [in] The data of the item being matched.
	void*       pUserData, //< [in] Data provided by the caller.
	                       //<      Usually contains data to match the item against.
	SG_bool*    pMatch     //< [out] Whether or not the item matches the condition.
	);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VECTOR_TYPEDEFS_H
