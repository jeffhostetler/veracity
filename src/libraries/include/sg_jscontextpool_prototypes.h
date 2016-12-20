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

#ifndef H_SG_JSCONTEXTPOOL_PROTOTYPES_H
#define H_SG_JSCONTEXTPOOL_PROTOTYPES_H

BEGIN_EXTERN_C;

///////////////////////////////////////////////////////////////////////////////


// Start up and shutdown the entire pool of jscontexts. This is used solely by
// SG_uridispatch.
void SG_jscontextpool__init(
	SG_context * pCtx,
	const char * szApplicationRoot
	);
void SG_jscontextpool__teardown(SG_context * pCtx);

// Acquire an SG_jscontext. Note: you should only ever access the jscontext on
// one thread between acquiring it and releasing it (Otherwise you'd need to do
// some JS_ClearContextThread() and JS_SetContextThread() action yourself.).
void SG_jscontext__acquire(SG_context * pCtx, SG_jscontext ** ppJs);

// Release a jscontext. Put's it back in the pool to be used again later.
void SG_jscontext__release(SG_context * pCtx, SG_jscontext ** ppJs);


// Wrapper around JS_EndRequest. Indicates that you're done using this context
// for now, while waiting for a blocking call or long running operation.
void SG_jscontext__suspend(SG_jscontext * pJs);

// Wrapper around JS_BeginRequest. Essentially, indicates that you're about to
// start using the JSAPI (without performing any blocking calls or long running
// operations).
void SG_jscontext__resume(SG_jscontext * pJs);


///////////////////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif
