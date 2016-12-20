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
 * @file sg_vv_verbs.c
 *
 * @details Routines to (eventually) do most of the major command-level verbs.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

//////////////////////////////////////////////////////////////////

#include "sg_vv_verbs__incoming.h"
#include "sg_vv_verbs__outgoing.h"
#include "sg_vv_verbs__pull.h"
#include "sg_vv_verbs__push.h"
#include "sg_vv_verbs__upgrade.h"
#include "sg_vv_verbs__heads.h"
#include "sg_vv_verbs__work_items.h"
