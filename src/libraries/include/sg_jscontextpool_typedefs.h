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

#ifndef H_SG_JSCONTEXTPOOL_TYPEDEFS_H
#define H_SG_JSCONTEXTPOOL_TYPEDEFS_H

BEGIN_EXTERN_C;

///////////////////////////////////////////////////////////////////////////////


typedef struct sg_jscontext_struct SG_jscontext;

// An SG_jscontext is a JSContext that has been initialized with everything the
// server needs in order to handle an incoming http request using the SSJS.
//
// A pool of them is kept around for reuse, so that the SSJS files don't have
// to be reparsed and reexecuted with every request.
//
// This class is used solely by SG_uridispatch.
struct sg_jscontext_struct
{
	// These are the "public" members to be used by the caller. The rest are
	// for internal bookkeepping.
	JSContext * cx;
	JSObject * glob;

	SG_bool isInARequest; // "Request" as in the SpiderMonkey sense of the term

	// Pointer to next for when we're in the linked list of all SG_jscontexts
	// that are not currently in use by SG_uridispatch.
	SG_jscontext * pNextAvailableContext;
};


///////////////////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif
