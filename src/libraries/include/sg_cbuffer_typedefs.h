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
 * @file sg_cbuffer_typedefs.h
 *
 * Buffer 'n' Length.
 *
    An SG_cbuffer is a simple, lightweight encapsulation of a byte array and
    its length. "pBuf" and "len" should be treated as constants: After creating
    the buffer, we never resize it or allocate any new memory. It's just for
    storing an existing buffer, noteably one that we want to pass through a
    layer of JavaScript.
 */

#ifndef H_SG_CBUFFER_TYPEDEFS_H
#define H_SG_CBUFFER_TYPEDEFS_H

BEGIN_EXTERN_C;

///////////////////////////////////////////////////////////////////////////////


typedef struct
{
	SG_byte * pBuf; //< Pointer to the byte array.

	SG_uint32 len; //< Length of the byte array. (Note the actual allocated
	               //  length may be different than this, but it's *at least*
	               //  this long.)
} SG_cbuffer;


///////////////////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_CBUFFER_TYPEDEFS_H

