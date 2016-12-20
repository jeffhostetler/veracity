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

#ifndef H_SG_REV_SPEC_TYPEDEFS_H
#define H_SG_REV_SPEC_TYPEDEFS_H

BEGIN_EXTERN_C;

typedef enum
{
	SPECTYPE_REV,
	SPECTYPE_TAG,
	SPECTYPE_BRANCH
} SG_rev_spec_type;

typedef struct _sg_rev_spec_is_opaque SG_rev_spec;

typedef void (SG_rev_spec_foreach_callback)(
	SG_context* pCtx, 
	SG_repo* pRepo,
	SG_rev_spec_type specType,
	const char* pszGiven,    /* The value as added to the SG_rev_spec. */
	const char* pszFullHid,	 /* The full HID of the looked-up spec. */
	void* ctx);

END_EXTERN_C;

#endif // H_SG_REV_SPEC_TYPEDEFS_H