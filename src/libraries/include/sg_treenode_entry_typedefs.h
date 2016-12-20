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
 * @file sg_treenode_entry_typedefs.h
 *
 * @details A Treenode-Entry is a representation of a single directory-entry
 * within a directory.  A Treenode-Entry refers to a versioned file,
 * sub-directory, symlink, etc within a version controlled directory.
 * A Treenode is a container for a collection of Treenode-Entry's.
 *
 * A Treenode-Entry is an opaque structure.  There are routines to access
 * fields within it.  The containing Treenode structure is responsible for
 * serialization.
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_TREENODE_ENTRY_TYPEDEFS_H
#define H_SG_TREENODE_ENTRY_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * A Treenode-Entry describes a single directory-entry in a Treenode.
 * That is a single versioned file/sub-directory/etc in a versioned
 * directory.
 *
 * Each Treenode-Entry is created in the context of a Treenode;
 * the Treenode is the owner of the Treenode-Entry.  Limited access
 * to the Treenode-Entry is allowed, but the Treenode is responsible
 * for allocating/freeing/serializing.
 */
typedef struct SG_treenode_entry SG_treenode_entry;

/**
 * Treenode-Entry Type.
 *
 * Identifies whether a directory entry is a sub-directory, a regular
 * file, a symlink, or etc.
 *
 */
enum SG_treenode_entry_type
{
	SG_TREENODEENTRY_TYPE__INVALID=-1,					/**< Invalid Value, must be -1. */

	SG_TREENODEENTRY_TYPE_REGULAR_FILE=1,				/**< A regular file. */
	SG_TREENODEENTRY_TYPE_DIRECTORY=2,					/**< A sub-directory. */
	SG_TREENODEENTRY_TYPE_SYMLINK=3,					/**< A Linux/MAC symlink. */

	SG_TREENODEENTRY_TYPE_SUBMODULE=4,					/**< A submodule directory (reserved for future use) */

	// The value __DEVICE is ***NEVER*** stored in a treenode-entry
	// and will never appear in a changeset.  It is used only to
	// have something to map SG_FSOBJ_TYPE__DEVICE into as we get
	// into higher levels.  It represents something (like a FIFO
	// or BlkDev) that readdir() found in the directory that cannot
	// be placed under version control, but yet it exists on the
	// disk and so the LVI layer must account for the entryname.
	SG_TREENODEENTRY_TYPE__DEVICE=0xff
};

typedef enum SG_treenode_entry_type SG_treenode_entry_type;

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_TREENODE_ENTRY_TYPEDEFS_H
