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
 * @file sg_wc__public_typedefs.h
 *
 * @details Declarations visible OUTSIDE of this directory.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC__PUBLIC_TYPEDEFS_H
#define H_SG_WC__PUBLIC_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////
// TODO 2012/04/02 Move these declarations somewhere besides the
// TODO            top-level header.
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// "A", "B", and "C" are somewhat special because the
// status formatter will print "Was" lines for MOVES/RENAMES
// using the values in each of these subsections.

#define SG_WC__REPO_PATH_PREFIX__LIVE			"@/"
#define SG_WC__REPO_PATH_PREFIX__A				"@a/"
#define SG_WC__REPO_PATH_PREFIX__B				"@b/"
#define SG_WC__REPO_PATH_PREFIX__C				"@c/"
#define SG_WC__REPO_PATH_PREFIX__M				"@m/"
//#define SG_WC__REPO_PATH_PREFIX__R				"@r/"
#define SG_WC__REPO_PATH_PREFIX__MERGE__Sx		"@s/"	// place holder

#define SG_WC__REPO_PATH_DOMAIN__LIVE		'/'		// actually just the lack of a domain
#define SG_WC__REPO_PATH_DOMAIN__A			'a'
#define SG_WC__REPO_PATH_DOMAIN__B			'b'
#define SG_WC__REPO_PATH_DOMAIN__C			'c'
#define SG_WC__REPO_PATH_DOMAIN__M			'm'
//#define SG_WC__REPO_PATH_DOMAIN__R			'r'
#define SG_WC__REPO_PATH_DOMAIN__MERGE__Sx	's'		// place holder

#define SG_WC__REPO_PATH_DOMAIN__G			'g'
#define SG_WC__REPO_PATH_DOMAIN__T			't'

#define SG_WC__STATUS_SUBSECTION__WC		"WC"
#define SG_WC__STATUS_SUBSECTION__A			"A"
#define SG_WC__STATUS_SUBSECTION__B			"B"
#define SG_WC__STATUS_SUBSECTION__C			"C"
#define SG_WC__STATUS_SUBSECTION__M			"M"
//#define SG_WC__STATUS_SUBSECTION__R			"R"

//////////////////////////////////////////////////////////////////

#include    "wc0util/sg_rbtree_ui64__public_typedefs.h"
#include    "wc0util/sg_wc_attrbits__public_typedefs.h"
#include    "wc0util/sg_wc_port__public_typedefs.h"
#include  "wc4status/sg_wc__status__public_typedefs.h"
#include   "wc7txapi/sg_wc7txapi__public_typedefs.h"
#include     "wc8api/sg_wc__commit__public_typedefs.h"
#include     "wc8api/sg_wc__merge__public_typedefs.h"
#include     "wc8api/sg_wc__undo_delete__public_typedefs.h"
#include     "wc8api/sg_wc__update__public_typedefs.h"

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC__PUBLIC_TYPEDEFS_H
