/*
Copyright 2012-2013 SourceGear, LLC

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
 * @file sg_httprequestprofiler_prototypes.h
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_HTTPREQUESTPROFILER_PROTOTYPES_H
#define H_SG_HTTPREQUESTPROFILER_PROTOTYPES_H

BEGIN_EXTERN_C;


void SG_httprequestprofiler__start_request(void);
void SG_httprequestprofiler__start(SG_httprequestprofiler_category category);
void SG_httprequestprofiler__stop(void);
void SG_httprequestprofiler__switch(SG_httprequestprofiler_category new_category); // Same as stop, then start.
void SG_httprequestprofiler__stop_request(void);

void SG_httprequestprofiler__dump_all(void);

void SG_httprequestprofiler__global_init(SG_context *pCtx);
void SG_httprequestprofiler__global_cleanup(void);


END_EXTERN_C;

#endif //H_SG_HTTPREQUESTPROFILER_PROTOTYPES_H

