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
 * @file sg_treenode_entry.c
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

//////////////////////////////////////////////////////////////////

/**
 * struct SG_treenode_entry { ...Do not define.... };
 *
 * A SG_treenode_entry represents all of the information about a single
 * versioned directory-entry in a Treenode.  But really, SG_treenode_entry
 * doesn't really exist.  We use the name as an opaque label for our
 * prototypes.  Internally, we use a vhash to store directory-entry
 * attributes.  We don't wrap this with a structure (like SG_treenode)
 * because we cannot put pointers into the containing SG_treenode vhash.
 *
 * The Treenode is the owner of all of each SG_treenode_entry, but we
 * allow the caller to have limited read/write access to it.
 *
 * The information in a Treenode-Entry is converted to JSON as part of
 * the overall serialization of the Treenode.  We do not do any direct
 * serialization here.
 */

/**
 * The following keys are present in Version 1 Treenode-Entries (may also be
 * used by future versions).  Attribute bits and XAttrs are only in the
 * vhash when non-zero.
 *
 * 2012/11/16 We're removing XAttrs for the 2.5/WC release.  They haven't
 *            officially been turned on since before 1.0 and they aren't
 *            worth the bother.  And they are completely different on MAC
 *            and (seemingly) each flavor of Linux.
 *            
 */
#define KEY_TYPE			"type"
#define KEY_HID_BLOB		"hid"
// #define KEY_HID_XATTRS	    "xattrs"
#define KEY_ATTRIBUTE_BITS	"bits"
#define KEY_ENTRYNAME		"name"

//////////////////////////////////////////////////////////////////

void SG_treenode_entry__alloc(SG_context * pCtx, SG_treenode_entry ** ppNew)
{
	SG_vhash * pvh = NULL;

	SG_NULLARGCHECK_RETURN(ppNew);

	SG_ERR_CHECK_RETURN(  SG_VHASH__ALLOC(pCtx, &pvh)  );

	*ppNew = (SG_treenode_entry *)pvh;
}

void SG_treenode_entry__alloc__copy(SG_context * pCtx, SG_treenode_entry ** ppNew, const SG_treenode_entry* ptne)
{
	SG_vhash* pvh = NULL;

	SG_NULLARGCHECK_RETURN(ppNew);
	SG_NULLARGCHECK_RETURN(ptne);

	SG_ERR_CHECK_RETURN(  SG_VHASH__ALLOC__COPY(pCtx, &pvh, (const SG_vhash *)ptne)  );

	*ppNew = (SG_treenode_entry*) pvh;
}

void SG_treenode_entry__free(SG_context * pCtx, SG_treenode_entry * pTreenodeEntry)
{
	SG_vhash* p = (SG_vhash*)pTreenodeEntry;
	SG_VHASH_NULLFREE(pCtx, p);
}

//////////////////////////////////////////////////////////////////

void SG_treenode_entry__set_entry_type(
	SG_context * pCtx,
	SG_treenode_entry * pTreenodeEntry,
	SG_treenode_entry_type tneType
	)
{
	SG_NULLARGCHECK_RETURN(pTreenodeEntry);
	SG_ERR_CHECK_RETURN(  SG_vhash__update__int64(pCtx, (SG_vhash *)pTreenodeEntry,KEY_TYPE,(SG_int64)tneType) );
}

void SG_treenode_entry__get_entry_type(
	SG_context * pCtx,
	const SG_treenode_entry * pTreenodeEntry,
	SG_treenode_entry_type * ptneType
	)
{
	SG_int64 v;

	SG_NULLARGCHECK_RETURN(pTreenodeEntry);
	SG_NULLARGCHECK_RETURN(ptneType);

	// type should ***always*** be present.

	SG_vhash__get__int64(pCtx,(const SG_vhash *)pTreenodeEntry,KEY_TYPE,&v);
	if (!SG_CONTEXT__HAS_ERR(pCtx))
		*ptneType = (SG_treenode_entry_type)v;
	else if (SG_context__err_equals(pCtx, SG_ERR_VHASH_KEYNOTFOUND))
	{
		*ptneType = SG_TREENODEENTRY_TYPE__INVALID;
		SG_context__err_reset(pCtx);
	}
	else
		SG_ERR_RETHROW_RETURN;
}

//////////////////////////////////////////////////////////////////

void SG_treenode_entry__set_hid_blob(
	SG_context * pCtx,
	SG_treenode_entry * pTreenodeEntry,
	const char* pszidHidBlob
	)
{
	SG_NULLARGCHECK_RETURN(pTreenodeEntry);
	SG_ERR_CHECK_RETURN(  SG_vhash__update__string__sz(pCtx, (SG_vhash *)pTreenodeEntry, KEY_HID_BLOB, pszidHidBlob)  );
}

