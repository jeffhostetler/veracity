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
 * @file sg_httprequestprofiler_typedefs.h
 *
 */

#ifndef H_SG_HTTPREQUESTPROFILER_TYPEDEFS_H
#define H_SG_HTTPREQUESTPROFILER_TYPEDEFS_H

BEGIN_EXTERN_C;

////////////////////////////////////////////////////////////////////////////////


// Categories of where time is spent during the handling of an http request:
typedef enum
{
	// For the following categories we also look at whether or not the category
	// was ever used during a given request, and create aggregated results
	// accordingly.
	SG_HTTPREQUESTPROFILER_CATEGORY__JSCONTEXT_CREATION,
	SG_HTTPREQUESTPROFILER_CATEGORY__COOKIE_AUTH,
	SG_HTTPREQUESTPROFILER_CATEGORY__ZING_QUERY,
	SG_HTTPREQUESTPROFILER_CATEGORY__ONPOSTRECEIVED_CB,
	SG_HTTPREQUESTPROFILER_CATEGORY__ACTIVITY_DISPATCH,
	SG_HTTPREQUESTPROFILER_CATEGORY__CACHE_WRITE,
	SG_HTTPREQUESTPROFILER_CATEGORY__CACHE_READ,
	SG_HTTPREQUESTPROFILER_CATEGORY__SESSION_READ,
	
	// The following categories are "normal"  (We do not aggregate based on
	// their presence or absence in a request).
	SG_HTTPREQUESTPROFILER_CATEGORY__CACHE_CHECK,
	SG_HTTPREQUESTPROFILER_CATEGORY__JSGLUE_MUTEX_LOCK,
	SG_HTTPREQUESTPROFILER_CATEGORY__SESSION_WRITE,
	SG_HTTPREQUESTPROFILER_CATEGORY__SESSION_CLEANUP,
	SG_HTTPREQUESTPROFILER_CATEGORY__JSREQUEST_TOGGLING,
	SG_HTTPREQUESTPROFILER_CATEGORY__OPEN_REPO,
	SG_HTTPREQUESTPROFILER_CATEGORY__CONFIG_READ,
	SG_HTTPREQUESTPROFILER_CATEGORY__C_BEGIN_REQUEST,
	SG_HTTPREQUESTPROFILER_CATEGORY__JAVASCRIPT_DISPATCH,
	SG_HTTPREQUESTPROFILER_CATEGORY__AUTH,
	SG_HTTPREQUESTPROFILER_CATEGORY__ONDISPATCH_CB,
	SG_HTTPREQUESTPROFILER_CATEGORY__C_GET_RESPONSE_HEADERS,
	SG_HTTPREQUESTPROFILER_CATEGORY__C_CHUNK_REQUEST,
	SG_HTTPREQUESTPROFILER_CATEGORY__C_CHUNK_RESPONSE,
	SG_HTTPREQUESTPROFILER_CATEGORY__LOGGING,
	SG_HTTPREQUESTPROFILER_CATEGORY__TRANSFER,
	SG_HTTPREQUESTPROFILER_CATEGORY__OTHER,
	
	// Total number of categories.
	SG_HTTPREQUESTPROFILER_CATEGORY__COUNT,
	
	// Dummy category.
	SG_HTTPREQUESTPROFILER_CATEGORY__NOT_APPLICABLE,
	
	// Number of categories that we do aggregation based on.
	SG_HTTPREQUESTPROFILER_CATEGORY__COUNT__SPECIAL = SG_HTTPREQUESTPROFILER_CATEGORY__SESSION_READ + 1
} SG_httprequestprofiler_category;


////////////////////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_HTTPREQUESTPROFILER_TYPEDEFS_H

