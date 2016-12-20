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

#ifndef H_SG_FS3_PRIVATE_H
#define H_SG_FS3_PRIVATE_H

#include <sg.h>

#include "sg_treendx_typedefs.h"
#include "sg_treendx_prototypes.h"
#include "sg_dbndx_typedefs.h"
#include "sg_dbndx_prototypes.h"
#include "sg_dag_sqlite3_typedefs.h"
#include "sg_dag_sqlite3_prototypes.h"
#include "sg_repo_vtable__fs3.h"

#define SG_DBNDX_QUERY_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_dbndx_query__free)
#define SG_DBNDX_UPDATE_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_dbndx_update__free)
#define SG_TREENDX_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_treendx__free)

#endif//H_SG_FS3_PRIVATE_H