void SG_treenode_entry__get_hid_blob(
	SG_context * pCtx,
	const SG_treenode_entry * pTreenodeEntry,
	const char** ppszidHidBlob
	)
{
	SG_NULLARGCHECK_RETURN(pTreenodeEntry);
	SG_NULLARGCHECK_RETURN(ppszidHidBlob);

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, (const SG_vhash *)pTreenodeEntry, KEY_HID_BLOB, ppszidHidBlob)  );
}

//////////////////////////////////////////////////////////////////

void SG_treenode_entry__set_attribute_bits(
	SG_context * pCtx,
	SG_treenode_entry * pTreenodeEntry,
	SG_int64 bits
	)
{
	SG_NULLARGCHECK_RETURN(pTreenodeEntry);

    if (bits)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__update__int64(pCtx, (SG_vhash *)pTreenodeEntry,KEY_ATTRIBUTE_BITS,bits)  );
    }
    else
    {
        SG_bool b_has = SG_FALSE;

        SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, (const SG_vhash *)pTreenodeEntry, KEY_ATTRIBUTE_BITS, &b_has)  );
        if (b_has)
        {
            SG_ERR_CHECK_RETURN(  SG_vhash__remove(pCtx, (SG_vhash *)pTreenodeEntry, KEY_ATTRIBUTE_BITS)  );
        }
    }
}

void SG_treenode_entry__get_attribute_bits(
	SG_context * pCtx,
	const SG_treenode_entry * pTreenodeEntry,
	SG_int64* piBits
	)
{
    SG_bool b_has = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pTreenodeEntry);
	SG_NULLARGCHECK_RETURN(piBits);

    SG_ERR_CHECK_RETURN(  SG_vhash__has(pCtx, (const SG_vhash *)pTreenodeEntry, KEY_ATTRIBUTE_BITS, &b_has)  );

    if (b_has)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, (const SG_vhash*)pTreenodeEntry, KEY_ATTRIBUTE_BITS, piBits)  );
    }
    else
    {
        *piBits = 0;
    }
}

//////////////////////////////////////////////////////////////////

void SG_treenode_entry__set_entry_name(
	SG_context * pCtx,
	SG_treenode_entry * pTreenodeEntry,
	const char * szEntryname
	)
{
	SG_NULLARGCHECK_RETURN(pTreenodeEntry);

	SG_NONEMPTYCHECK_RETURN(szEntryname);

	SG_ERR_CHECK_RETURN(  SG_vhash__update__string__sz(pCtx, (SG_vhash *)pTreenodeEntry, KEY_ENTRYNAME, szEntryname)  );
}

void SG_treenode_entry__get_entry_name(
	SG_context * pCtx,
	const SG_treenode_entry * pTreenodeEntry,
	const char ** pszEntryname
	)
{
	SG_NULLARGCHECK_RETURN(pTreenodeEntry);
	SG_NULLARGCHECK_RETURN(pszEntryname);

	SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, (const SG_vhash *)pTreenodeEntry, KEY_ENTRYNAME, pszEntryname)  );
}

//////////////////////////////////////////////////////////////////

static SG_bool _sg_treenode_entry__verify_type(SG_treenode_version ver,
											   SG_treenode_entry_type tneType)
{
	// Is the given Entry-Type valid for a Treenode-Entry of this Entry-Version?
	//
	// Treenode-Entries DO NOT have a version number within.
	// It is assumed that all entries within a treenode (directory)
	// will have the same version.  So the caller must pass this in.

	switch (ver)
	{
	case SG_TN_VERSION_1:
		switch (tneType)
		{
		case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		case SG_TREENODEENTRY_TYPE_DIRECTORY:
		case SG_TREENODEENTRY_TYPE_SYMLINK:
		case SG_TREENODEENTRY_TYPE_SUBMODULE:
			return SG_TRUE;

		// case SG_TREENODEENTRY_TYPE__DEVICE:	// will never appear in a treenode
		default:
			return SG_FALSE;
		}

	default:
		return SG_FALSE;
	}
}

const char * SG_treenode_entry_type__type_to_label(SG_treenode_entry_type tneType)
{
	switch (tneType)
	{
	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:	return "File";
	case SG_TREENODEENTRY_TYPE_DIRECTORY:		return "Directory";
	case SG_TREENODEENTRY_TYPE_SYMLINK:			return "Symlink";	// a Linux/Mac symlink
	case SG_TREENODEENTRY_TYPE_SUBMODULE:		return "Submodule";	// TODO 2012/04/18 rename this "Subrepo".
	default:									return "Invalid";

	case SG_TREENODEENTRY_TYPE__DEVICE:			return "*DEVICE*";	// will never appear in a treenode
	}
}

