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
 * @file sg_cbuffer_prototypes.h
 *
 */

#ifndef H_SG_CBUFFER_PROTOTYPES_H
#define H_SG_CBUFFER_PROTOTYPES_H

BEGIN_EXTERN_C

///////////////////////////////////////////////////////////////////////////////


// Create a cbuffer (see sg_cbuffer_typedefs.h) and give it a newly-allocated
// (and thus 0-initialized) buffer of the given length.
void SG_cbuffer__alloc__new(
	SG_context * pCtx,
	SG_cbuffer ** ppCbuffer,
	SG_uint32 len
	);

// Create a cbuffer out of an SG_string.
void SG_cbuffer__alloc__string(
	SG_context * pCtx,
	SG_cbuffer ** ppCbuffer,
	SG_string ** ppString // We take ownership of the string and null the callers copy.
	);

void SG_cbuffer__nullfree(
	SG_cbuffer ** ppCbuffer
	);


///////////////////////////////////////////////////////////////////////////////

END_EXTERN_C

#endif
