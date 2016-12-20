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
 * @file sg_changeset_typedefs.h
 *
 * @details A Changeset is a representation of a check-in/commit.
 * Everything we need to rebuild the state of the user's project as
 * of a particular version.  The Changeset does not directly contain
 * all of the change information; rather, it should be thought of as
 * a handle (containing pointers) to the various data structures.
 *
 * The actual in-memory format of a Changeset is opaque.
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_CHANGESET_TYPEDEFS_H
#define H_SG_CHANGESET_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * (CSET) Version Identifier.
 *
 * Indicates what version of the software created the Changeset object.
 * This is important when we read an object from a blob.  For
 * newly created Changeset objects, we should always use the current
 * software versrion.
 */
enum SG_changeset_version
{
	SG_CSET_VERSION__INVALID=-1,					/**< Invalid Value, must be -1. */
	SG_CSET_VERSION_1=1,							/**< Version 1. */

	SG_CSET_VERSION__RANGE,									/**< Last defined valued + 1. */
	SG_CSET_VERSION__CURRENT=SG_CSET_VERSION__RANGE-1,		/**< Current Software Version.  Must be latest version defined. */
};

typedef enum SG_changeset_version SG_changeset_version;

/**
 * A Changeset is a representation of a check-in/commit.
 *
 * A SG_changeset is used to represent an existing commit already
 * stored in the repository.  To create a new Changeset, use SG_committing.
 */
typedef struct SG_changeset SG_changeset;

#define SG_CHANGEESET__TREE__CHANGES__TYPE			    "type"
#define SG_CHANGEESET__TREE__CHANGES__DIR			    "dir"
#define SG_CHANGEESET__TREE__CHANGES__HID               "hid"
#define SG_CHANGEESET__TREE__CHANGES__BITS	            "bits"
#define SG_CHANGEESET__TREE__CHANGES__NAME		        "name"

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_CHANGESET_TYPEDEFS_H
