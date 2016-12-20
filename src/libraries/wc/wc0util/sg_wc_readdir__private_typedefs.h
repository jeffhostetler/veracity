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

#ifndef H_SG_WC_READDIR__PRIVATE_TYPEDEFS_H
#define H_SG_WC_READDIR__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

struct _sg_wc_readdir__row
{
	SG_string * pStringEntryname;
	SG_fsobj_stat * pfsStat;
	SG_treenode_entry_type tneType;
	sg_wc_prescan_row * pPrescanRow;	// back ptr.  we do not own this

	// The dynamically computed ATTRBITS (based strictly
	// off of what we have observed on the disk during the
	// READDIR).
	SG_uint64 * pAttrbits;

	// The dynamically computed content-HID.  (When computed
	// this reflects the observed content of the item.)
	// 
	// For FILES we may actually compute it or get the value
	// from the TimeStampCache.
	//
	// For DIRECTORIES, this field cannot be computed until
	// after the changeset is committed.
	char * pszHidContent;

	// When we set pszHidContent, we also set the size of
	// the data that the hash was computed on.  if the HID
	// is not set, neither is this field (otherwise we can't
	// tell an unset/initialized 0 from a 0 length file).
	SG_uint64 sizeContent;

	// If the item is a SYMLINK (and we took the trouble to
	// read it from the WD), we might as well cache it here
	// because the above HID (which refers to this target)
	// may not exist yet in the repo.
	SG_string * pStringSymlinkTarget;

};

typedef struct _sg_wc_readdir__row sg_wc_readdir__row;


//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_READDIR__PRIVATE_TYPEDEFS_H