void SG_treenode_entry__validate__v1(SG_context * pCtx, const SG_treenode_entry * pTreenodeEntry)
{
	// Validate a treenode-entry found in a SG_TN_VERSION_1 version treenode.

	SG_treenode_entry_type tneType = SG_TREENODEENTRY_TYPE__INVALID;
	const char* pid = NULL;
	const char * szEntryname = NULL;
    SG_int64 bits = 0;

	// lookup each field and see if the values are sensible.

	SG_ERR_CHECK_RETURN(  SG_treenode_entry__get_entry_type(pCtx, pTreenodeEntry,&tneType)  );
	if (!_sg_treenode_entry__verify_type(SG_TN_VERSION_1,tneType))
		SG_ERR_THROW_RETURN(SG_ERR_TREENODE_ENTRY_VALIDATION_FAILED);

	SG_ERR_CHECK_RETURN(  SG_treenode_entry__get_hid_blob(pCtx, pTreenodeEntry,&pid)  );
	if (!pid)
		SG_ERR_THROW_RETURN(SG_ERR_TREENODE_ENTRY_VALIDATION_FAILED);

	SG_ERR_CHECK_RETURN(  SG_treenode_entry__get_attribute_bits(pCtx, pTreenodeEntry,&bits)  );

	SG_ERR_CHECK_RETURN(  SG_treenode_entry__get_entry_name(pCtx, pTreenodeEntry,&szEntryname)  );
}

//////////////////////////////////////////////////////////////////

void SG_treenode_entry__equal(
	SG_context * pCtx,
	const SG_treenode_entry * pTreenodeEntry1,
	const SG_treenode_entry * pTreenodeEntry2,
	SG_bool * pbResult
	)
{
	SG_NULLARGCHECK_RETURN(pTreenodeEntry1);
	SG_NULLARGCHECK_RETURN(pTreenodeEntry2);

	SG_ERR_CHECK_RETURN(  SG_vhash__equal(pCtx, (const SG_vhash *)pTreenodeEntry1, (const SG_vhash *)pTreenodeEntry2, pbResult)  );
}

//////////////////////////////////////////////////////////////////

#if DEBUG && !defined(SG_IOS)
void SG_treenode_entry_debug__dump__v1(
	SG_context * pCtx,
	const SG_treenode_entry * pTreenodeEntry,
	SG_repo * pRepo,
	const char * szGidObject,
	int indent,
	SG_bool bRecursive,
	SG_string * pStringResult
	)
{
	// for a version 1 treenode-entry, dump the fields we have.
	SG_treenode_entry_type tneType;
	SG_int64 attrs;
	const char * szHidBlob;
	const char * szEntryname;

	char bufLine[SG_ERROR_BUFFER_SIZE*2];
	char bufHexBits[20];

	SG_ERR_IGNORE(  SG_treenode_entry__get_entry_type(pCtx,pTreenodeEntry,&tneType)  );
	SG_ERR_IGNORE(  SG_treenode_entry__get_hid_blob(pCtx,pTreenodeEntry,&szHidBlob)  );
	SG_ERR_IGNORE(  SG_treenode_entry__get_attribute_bits(pCtx,pTreenodeEntry,&attrs)  );
	SG_ERR_IGNORE(  SG_treenode_entry__get_entry_name(pCtx,pTreenodeEntry,&szEntryname)  );

	SG_hex__format_uint64(bufHexBits,(SG_uint64)attrs);

	SG_ERR_IGNORE(  SG_sprintf(pCtx,bufLine,SG_NrElements(bufLine),
							   "%*cTNE1  [gid %s]: [type %d][hid %s][attrs %s] %s\n",
							   indent,' ',
							   szGidObject,
							   (SG_uint32)tneType,
							   szHidBlob,
							   bufHexBits,
							   szEntryname)  );
	SG_ERR_IGNORE(  SG_string__append__sz(pCtx,pStringResult,bufLine)  );

	if (tneType == SG_TREENODEENTRY_TYPE_SUBMODULE)
	{
#if 0
		SG_ERR_IGNORE(  SG_treenode_submodule_debug__dump_link__by_hid(pCtx, szHidBlob,pRepo,indent+2,pStringResult)  );
#endif
	}
	else if (bRecursive && (tneType == SG_TREENODEENTRY_TYPE_DIRECTORY))
	{
		SG_ERR_IGNORE(  SG_treenode_debug__dump__by_hid(pCtx, szHidBlob,pRepo,indent+2,bRecursive,pStringResult)  );
	}
}
#endif

//////////////////////////////////////////////////////////////////
