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

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC7TXAPI__PUBLIC_TYPEDEFS_H
#define H_SG_WC7TXAPI__PUBLIC_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

typedef struct _sg_wc_tx SG_wc_tx;

// TODO 2012/04/10 I'm promoting SG_resolve to be a public typedef
// TODO            for a test.  See sg_wc_tx__resolve__private_typedefs.h.
typedef struct _sg_resolve SG_resolve;

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC7TXAPI__PUBLIC_TYPEDEFS_H
