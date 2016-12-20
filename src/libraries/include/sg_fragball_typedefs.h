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
 * @file sg_fragball_typedefs.h
 *
 */

#ifndef H_SG_FRAGBALL_TYPEDEFS_H
#define H_SG_FRAGBALL_TYPEDEFS_H

BEGIN_EXTERN_C;

typedef struct _sg_fragball_writer SG_fragball_writer;

#define SG_FRAGBALL_V3_TYPE__BLOB      1
#define SG_FRAGBALL_V3_TYPE__FRAG      2
#define SG_FRAGBALL_V3_TYPE__AUDITS    3

#define SG_FRAGBALL_V3_TYPE__BINDEX    4
#define SG_FRAGBALL_V3_TYPE__BFILE     5

#define SG_FRAGBALL_V3_FLAGS__NONE     0

END_EXTERN_C;

#endif//H_SG_FRAGBALL_TYPEDEFS_H


