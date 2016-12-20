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

#ifndef H_SG_RBTREE_UI64__PUBLIC_TYPEDEFS_H
#define H_SG_RBTREE_UI64__PUBLIC_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

typedef struct _sg_rbtree_ui64        SG_rbtree_ui64;
typedef struct _sg_rbtree_ui64_iter   SG_rbtree_ui64_iterator;

typedef void (SG_rbtree_ui64_foreach_callback)(SG_context * pCtx,
											   SG_uint64 ui64_key,
											   void * pVoidAssoc,
											   void * pVoidData);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_RBTREE_UI64__PUBLIC_TYPEDEFS_H
