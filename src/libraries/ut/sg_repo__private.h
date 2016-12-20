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
 * @file sg_repo__private.h
 *
 * @details Private declarations used by the various sg_repo*.c files.
 * Everything in this file should be considered STATIC (hidden).
 * 
 */

// TODO get rid of treendx_prototypes.h and dbndx (same).  move to
// private header files.

// TODO add error code for db/tree ndx not supported by this repo instance
//
// TODO add parameter to create to leave ndx off
//
// TODO comments explaining that only ndx methods are allowed to look inside blobs

//////////////////////////////////////////////////////////////////

#ifndef H_SG_REPO__PRIVATE_H
#define H_SG_REPO__PRIVATE_H

BEGIN_EXTERN_C;

/**
 * Private routine to select a specific VTABLE implementation
 * and bind it to the given pRepo.
 *
 * Note that this only sets pRepo->p_vtable.  It DOES NOT imply
 * that pRepo is associated with anything on disk; that is,
 * we DO NOT set pRepo->p_vtable_instance_data. 
 */
void sg_repo__bind_vtable(SG_context* pCtx, SG_repo * pRepo, const char * pszStorage);

/**
 * Private routine to unbind the VTABLE from the given pRepo.
 * This routine will NULL the vtable pointer in the SG_repo and
 * free the vtable if appropriate.
 */
void sg_repo__unbind_vtable(SG_context* pCtx, SG_repo * pRepo);

/**
 * Private routine to list all of the installed REPO implementations.
 *
 * You own the returned vhash.
 */
void sg_repo__query_implementation__list_vtables(SG_context * pCtx, SG_vhash ** pp_vhash);

//////////////////////////////////////////////////////////////////

/**
 * Private REPO instance data.  This is hidden from everybody except
 * sg_repo__ routines.
 */
struct _SG_repo
{
	char*								psz_descriptor_name;
    SG_vhash*							pvh_descriptor;			// this is used to select a VTABLE implementation

	sg_repo__vtable *		            p_vtable;		        // the binding to a specific REPO VTABLE implementation
	void*	                            p_vtable_instance_data;	// binding-specific instance data (opaque outside of imp)
};

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_REPO__PRIVATE_H

