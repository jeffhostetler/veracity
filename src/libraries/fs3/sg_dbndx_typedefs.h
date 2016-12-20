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
 * @file sg_dbndx_typedefs.h
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_DBNDX_TYPEDEFS_H
#define H_SG_DBNDX_TYPEDEFS_H

BEGIN_EXTERN_C;

#define SG_DBNDX_RECORD_TABLE_PREFIX "rec$"
#define SG_DBNDX_FTS_TABLE_PREFIX "fts$"

typedef struct _SG_dbndx_query SG_dbndx_query;
typedef struct _SG_dbndx_update SG_dbndx_update;

END_EXTERN_C;

#endif//H_SG_DBNDX_TYPEDEFS_H

