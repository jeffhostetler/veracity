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

#include "precompiled.h"
#include "vvVerbs.h"

#include "sg.h"
#include "sg_workingdir_prototypes.h"
#include <sg_wc__public_prototypes.h>
#include "sg_vv2__public_prototypes.h"
#include <sg_vv2__public_typedefs.h>
#include <vv6history/sg_vv6history__private_prototypes.h>
#include "../../libraries/mongoose/sg_mongoose.h"
#include "wc6resolve/sg_wc_tx__resolve__private_typedefs.h"
#include "wc8api/sg_wc8api__public_prototypes.h"
#include "vvContext.h"
#include "vvRevSpec.h"
#include "vvSgHelpers.h"
#include "vvHistoryObjects.h"
#include "vvCacheMessaging.h"

#include  "wc6resolve/sg_wc_tx__resolve__private_typedefs.h"

#include  "wc6resolve/sg_wc_tx__resolve__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__private_prototypes2.h"
#include  "wc6resolve/sg_wc_tx__resolve__choice__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__item__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__state__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__value__private_prototypes.h"

/*
**
** Globals
**
*/

const char* vvVerbs::sszRepoRoot = "@";
const char* vvVerbs::sszNobodyUsername = SG_NOBODY__USERNAME;
const char* vvVerbs::sszMainBranch = SG_VC_BRANCHES__DEFAULT;

const char* vvVerbs::sszResolveLabel_Ancestor  = SG_RESOLVE__LABEL__ANCESTOR;
const char* vvVerbs::sszResolveLabel_Baseline  = SG_RESOLVE__LABEL__BASELINE;
const char* vvVerbs::sszResolveLabel_Other     = SG_RESOLVE__LABEL__OTHER;
const char* vvVerbs::sszResolveLabel_Working   = SG_RESOLVE__LABEL__WORKING;
const char* vvVerbs::sszResolveLabel_Automerge = SG_RESOLVE__LABEL__AUTOMERGE;
const char* vvVerbs::sszResolveLabel_Merge     = SG_RESOLVE__LABEL__MERGE;


/*
**
** Internal Functions
**
*/

void _TranslateItemType(
	SG_context*                   pCtx,
	SG_wc_status_flags        eType,
	vvVerbs::DiffEntry::ItemType* pType
	)
{
	SG_NULLARGCHECK(pType);

	switch (eType & SG_WC_STATUS_FLAGS__T__MASK)
	{
	case SG_WC_STATUS_FLAGS__T__FILE:
		*pType = vvVerbs::DiffEntry::ITEM_FILE;
		break;
	case SG_WC_STATUS_FLAGS__T__DIRECTORY:
		*pType = vvVerbs::DiffEntry::ITEM_FOLDER;
		break;
	case SG_WC_STATUS_FLAGS__T__SYMLINK:
		*pType = vvVerbs::DiffEntry::ITEM_SYMLINK;
		break;
	case SG_WC_STATUS_FLAGS__T__SUBREPO:
		*pType = vvVerbs::DiffEntry::ITEM_SUBMODULE;
		break;
	default:
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Unknown SG_TREENODEENTRY_TYPE: %d", eType));
	}

fail:
	return;
}

void _TranslateUser(
	SG_context*        pCtx,
	const SG_vhash*    pUser,
	vvVerbs::UserData& cUser
	)
{
	static const char* szKey_Gid      = "recid";
	static const char* szKey_Name     = "name";
	static const char* szKey_Prefix   = "prefix";
	static const char* szKey_Inactive = "inactive";

	SG_bool     bValue  = SG_FALSE;
	const char* szValue = NULL;

	SG_NULLARGCHECK(pUser);

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pUser, szKey_Gid, &szValue)  );
	cUser.sGid = vvSgHelpers::Convert_sglibString_wxString(szValue);
	cUser.bNobody = (strcmp(szValue, SG_NOBODY__USERID) == 0);
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pUser, szKey_Name, &szValue)  );
	cUser.sName = vvSgHelpers::Convert_sglibString_wxString(szValue);
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pUser, szKey_Prefix, &szValue)  );
	cUser.sPrefix = vvSgHelpers::Convert_sglibString_wxString(szValue);

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pUser, szKey_Inactive, &bValue)  );
	if (bValue != SG_FALSE)
	{
		SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pUser, szKey_Inactive, &bValue)  );
	}
	cUser.bActive = !vvSgHelpers::Convert_sgBool_cppBool(bValue);

fail:
	return;
}

void _TranslateDiffStatusFlags(
	SG_context*         pCtx,
	SG_wc_status_flags eFlags,
	bool * pbItemIgnored,
	vvFlags*            pFlags
	)
{
	vvFlags cIncomingFlags = eFlags;
	vvFlags cOutgoingFlags = 0;

	SG_NULLARGCHECK(pFlags);

	if (pbItemIgnored)
		*pbItemIgnored = cIncomingFlags.HasAnyFlag(SG_WC_STATUS_FLAGS__U__IGNORED);

	#define CHECK_FLAG(sgFlag, vvFlag)                           \
		if (cIncomingFlags.HasAnyFlag(sgFlag))                   \
		{                                                        \
			cOutgoingFlags.AddFlags(vvVerbs::DiffEntry::vvFlag); \
		}
	CHECK_FLAG(SG_WC_STATUS_FLAGS__S__ADDED,                   CHANGE_ADDED);
	CHECK_FLAG(SG_WC_STATUS_FLAGS__S__MERGE_CREATED,           CHANGE_ADDED);
	CHECK_FLAG(SG_WC_STATUS_FLAGS__XR__EXISTENCE,              CHANGE_ADDED);
	CHECK_FLAG(SG_WC_STATUS_FLAGS__S__DELETED,                 CHANGE_DELETED);
	CHECK_FLAG(SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,        CHANGE_MODIFIED);
	CHECK_FLAG(SG_WC_STATUS_FLAGS__U__LOST,                    CHANGE_LOST);
	CHECK_FLAG(SG_WC_STATUS_FLAGS__U__FOUND,                   CHANGE_FOUND);
	
	CHECK_FLAG(SG_WC_STATUS_FLAGS__S__RENAMED,                 CHANGE_RENAMED);
	CHECK_FLAG(SG_WC_STATUS_FLAGS__XR__NAME,                   CHANGE_RENAMED);
	CHECK_FLAG(SG_WC_STATUS_FLAGS__XR__LOCATION,               CHANGE_RENAMED);
	CHECK_FLAG(SG_WC_STATUS_FLAGS__S__MOVED,                   CHANGE_MOVED);
	CHECK_FLAG(SG_WC_STATUS_FLAGS__L__LOCKED_BY_USER,          CHANGE_LOCKED);
	CHECK_FLAG(SG_WC_STATUS_FLAGS__L__PENDING_VIOLATION,       CHANGE_LOCK_CONFLICT);
	CHECK_FLAG(SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,        CHANGE_ATTRS);
	#undef CHECK_FLAG

	*pFlags = cOutgoingFlags;

fail:
	return;
}

void _TranslateResolveState(
	SG_context*                  pCtx,
	SG_resolve__state            eState,
	vvVerbs::ResolveChoiceState* pState
	)
{
	SG_NULLARGCHECK(pState);

	switch (eState)
	{
	case SG_RESOLVE__STATE__EXISTENCE:
		*pState = vvVerbs::RESOLVE_STATE_EXISTENCE;
		break;
	case SG_RESOLVE__STATE__NAME:
		*pState = vvVerbs::RESOLVE_STATE_NAME;
		break;
	case SG_RESOLVE__STATE__LOCATION:
		*pState = vvVerbs::RESOLVE_STATE_LOCATION;
		break;
	case SG_RESOLVE__STATE__ATTRIBUTES:
		*pState = vvVerbs::RESOLVE_STATE_ATTRIBUTES;
		break;
	case SG_RESOLVE__STATE__CONTENTS:
		*pState = vvVerbs::RESOLVE_STATE_CONTENTS;
		break;
	case SG_RESOLVE__STATE__COUNT:
		*pState = vvVerbs::RESOLVE_STATE_COUNT;
		break;
	default:
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Unknown SG_resolve__state: %d", eState));
	}

fail:
	return;
}

/**
 * User data used by our SG_treediff2 callbacks.
 */
struct _TranslateDiffObject_Data
{
	SG_varray*      pvaStatus; //< The SG_varray the callback is from.
	vvVerbs::DiffData* pDiffData; //< The DiffData that we're operating on.
};

/**
 * Gets the GID of the item associated with a diff object.
 */
static wxString _TranslateDiffObject_GetGid(
	SG_context*                    pCtx,                //< [in] Error and context info.
	const SG_varray*                  WXUNUSED(pvaStatus), //< [in] The SG_varray that owns the object.
	const SG_vhash* pObject              //< [in] The object to get the data from.
	)
{
	const char* szGid   = NULL;
	wxString    sResult = wxEmptyString;
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pObject, "gid", &szGid)  );
	sResult = vvSgHelpers::Convert_sglibString_wxString(szGid);

fail:
	return sResult;
}

/**
 * Gets the item type associated with a diff object.
 */
static void _TranslateDiffObject_GetItemType_and_GetChangeFlags(
	SG_context*                    pCtx,                //< [in] Error and context info.
	const SG_varray*                  WXUNUSED(pvaStatus), //< [in] The SG_varray that owns the object.
	const SG_vhash* pObject,              //< [in] The object to get the data from.
	vvVerbs::DiffEntry::ItemType * pItemType,
	bool * pbItemIgnored,
	vvFlags * pChangeFlags
	)
{
	SG_int64 i64;
	SG_wc_status_flags statusFlags;
	SG_vhash * pvhStatus = NULL;
	vvVerbs::DiffEntry::ItemType eNewType = vvVerbs::DiffEntry::ITEM_FILE;
	vvFlags cFlags;
	
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pObject, "status", &pvhStatus)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhStatus, "flags", &i64)  );
	statusFlags = (SG_wc_status_flags)i64;
#if DEBUG
	SG_ERR_IGNORE(  SG_wc_debug__status__dump_flags_to_console(pCtx, statusFlags, "Flags2Properties")  );
#endif
	//Get the values first, before setting anything 
	if (pChangeFlags)
		SG_ERR_CHECK(  _TranslateDiffStatusFlags(pCtx, statusFlags, pbItemIgnored, &cFlags)  );
	
	if (pItemType)
		SG_ERR_CHECK(  _TranslateItemType(pCtx, statusFlags, &eNewType)  );

	//Pass what we found back to the client.
	if (pChangeFlags)
		*pChangeFlags = cFlags;
	
	if (pItemType)
		*pItemType = eNewType;
fail:
	;
}

/**
 * Gets the path associated with a diff object.
 */
static wxString _TranslateDiffObject_GetItemPath(
	SG_context*                    pCtx,                //< [in] Error and context info.
	const SG_varray*                  WXUNUSED(pvaStatus), //< [in] The SG_varray that owns the object.
	const SG_vhash* pObject              //< [in] The object to get the data from.
	)
{
	const char* szValue = NULL;
	wxString    sResult = wxEmptyString;
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pObject, "path", &szValue)  );
	sResult = vvSgHelpers::Convert_sglibString_wxString(szValue);

fail:
	return sResult;
}

static vvVerbs::LockList _TranslateDiffObject_Locks(
	SG_context*                    pCtx,                //< [in] Error and context info.
	const SG_varray*                  WXUNUSED(pvaStatus), //< [in] The SG_varray that owns the object.
	const SG_vhash* pObject              //< [in] The object to get the data from.
	)
{
	SG_varray* pvaLocks = NULL;
	vvVerbs::LockList returnList;
	SG_ERR_CHECK(  SG_vhash__check__varray(pCtx, pObject, "locks", &pvaLocks)  );

	if (pvaLocks)
	{
		SG_uint32 nCount = 0;
		SG_uint32 nIndex = 0;
		SG_vhash* pvhLockRecord = NULL;
		SG_ERR_CHECK(  SG_varray__count(pCtx, pvaLocks, &nCount)  );
		for (nIndex = 0; nIndex < nCount; nIndex++)
		{
            const char* psz_username = NULL;
            const char* psz_path = NULL;
            const char* psz_end_csid = NULL;
			const char* psz_start_hid = NULL;
            
            SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaLocks, nIndex, &pvhLockRecord)  );
			
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhLockRecord, "username", &psz_username)  );
			SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhLockRecord, "start_hid", &psz_start_hid)  );
			SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhLockRecord, "end_csid", &psz_end_csid)  );

			
			vvVerbs::LockEntry le;
			le.sUsername = psz_username;
			le.sStartHid = psz_start_hid;
			if (psz_end_csid != NULL)
				le.sEndCSID = psz_end_csid;
			if (psz_path != NULL)
				le.sRepoPath = psz_path;
			returnList.push_back(le);
		}
	}
fail:
	return returnList;
	
}
/**
 * Gets the content HID associated with a diff object.
 */
static wxString _TranslateDiffObject_GetContentHid(
	SG_context*                    pCtx,                //< [in] Error and context info.
	const SG_varray*                  WXUNUSED(pvaStatus), //< [in] The SG_varray that owns the object.
	const SG_vhash* pObject              //< [in] The object to get the data from.
	)
{
	const char* szValue = NULL;
	wxString    sResult = wxEmptyString;
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pObject, "hid", &szValue)  );
	
	if (szValue != NULL)
		sResult = vvSgHelpers::Convert_sglibString_wxString(szValue);

fail:
	return sResult;
}

/**
 * Gets the old content HID associated with a diff object.
 */
static wxString _TranslateDiffObject_GetOldContentHid(
	SG_context*                    pCtx,                //< [in] Error and context info.
	const SG_varray*                  WXUNUSED(pvaStatus), //< [in] The SG_varray that owns the object.
	const SG_vhash* pObject              //< [in] The object to get the data from.
	)
{
	const char* szValue = NULL;
	wxString    sResult = wxEmptyString;
	SG_vhash* pvh_subsection_baseline = NULL;
	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pObject, SG_WC__STATUS_SUBSECTION__B, &pvh_subsection_baseline)  );
	if (pvh_subsection_baseline)
	{
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_subsection_baseline, "hid", &szValue)  );
		sResult = vvSgHelpers::Convert_sglibString_wxString(szValue, wxEmptyString);
	}

fail:
	return sResult;
}

/**
 * Gets the old repo path associated with a diff object.
 */
static wxString _TranslateDiffObject_GetMoved_From(
	SG_context*                    pCtx,      //< [in] Error and context info.
	const SG_varray*                  pvaStatus, //< [in] The SG_varray that owns the object.
	const SG_vhash* pObject    //< [in] The object to get the data from.
	)
{
	const char* szValue = NULL;
	wxString    sResult = wxEmptyString;
	SG_vhash* pvh_subsection_baseline = NULL;
	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pObject, SG_WC__STATUS_SUBSECTION__B, &pvh_subsection_baseline)  );
	if (pvh_subsection_baseline)
	{
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_subsection_baseline, "path", &szValue)  );
		sResult = vvSgHelpers::Convert_sglibString_wxString(szValue, wxEmptyString);
	}

fail:
	return sResult;
}

/**
 * Gets the old name associated with a diff object.
 */
static wxString _TranslateDiffObject_GetRenamed_From(
	SG_context*                    pCtx,                //< [in] Error and context info.
	const SG_varray*                  WXUNUSED(pvaStatus), //< [in] The SG_varray that owns the object.
	const SG_vhash* pObject              //< [in] The object to get the data from.
	)
{
	const char* szValue = NULL;
	wxString    sResult = wxEmptyString;
	SG_vhash* pvh_subsection_baseline = NULL;
	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pObject, SG_WC__STATUS_SUBSECTION__B, &pvh_subsection_baseline)  );
	if (pvh_subsection_baseline)
	{
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_subsection_baseline, "name", &szValue)  );
		sResult = vvSgHelpers::Convert_sglibString_wxString(szValue, wxEmptyString);
	}

fail:
	return sResult;
}

#if 0
/**
 * Gets the original submodule description associated with a diff object.
 */
static wxString _TranslateDiffObject_GetSubmodule_Original(
	SG_context*                    pCtx,                //< [in] Error and context info.
	const SG_varray*                  WXUNUSED(pvaStatus), //< [in] The SG_varray that owns the object.
	const SG_vhash* pObject              //< [in] The object to get the data from.
	)
{
	SG_treenode_submodule* pSubmodule = NULL;
	char*                  szValue    = NULL;
	wxString               sResult    = wxEmptyString;
	SG_ERR_CHECK(  SG_treediff2__ObjectData__get_old_tsm(pCtx, pObject, &pSubmodule)  );
	SG_ERR_CHECK(  SG_treenode_submodule__describe_for_status(pCtx, pSubmodule, &szValue)  );
	sResult = vvSgHelpers::Convert_sglibString_wxString(szValue);

fail:
	SG_TREENODE_SUBMODULE_NULLFREE(pCtx, pSubmodule);
	SG_NULLFREE(pCtx, szValue);
	return sResult;
}

/**
 * Gets the current submodule description associated with a diff object.
 */
static wxString _TranslateDiffObject_GetSubmodule_Current(
	SG_context*                    pCtx,                //< [in] Error and context info.
	const SG_varray*                  WXUNUSED(pvaStatus), //< [in] The SG_varray that owns the object.
	const SG_vhash* pObject              //< [in] The object to get the data from.
	)
{
	SG_treenode_submodule* pSubmodule = NULL;
	char*                  szValue    = NULL;
	wxString               sResult    = wxEmptyString;
	SG_ERR_CHECK(  SG_treediff2__ObjectData__get_tsm(pCtx, pObject, &pSubmodule)  );
	SG_ERR_CHECK(  SG_treenode_submodule__describe_for_status(pCtx, pSubmodule, &szValue)  );
	sResult = vvSgHelpers::Convert_sglibString_wxString(szValue);

fail:
	SG_TREENODE_SUBMODULE_NULLFREE(pCtx, pSubmodule);
	SG_NULLFREE(pCtx, szValue);
	return sResult;
}

/**
 * Gets the dirty flag from the current submodule associated with a diff object.
 */
static bool _TranslateDiffObject_GetSubmodule_Dirty(
	SG_context*                    pCtx,                //< [in] Error and context info.
	const SG_varray*                  WXUNUSED(pvaStatus), //< [in] The SG_varray that owns the object.
	const SG_vhash* pObject              //< [in] The object to get the data from.
	)
{
	SG_treenode_submodule* pSubmodule = NULL;
	SG_bool                bValue     = SG_FALSE;
	bool                   bResult    = false;
	SG_ERR_CHECK(  SG_treediff2__ObjectData__get_tsm(pCtx, pObject, &pSubmodule)  );
	SG_ERR_CHECK(  SG_treenode_submodule__is_dirty(pCtx, pSubmodule, &bValue)  );
	bResult = vvSgHelpers::Convert_sgBool_cppBool(bValue);

fail:
	SG_TREENODE_SUBMODULE_NULLFREE(pCtx, pSubmodule);
	return bResult;
}
#endif

/**
 * Translates an SG_vhash into a vvVerbs::DiffEntry and adds it to a vvVerbs::DiffData.
 */
static void _TranslateDiffObject(
	SG_context*                    pCtx,      //< [in] [out] Error and context info.
	const SG_varray*                  pvaStatus, //< [in] The SG_varray that owns the object.
	const SG_vhash* pObject,   //< [in] The current object.
	vvVerbs::DiffEntry&            cEntry     //< [out] The translated diff entry.
	)
{
	// the logic for when to retrieve which fields is pulled from
	// _sg_report_status_for_section_vhash_cb in sg_treediff2.c
	SG_ERR_CHECK(  cEntry.sItemGid     = _TranslateDiffObject_GetGid(pCtx, pvaStatus, pObject)  );

	SG_ERR_CHECK(  _TranslateDiffObject_GetItemType_and_GetChangeFlags(pCtx, pvaStatus, pObject, &cEntry.eItemType, &cEntry.bItemIgnored, &cEntry.cItemChanges)  );
	SG_ERR_CHECK(  cEntry.sItemPath    = _TranslateDiffObject_GetItemPath(pCtx, pvaStatus, pObject)  );

	SG_ERR_CHECK(  cEntry.sContentHid  = _TranslateDiffObject_GetContentHid(pCtx, pvaStatus, pObject)  );
	if (cEntry.eItemType != vvVerbs::DiffEntry::ITEM_FOLDER && cEntry.cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_MODIFIED | vvVerbs::DiffEntry::CHANGE_ATTRS))
	{
		SG_ERR_CHECK(  cEntry.sOldContentHid = _TranslateDiffObject_GetOldContentHid(pCtx, pvaStatus, pObject)  );
	}
	if (cEntry.cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_MOVED))
	{
		SG_ERR_CHECK(  cEntry.sMoved_From = _TranslateDiffObject_GetMoved_From(pCtx, pvaStatus, pObject)  );
	}
	if (cEntry.cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_RENAMED))
	{
		SG_ERR_CHECK(  cEntry.sRenamed_From = _TranslateDiffObject_GetRenamed_From(pCtx, pvaStatus, pObject)  );
	}

	SG_ERR_CHECK(  cEntry.cLocks = _TranslateDiffObject_Locks(pCtx, pvaStatus, pObject)  );
#if 0
	if (cEntry.eItemType == vvVerbs::DiffEntry::ITEM_SUBMODULE)
	{
		if (cEntry.cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_MODIFIED | vvVerbs::DiffEntry::CHANGE_ADDED | vvVerbs::DiffEntry::CHANGE_FOUND))
		{
			SG_ERR_CHECK(  cEntry.sSubmodule_Current = _TranslateDiffObject_GetSubmodule_Current(pCtx, pvaStatus, pObject)  );
			SG_ERR_CHECK(  cEntry.bSubmodule_Dirty   = _TranslateDiffObject_GetSubmodule_Dirty(pCtx, pvaStatus, pObject)  );
		}
		if (cEntry.cItemChanges.HasAnyFlag(vvVerbs::DiffEntry::CHANGE_MODIFIED | vvVerbs::DiffEntry::CHANGE_DELETED | vvVerbs::DiffEntry::CHANGE_LOST))
		{
			SG_ERR_CHECK(  cEntry.sSubmodule_Original = _TranslateDiffObject_GetSubmodule_Original(pCtx, pvaStatus, pObject)  );
		}
	}
#endif

fail:
	return;
}

/**
 * An SG_varray_foreach_callback that translates each received object
 * into a vvVerbs::DiffEntry and adds it to a vvVerbs::DiffData list.
 */
static void _TranslateStandardDiffObject(
		SG_context* pCtx, 
		void* pVoidData, 
		const SG_varray* pva, 
		SG_uint32 ndx, 
		const SG_variant* pv)
{
	vvVerbs::DiffEntry         cEntry;
	_TranslateDiffObject_Data* pData  = (_TranslateDiffObject_Data*)pVoidData;
	SG_vhash * pvh = NULL;

	SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvh)  );

	SG_ERR_CHECK(  _TranslateDiffObject(pCtx, pva, pvh, cEntry)  );
	pData->pDiffData->push_back(cEntry);

fail:
	return;
}

/*
**
** Public Functions
**
*/

bool vvVerbs::Initialize(
	vvContext& pCtx
	)
{
	SG_lib__global_initialize(pCtx);
	return !pCtx.Error_Check();
}

void vvVerbs::Shutdown(
	vvContext& pCtx
	)
{
	SG_lib__global_cleanup(pCtx);
}


/*
**
** Public Functions
**
*/

wxString vvVerbs::GetLogFilePath(class vvContext&     pCtx,
	const char * szFilenameFormat)
{
	wxString returnVal;
	SG_string* pFilename = NULL;
	SG_pathname * pLogPath           = NULL;
	char*  szLogPath         = NULL;
	SG_time      cCurrentTime;
	
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__LOG_PATH, NULL, &szLogPath, NULL)  );
	if (szLogPath != NULL)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pLogPath, szLogPath)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__LOG_DIRECTORY(pCtx, &pLogPath)  );
	}

	SG_ERR_CHECK(  SG_time__local_time(pCtx, &cCurrentTime)  );
	
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pFilename, szFilenameFormat, cCurrentTime.year, cCurrentTime.month, cCurrentTime.mday)  );
	SG_ERR_CHECK(  SG_pathname__append__from_string(pCtx, pLogPath, pFilename)  );
	returnVal = SG_pathname__sz(pLogPath);
fail:
	SG_NULLFREE(pCtx, szLogPath);
	SG_STRING_NULLFREE(pCtx, pFilename);
	SG_PATHNAME_NULLFREE(pCtx, pLogPath);
	return returnVal;
}

void _ReportMergeStats(vvContext&       pCtx,
								SG_vhash * pvhStats)
{
	SG_int64                nCount;
	SG_int64                nCount2;
	wxString strMessage;

	nCount = 0;
	SG_ERR_CHECK(  SG_vhash__check__int64(pCtx, pvhStats, "unchanged",       &nCount)  );
	pCtx.Log_SetValue("UnChangedFiles", nCount, vvContext::LOG_FLAG_OUTPUT);

	nCount = 0;
	SG_ERR_CHECK(  SG_vhash__check__int64(pCtx, pvhStats, "changed",       &nCount)  );
	pCtx.Log_SetValue("Changed", nCount, vvContext::LOG_FLAG_OUTPUT);
	if (nCount > 0)
	{
		if (!strMessage.IsEmpty())
			strMessage.Append(", ");
		strMessage.Append(wxString::Format("Changed: %u", (SG_uint32)nCount));
	}

	nCount = 0;
	SG_ERR_CHECK(  SG_vhash__check__int64(pCtx, pvhStats, "deleted",       &nCount)  );
	pCtx.Log_SetValue("Deleted", nCount, vvContext::LOG_FLAG_OUTPUT);
	if (nCount > 0)
	{
		if (!strMessage.IsEmpty())
			strMessage.Append(", ");
		strMessage.Append(wxString::Format("Deleted: %u", (SG_uint32)nCount));
	}

	nCount = 0;
	nCount2 = 0;
	SG_ERR_CHECK(  SG_vhash__check__int64(pCtx, pvhStats, "added_by_other",       &nCount)  );
	SG_ERR_CHECK(  SG_vhash__check__int64(pCtx, pvhStats, "override_delete",       &nCount2)  );
	pCtx.Log_SetValue("Added", nCount, vvContext::LOG_FLAG_OUTPUT);
	if (nCount+nCount2 > 0)
	{
		if (!strMessage.IsEmpty())
			strMessage.Append(", ");
		strMessage.Append(wxString::Format("Added: %u", (SG_uint32)(nCount+nCount2)));
	}

	nCount = 0;
	SG_ERR_CHECK(  SG_vhash__check__int64(pCtx, pvhStats, "merged",       &nCount)  );
	pCtx.Log_SetValue("Merged", nCount, vvContext::LOG_FLAG_OUTPUT);
	if (nCount > 0)
	{
		if (!strMessage.IsEmpty())
			strMessage.Append(", ");
		strMessage.Append(wxString::Format("Merged: %u", (SG_uint32)nCount));
	}

	nCount = 0;
	SG_ERR_CHECK(  SG_vhash__check__int64(pCtx, pvhStats, "restored_lost",       &nCount)  );
	pCtx.Log_SetValue("Restored", nCount, vvContext::LOG_FLAG_OUTPUT);
	if (nCount > 0)
	{
		if (!strMessage.IsEmpty())
			strMessage.Append(", ");
		strMessage.Append(wxString::Format("Restored: %u", (SG_uint32)nCount));
	}

	nCount = 0;
	SG_ERR_CHECK(  SG_vhash__check__int64(pCtx, pvhStats, "resolved",       &nCount)  );
	pCtx.Log_SetValue("Resolved", nCount, vvContext::LOG_FLAG_OUTPUT);
	if (nCount > 0)
	{
		if (!strMessage.IsEmpty())
			strMessage.Append(", ");
		strMessage.Append(wxString::Format("Resolved: %u", (SG_uint32)nCount));
	}

	if (!strMessage.IsEmpty())
		pCtx.Log_ReportMessage(vvContext::LOG_MESSAGE_INFO, "%s", strMessage);
	nCount = 0;
	SG_ERR_CHECK(  SG_vhash__check__int64(pCtx, pvhStats, "unresolved",       &nCount)  );
	pCtx.Log_SetValue("Unresolved", nCount, vvContext::LOG_FLAG_OUTPUT);
	if (nCount > 0)
		pCtx.Log_ReportMessage(vvContext::LOG_MESSAGE_WARNING, "Conflicts: %u", (SG_uint32)nCount);

fail:
	;
}


bool vvVerbs::FindWorkingFolder(
	class vvContext&     cContext,
	const wxArrayString& cPaths,
	wxString*            pFolder,
	wxString*            pRepo,
	wxArrayString*        pBaselineHIDs
	)
{
	wxString sFolder = wxEmptyString;
	wxString sRepo   = wxEmptyString;

	// run through each specified path
	for (wxArrayString::const_iterator it = cPaths.begin(); it != cPaths.end(); ++it)
	{
		// find a working copy from this path
		wxString sCurrentFolder;
		wxString sCurrentRepo;
		if (!this->FindWorkingFolder(cContext, *it, &sCurrentFolder, &sCurrentRepo, pBaselineHIDs))
		{
			return false;
		}

		// if we already found a working copy, make sure this one matches
		if (!sFolder.IsEmpty() && sFolder != sCurrentFolder)
		{
			return false;
		}

		// store this working copy and repo
		if (sFolder.IsEmpty())
		{
			sFolder = sCurrentFolder;
			sRepo   = sCurrentRepo;
		}
	}

	// if we got this far, we found exactly one working copy
	if (pFolder != NULL)
	{
		*pFolder = sFolder;
	}
	if (pRepo != NULL)
	{
		*pRepo = sRepo;
	}
	return true;
}

bool vvVerbs::GetRepoPaths(
	class vvContext&     cContext,
	const wxArrayString& cAbsolutePaths,
	const wxString&      sWorkingFolder,
	wxArrayString&       cRepoPaths
	)
{
	for (wxArrayString::const_iterator it = cAbsolutePaths.begin(); it != cAbsolutePaths.end(); ++it)
	{
		wxString sRepoPath;
		if (!this->GetRepoPath(cContext, *it, sWorkingFolder, sRepoPath))
		{
			return false;
		}
		cRepoPaths.Add(sRepoPath);
	}
	return true;
}

bool vvVerbs::GetRelativePath(
	class vvContext& cContext,
	const wxString&  sRepoPath,
	wxString&        sRelativePath,
	bool bLogError
	)
{
	wxString sRepoRoot   = vvVerbs::sszRepoRoot;
	wxString sSeparators = wxFileName::GetPathSeparators();
	for (wxString::const_iterator it = sSeparators.begin(); it != sSeparators.end(); ++it)
	{
		wxString sPrefix = sRepoRoot + *it;
		if (sRepoPath.StartsWith(sPrefix, &sRelativePath))
		{
			return true;
		}
	}
	if (bLogError)
		cContext.Log_ReportError("Given path isn't a repository path: %s", sRepoPath);
	return false;
}

bool vvVerbs::GetRelativePaths(
	class vvContext&     cContext,
	const wxArrayString& cRepoPaths,
	wxArrayString&       cRelativePaths
	)
{
	for (wxArrayString::const_iterator it = cRepoPaths.begin(); it != cRepoPaths.end(); ++it)
	{
		wxString sRelativePath;
		if (!this->GetRelativePath(cContext, *it, sRelativePath))
		{
			return false;
		}
		cRelativePaths.Add(sRelativePath);
	}
	return true;
}

bool vvVerbs::GetAbsolutePath(
	class vvContext& cContext,
	const wxString&  sFolder,
	const wxString&  sPath,
	wxString&        sAbsolutePath,
	bool bLogError
	)
{
	wxString  sPathSeparators = wxFileName::GetPathSeparators();
	wxUniChar cPathSeparator  = wxFileName::GetPathSeparator();

	// use the given folder as the root path
	// append a path separator if it doesn't already end with one
	wxString sRootPath = sFolder;
	if (sPathSeparators.Find(sRootPath[sRootPath.Length()-1u]) == wxNOT_FOUND)
	{
		sRootPath.Append(cPathSeparator);
	}

	// if the given path is a repo path, convert it to a relative one
	// if it's already relative, then we're good to go
	wxString sRelativePath = sPath;
	if (sPath.StartsWith(sszRepoRoot))
	{
		if (!this->GetRelativePath(cContext, sPath, sRelativePath, bLogError))
		{
			return false;
		}
	}

	// convert the relative path to an absolute one
	// just by appending it to the working copy root
	sAbsolutePath = sRootPath + sRelativePath;

	// convert all the path separators to the default one
	for (wxString::const_iterator it = sPathSeparators.begin(); it != sPathSeparators.end(); ++it)
	{
		if (*it != cPathSeparator)
		{
			sAbsolutePath.Replace(*it, cPathSeparator);
		}
	}

	// success
	return true;
}

bool vvVerbs::CheckForOldRepoFormat(class vvContext& pCtx, const wxString& sRepoName)
{
	sRepoName;
	bool returnVal = false;
	SG_repo* pRepo = NULL;
    SG_varray* pva = NULL;
    SG_int64 t1 = -1;
    SG_int64 t2 = -1;

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, sRepoName.utf8_str(), &pRepo)  );

	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &t2)  );
    t1 = t2 - (5*1000);

	//This queries the last 5 seconds of audits, which should be a very small number.
    SG_repo__query_audits(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, t1, t2, NULL, &pva);
    if (SG_context__err_equals(pCtx, SG_ERR_OLD_AUDITS))
    {
		pCtx.Error_Reset();
		returnVal = true;
        goto fail;
    }
    else
    {
		SG_ERR_CHECK_CURRENT;
	}

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);

    SG_REPO_NULLFREE(pCtx, pRepo);

	return returnVal;
}

bool vvVerbs::GetAbsolutePaths(
	class vvContext&     cContext,
	const wxString&      sFolder,
	const wxArrayString& cPaths,
	wxArrayString&       cAbsolutePaths,
	bool			 bLogError
	)
{
	for (wxArrayString::const_iterator it = cPaths.begin(); it != cPaths.end(); ++it)
	{
		wxString sAbsolutePath;
		if (!this->GetAbsolutePath(cContext, sFolder, *it, sAbsolutePath, bLogError))
		{
			return false;
		}
		cAbsolutePaths.Add(sAbsolutePath);
	}
	return true;
}

bool vvVerbs::GetGid(
	vvContext&      pCtx,
	const wxString& sAbsolutePath,
	wxString&       sGid
	)
{
	char*           szGid         = NULL;
	SG_pathname * pWCPath = NULL;
	SG_ERR_CHECK(  pWCPath = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sAbsolutePath)  );

	SG_ERR_CHECK(  SG_wc__get_item_gid(pCtx, pWCPath, vvSgHelpers::Convert_wxString_sglibString(pCtx, sAbsolutePath), &szGid)  );
	sGid = vvSgHelpers::Convert_sglibString_wxString(szGid);

fail:
	SG_PATHNAME_NULLFREE(pCtx, pWCPath);
	SG_NULLFREE(pCtx, szGid);
	return !pCtx.Error_Check();
}

bool vvVerbs::GetLocalRepos(
	vvContext&     pCtx,
	wxArrayString& cLocalRepos
	)
{
	// regular expression to filter out test repos
	static wxRegEx cTestNamePattern("^t[0-9A-Fa-f]{32}$|^vscript test repo for repo[0-9]{13}$");

	SG_vhash*  pRepoHash  = NULL;
	SG_varray* pRepoList  = NULL;
	SG_uint32  uRepoCount = NULL;

	// setup
	pCtx.Log_PushOperation("Retrieving local repositories", vvContext::LOG_FLAG_VERBOSE);
	SG_ASSERT(cTestNamePattern.IsValid());

	// get the list of repos from the closet
	SG_ERR_CHECK(  SG_closet__descriptors__list(pCtx, &pRepoHash)  );
	SG_ERR_CHECK(  SG_vhash__get_keys(pCtx, pRepoHash, &pRepoList)  );

	// run through the list
	SG_ERR_CHECK(  SG_varray__count(pCtx, pRepoList, &uRepoCount)  );
	pCtx.Log_SetSteps(uRepoCount);
	for (SG_uint32 uIndex = 0u; uIndex < uRepoCount; ++uIndex)
	{
		const char* szRepoName = NULL;
		wxString    sRepoName;

		// get the current repo name
		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pRepoList, uIndex, &szRepoName)  );
		sRepoName = vvSgHelpers::Convert_sglibString_wxString(szRepoName);
		pCtx.Log_SetStep(sRepoName);

		// check if this is a test repo
		if (cTestNamePattern.Matches(sRepoName))
		{
			// skip the test repo
			pCtx.Log_ReportVerbose("Skipping test repository: %s", sRepoName);
		}
		else
		{
			// add the real repo to the list
			pCtx.Log_ReportVerbose("Found repository: %s", sRepoName);
			cLocalRepos.Add(sRepoName);
		}

		// next
		pCtx.Log_FinishStep();
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pRepoList);
	SG_VHASH_NULLFREE(pCtx, pRepoHash);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::LocalRepoExists(
	vvContext&      pCtx,
	const wxString& sName
	)
{
	SG_vhash*  pRepoHash  = NULL;
	SG_varray* pRepoList  = NULL;
	SG_uint32  uRepoCount = NULL;
	bool       bFound     = false;

	pCtx.Log_PushOperation("Checking if repository exists", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sName, vvContext::LOG_FLAG_INPUT);

	// get the list of repos from the closet
	SG_ERR_CHECK(  SG_closet__descriptors__list(pCtx, &pRepoHash)  );
	SG_ERR_CHECK(  SG_vhash__get_keys(pCtx, pRepoHash, &pRepoList)  );

	// run through the list
	SG_ERR_CHECK(  SG_varray__count(pCtx, pRepoList, &uRepoCount)  );
	pCtx.Log_SetSteps(uRepoCount);
	for (SG_uint32 uIndex = 0u; uIndex < uRepoCount && !bFound; ++uIndex)
	{
		const char* szRepoName = NULL;
		wxString    sRepoName;

		// get the current repo name
		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pRepoList, uIndex, &szRepoName)  );
		sRepoName = vvSgHelpers::Convert_sglibString_wxString(szRepoName);
		pCtx.Log_SetStep(sRepoName);

		// check if this is the name we're looking for
		if (sName == sRepoName) {
			bFound = true;
		}
		pCtx.Log_FinishStep();
	}

	// mention our result
	pCtx.Log_SetValue("Exists", bFound, vvContext::LOG_FLAG_OUTPUT);

fail:
	SG_VARRAY_NULLFREE(pCtx, pRepoList);
	SG_VHASH_NULLFREE(pCtx, pRepoHash);
	pCtx.Log_PopOperation();
	return bFound;
}

bool vvVerbs::GetRemoteRepos(
	vvContext&     pCtx,
	const wxString& sRepoDescriptorName,
	wxArrayString& cRemoteRepos,
	wxArrayString& cRemoteRepoAliases
	)
{
	SG_string * pString = NULL;
	SG_vhash * pvhBindings = NULL;
	SG_uint32 nCountBindings = 0;
	SG_uint32 i = 0;
	const SG_variant * pVariant = NULL;
	const char * pszKeyName = NULL;
	const char * pszRepoString = NULL;
	SG_varray * pva_targets = NULL;
	wxArrayString repos;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	if (sRepoDescriptorName.IsEmpty())
	{
		GetLocalRepos(pCtx, repos);
	}
	else
		repos.Add(sRepoDescriptorName);

	for (wxArrayString::const_iterator it = repos.begin(); it != repos.end(); ++it)
	{
		const char* sgit = vvSgHelpers::Convert_wxString_sglibString(*it).data();

		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString, "%s/%s/%s",
									  SG_LOCALSETTING__SCOPE__INSTANCE,
									  sgit,
									  SG_LOCALSETTING__PATHS)  );
	

		SG_ERR_IGNORE(  SG_localsettings__get__vhash(pCtx,
												 SG_string__sz(pString),
												 NULL,		// null repo because we only want /instance/ settings.
												 &pvhBindings,
												 NULL)  );

		if (pvhBindings != NULL)
		{
			SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhBindings, &nCountBindings)  );
			for (i = 0; i < nCountBindings; i++)
			{
				wxString sRepoString;

				SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhBindings, i, &pszKeyName, &pVariant)  );
				SG_ERR_CHECK(  SG_variant__get__sz(pCtx, pVariant, &pszRepoString)  );
				sRepoString = vvSgHelpers::Convert_sglibString_wxString(pszRepoString);

				if (cRemoteRepos.Index(sRepoString, false) < 0)
				{
					cRemoteRepoAliases.Add(vvSgHelpers::Convert_sglibString_wxString(pszKeyName));
					cRemoteRepos.Add(sRepoString);
				}
			}
		}
		//Now get the "previous sync targets" that we save in localsettings
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString, "%s/%s/%s",
										SG_LOCALSETTING__SCOPE__INSTANCE,
										sgit,
										SG_LOCALSETTING__SYNC_TARGETS)  );
		SG_ERR_CHECK(  SG_localsettings__get__varray(pCtx, SG_string__sz(pString), NULL, &pva_targets, NULL)  );
		if (pva_targets)
		{
			SG_uint32 nCountTargets = 0;
			SG_uint32 nIndex = 0;
			SG_ERR_CHECK(  SG_varray__count(pCtx, pva_targets, &nCountTargets)  );
			for (nIndex = 0; nIndex < nCountTargets; nIndex++)
			{
				wxString sRepoString;

				SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_targets, nIndex, &pszRepoString)  );
				sRepoString = vvSgHelpers::Convert_sglibString_wxString(pszRepoString);

				if (cRemoteRepos.Index(sRepoString, false) < 0)
				{
					//We haven't added this path yet, add it (with an empty alias)
					cRemoteRepoAliases.Add("");
					cRemoteRepos.Add(sRepoString);
				}
			}
		}
	

	}
	

	SG_VARRAY_NULLFREE(pCtx, pva_targets);
	SG_STRING_NULLFREE(pCtx, pString);
	
	SG_VHASH_NULLFREE(pCtx, pvhBindings);
	// success
	return true;
fail:
	SG_VARRAY_NULLFREE(pCtx, pva_targets);
	SG_STRING_NULLFREE(pCtx, pString);
	SG_VHASH_NULLFREE(pCtx, pvhBindings);
	return false;
}

bool vvVerbs::FindWorkingFolder(
	class vvContext& pCtx,
	const wxString&  sPath,
	wxString*        pFolder,
	wxString*        pRepo,
	wxArrayString*        pBaselineHIDs
	)
{
	SG_pathname*     pInputPath   = NULL;
	SG_pathname*     pOutputPath  = NULL;
	SG_string*       pOutputRepo  = NULL;
	SG_stringarray*	 psa_parents  = NULL;
	pCtx.Log_PushOperation("Finding working copy", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Path", sPath, vvContext::LOG_FLAG_INPUT);

	SG_ERR_CHECK(  pInputPath = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sPath)  );
	SG_ERR_CHECK(  SG_workingdir__find_mapping2(pCtx, pInputPath, SG_TRUE, &pOutputPath, &pOutputRepo, NULL)  );

	if (pFolder != NULL)
	{
		pFolder->assign(vvSgHelpers::Convert_sgPathname_wxString(pOutputPath));
		pCtx.Log_SetValue("Working copy", *pFolder, vvContext::LOG_FLAG_OUTPUT);
	}
	if (pRepo != NULL)
	{
		pRepo->assign(vvSgHelpers::Convert_sgString_wxString(pOutputRepo));
		pCtx.Log_SetValue("Repository", *pRepo, vvContext::LOG_FLAG_OUTPUT);
	}
	if (pBaselineHIDs != NULL)
	{
		SG_ERR_CHECK(  SG_wc__get_wc_parents__stringarray(pCtx, pOutputPath, &psa_parents)  );
		SG_ERR_CHECK(  vvSgHelpers::Convert_sgStringArray_wxArrayString(pCtx, psa_parents, *pBaselineHIDs)  );
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pInputPath);
	SG_PATHNAME_NULLFREE(pCtx, pOutputPath);
	SG_STRING_NULLFREE(pCtx, pOutputRepo);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_parents);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::GetRepoPath(
	vvContext&      pCtx,
	const wxString& sAbsolutePath,
	const wxString& sWorkingFolder,
	wxString&       sRepoPath
	)
{
	SG_pathname* pWorkingFolder = NULL;
	SG_string*   pRepoPath      = NULL;

	// setup
	pCtx.Log_PushOperation("Converting to repo path", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Absolute Path", sAbsolutePath, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("Working Copy", sWorkingFolder, vvContext::LOG_FLAG_INPUT);

	// do the actual conversion
	SG_pathname * pWCPath = NULL;
	SG_ERR_CHECK(  pWCPath = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sAbsolutePath)  );

	pWorkingFolder = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sWorkingFolder);
	SG_ERR_CHECK(  SG_workingdir__wdpath_to_repopath(pCtx, pWorkingFolder, pWCPath, SG_FALSE, &pRepoPath)  );

	// return the result
	sRepoPath.assign(vvSgHelpers::Convert_sgString_wxString(pRepoPath));
	pCtx.Log_SetValue("Repository Path", sRepoPath, vvContext::LOG_FLAG_OUTPUT);

fail:
	SG_PATHNAME_NULLFREE(pCtx, pWCPath);
	SG_PATHNAME_NULLFREE(pCtx, pWorkingFolder);
	SG_STRING_NULLFREE(pCtx, pRepoPath);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

wxArrayString vvVerbs::GetBranchDisplayNames(vvVerbs::BranchList bl)
{
	wxArrayString names;
	for (vvVerbs::BranchList::const_iterator it = bl.begin(); it != bl.end(); ++it)
	{
		names.Add(it->sBranchNameForDisplay);
	}
	return names;
}

bool vvVerbs::BranchEntryComparer(const vvVerbs::BranchEntry& b1, const vvVerbs::BranchEntry& b2)
{
	return b1.sBranchNameForDisplay < b2.sBranchNameForDisplay;
}

bool vvVerbs::GetBranches(
	vvContext&      pCtx,
	const wxString& sRepo,
	bool bIncludeClosed,
	vvVerbs::BranchList&  cBranches
	)
{
	static const char* szKey_Branches = "branches";
	static const char* szKey_Closed = "closed";
	static const char* szKey_Values   = "values";

	SG_repo*  pRepo           = NULL;
	SG_vhash* pBranchPile     = NULL;
	SG_vhash* pBranches       = NULL;
	SG_vhash* pvh_pile_closed = NULL;
	SG_uint32 uBranchCount    = 0u;

	// setup
	pCtx.Log_PushOperation("Retrieving branches", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);

	// open the repo and get a list of branches from it
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pBranchPile)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pBranchPile, szKey_Branches, &pBranches)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pBranchPile, szKey_Closed, &pvh_pile_closed)  );

	// run through the list
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pBranches, &uBranchCount)  );
	pCtx.Log_SetSteps(uBranchCount);
	for (SG_uint32 uIndex = 0u; uIndex < uBranchCount; ++uIndex)
	{
		const char* szName = NULL;
		wxString    sName;
		SG_bool b_include_this_one = SG_FALSE;

		// get the current branch name and add it to the list
		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pBranches, uIndex, &szName, NULL)  );
		sName = vvSgHelpers::Convert_sglibString_wxString(szName);

		SG_bool b_closed = SG_FALSE;
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pile_closed, szName, &b_closed)  );

		if (bIncludeClosed)
        {
            b_include_this_one = SG_TRUE;
        }
        else
        {
            if (b_closed)
            {
                b_include_this_one = SG_FALSE;
            }
            else
            {
                b_include_this_one = SG_TRUE;
            }
        }

		if (b_include_this_one == SG_FALSE)
		{
			pCtx.Log_ReportVerbose("Skipping closed branch: %s", sName);
		}
		else
		{
			pCtx.Log_ReportVerbose("Found branch: %s", sName);
			BranchEntry cBranch;
			cBranch.sBranchName = sName;
			if (b_closed)
			{
				cBranch.bClosed =  vvSgHelpers::Convert_sgBool_cppBool(b_closed);
				cBranch.sBranchNameForDisplay = cBranch.sBranchName + " (closed)";
			}
			else
				cBranch.sBranchNameForDisplay = cBranch.sBranchName;
			cBranches.push_back(cBranch);
		}
		pCtx.Log_FinishStep();
	}

	std::sort(cBranches.begin(), cBranches.end());

fail:
	SG_VHASH_NULLFREE(pCtx, pBranchPile);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::BranchExists(
	vvContext&       pCtx,
	const wxString&  sRepo,
	const wxString&  sBranch,
	bool&            bExists,
	bool&			 bOpen
	)
{
	static const char* szKey_Branches = "branches";
	static const char* szKey_Closed   = "closed";

	SG_repo*  pRepo       = NULL;
	SG_vhash* pBranchPile = NULL;
	SG_vhash* pBranches   = NULL;
	SG_vhash* pClosed     = NULL;
	SG_bool   bFound      = SG_FALSE;

	// setup
	pCtx.Log_PushOperation("Checking for branch existence", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("Branch", sBranch, vvContext::LOG_FLAG_INPUT);

	// open the repo and get a list of branches from it
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pBranchPile)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pBranchPile, szKey_Branches, &pBranches)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pBranchPile, szKey_Closed, &pClosed)  );

	// check if the branch we're looking for appears in the list
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pBranches, vvSgHelpers::Convert_wxString_sglibString(sBranch), &bFound)  );

	// if it exists and we care about its closed state, check it
	if (bFound == SG_TRUE)
	{
		SG_bool bClosed = SG_FALSE;

		SG_ERR_CHECK(  SG_vhash__has(pCtx, pClosed, vvSgHelpers::Convert_wxString_sglibString(sBranch), &bClosed)  );
		//If the branch is in the closed vhash, we need to set the bOpen to false.
		bOpen = !vvSgHelpers::Convert_sgBool_cppBool(bClosed);
	}
	else
	{
		//The branch doesn't exist, so it's not open.
		bOpen = false;
	}

	// return/output what we found
	bExists = vvSgHelpers::Convert_sgBool_cppBool(bFound);
	pCtx.Log_SetValue("Exists", bExists, vvContext::LOG_FLAG_OUTPUT);
	pCtx.Log_SetValue("IsOpen", bOpen, vvContext::LOG_FLAG_OUTPUT);

fail:
	SG_VHASH_NULLFREE(pCtx, pBranchPile);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::GetUsers(
	vvContext&       pCtx,
	const wxString&  sRepo,
	stlUserDataList& cUsers
	)
{
	SG_repo*   pRepo  = NULL;
	SG_varray* pUsers = NULL;
	SG_uint32  uCount = 0u;

	// setup
	pCtx.Log_PushOperation("Retrieving users", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);

	// open the repo and get a list of users from it
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	SG_ERR_CHECK(  SG_user__list_all(pCtx, pRepo, &pUsers)  );

	// run through the list
	SG_ERR_CHECK(  SG_varray__count(pCtx, pUsers, &uCount)  );
	pCtx.Log_SetSteps(uCount);
	for (SG_uint32 uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		SG_vhash*   pUser = NULL;
		UserData    cUser;

		// get the current user data and add it to the list
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pUsers, uIndex, &pUser)  );
		SG_ERR_CHECK(  _TranslateUser(pCtx, pUser, cUser)  );
		pCtx.Log_ReportVerbose("Found user: %s", cUser.sName);
		cUsers.push_back(cUser);

		// done with this one
		pCtx.Log_FinishStep();
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pUsers);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::GetUsers(
	vvContext&       pCtx,
	const wxString&  sRepo,
	wxArrayString&   cUsers,
	vvNullable<bool> nbActive,
	bool             bNobody
	)
{
	SG_repo*   pRepo  = NULL;
	SG_varray* pUsers = NULL;
	SG_uint32  uCount = 0u;

	// setup
	pCtx.Log_PushOperation("Retrieving user names", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);
	if (nbActive.IsValid())
	{
		pCtx.Log_SetValue("ActiveEquals", nbActive.GetValue(), vvContext::LOG_FLAG_INPUT);
	}
	pCtx.Log_SetValue("IncludeNobody", bNobody, vvContext::LOG_FLAG_INPUT);

	// open the repo and get a list of users from it
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	SG_ERR_CHECK(  SG_user__list_all(pCtx, pRepo, &pUsers)  );

	// run through the list
	SG_ERR_CHECK(  SG_varray__count(pCtx, pUsers, &uCount)  );
	pCtx.Log_SetSteps(uCount);
	for (SG_uint32 uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		SG_vhash*   pUser = NULL;
		UserData    cUser;

		// get the current user data and add it to the list
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pUsers, uIndex, &pUser)  );
		SG_ERR_CHECK(  _TranslateUser(pCtx, pUser, cUser)  );
		if (!bNobody && cUser.bNobody)
		{
			pCtx.Log_ReportVerbose("Skipping nobody user: %s", cUser.sName);
		}
		else if (nbActive.IsValidAndEqual(true) && !cUser.bActive)
		{
			pCtx.Log_ReportVerbose("Skipping inactive user: %s", cUser.sName);
		}
		else if (nbActive.IsValidAndEqual(false) && cUser.bActive)
		{
			pCtx.Log_ReportVerbose("Skipping active user: %s", cUser.sName);
		}
		else
		{
			pCtx.Log_ReportVerbose("Found user: %s", cUser.sName);
			cUsers.push_back(cUser.sName);
		}

		// done with this one
		pCtx.Log_FinishStep();
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pUsers);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::GetUserById(
	vvContext&       pCtx,
	const wxString&  sRepo,
	const wxString&  sUserId,
	bool&            bExists,
	UserData*        pUser
	)
{
	SG_repo*  pRepo = NULL;
	SG_vhash* pHash = NULL;

	// setup
	pCtx.Log_PushOperation("Retrieving user by ID", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("User ID", sUserId, vvContext::LOG_FLAG_INPUT);

	// open the repo and find the user
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	SG_ERR_CHECK(  SG_user__lookup_by_userid(pCtx, pRepo, NULL, vvSgHelpers::Convert_wxString_sglibString(sUserId), &pHash)  );
	if (pHash != NULL)
	{
		bExists = true;

		// if they want the user's data, translate it
		if (pUser != NULL)
		{
			SG_ERR_CHECK(  _TranslateUser(pCtx, pHash, *pUser)  );
		}
	}
	else
	{
		bExists = false;
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pHash);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::GetUserByName(
	vvContext&       pCtx,
	const wxString&  sRepo,
	const wxString&  sUserName,
	bool&            bExists,
	UserData*        pUser
	)
{
	SG_repo*  pRepo = NULL;
	SG_vhash* pHash = NULL;

	// setup
	pCtx.Log_PushOperation("Retrieving user by name", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("Username", sUserName, vvContext::LOG_FLAG_INPUT);

	// open the repo and find the user
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	SG_ERR_CHECK(  SG_user__lookup_by_name(pCtx, pRepo, NULL, vvSgHelpers::Convert_wxString_sglibString(sUserName), &pHash)  );
	if (pHash != NULL)
	{
		bExists = true;

		// if they want the user's data, translate it
		if (pUser != NULL)
		{
			SG_ERR_CHECK(  _TranslateUser(pCtx, pHash, *pUser)  );
		}
	}
	else
	{
		bExists = false;
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pHash);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::GetCurrentUser(
	class vvContext& pCtx,
	const wxString&  sRepo,
	wxString&        sUser
	)
{
	SG_repo* pRepo  = NULL;
	char*    szUser = NULL;

	// setup
	pCtx.Log_PushOperation("Retrieving current user", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);

	// open the repo
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );

	// get the current username
	SG_user__get_username_for_repo(pCtx, pRepo, &szUser);
	if (SG_context__err_equals(pCtx, SG_ERR_NO_CURRENT_WHOAMI) == SG_TRUE)
	{
		sUser = wxEmptyString;
		pCtx.Error_Reset();
	}
	else if (SG_context__has_err(pCtx) == SG_TRUE)
	{
		SG_ERR_CHECK_CURRENT;
	}
	else
	{
		sUser = vvSgHelpers::Convert_sglibString_wxString(szUser);
	}

fail:
	SG_NULLFREE(pCtx, szUser);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::SetCurrentUser(
	class vvContext& pCtx,
	const wxString&  sRepo,
	const wxString&  sUser,
	bool             bCreate
	)
{
	SG_repo* pRepo = NULL;

	// setup
	pCtx.Log_PushOperation("Setting current user", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("User", sUser, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("Create", bCreate, vvContext::LOG_FLAG_INPUT);

	// open the repo
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );

	// if we should create non-existent users, do that
	if (bCreate)
	{
		bool bCreated = true;

		SG_ERR_TRY(  SG_user__create(pCtx, pRepo, vvSgHelpers::Convert_wxString_sglibString(sUser), NULL)  );
		SG_ERR_CATCH_SET(SG_ERR_ZING_CONSTRAINT, bCreated, false)
		SG_ERR_CATCH_END;

		pCtx.Log_ReportVerbose(bCreated ? "User created." : "User already exists.");
	}

	// set the current user
	SG_ERR_CHECK(  SG_user__set_user__repo(pCtx, pRepo, vvSgHelpers::Convert_wxString_sglibString(sUser))  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::CreateUser(
	vvContext&      pCtx,
	const wxString& sRepo,
	const wxString& sUsername,
	wxString*       pId
	)
{
	SG_repo* pRepo = NULL;
	char*    szGid = NULL;

	// setup
	pCtx.Log_PushOperation("Creating user", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("Username", sUsername, vvContext::LOG_FLAG_INPUT);

	// open the repo
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );

	// create the user
	SG_ERR_CHECK(  SG_user__create(pCtx, pRepo, vvSgHelpers::Convert_wxString_sglibString(sUsername), &szGid)  );

	// return the ID if they want it
	if (pId != NULL)
	{
		*pId = vvSgHelpers::Convert_sglibString_wxString(szGid);
	}

fail:
	SG_NULLFREE(pCtx, szGid);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::GetStamps(
	vvContext&        pCtx,
	const wxString&   sRepo,
	stlStampDataList& cStamps
	)
{
	static const char* szKey_Name  = "stamp";
	static const char* szKey_Count = "count";

	SG_repo*   pRepo   = NULL;
	SG_varray* pStamps = NULL;
	SG_uint32  uCount  = 0u;

	// setup
	pCtx.Log_PushOperation("Retrieving stamps", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);

	// open the repo and get a list of stamps from it
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	SG_ERR_CHECK(  SG_vc_stamps__list_all_stamps(pCtx, pRepo, &pStamps)  );

	// run through the list
	SG_ERR_CHECK(  SG_varray__count(pCtx, pStamps, &uCount)  );
	pCtx.Log_SetSteps(uCount);
	for (SG_uint32 uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		SG_vhash*   pStamp  = NULL;
		const char* szValue = NULL;
		SG_int64    iValue  = 0u;
		StampData   cStamp;

		// get the current stamp data and add it to the list
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pStamps, uIndex, &pStamp)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pStamp, szKey_Name, &szValue)  );
		cStamp.sName = vvSgHelpers::Convert_sglibString_wxString(szValue);
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pStamp, szKey_Count, &iValue)  );
		cStamp.uCount = static_cast<unsigned int>(iValue);
		pCtx.Log_ReportVerbose("Found stamp: %s", cStamp.sName);
		cStamps.push_back(cStamp);

		// done with this one
		pCtx.Log_FinishStep();
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pStamps);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::GetTags(
	vvContext&               pCtx,
	const wxString&          sRepo,
	vvVerbs::stlTagDataList& cTags
	)
{
	static const char* szKey_Name = "tag";
	static const char* szKey_CSID = "csid";

	SG_repo*   pRepo  = NULL;
	SG_varray* pTags  = NULL;
	SG_uint32  uCount = 0u;

	// setup
	pCtx.Log_PushOperation("Retrieving tags", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);

	// open the repo and get a list of tags from it
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	SG_ERR_CHECK(  SG_vc_tags__list_all(pCtx, pRepo, &pTags)  );

	// run through the list
	SG_ERR_CHECK(  SG_varray__count(pCtx, pTags, &uCount)  );
	pCtx.Log_SetSteps(uCount);
	for (SG_uint32 uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		SG_vhash*   pTag    = NULL;
		const char* szValue = NULL;
		vvVerbs::TagData cTag;

		// get the current tag data and add it to the list
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pTags, uIndex, &pTag)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pTag, szKey_Name, &szValue)  );
		cTag.sName = vvSgHelpers::Convert_sglibString_wxString(szValue);
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pTag, szKey_CSID, &szValue)  );
		cTag.sCSID = vvSgHelpers::Convert_sglibString_wxString(szValue);
		pCtx.Log_ReportVerbose("Found tag: %s", cTag.sName);
		cTags.push_back(cTag);

		// done with this one
		pCtx.Log_FinishStep();
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pTags);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::GetTags(
	vvContext&      pCtx,
	const wxString& sRepo,
	wxArrayString&  cTags
	)
{
	static const char* szKey_Name = "tag";

	SG_repo*   pRepo  = NULL;
	SG_varray* pTags  = NULL;
	SG_uint32  uCount = 0u;

	// setup
	pCtx.Log_PushOperation("Retrieving tags", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);

	// open the repo and get a list of tags from it
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	SG_ERR_CHECK(  SG_vc_tags__list_all(pCtx, pRepo, &pTags)  );

	// run through the list
	SG_ERR_CHECK(  SG_varray__count(pCtx, pTags, &uCount)  );
	pCtx.Log_SetSteps(uCount);
	for (SG_uint32 uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		SG_vhash*   pTag    = NULL;
		const char* szValue = NULL;

		// get the current tag name and add it to the list
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pTags, uIndex, &pTag)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pTag, szKey_Name, &szValue)  );
		cTags.push_back(vvSgHelpers::Convert_sglibString_wxString(szValue));
		pCtx.Log_ReportVerbose("Found tag: %s", cTags.back());

		// done with this one
		pCtx.Log_FinishStep();
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pTags);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::TagExists(
	vvContext&      pCtx,
	const wxString& sRepo,
	const wxString& sTag,
	bool&           bExists
	)
{
	SG_repo*   pRepo  = NULL;
	SG_rbtree* pTags  = NULL;
	SG_bool    bFound = SG_TRUE;

	// setup
	pCtx.Log_PushOperation("Checking for tag existence", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("Tag", sTag, vvContext::LOG_FLAG_INPUT);

	// open the repo and get a list of tags from it
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	SG_ERR_CHECK(  SG_vc_tags__list(pCtx, pRepo, &pTags)  );

	// check if the specified tag is in the list
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pTags, vvSgHelpers::Convert_wxString_sglibString(sTag), &bFound, NULL)  );

	// output/return the result
	bExists = vvSgHelpers::Convert_sgBool_cppBool(bFound);
	pCtx.Log_SetValue("Exists", bExists, vvContext::LOG_FLAG_OUTPUT);

fail:
	SG_RBTREE_NULLFREE(pCtx, pTags);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::RevisionExists(
	vvContext&      pCtx,
	const wxString& sRepo,
	const wxString& sRevision,
	bool&           bExists,
	wxString*       pFullHid
	)
{
	SG_repo* pRepo     = NULL;
	SG_bool  bFound    = SG_TRUE;
	char*    szFullHid = NULL;

	// setup
	pCtx.Log_PushOperation("Checking for revision existence", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Repository", sRepo, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("Revision", sRevision, vvContext::LOG_FLAG_INPUT);

	// open the repo
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );

	// lookup the revision
	SG_ERR_TRY(  SG_repo__hidlookup__dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, vvSgHelpers::Convert_wxString_sglibString(sRevision), &szFullHid)  );
	SG_ERR_CATCH_SET(SG_ERR_AMBIGUOUS_ID_PREFIX, bFound, SG_FALSE);
	SG_ERR_CATCH_SET(SG_ERR_NO_CHANGESET_WITH_PREFIX, bFound, SG_FALSE);
	SG_ERR_CATCH_END;

	// output/return what we found
	bExists = vvSgHelpers::Convert_sgBool_cppBool(bFound);
	pCtx.Log_SetValue("Exists", bExists, vvContext::LOG_FLAG_OUTPUT);
	if (bExists && pFullHid != NULL)
	{
		pFullHid->assign(vvSgHelpers::Convert_sglibString_wxString(szFullHid));
		pCtx.Log_SetValue("HID", *pFullHid, vvContext::LOG_FLAG_OUTPUT);
	}

fail:
	SG_NULLFREE(pCtx, szFullHid);
	SG_REPO_NULLFREE(pCtx, pRepo);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::GetBranch(
	vvContext&      pCtx,
	const wxString& sFolder,
	wxString&       sBranch
	)
{
	char*        szBranch = NULL;
	SG_pathname* pPath    = NULL;

	pCtx.Log_PushOperation("Retrieving tied branch", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Folder", sFolder, vvContext::LOG_FLAG_INPUT);

	SG_ERR_CHECK(  pPath = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFolder)  );
	SG_ERR_CHECK(  SG_wc__branch__get(pCtx, pPath, &szBranch)  );

	sBranch = vvSgHelpers::Convert_sglibString_wxString(szBranch, wxEmptyString);
	pCtx.Log_SetValue("Branch", sBranch, vvContext::LOG_FLAG_OUTPUT);

fail:
	SG_NULLFREE(pCtx, szBranch);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::New(
	vvContext&      pCtx,
	const wxString& sName,
	const wxString& sFolder,
	const wxString& sSharedUsersRepo,
	const wxString& sStorageDriver,
	const wxString& sHashMethod
	)
{
	const char* szName          = NULL;
	const char* szFolder        = NULL;
	SG_bool     bNoWD           = SG_TRUE;
	const char* szStorageDriver = NULL;
	const char* szHashMethod    = NULL;
	const char* szSharedUsersRepo = NULL;

	pCtx.Log_PushOperation("Creating new repository", vvContext::LOG_FLAG_NONE);
	pCtx.Log_SetValue("Repository", sName, vvContext::LOG_FLAG_INPUT);

	SG_ARGCHECK(!sName.IsEmpty(), sName);

	szName = vvSgHelpers::Convert_wxString_sglibString(sName);
	if (!sFolder.IsEmpty())
	{
		szFolder = vvSgHelpers::Convert_wxString_sglibString(sFolder);
		bNoWD    = SG_FALSE;
		pCtx.Log_SetValue("Checkout folder", sFolder, vvContext::LOG_FLAG_INPUT);
	}
	if (!sStorageDriver.IsEmpty())
	{
		szStorageDriver = vvSgHelpers::Convert_wxString_sglibString(sStorageDriver);
		pCtx.Log_SetValue("Storage driver", sStorageDriver, vvContext::LOG_FLAG_INPUT);
	}
	if (!sHashMethod.IsEmpty())
	{
		szHashMethod = vvSgHelpers::Convert_wxString_sglibString(sHashMethod);
		pCtx.Log_SetValue("Hash method", sHashMethod, vvContext::LOG_FLAG_INPUT);
	}
	if (!sSharedUsersRepo.IsEmpty())
	{
		szSharedUsersRepo = vvSgHelpers::Convert_wxString_sglibString(sSharedUsersRepo);
		pCtx.Log_SetValue("Shared users repo", sSharedUsersRepo, vvContext::LOG_FLAG_INPUT);
	}

	SG_ERR_CHECK(  SG_vv2__init_new_repo(pCtx, szName, szFolder, szStorageDriver, szHashMethod, bNoWD, szSharedUsersRepo, SG_FALSE, NULL, NULL)  );

fail:
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::Clone(
	vvContext&      pCtx,
	const wxString& sExistingRepo,
	const wxString& sNewRepoName,
	const wxString& sUsername,
	const wxString& sPassword
	)
{
	const char* szExistingRepo = NULL;
	const char* szNewRepoName  = NULL;
	const char* szUsername     = NULL;
	const char* szPassword     = NULL;
    SG_bool bNewIsRemote = SG_FALSE;
	SG_vhash* pvhCloneResult = NULL;

	pCtx.Log_PushOperation("Cloning repository", vvContext::LOG_FLAG_NONE);
	pCtx.Log_SetValue("Source", sExistingRepo, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("Target", sNewRepoName, vvContext::LOG_FLAG_INPUT);

	SG_ARGCHECK(!sExistingRepo.IsEmpty(), sExistingRepo);
	SG_ARGCHECK(!sNewRepoName.IsEmpty(), sNewRepoName);

	szExistingRepo = vvSgHelpers::Convert_wxString_sglibString(sExistingRepo);
	szNewRepoName  = vvSgHelpers::Convert_wxString_sglibString(sNewRepoName);
	szUsername     = vvSgHelpers::Convert_wxString_sglibString(sUsername, true);
	szPassword     = vvSgHelpers::Convert_wxString_sglibString(sPassword, true);

	SG_ERR_CHECK(  SG_sync_client__spec_is_remote(pCtx, szNewRepoName, &bNewIsRemote)  );
    if (bNewIsRemote)
    {
        SG_ERR_CHECK(  SG_clone__to_remote(
                pCtx, 
                szExistingRepo, 
                szUsername, 
                szPassword, 
                szNewRepoName,
                &pvhCloneResult
                )  );

		if (pvhCloneResult)
		{
			SG_closet__repo_status status = SG_REPO_STATUS__UNSPECIFIED;
			SG_ERR_CHECK(  SG_vhash__check__int32(pCtx, pvhCloneResult, SG_SYNC_CLONE_TO_REMOTE_KEY__STATUS, (SG_int32*)&status)  );
			if (SG_REPO_STATUS__NEED_USER_MAP == status)
				pCtx.Log_ReportWarning("NOTE: The clone was successful, but the new repository requires mapping to valid users before it can be used.");
		}
    }
    else
    {
        SG_ERR_CHECK(  SG_clone__to_local(
                    pCtx, 
                    szExistingRepo, 
                    szUsername, 
                    szPassword, 
                    szNewRepoName,
                    NULL,
                    NULL,
                    NULL
                    )  );
        SG_ERR_CHECK(  SG_localsettings__descriptor__update__sz(pCtx, szNewRepoName, SG_LOCALSETTING__PATHS_DEFAULT, szExistingRepo)  );
    }

fail:
	SG_VHASH_NULLFREE(pCtx, pvhCloneResult);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::Update(
	vvContext&      pCtx,
	const wxString& sFolder,
	const wxString& sBranchAttachment,
	const vvRevSpec * pVVRevSpec,
	const vvNullable<bool> nbDetach
	)
{
	SG_vhash *         pvhStats        = NULL;
	SG_pathname*		   pPathWF			= NULL;
	
	SG_wc_update_args ua;

	pCtx.Log_PushOperation("Updating Working Folder", vvContext::LOG_FLAG_NONE);
	pCtx.Log_SetValue("Target Folder", sFolder, vvContext::LOG_FLAG_INPUT);

	SG_ERR_CHECK(  pPathWF = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFolder)  );
	
	memset(&ua, 0, sizeof(ua));
	if (pVVRevSpec != NULL)
		ua.pRevSpec = *pVVRevSpec;
	ua.pszAttach = (sBranchAttachment.IsEmpty() ? (const char*)NULL : vvSgHelpers::Convert_wxString_sglibString(sBranchAttachment));
	ua.pszAttachNew = NULL;
	if (!nbDetach.IsNull())
	{
		if (!nbDetach.GetValue())
			ua.bAttachCurrent = true;
		else
			ua.bDetached = true;
	}
	// TODO 2013/01/04 decide if we want an --allow-dirty option and change the default
	ua.bDisallowWhenDirty = SG_FALSE;


	SG_ERR_CHECK(  SG_wc__update(pCtx, pPathWF, &ua, SG_FALSE, NULL, &pvhStats, NULL, NULL)  );

	SG_ERR_CHECK(  _ReportMergeStats(pCtx, pvhStats)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathWF);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}
bool vvVerbs::Checkout(
	vvContext&      pCtx,
	const wxString& sName,
	const wxString& sFolder,
	CheckoutType    eType,
	const wxString& sTarget
	)
{
	wxScopedCharBuffer szName;
	wxScopedCharBuffer szFolder;
	const char*              szResolvedBranch = NULL;
	SG_rev_spec*       pRevSpec         = NULL;

	pCtx.Log_PushOperation("Checking out repository", vvContext::LOG_FLAG_NONE);
	pCtx.Log_SetValue("Repository", sName, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("Target Folder", sFolder, vvContext::LOG_FLAG_INPUT);

	SG_ARGCHECK(!sName.IsEmpty(), sName);
	SG_ARGCHECK(!sFolder.IsEmpty(), sFolder);
	SG_ARGCHECK(0 <= eType && eType < CHECKOUT_COUNT, eType);

	SG_ERR_CHECK(  SG_rev_spec__alloc(pCtx, &pRevSpec)  );

	szName   = vvSgHelpers::Convert_wxString_sglibString(sName);
	szFolder = vvSgHelpers::Convert_wxString_sglibString(sFolder);
	szResolvedBranch = NULL;
	switch (eType)
	{
	case CHECKOUT_HEAD:
		break;
	case CHECKOUT_BRANCH:
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, vvSgHelpers::Convert_wxString_sglibString(sTarget))  );
		szResolvedBranch = vvSgHelpers::Convert_wxString_sglibString(sTarget);
		pCtx.Log_SetValue("Target Branch", sTarget, vvContext::LOG_FLAG_INPUT);
		break;
	case CHECKOUT_CHANGESET:
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, vvSgHelpers::Convert_wxString_sglibString(sTarget))  );
		pCtx.Log_SetValue("Target Changeset", sTarget, vvContext::LOG_FLAG_INPUT);
		break;
	case CHECKOUT_TAG:
		SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, vvSgHelpers::Convert_wxString_sglibString(sTarget))  );
		pCtx.Log_SetValue("Target Tag", sTarget, vvContext::LOG_FLAG_INPUT);
		break;
	default:
		SG_ERR_THROW(SG_ERR_INVALIDARG);
	}

	SG_ERR_CHECK(  SG_wc__checkout(pCtx, szName, szFolder, pRevSpec, eType == CHECKOUT_BRANCH ? NULL : szResolvedBranch, NULL, NULL)  );

fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::Status(
	vvContext&           pCtx,
	const vvRepoLocator& cRepoLocator,
	const wxString&      sRevision,
	const wxString&      sRevision2,
	DiffData&            cDiff
	)
{
	SG_pathname*              pPath              = NULL;
	SG_rev_spec*			  pRevSpec			 = NULL;
	SG_varray*				  pvaStatus			 = NULL;
	SG_vhash*				  pvhLegend			 = NULL;
	_TranslateDiffObject_Data cCallbackData;

	pCtx.Log_PushOperation("Getting status", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Folder", cRepoLocator.GetWorkingFolder(), vvContext::LOG_FLAG_INPUT);
	if (sRevision != wxEmptyString)
	{
		pCtx.Log_SetValue("Revision", sRevision, vvContext::LOG_FLAG_INPUT);
	}

	if (cRepoLocator.IsWorkingFolder() && sRevision.IsEmpty() && sRevision2.IsEmpty())
	{
		SG_ERR_CHECK(  pPath = vvSgHelpers::Convert_wxString_sgPathname(pCtx, cRepoLocator.GetWorkingFolder())  );
		// run the status
		SG_ERR_CHECK(  SG_wc__status(pCtx, pPath, NULL, SG_UINT32_MAX, SG_FALSE, SG_FALSE, SG_FALSE, SG_FALSE, SG_FALSE, SG_FALSE, &pvaStatus, &pvhLegend)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, vvSgHelpers::Convert_wxString_sglibString(sRevision))  );
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, vvSgHelpers::Convert_wxString_sglibString(sRevision2))  );
		SG_ERR_CHECK(  SG_vv2__status(pCtx, vvSgHelpers::Convert_wxString_sglibString(cRepoLocator.GetRepoName()), pRevSpec, NULL, SG_INT32_MAX, SG_TRUE, &pvaStatus, &pvhLegend)  ); 
	}

	// translate each object in the diff into a standard DiffEntry and add it to the diff data
	cCallbackData.pvaStatus = pvaStatus;
	cCallbackData.pDiffData = &cDiff;
	SG_ERR_CHECK(  SG_varray__foreach(pCtx, pvaStatus, _TranslateStandardDiffObject, (void*)&cCallbackData)  );

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::Commit(
	vvContext&           pCtx,
	const wxString&      sFolder,
	const wxString&      sComment,
	const wxArrayString* pPaths,
	const wxArrayString* pStamps,
	const wxArrayString * pWorkItemAssociations,
	const wxString*      pUser,
	vvFlags              cFlags,
	wxString*            pHid
	)
{
	bool            bResult          = true;

	SG_stringarray* psa_paths		= NULL;
	SG_stringarray* pConvertedStamps = NULL;
	SG_stringarray* psa_associations = NULL;

	const char*     szUser           = SG_AUDIT__WHO__FROM_SETTINGS;
	char*           szHid            = NULL;
	wxString        sHid;
	SG_pathname*	pPathWF			 = NULL;
	SG_wc_commit_args ca;

	pCtx.Log_PushOperation("Committing", 0u);
	pCtx.Log_SetValue("Folder", sFolder, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("Comment", sComment, vvContext::LOG_FLAG_INPUT);
	pCtx.Log_SetValue("Flags", cFlags, vvContext::LOG_FLAG_INPUT | vvContext::LOG_FLAG_VERBOSE);
	if (pPaths != NULL)
	{
		pCtx.Log_SetValue("Items", wxJoin(*pPaths, ','), vvContext::LOG_FLAG_INPUT);
	}
	if (pStamps != NULL)
	{
		pCtx.Log_SetValue("Stamps", wxJoin(*pStamps, ','), vvContext::LOG_FLAG_INPUT);
	}
	if (pUser != NULL)
	{
		pCtx.Log_SetValue("User", *pUser, vvContext::LOG_FLAG_INPUT);
	}

	if (pPaths != NULL)
	{
		SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgStringArray(pCtx, *pPaths, &psa_paths)  );
	}
	if (pStamps != NULL)
	{
		SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgStringArray(pCtx, *pStamps, &pConvertedStamps)  );
	}
	if (pWorkItemAssociations != NULL)
	{
		SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgStringArray(pCtx, *pWorkItemAssociations, &psa_associations)  );
	}
	if (pUser != NULL)
	{
		szUser = vvSgHelpers::Convert_wxString_sglibString(*pUser);
	}

	SG_ERR_CHECK(  pPathWF = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFolder)  );

	memset(&ca, 0, sizeof(ca));
	ca.bDetached = cFlags == COMMIT_DETACHED;
	ca.pszUser = szUser;
	ca.pszWhen = NULL;
	ca.pszMessage = vvSgHelpers::Convert_wxString_sglibString(sComment);
	ca.pfnPrompt = NULL;
	ca.psaInputs = psa_paths;		// null for a complete commit; non-null for a partial commit.
	ca.depth = (pPaths != NULL && pPaths->Count() > 0) ? 0 : SG_INT32_MAX;
	ca.psaAssocs = psa_associations;
	ca.bAllowLost = SG_TRUE;
	ca.psaStamps = pConvertedStamps;

	SG_ERR_CHECK(  SG_wc__commit(
		pCtx,
		pPathWF,
		&ca,
		SG_FALSE,
		NULL,
		&szHid
	)  );

	sHid = vvSgHelpers::Convert_sglibString_wxString(szHid);
	pCtx.Log_SetValue("Hid", sHid, vvContext::LOG_FLAG_OUTPUT);
	if (pHid != NULL)
	{
		*pHid = sHid;
	}

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psa_paths);
	SG_STRINGARRAY_NULLFREE(pCtx, pConvertedStamps);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_associations);
	SG_PATHNAME_NULLFREE(pCtx, pPathWF);
	SG_NULLFREE(pCtx, szHid);
	pCtx.Log_PopOperation();
	return bResult && !pCtx.Error_Check();
}

bool vvVerbs::Rename(
	vvContext&      pCtx,
	const wxString& sFullPath_objectToRename,
	const wxString& sNewName
	)
{
	SG_pathname*    pPath        = NULL;

	pCtx.Log_PushOperation("Renaming item", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Item", sFullPath_objectToRename, vvContext::LOG_FLAG_INPUT);

	SG_ERR_CHECK(  pPath = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFullPath_objectToRename)  );

	// rename the object
	SG_ERR_CHECK(  SG_wc__rename(pCtx, pPath, vvSgHelpers::Convert_wxString_sglibString(sFullPath_objectToRename), vvSgHelpers::Convert_wxString_sglibString(sNewName), SG_FALSE, SG_FALSE, NULL)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::AddComment(
		class vvContext& pCtx, 
		const wxString&  sRepoName,    
		const wxString&  sChangesetHID,  
		const wxString&  sNewComment	
		)
{
	SG_repo * pRepo = NULL;
	SG_audit pAudit;
	pCtx;
	sRepoName;
	sChangesetHID;
	sNewComment;
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepoName), &pRepo)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx, &pAudit, pRepo,
								  SG_AUDIT__WHEN__NOW,
								  SG_AUDIT__WHO__FROM_SETTINGS)  );
	SG_ERR_CHECK(  SG_vc_comments__add(pCtx, pRepo, vvSgHelpers::Convert_wxString_sglibString(sChangesetHID), vvSgHelpers::Convert_wxString_sglibString(sNewComment), &pAudit)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	return !pCtx.Error_Check();
}

bool vvVerbs::AddStamp(
		class vvContext& pCtx, 
		const wxString&  sRepoName,    
		const wxString&  sChangesetHID,  
		const wxString&  sNewStamp
		)
{
	SG_repo * pRepo = NULL;
	SG_audit pAudit;

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepoName), &pRepo)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx, &pAudit, pRepo,
								  SG_AUDIT__WHEN__NOW,
								  SG_AUDIT__WHO__FROM_SETTINGS)  );
	SG_ERR_CHECK(  SG_vc_stamps__add(pCtx, pRepo, vvSgHelpers::Convert_wxString_sglibString(sChangesetHID), vvSgHelpers::Convert_wxString_sglibString(sNewStamp), &pAudit, NULL)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	return !pCtx.Error_Check();
}

bool vvVerbs::AddTag(
		class vvContext& pCtx, 
		const wxString&  sRepoName,    
		const wxString&  sChangesetHID,  
		const wxString&  sNewTag
		)
{
	SG_repo * pRepo = NULL;
	SG_audit pAudit;
	
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepoName), &pRepo)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx, &pAudit, pRepo,
								  SG_AUDIT__WHEN__NOW,
								  SG_AUDIT__WHO__FROM_SETTINGS)  );
	SG_ERR_CHECK(  SG_vc_tags__add(pCtx, pRepo, vvSgHelpers::Convert_wxString_sglibString(sChangesetHID), vvSgHelpers::Convert_wxString_sglibString(sNewTag), &pAudit, false)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	return !pCtx.Error_Check();
}

bool vvVerbs::DeleteStamp(
		class vvContext& pCtx, 
		const wxString&  sRepoName,    
		const wxString&  sChangesetHID,  
		const wxString&  sRemoveMe	
		)
{
	SG_repo * pRepo = NULL;
	SG_audit pAudit;
	wxScopedCharBuffer szRemoveMeBuffer = vvSgHelpers::Convert_wxString_sglibString(sRemoveMe);
	const char * szRemoveMe = szRemoveMeBuffer.data();
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepoName), &pRepo)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx, &pAudit, pRepo,
								  SG_AUDIT__WHEN__NOW,
								  SG_AUDIT__WHO__FROM_SETTINGS)  );
	SG_ERR_CHECK(  SG_vc_stamps__remove(pCtx, pRepo, &pAudit, vvSgHelpers::Convert_wxString_sglibString(sChangesetHID), 1, &szRemoveMe)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	return !pCtx.Error_Check();
}

bool vvVerbs::DeleteTag(
		class vvContext& pCtx, 
		const wxString&  sRepoName,    
		const wxString&  sRemoveMe	
		)
{
	SG_repo * pRepo = NULL;
	SG_audit pAudit;
	wxScopedCharBuffer szRemoveMeBuffer = vvSgHelpers::Convert_wxString_sglibString(sRemoveMe);
	const char * szRemoveMe = szRemoveMeBuffer.data();
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepoName), &pRepo)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx, &pAudit, pRepo,
								  SG_AUDIT__WHEN__NOW,
								  SG_AUDIT__WHO__FROM_SETTINGS)  );
	SG_ERR_CHECK(  SG_vc_tags__remove(pCtx, pRepo, &pAudit, 1, &szRemoveMe)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	return !pCtx.Error_Check();
}

bool vvVerbs::Move(
	vvContext&      pCtx,
	const wxArrayString& sFullPath_objectsToMove,
	const wxString& sNewParentPath
	)
{
	SG_pathname*    pPath        = NULL;
	char * psz_dest = NULL;
	SG_stringarray * psa_objectsToMove = NULL;

	pCtx.Log_PushOperation("Moving item", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Item", sFullPath_objectsToMove[0], vvContext::LOG_FLAG_INPUT);

	SG_ERR_CHECK(  pPath = vvSgHelpers::Convert_wxArrayString_sgPathname(pCtx, sFullPath_objectsToMove)  );

	SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgStringArray(pCtx, sFullPath_objectsToMove, &psa_objectsToMove)  );
	
	SG_ERR_CHECK(  psz_dest = vvSgHelpers::Convert_wxString_sglibString(pCtx, sNewParentPath)  );
	// move the objects
	SG_ERR_CHECK(  SG_wc__move__stringarray(pCtx, pPath, psa_objectsToMove, psz_dest, SG_TRUE, SG_FALSE, NULL)   );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psa_objectsToMove);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::Add(
	vvContext&      pCtx,
	const wxArrayString& sFullPath_objectsToAdd
	)
{
	SG_stringarray* psa_pathsToAdd = NULL;
	SG_pathname*    pPathWorkingDir = NULL;
	
	//Nothing to add.
	if (sFullPath_objectsToAdd.size() == 0)
		return true;

	pCtx.Log_PushOperation("Adding item", vvContext::LOG_FLAG_VERBOSE);

	SG_ERR_CHECK(  pPathWorkingDir = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFullPath_objectsToAdd[0])  );
	
	SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgStringArray(pCtx, sFullPath_objectsToAdd, &psa_pathsToAdd)  );
	SG_ERR_CHECK(  SG_wc__add__stringarray(pCtx, pPathWorkingDir, psa_pathsToAdd, SG_UINT32_MAX, false, false, NULL)  );

	
fail:
	
	SG_STRINGARRAY_NULLFREE(pCtx, psa_pathsToAdd);
	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::Remove(
	vvContext&      pCtx,
	const wxArrayString& sFullPath_objectsToRemove
	)
{
	SG_pathname*    pPath        = NULL;
	SG_stringarray* psa_paths = NULL;

	//Nothing to add.
	if (sFullPath_objectsToRemove.size() == 0)
		return true;

	pCtx.Log_PushOperation("Removing item", vvContext::LOG_FLAG_VERBOSE);
	pCtx.Log_SetValue("Item", sFullPath_objectsToRemove[0], vvContext::LOG_FLAG_INPUT);

	SG_ERR_CHECK(  pPath = vvSgHelpers::Convert_wxArrayString_sgPathname(pCtx, sFullPath_objectsToRemove)  );

	SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgStringArray(pCtx, sFullPath_objectsToRemove, &psa_paths)  );

	// add the objects
	SG_ERR_CHECK(  SG_wc__remove__stringarray(pCtx, pPath, psa_paths, SG_FALSE, SG_FALSE, SG_FALSE, SG_FALSE, SG_FALSE, NULL)  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psa_paths);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::Revert(
	vvContext&      pCtx,
	wxString sFolder,
	const wxArrayString *pRepoPaths,
	bool bNoBackups
	)
{
	SG_vhash * pvhResult = NULL;
	bool bResult = true;
	SG_pathname* pPath = NULL;
	SG_stringarray* psa_paths = NULL;

	SG_ERR_CHECK(  pPath = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFolder)  );

	//pRepoPaths == NULL is the signifier that they want to revert all changes
	//That forces recursive, pStringPaths = NULL
	if (pRepoPaths == NULL)
		SG_ERR_CHECK(  SG_wc__revert_all(pCtx, pPath, bNoBackups, SG_FALSE, NULL, &pvhResult)   );
	else
	{
		SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgStringArray(pCtx, *pRepoPaths, &psa_paths)  );
		SG_wc_status_flags flagMask_type = SG_WC_STATUS_FLAGS__T__MASK;
		SG_wc_status_flags flagMask_dirt = (SG_WC_STATUS_FLAGS__U__MASK
											|SG_WC_STATUS_FLAGS__S__MASK
											|SG_WC_STATUS_FLAGS__C__MASK);
		SG_ERR_CHECK(  SG_wc__revert_items__stringarray(pCtx, pPath, psa_paths, SG_UINT32_MAX, flagMask_type | flagMask_dirt, bNoBackups, SG_FALSE, NULL)  );
	}
	

fail:
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_paths);
	return bResult && !pCtx.Error_Check();
}

bool vvVerbs::Pull(
		class vvContext& pCtx,						
		const wxString&  sSrcLocalRepoNameOrRemoteURL,		
		const wxString&  sDestLocalRepoName,				
		const wxArrayString& saAreas,					
		const wxArrayString& saTags,					
		const wxArrayString& saDagnodes,
		const wxArrayString& saBranchNames,
		const wxString&  sUsername,	
		const wxString&  sPassword
		)
{
	SG_vhash * pvhAreas = NULL;
	SG_vhash * pvhAccount = NULL;
	vvRevSpec cRevSpec(pCtx, &saDagnodes, &saTags, &saBranchNames);
	bool bPulled = false;

	SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgVhash(pCtx, saAreas, &pvhAreas, true)  );

	SG_rev_spec* pRevSpec = cRevSpec;

	SG_ERR_CHECK(  SG_vv_verbs__pull(pCtx, vvSgHelpers::Convert_wxString_sglibString(sSrcLocalRepoNameOrRemoteURL), vvSgHelpers::Convert_wxString_sglibString(sUsername), vvSgHelpers::Convert_wxString_sglibString(sPassword), vvSgHelpers::Convert_wxString_sglibString(sDestLocalRepoName), pvhAreas, &pRevSpec, &pvhAccount, NULL)  );

	if (pvhAccount != NULL)
	{
		const char* pszMessage = NULL;
		SG_ERR_CHECK(  SG_sync__get_account_info__details(pCtx, pvhAccount, &pszMessage, NULL)  );
		if (pszMessage != NULL)
		{
			pCtx.Log_ReportWarning(pszMessage);
		}
	}

	bPulled = true;
fail:
	SG_VHASH_NULLFREE(pCtx, pvhAreas);
	SG_VHASH_NULLFREE(pCtx, pvhAccount);
	//Do not nullfree the revspec, since the pull command will
	//nullfree the underlying SG_rev_spec.
	if (!bPulled)
		cRevSpec.Nullfree(pCtx);
	return !pCtx.Error_Check();
}

bool vvVerbs::Push(
		class vvContext& pCtx,						
		const wxString&  sDestLocalRepoNameOrRemoteURL,	
		const wxString&  sSrcLocalRepoName,				
		const wxArrayString& saAreas,					
		const wxArrayString& saTags,					
		const wxArrayString& saDagnodes,
		const wxArrayString& saBranchNames,
		const wxString&  sUsername,	
		const wxString&  sPassword
		)
{
	SG_vhash * pvhAreas = NULL;
	SG_vhash * pvhAccount = NULL;
	vvRevSpec cRevSpec(pCtx, &saDagnodes, &saTags, &saBranchNames);
	
	SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgVhash(pCtx, saAreas, &pvhAreas, true)  );

    // TODO the SG_FALSE in the line below is a force flag
	SG_rev_spec* pRevSpec = cRevSpec;
	SG_ERR_CHECK(  SG_vv_verbs__push(pCtx, vvSgHelpers::Convert_wxString_sglibString(sDestLocalRepoNameOrRemoteURL), vvSgHelpers::Convert_wxString_sglibString(sUsername), vvSgHelpers::Convert_wxString_sglibString(sPassword), vvSgHelpers::Convert_wxString_sglibString(sSrcLocalRepoName), pvhAreas, pRevSpec, SG_FALSE, &pvhAccount, NULL)  );
	
	if (pvhAccount != NULL)
	{
		const char* pszMessage = NULL;
		SG_ERR_CHECK(  SG_sync__get_account_info__details(pCtx, pvhAccount, &pszMessage, NULL)  );
		if (pszMessage != NULL)
		{
			pCtx.Log_ReportWarning(pszMessage);
		}
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvhAreas);
	SG_VHASH_NULLFREE(pCtx, pvhAccount);
	cRevSpec.Nullfree(pCtx);
	return !pCtx.Error_Check();
}

bool vvVerbs::Incoming(
		class vvContext& pCtx,							//< [in] [out] Error and context info.
		const wxString&  sSrcLocalRepoNameOrRemoteURL,	//< [in] The local repo name, or remote repo URL.  Changes will be pulled from that repo.
		const wxString&  sDestLocalRepoName,			//< [in] The name of the local repo to pull changes into.
		const wxString&  sUsername,						//< [in] The username that will be provided to the remote server for authentication.
		const wxString&  sPassword,						//< [in] The password that will be provided to the remote server for authentication.
		HistoryData& cHistoryData
		)
{
	SG_history_result* pInfo = NULL;
	SG_ERR_CHECK(  SG_vv_verbs__incoming(pCtx, vvSgHelpers::Convert_wxString_sglibString(sSrcLocalRepoNameOrRemoteURL), vvSgHelpers::Convert_wxString_sglibString(sUsername), vvSgHelpers::Convert_wxString_sglibString(sPassword), vvSgHelpers::Convert_wxString_sglibString(sDestLocalRepoName), &pInfo, NULL)  );
	cHistoryData = vvSgHelpers::Convert_sg_history_result_HistoryData(pCtx, pInfo, NULL);
	
fail:
	//SG_HISTORY_RESULT_NULLFREE(pCtx, pInfo);
	return !pCtx.Error_Check();
}

bool vvVerbs::Outgoing(
		class vvContext& pCtx,						//< [in] [out] Error and context info.
		const wxString&  sDestLocalRepoNameOrRemoteURL,	//< [in] The local repo name, or remote repo URL.  Changes will be pulled from that repo.
		const wxString&  sSrcLocalRepoName,			//< [in] The name of the local repo to pull changes into.
		const wxString&  sUsername,						//< [in] The username that will be provided to the remote server for authentication.
		const wxString&  sPassword,						//< [in] The password that will be provided to the remote server for authentication.
		HistoryData& cHistoryData
		)
{
	SG_history_result* pInfo = NULL;
	SG_ERR_CHECK(  SG_vv_verbs__outgoing(pCtx, vvSgHelpers::Convert_wxString_sglibString(sDestLocalRepoNameOrRemoteURL), vvSgHelpers::Convert_wxString_sglibString(sUsername), vvSgHelpers::Convert_wxString_sglibString(sPassword), vvSgHelpers::Convert_wxString_sglibString(sSrcLocalRepoName), &pInfo, NULL)  );
	cHistoryData = vvSgHelpers::Convert_sg_history_result_HistoryData(pCtx, pInfo, NULL);
	
fail:
	//SG_HISTORY_RESULT_NULLFREE(pCtx, pInfo);
	return !pCtx.Error_Check();
}


bool vvVerbs::CheckOtherRepo_IsItRemote(
		class vvContext& pCtx,							//< [in] [out] Error and context info.
		const wxString&  sOtherLocalRepoNameOrRemoteURL,	//< [in] The local repo name, or remote repo URL.  
		bool& bIsRemote										//< [out] If true, then the repo is remote.
		)
{
	SG_bool bsg_IsRemote = SG_FALSE;
	SG_ERR_CHECK(  SG_sync_client__spec_is_remote(pCtx, vvSgHelpers::Convert_wxString_sglibString(sOtherLocalRepoNameOrRemoteURL), &bsg_IsRemote)   );

fail:
	bIsRemote = vvSgHelpers::Convert_sgBool_cppBool(bsg_IsRemote);
	return !pCtx.Error_Check();
}

bool vvVerbs::GetSetting_Revert__Save_Backups(vvContext& pCtx, bool * pbSaveBackups)
{
	*pbSaveBackups = true;
	char * pszSettingValue = NULL;
	SG_bool bValue = SG_TRUE;

	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx,
												 SG_LOCALSETTING__TORTOISE_REVERT__SAVE_BACKUPS,
												 NULL,		// null repo because we only want /machine/ settings.
												 &pszSettingValue,
												 NULL)  );
	//Very specifically, we interperet any value that is not "false" as SG_TRUE,
	//since we would rather err on the side of saving backups.
	if (SG_stricmp__null(pszSettingValue, "false") == 0)
	{
		bValue = SG_FALSE;
	}
	*pbSaveBackups = bValue == SG_TRUE;
fail:
	SG_NULLFREE(pCtx, pszSettingValue);
	return !pCtx.Error_Check();
}

bool vvVerbs::SetSetting_Revert__Save_Backups(vvContext& pCtx, bool bSaveBackups)
{
	char * pszSettingValue = NULL;
	
	if (bSaveBackups)
		pszSettingValue = "true";
	else
		pszSettingValue = "false";

	SG_ERR_CHECK(  SG_localsettings__update__sz(pCtx,
												 SG_LOCALSETTING__TORTOISE_REVERT__SAVE_BACKUPS,
												 pszSettingValue)  );
fail:
	return !pCtx.Error_Check();
}


bool vvVerbs::GetHistoryFilterDefaults(
	vvContext&     pCtx,
	const wxString& sRepoName,
	vvHistoryFilterOptions& cFilterOptions
	)
{
	SG_string * pString = NULL;
	SG_vhash * pvhSetting = NULL;
	SG_bool bValue = SG_FALSE;

	//SG_context__alloc(&pCtx);
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pString, "%s/%s/%s",
									  SG_LOCALSETTING__SCOPE__INSTANCE,
									  vvSgHelpers::Convert_wxString_sglibString(sRepoName).data(),
									  SG_LOCALSETTING__TORTOISE_HISTORY_FILTER_DEFAULTS)  );
	

	SG_ERR_IGNORE(  SG_localsettings__get__vhash(pCtx,
												 SG_string__sz(pString),
												 NULL,		// null repo because we only want /machine/ settings.
												 &pvhSetting,
												 NULL)  );
	if (pvhSetting != NULL)
	{
		SG_bool bFound = SG_FALSE;
		const char * pszValue = NULL;
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhSetting, "hideMerges", &bFound)  );
		if (bFound)
		{
			SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvhSetting, "hideMerges", &bValue)  );
			cFilterOptions.bHideMerges = bValue == SG_TRUE;
		}
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhSetting, "user", &bFound)  );
		if (bFound)
		{
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSetting, "user", &pszValue)  );
			cFilterOptions.sUser = vvSgHelpers::Convert_sglibString_wxString(pszValue);
		}
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhSetting, "branch", &bFound)  );
		if (bFound)
		{
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSetting, "branch", &pszValue)  );
			cFilterOptions.sBranch = vvSgHelpers::Convert_sglibString_wxString(pszValue);
		}
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhSetting, "tag", &bFound)  );
		if (bFound)
		{
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSetting, "tag", &pszValue)  );
			cFilterOptions.sTag = vvSgHelpers::Convert_sglibString_wxString(pszValue);
		}
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhSetting, "stamp", &bFound)  );
		if (bFound)
		{
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSetting, "stamp", &pszValue)  );
			cFilterOptions.sStamp = vvSgHelpers::Convert_sglibString_wxString(pszValue);
		}
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhSetting, "fileOrFolder", &bFound)  );
		if (bFound)
		{
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSetting, "fileOrFolder", &pszValue)  );
			cFilterOptions.sFileOrFolderPath = vvSgHelpers::Convert_sglibString_wxString(pszValue);
		}
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhSetting, "beginDate", &bFound)  );
		if (bFound)
		{
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSetting, "beginDate", &pszValue)  );
			cFilterOptions.dBeginDate.ParseDate(vvSgHelpers::Convert_sglibString_wxString(pszValue));
		}
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhSetting, "endDate", &bFound)  );
		if (bFound)
		{
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSetting, "endDate", &pszValue)  );
			cFilterOptions.dEndDate.ParseDate(vvSgHelpers::Convert_sglibString_wxString(pszValue));
		}
	}
fail:
	SG_STRING_NULLFREE(pCtx, pString);
	SG_VHASH_NULLFREE(pCtx, pvhSetting);
	return !pCtx.Error_Check();
}

bool vvVerbs::History(
		class vvContext& pCtx,							//< [in] [out] Error and context info.
		const wxString&  sLocalRepoName,				//< [in] The name of the local repo to pull changes into.
		const vvHistoryFilterOptions filterOptions,		//< [in] History Options.
		HistoryData& cHistoryData,
		void ** ppHistoryToken_void
		)
{
	filterOptions;
	sLocalRepoName;
	cHistoryData;
	SG_history_result * pHistoryResults = NULL;
	SG_history_token * pHistoryToken = NULL;
	SG_rev_spec* pRevSpec = NULL;
	SG_rev_spec* pRevSpec_single_revisions = NULL;
	SG_vhash* pvhBranchPile = NULL;
	SG_stringarray * psa_args = NULL;

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec_single_revisions)  );

	if (filterOptions.sFileOrFolderPath != wxEmptyString)
	{
		SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psa_args, 1)  );
		SG_ERR_CHECK(  SG_stringarray__add(pCtx, psa_args, vvSgHelpers::Convert_wxString_sglibString(pCtx, filterOptions.sFileOrFolderPath))  );
	}
	
	if (filterOptions.sBranch != wxEmptyString)
	{
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, vvSgHelpers::Convert_wxString_sglibString(filterOptions.sBranch))  );
	}
	if (filterOptions.sTag != wxEmptyString)
	{
		SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec_single_revisions, vvSgHelpers::Convert_wxString_sglibString(filterOptions.sTag))  );
	}

	SG_ERR_CHECK(  sg_vv2__history__repo(
		pCtx,
		vvSgHelpers::Convert_wxString_sglibString(sLocalRepoName),
		psa_args,
		pRevSpec,
		pRevSpec_single_revisions,
		vvSgHelpers::Convert_wxString_sglibString(filterOptions.sUser, true),
		vvSgHelpers::Convert_wxString_sglibString(filterOptions.sStamp, true), 
		filterOptions.nResultsAtOneTime,
		vvSgHelpers::Convert_cppBool_sgBool(filterOptions.bHideMerges),
		vvSgHelpers::Convert_wxDateTime_sgTime(filterOptions.dBeginDate, 0),
		vvSgHelpers::Convert_wxDateTime_sgTime(filterOptions.dEndDate, SG_INT64_MAX),
		SG_FALSE,
		SG_FALSE,
		NULL,
		&pvhBranchPile,
		&pHistoryResults,
		&pHistoryToken
		)  );
	if (pHistoryToken != NULL && ppHistoryToken_void != NULL)
	{
		*ppHistoryToken_void = (void *)pHistoryToken;
		pHistoryToken = NULL;
	}
	cHistoryData = vvSgHelpers::Convert_sg_history_result_HistoryData(pCtx, pHistoryResults, pvhBranchPile);
fail:
	SG_HISTORY_TOKEN_NULLFREE(pCtx, pHistoryToken);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_args);
	SG_VHASH_NULLFREE(pCtx, pvhBranchPile);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec_single_revisions);
	return !pCtx.Error_Check();
}

bool vvVerbs::History__Fetch_More(
		class vvContext& pCtx,							//< [in] [out] Error and context info.
		const wxString&  sLocalRepoName,				//< [in] The name of the local repo to pull changes into.
		unsigned int nNumResultsDesired,				//< [in] The number of additional results desired.
		void * pHistoryToken_void,
		HistoryData& cHistoryData,
		void ** ppNewHistoryToken_void
		)
{
	sLocalRepoName;
	cHistoryData;
	SG_history_result * pHistoryResults = NULL;
	SG_history_token * pHistoryToken = (SG_history_token *)pHistoryToken_void;
	SG_history_token ** ppNewHistoryToken = (SG_history_token **) ppNewHistoryToken_void;
	SG_vhash * pvhBranchPile = NULL;

	SG_ERR_CHECK(  sg_vv2__history__fetch_more__repo(pCtx, vvSgHelpers::Convert_wxString_sglibString(sLocalRepoName), pHistoryToken, nNumResultsDesired,
		&pvhBranchPile, &pHistoryResults, ppNewHistoryToken)  );

	cHistoryData = vvSgHelpers::Convert_sg_history_result_HistoryData(pCtx, pHistoryResults, pvhBranchPile);
	//Free the old history token.
	SG_HISTORY_TOKEN_NULLFREE(pCtx, pHistoryToken);
	SG_VHASH_NULLFREE(pCtx, pvhBranchPile);
	return !pCtx.Error_Check();
fail:
	SG_HISTORY_TOKEN_NULLFREE(pCtx, pHistoryToken);
	if (ppNewHistoryToken != NULL && *ppNewHistoryToken != NULL)
		SG_HISTORY_TOKEN_NULLFREE(pCtx, *ppNewHistoryToken);
	return !pCtx.Error_Check();
}


bool vvVerbs::History__Free_Token(
		class vvContext& pCtx,							//< [in] [out] Error and context info.
		void * pHistoryToken_void)
{
	SG_history_token * pHistoryToken = (SG_history_token *)pHistoryToken_void;
	SG_HISTORY_TOKEN_NULLFREE(pCtx, pHistoryToken);
	return true;
}

bool vvVerbs::Repo_Item_Get_Details(
		class vvContext& pCtx,
		wxString sWorkingFolderRoot,
		wxString sChangesetHID,
		wxString sItemPath,
		RepoItem& cDetails)
{
	SG_wc_status_flags statusFlag;
	SG_pathname * pwcRoot = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sWorkingFolderRoot);
	SG_ERR_CHECK(  SG_wc__get_item_status_flags(pCtx, 
		pwcRoot, 
		vvSgHelpers::Convert_wxString_sglibString(sItemPath), SG_FALSE, SG_FALSE, &statusFlag, NULL  ) );

	if (statusFlag & SG_WC_STATUS_FLAGS__T__FILE)
		cDetails.eItemType = REGULAR_FILE;
	if (statusFlag & SG_WC_STATUS_FLAGS__T__DIRECTORY)
		cDetails.eItemType = DIRECTORY;
fail:
	SG_PATHNAME_NULLFREE(pCtx, pwcRoot);
	return !pCtx.Error_Check();
}

bool sortRepoItems (vvVerbs::RepoItem i,vvVerbs::RepoItem j) 
{ 
	if (i.eItemType == j.eItemType)
	{
		return i.sItemName.CmpNoCase(j.sItemName) < 0;
	}
	else
	{
		return i.eItemType > j.eItemType;
	}
}

bool vvVerbs::Repo_Item_Get_Children(
	class vvContext& pCtx,
	wxString sRepoName,
	wxString sChangesetHID,
	wxString sItemPath,
	vvVerbs::RepoItemList& cChildren)
{
	SG_repo * pRepo = NULL;
	SG_treenode * pnodeRoot = NULL;
	SG_treenode * pnodeFolder = NULL;
	char * pCS_root_HID = NULL;
	SG_treenode_entry * ptne = NULL;
	const SG_treenode_entry * pTreenodeEntrySub = NULL;
	const char * pFolderHID = NULL;
	SG_treenode_entry_type tneType;
	SG_treenode_entry_type tneTypeSub;
	SG_uint32 kLimit, k;
	const char * szEntryname = NULL;

	cChildren.clear();

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepoName), &pRepo)  );
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, vvSgHelpers::Convert_wxString_sglibString(sChangesetHID), &pCS_root_HID)  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pCS_root_HID, &pnodeRoot)  );
	SG_ERR_CHECK(  SG_treenode__find_treenodeentry_by_path(pCtx, pRepo, pnodeRoot, vvSgHelpers::Convert_wxString_sglibString(sItemPath), NULL, &ptne)  );

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne, &pFolderHID)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, ptne, &tneType)  );
	if (tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		//We found the one we were looking for, now load it and loop over its children.
		SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pFolderHID, &pnodeFolder)  );
		SG_ERR_CHECK(  SG_treenode__count(pCtx,pnodeFolder,&kLimit)  );
		for (k=0; k < kLimit; k++)
		{
			SG_treenode__get_nth_treenode_entry__ref(pCtx,pnodeFolder,k,NULL,&pTreenodeEntrySub);
			vvVerbs::RepoItem ri;
			SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx,pTreenodeEntrySub,&tneTypeSub)  );
			ri.eItemType = (vvVerbs::RepoItemType)tneTypeSub;
			
			//SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx,pTreenodeEntrySub,&szHidBlob)  );
			//SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx,pTreenodeEntrySub,&attrs)  );
			SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx,pTreenodeEntrySub,&szEntryname)  );
			ri.sItemName = vvSgHelpers::Convert_sglibString_wxString(szEntryname);
			cChildren.push_back(ri);
		}
	}
	sort (cChildren.begin(), cChildren.end(), sortRepoItems);
fail:
	SG_TREENODE_NULLFREE(pCtx, pnodeRoot);
	SG_TREENODE_NULLFREE(pCtx, pnodeFolder);

	SG_NULLFREE(pCtx, pCS_root_HID);
	SG_REPO_NULLFREE(pCtx, pRepo);
	return !pCtx.Error_Check();
}

bool vvVerbs::Get_Revision_Details(
	class vvContext& pCtx,							//< [in] [out] Error and context info.
	const wxString&  sLocalRepoName,				//< [in] The name of the local repo to pull changes into.
	const wxString& sRevID,							//< [in] The revision to load
	vvVerbs::HistoryEntry& cHistoryEntry,						//< [out] The details.
	vvVerbs::RevisionList& children						//< [out] A vector of the children of the revision.
)
{
	pCtx;
	sLocalRepoName;
	sRevID;
	cHistoryEntry;
	children;
	SG_repo * pRepo = NULL;
	vvVerbs::HistoryData cHistoryData;

	SG_bool bHasResult = SG_FALSE;
	SG_history_result * pHistoryResults = NULL;
	SG_varray * pvaChildren = NULL;
	SG_rev_spec * prev_spec = NULL;
	SG_vhash * pvhBranchPile = NULL;
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sLocalRepoName), &pRepo)  );
	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &prev_spec)  );
	SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, prev_spec, vvSgHelpers::Convert_wxString_sglibString(sRevID))  );
	SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhBranchPile)  );
	SG_ERR_CHECK(  sg_vv2__history__repo2(pCtx, pRepo, NULL, NULL, prev_spec, NULL, NULL, 1, SG_FALSE, 0, SG_INT64_MAX, SG_FALSE, SG_FALSE,  &bHasResult, &pHistoryResults, NULL) );

	//SG_ERR_CHECK(  SG_history__get_changeset_description(pCtx, pRepo, pszHid, SG_TRUE, &pvhChangesetDescription)  );
	
	cHistoryData = vvSgHelpers::Convert_sg_history_result_HistoryData(pCtx, pHistoryResults, pvhBranchPile);
	
	if (cHistoryData.size() > 0)
	{
		SG_ERR_CHECK(  SG_history__get_children_description(pCtx, pRepo, vvSgHelpers::Convert_wxString_sglibString(cHistoryData[0].sChangesetHID), &pvaChildren)  );
		SG_uint32 nChildrenLen = 0;
		SG_ERR_CHECK(  SG_varray__count(pCtx, pvaChildren, &nChildrenLen)  );
		
		SG_vhash * pvh_currentChild = NULL;
		SG_int64 nRevNo = 0;
		const char * pszChildHid = NULL;
		
		for (SG_uint32 i = 0; i < nChildrenLen; i++)
		{
			vvVerbs::RevisionEntry childRev;
			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaChildren, i, &pvh_currentChild)  );
			SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvh_currentChild, "revno", &nRevNo)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_currentChild, "changeset_id", &pszChildHid)  );
			childRev.nRevno = nRevNo;
			childRev.sChangesetHID = vvSgHelpers::Convert_sglibString_wxString(pszChildHid);
			children.push_back(childRev);

		}
	}
	
	if (cHistoryData.size() > 0)
		cHistoryEntry = cHistoryData[0];

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaChildren);
	SG_HISTORY_RESULT_NULLFREE(pCtx, pHistoryResults);
	SG_REV_SPEC_NULLFREE(pCtx, prev_spec);
	SG_VHASH_NULLFREE(pCtx, pvhBranchPile);
	SG_REPO_NULLFREE(pCtx, pRepo);
	return !pCtx.Error_Check();
}

bool vvVerbs::Export_Revision(
	class vvContext& pCtx,							//< [in] [out] Error and context info.
	const wxString&  sLocalRepoName,				//< [in] The name of the local repo to pull changes into.
	const wxString& sRevID,							//< [in] The revision to load
	wxString sLocalOutputPath						//< [in] The local path to output to.
)
{
	SG_rev_spec * pRevSpec = NULL;

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, vvSgHelpers::Convert_wxString_sglibString(pCtx, sRevID))  );

	SG_ERR_CHECK(  SG_vv2__export(pCtx,
									   vvSgHelpers::Convert_wxString_sglibString(sLocalRepoName),
									   vvSgHelpers::Convert_wxString_sglibString(sLocalOutputPath),
									   pRevSpec,
									   NULL)  );

fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	return !pCtx.Error_Check();
}

bool vvVerbs::Diff(
	vvContext&           pCtx,
	const vvRepoLocator& cRepoLocator,
	const wxArrayString* pPaths,
	bool bRecursive,
	wxString* sRev1,
	wxString* sRev2,
	wxString* sTool)
{
	bool bResult = true;
	SG_stringarray* psa_paths = NULL;
	SG_pathname* pPathWF = NULL;
	char * szRev1 = NULL;
	char * szRev2 = NULL;
	char * szRepoName = NULL;
	SG_rev_spec * prevspec = NULL;

	pPathWF = vvSgHelpers::Convert_wxString_sgPathname(pCtx, cRepoLocator.GetWorkingFolder());

	if (pPaths != NULL)
	{
		pCtx.Log_SetValue("Items", wxJoin(*pPaths, ','), vvContext::LOG_FLAG_INPUT);
	}

	if (pPaths != NULL)
	{
		SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgStringArray(pCtx, *pPaths, &psa_paths)  );
	}

	if (sRev1 != NULL && sRev2 != NULL)
	{
		SG_ERR_CHECK(  szRev1 = vvSgHelpers::Convert_wxString_sglibString(pCtx, *sRev1)  );
		SG_ERR_CHECK(  szRev2 = vvSgHelpers::Convert_wxString_sglibString(pCtx, *sRev2)  );
		SG_ERR_CHECK(  szRepoName = vvSgHelpers::Convert_wxString_sglibString(pCtx, cRepoLocator.GetRepoName())  );
		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &prevspec)  );
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx , prevspec, szRev1)  );
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx , prevspec, szRev2)  );
		SG_ERR_CHECK(  SG_vv2__diff_to_stream__throw(pCtx, szRepoName, prevspec, psa_paths, SG_INT32_MAX, SG_FALSE, SG_TRUE, vvSgHelpers::Convert_wxString_sglibString(sTool))    );
	}
	else
	{
		SG_ERR_CHECK(  SG_wc__diff__throw(
			pCtx,
			pPathWF,
			NULL,
			psa_paths,
			SG_INT32_MAX, 
			SG_TRUE,
			SG_FALSE,
			SG_TRUE,
			vvSgHelpers::Convert_wxString_sglibString(sTool)
			)  );
	}


fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathWF);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_paths);
	SG_NULLFREE(pCtx, szRev1);
	SG_NULLFREE(pCtx, szRev2);
	SG_REV_SPEC_NULLFREE(pCtx, prevspec);
	return bResult && !pCtx.Error_Check();
}

bool vvVerbs::GetBranchHeads(
		vvContext&           pCtx,						//< [in] [out] Error and context info.
		const wxString&  sLocalRepoName,				//< [in] The name of the local repo to pull changes into.
		const wxString& sBranchName,					//< [in] The branch to query
		wxArrayString& sBranchHeadHids					//< [out] The current heads of that branch.
		) 
{
	SG_rbtree * prbLeafHids = NULL;
	SG_rbtree * prbNonLeafHids = NULL;
	SG_stringarray * psaBranchNames = NULL;

	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaBranchNames, 1)  );
	SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaBranchNames, vvSgHelpers::Convert_wxString_sglibString(sBranchName))  );
	SG_ERR_CHECK(  SG_vv_verbs__heads(pCtx, vvSgHelpers::Convert_wxString_sglibString(sLocalRepoName), psaBranchNames, SG_FALSE, &prbLeafHids, &prbNonLeafHids)  );

	vvSgHelpers::Convert_rbTreeKeys_wxArrayString(pCtx, prbLeafHids, sBranchHeadHids);
	vvSgHelpers::Convert_rbTreeKeys_wxArrayString(pCtx, prbNonLeafHids, sBranchHeadHids);
fail:
	SG_RBTREE_NULLFREE(pCtx, prbLeafHids);
	SG_RBTREE_NULLFREE(pCtx, prbNonLeafHids);
	SG_STRINGARRAY_NULLFREE(pCtx, psaBranchNames);
	return !pCtx.Error_Check();
}

bool vvVerbs::Merge(
	vvContext&       pCtx,
	const wxString&  sFolder,
	const vvRevSpec& cRevSpec,
	const bool bAllowFastForward,
	const wxString&  sTool
	)
{
	wxScopedCharBuffer szTool;
	char*              szRevision = NULL;
	SG_wc_merge_args mergeArgs;
	SG_vhash * pvhStats = NULL;
	SG_pathname* pPathWF = NULL;

	pCtx.Log_PushOperation("Merging", vvContext::LOG_FLAG_NONE);
	pCtx.Log_SetValue("Folder", sFolder, vvContext::LOG_FLAG_INPUT);

	SG_ARGCHECK(!sFolder.IsEmpty(), sFolder);
	SG_ARGCHECK(cRevSpec.IsAllocated(), cRevSpec);

	SG_ERR_CHECK(  pPathWF = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFolder)  );

	if (!sTool.IsEmpty())
	{
		szTool = vvSgHelpers::Convert_wxString_sglibString(sTool);
		pCtx.Log_SetValue("Tool", sTool, vvContext::LOG_FLAG_INPUT);
	}

	// find the revision specified by the rev spec
	memset(&mergeArgs, 0, sizeof(mergeArgs));
	mergeArgs.pRevSpec                   = cRevSpec;
	mergeArgs.bNoAutoMergeFiles          = SG_FALSE;
	mergeArgs.bNoFFMerge				 = vvSgHelpers::Convert_cppBool_sgBool(!bAllowFastForward);
	mergeArgs.bComplainIfBaselineNotLeaf = SG_FALSE;	// TODO 2012/01/20 see note in SG_wc_tx__merge().

	// do the merge
	SG_ERR_CHECK(  SG_wc__merge(pCtx, pPathWF, &mergeArgs, SG_FALSE, NULL, &pvhStats, NULL, NULL)  );
	
	_ReportMergeStats(pCtx, pvhStats);

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathWF);
	SG_NULLFREE(pCtx, szRevision);
	pCtx.Log_PopOperation();
	return !pCtx.Error_Check();
}

bool vvVerbs::GetDiffTools(
	vvContext&      pCtx,
	const wxString& sRepo,
	wxArrayString&  cTools,
	bool            bInternal
	)
{
	SG_repo*        pRepo  = NULL;
	SG_stringarray* pTools = NULL;
	SG_uint32       uCount = 0u;
	SG_uint32       uIndex = 0u;

	// if they specified a repo, open it
	if (!sRepo.IsEmpty())
	{
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	}

	// get a list of tools and iterate through it
	SG_ERR_CHECK(  SG_difftool__list(pCtx, pRepo, &pTools)  );
	SG_ERR_CHECK(  SG_stringarray__count(pCtx, pTools, &uCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		const char* szTool = NULL;

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pTools, uIndex, &szTool)  );
		if (bInternal || szTool[0] != ':' || strcmp(szTool, SG_DIFFTOOL__INTERNAL__DIFFMERGE) == 0)
		{
			cTools.push_back(vvSgHelpers::Convert_sglibString_wxString(szTool));
		}
	}

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_STRINGARRAY_NULLFREE(pCtx, pTools);
	return !pCtx.Error_Check();
}

bool vvVerbs::GetDiffTool(
	vvContext&      pCtx,
	const wxString& sItem,
	const wxString& sSuggestion,
	wxString&       sTool
	)
{
	SG_pathname* pPath  = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sItem);
	wxString     sRepo  = wxEmptyString;
	SG_repo*     pRepo  = NULL;
	SG_filetool* pTool  = NULL;
	const char*  szName = NULL;

	// find the repo associated with the item and open it
	if (!this->FindWorkingFolder(pCtx, sItem, NULL, &sRepo, NULL))
	{
		return false;
	}
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );

	// select an appropriate tool and return its name
	SG_ERR_CHECK(  SG_difftool__select(pCtx, SG_DIFFTOOL__CONTEXT__GUI, NULL, pPath, vvSgHelpers::Convert_wxString_sglibString(sSuggestion, true), pRepo, &pTool)  );
	SG_ERR_CHECK(  SG_filetool__get_name(pCtx, pTool, &szName)  );
	sTool = vvSgHelpers::Convert_sglibString_wxString(szName);

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_FILETOOL_NULLFREE(pCtx, pTool);
	return !pCtx.Error_Check();
}

bool vvVerbs::GetMergeTools(
	vvContext&      pCtx,
	const wxString& sRepo,
	wxArrayString&  cTools,
	bool            bInternal
	)
{
	SG_repo*        pRepo  = NULL;
	SG_stringarray* pTools = NULL;
	SG_uint32       uCount = 0u;
	SG_uint32       uIndex = 0u;

	// if they specified a repo, open it
	if (!sRepo.IsEmpty())
	{
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	}

	// get a list of tools and iterate through it
	SG_ERR_CHECK(  SG_mergetool__list(pCtx, pRepo, &pTools)  );
	SG_ERR_CHECK(  SG_stringarray__count(pCtx, pTools, &uCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		const char* szTool = NULL;

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pTools, uIndex, &szTool)  );
		if (bInternal || szTool[0] != ':' || strcmp(szTool, SG_DIFFTOOL__INTERNAL__DIFFMERGE) == 0)
		{
			cTools.push_back(vvSgHelpers::Convert_sglibString_wxString(szTool));
		}
	}


fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_STRINGARRAY_NULLFREE(pCtx, pTools);
	return !pCtx.Error_Check();
}

static void _TranslateMergeToolContext(
	SG_context*               pCtx,
	vvVerbs::MergeToolContext eContext,
	const char**              pContext
	)
{
	switch (eContext)
	{
	case vvVerbs::MERGETOOL_CONTEXT_NONE:
		*pContext = NULL;
		break;
	case vvVerbs::MERGETOOL_CONTEXT_MERGE:
		*pContext = SG_MERGETOOL__CONTEXT__MERGE;
		break;
	case vvVerbs::MERGETOOL_CONTEXT_RESOLVE:
		*pContext = SG_MERGETOOL__CONTEXT__RESOLVE;
		break;
	default:
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Unknown merge context: %d", eContext));
	}

fail:
	return;
}

bool vvVerbs::GetMergeTool(
	vvContext&       pCtx,
	const wxString&  sItem,
	MergeToolContext eContext,
	const wxString&  sSuggestion,
	wxString&        sTool
	)
{
	SG_pathname* pPath     = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sItem);
	const char*  szContext = NULL;
	wxString     sRepo     = wxEmptyString;
	SG_repo*     pRepo     = NULL;
	SG_filetool* pTool     = NULL;
	const char*  szName    = NULL;

	// translate the given context to its sglib equivalent
	SG_ERR_CHECK(  _TranslateMergeToolContext(pCtx, eContext, &szContext)  );

	// find the repo associated with the item and open it
	if (!this->FindWorkingFolder(pCtx, sItem, NULL, &sRepo, NULL))
	{
		return false;
	}
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );

	// select an appropriate tool and return its name
	SG_ERR_CHECK(  SG_mergetool__select(pCtx, szContext, NULL, pPath, vvSgHelpers::Convert_wxString_sglibString(sSuggestion, true), pRepo, &pTool)  );
	SG_ERR_CHECK(  SG_filetool__get_name(pCtx, pTool, &szName)  );
	sTool = vvSgHelpers::Convert_sglibString_wxString(szName);

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_FILETOOL_NULLFREE(pCtx, pTool);
	return !pCtx.Error_Check();
}

static void _Resolve_GetData_FillValue(
	SG_context*            pCtx,
	SG_resolve__value*     pValue,
	vvVerbs::ResolveValue& cValue
	)
{
	SG_bool            bValue    = SG_FALSE;
	SG_bool            bValue2   = SG_FALSE;
	SG_int64           i64Value  = 0;
	SG_int32           i32Value  = 0;
	SG_uint64          u64Value  = 0u;
	SG_uint32          u32Value  = 0u;
	const char*        szValue   = NULL;
	SG_string*         sValue    = NULL;
	bool               bFound    = false;
	SG_resolve__value* pBaseline = NULL;
	SG_resolve__value* pOther    = NULL;

	SG_NULLARGCHECK(pValue);

	SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pValue, &szValue)  );
	cValue.sLabel = vvSgHelpers::Convert_sglibString_wxString(szValue);

	bFound = true;
	SG_ERR_TRY(  SG_resolve__value__get_data__bool(pCtx, pValue, &bValue)  );
	SG_ERR_CATCH_SET(SG_ERR_USAGE, bFound, false);
	SG_ERR_CATCH_END;
	if (bFound)
	{
		cValue.cData = vvSgHelpers::Convert_sgBool_cppBool(bValue);
	}
	else
	{
		bFound = true;
		SG_ERR_TRY(  SG_resolve__value__get_data__int64(pCtx, pValue, &i64Value)  );
		SG_ERR_CATCH_SET(SG_ERR_USAGE, bFound, false);
		SG_ERR_CATCH_END;
		if (bFound)
		{
			cValue.cData = (wxInt64)i64Value;
		}
		else
		{
			bFound = true;
			SG_ERR_TRY(  SG_resolve__value__get_data__sz(pCtx, pValue, &szValue)  );
			SG_ERR_CATCH_SET(SG_ERR_USAGE, bFound, false);
			SG_ERR_CATCH_END;
			if (bFound)
			{
				cValue.cData = vvSgHelpers::Convert_sglibString_wxString(szValue);
			}
			else
			{
				SG_ERR_THROW2(SG_ERR_NOT_FOUND, (pCtx, "No data found for resolve value."));
			}
		}
	}

	SG_ERR_CHECK(  SG_resolve__value__get_summary(pCtx, pValue, &sValue)  );
	cValue.sSummary = vvSgHelpers::Convert_sgString_wxString(sValue, wxEmptyString);

	SG_ERR_CHECK(  SG_resolve__value__get_flags(pCtx, pValue, &u32Value)  );
	cValue.cFlags = u32Value;

	SG_ERR_CHECK(  SG_resolve__value__get_size(pCtx, pValue, &u64Value)  );
	cValue.uSize = u64Value;

	SG_ERR_CHECK(  SG_resolve__value__get_changeset__mnemonic(pCtx, pValue, &bValue, &szValue)  );
	if (bValue == SG_TRUE)
	{
		cValue.sChangeset = vvSgHelpers::Convert_sglibString_wxString(szValue);
	}

	SG_ERR_CHECK(  SG_resolve__value__get_mergeable_working(pCtx, pValue, &bValue, &pBaseline, &bValue2)  );
	if (bValue == SG_TRUE)
	{
		SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pBaseline, &szValue)  );
		cValue.sMWParent = vvSgHelpers::Convert_sglibString_wxString(szValue);
		cValue.bMWModified = vvSgHelpers::Convert_sgBool_cppBool(bValue2);
	}

	SG_ERR_CHECK(  SG_resolve__value__get_merge(pCtx, pValue, &bValue, &pBaseline, &pOther, NULL, &szValue, &i32Value, &i64Value)  );
	if (bValue == SG_TRUE)
	{
		cValue.sTool = vvSgHelpers::Convert_sglibString_wxString(szValue);
		cValue.iExit = i32Value;
		cValue.iTime = vvSgHelpers::Convert_sgTime_wxDateTime(i64Value);

		SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pBaseline, &szValue)  );
		cValue.sBaseline = vvSgHelpers::Convert_sglibString_wxString(szValue);

		SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pOther, &szValue)  );
		cValue.sOther = vvSgHelpers::Convert_sglibString_wxString(szValue);
	}

fail:
	SG_STRING_NULLFREE(pCtx, sValue);
	return;
}

static void _Resolve_GetData_ValueCallback(
	SG_context*        pCtx,
	SG_resolve__value* pValue,
	void*              pUserData,
	SG_bool*           pContinue
	)
{
	vvVerbs::stlResolveValueMap* pValues = (vvVerbs::stlResolveValueMap*)pUserData;
	vvVerbs::ResolveValue        cValue;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	SG_ERR_CHECK(  _Resolve_GetData_FillValue(pCtx, pValue, cValue)  );
	pValues->insert(vvVerbs::stlResolveValueMap::value_type(cValue.sLabel, cValue));

fail:
	*pContinue = SG_TRUE;
	return;
}

static void _Resolve_GetData_FillRelatedItem(
	SG_context*                  pCtx,
	wxString&					 sWorkingCopyRoot,
	SG_resolve__item*            pItem,
	vvVerbs::ResolveRelatedItem& cItem
	)
{
	const char * szValue;
	SG_treenode_entry_type eType;
	SG_wc_status_flags    eStatus;
	SG_string*			  psgstr_value = NULL;
	SG_pathname *		  pPathname = NULL;
	SG_bool				  bShowInParens = SG_FALSE;
	vvContext cContext = pCtx;
	SG_NULLARGCHECK(pItem);

	SG_ERR_CHECK(  SG_resolve__item__get_gid(pCtx, pItem, &szValue)  );
	cItem.sGid = vvSgHelpers::Convert_sglibString_wxString(szValue);

	SG_ERR_CHECK(  SG_resolve__item__get_type(pCtx, pItem, &eType)  );
	SG_ERR_CHECK(  _TranslateItemType(pCtx, eType, &cItem.eType)  );

	SG_ERR_CHECK(  SG_resolve__item__get_repo_path__working(pCtx, pItem, &psgstr_value, &bShowInParens)  );
	cItem.sRepoPath_Working = vvSgHelpers::Convert_sglibString_wxString(SG_string__sz(psgstr_value), wxEmptyString);

	SG_ERR_CHECK(  pPathname = vvSgHelpers::Convert_wxString_sgPathname(cContext, sWorkingCopyRoot)  );
	SG_ERR_CHECK(  SG_wc__get_item_status_flags(pCtx, pPathname, SG_string__sz(psgstr_value), SG_FALSE, SG_FALSE, &eStatus, NULL)  );
	SG_ERR_CHECK(  _TranslateDiffStatusFlags(pCtx, eStatus, NULL, &cItem.cStatus)  );

/* Note-- This was present in the pre-WC version of tortoise.  sRepoPath_Baseline was unused.  It now sits
			here gathering dust.
	SG_ERR_CHECK(  SG_resolve__item__get_repo_path__baseline(pCtx, pItem, &szValue)  );
	cItem.sRepoPath_Baseline = vvSgHelpers::Convert_sglibString_wxString(szValue, wxEmptyString);
	*/
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	SG_STRING_NULLFREE(pCtx, psgstr_value);
	return;
}

static void _Resolve_GetData_RelatedItemCallback(
	SG_context*       pCtx,
	SG_resolve__item* pItem,
	void*             pUserData,
	SG_bool*          pContinue
	)
{
	vvVerbs::WcResolveRelatedData* pItems = (vvVerbs::WcResolveRelatedData*)pUserData;
	vvVerbs::ResolveRelatedItem    cItem;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	SG_ERR_CHECK(  _Resolve_GetData_FillRelatedItem(pCtx, pItems->sWorkingCopyRoot, pItem, cItem)  );
	pItems->cRelated.insert(vvVerbs::stlResolveRelatedMap::value_type(cItem.sGid, cItem));

fail:
	*pContinue = SG_TRUE;
	return;
}

static void _Resolve_GetData_FillChoice(
	SG_context*             pCtx,
	wxString&				sWorkingCopyPath,
	SG_resolve__choice*     pChoice,
	vvVerbs::ResolveChoice& cChoice
	)
{
	SG_bool            bValue   = SG_FALSE;
	//TODO: Figure out if this is necessary - jeremy
	//SG_bool            bValue2  = SG_FALSE;
	SG_uint32          uValue   = 0u;
	const char*        szValue  = NULL;
	const char*        szValue2 = NULL;
	const char*        szKey    = NULL;
	SG_vhash*          pHash    = NULL;
	SG_uint32          uIndex   = 0u;
	SG_resolve__state  eState   = SG_RESOLVE__STATE__COUNT;
	SG_resolve__value* pValue   = NULL;
	vvVerbs::WcResolveRelatedData cRelatedData2;
	vvVerbs::WcResolveRelatedData cRelatedData;

	SG_NULLARGCHECK(pChoice);

	SG_ERR_CHECK(  SG_resolve__choice__get_state(pCtx, pChoice, &eState)  );
	SG_ERR_CHECK(  _TranslateResolveState(pCtx, eState, &cChoice.eState)  );

	SG_ERR_CHECK(  SG_resolve__state__get_info(pCtx, eState, &szKey, &szValue, &szValue2)  );
	cChoice.sName = vvSgHelpers::Convert_sglibString_wxString(szKey);
	cChoice.sProblem = vvSgHelpers::Convert_sglibString_wxString(szValue);
	cChoice.sCommand = vvSgHelpers::Convert_sglibString_wxString(szValue2);

	cChoice.cCauses.clear();
	SG_ERR_CHECK(  SG_resolve__choice__get_causes(pCtx, pChoice, NULL, NULL, &pHash)  );
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pHash, &uValue)  );
	for (uIndex = 0u; uIndex < uValue; ++uIndex)
	{
		SG_ERR_CHECK(  SG_vhash__get_nth_pair__sz(pCtx, pHash, uIndex, &szKey, &szValue)  );
		cChoice.cCauses.insert(vvVerbs::stlResolveCauseMap::value_type(vvSgHelpers::Convert_sglibString_wxString(szKey), vvSgHelpers::Convert_sglibString_wxString(szValue)));
	}

	SG_ERR_CHECK(  SG_resolve__choice__get_mergeable(pCtx, pChoice, &bValue)  );
	cChoice.bMergeable = vvSgHelpers::Convert_sgBool_cppBool(bValue);

	//TODO - 9/10/2012 investigate if this needs to care about deleted.
	//The old call, before switching to Jeff's new WC code. -jeremy
	//SG_ERR_CHECK(  SG_resolve__choice__get_resolved(pCtx, pChoice, &bValue, &bValue2)  );
	SG_ERR_CHECK(  SG_resolve__choice__get_resolved(pCtx, pChoice, &bValue)  );
	cChoice.bResolved = vvSgHelpers::Convert_sgBool_cppBool(bValue);
	cChoice.bDeleted = false;
	//cChoice.bDeleted  = vvSgHelpers::Convert_sgBool_cppBool(bValue2);

	SG_ERR_CHECK(  SG_resolve__choice__get_value__accepted(pCtx, pChoice, &pValue)  );
	if (pValue != NULL)
	{
		SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pValue, &szValue)  );
		cChoice.sResolution = vvSgHelpers::Convert_sglibString_wxString(szValue);
	}

	cChoice.cColliding.clear();
	cRelatedData.sWorkingCopyRoot = sWorkingCopyPath;
	SG_ERR_CHECK(  SG_resolve__choice__foreach_colliding_item(pCtx, pChoice, _Resolve_GetData_RelatedItemCallback, (void*)&cRelatedData)  );
	cChoice.cColliding = cRelatedData.cRelated;
	for (vvVerbs::stlResolveRelatedMap::iterator it = cChoice.cColliding.begin(); it != cChoice.cColliding.end(); ++it)
	{
		it->second.eRelation = vvVerbs::RESOLVE_RELATION_COLLISION;
	}

	cChoice.cCycling.clear();
	cRelatedData2.sWorkingCopyRoot = sWorkingCopyPath;
	SG_ERR_CHECK(  SG_resolve__choice__foreach_cycling_item(pCtx, pChoice, _Resolve_GetData_RelatedItemCallback, (void*)&cRelatedData2)  );
	cChoice.cCycling = cRelatedData2.cRelated;
	for (vvVerbs::stlResolveRelatedMap::iterator it = cChoice.cCycling.begin(); it != cChoice.cCycling.end(); ++it)
	{
		it->second.eRelation = vvVerbs::RESOLVE_RELATION_CYCLE;
	}

	SG_ERR_CHECK(  SG_resolve__choice__get_cycle_info(pCtx, pChoice, &szValue)  );
	cChoice.sCycle = vvSgHelpers::Convert_sglibString_wxString(szValue);

	cChoice.cValues.clear();
	SG_ERR_CHECK(  SG_resolve__choice__foreach_value(pCtx, pChoice, SG_FALSE, _Resolve_GetData_ValueCallback, (void*)&cChoice.cValues)  );

fail:
	return;
}

static void _Resolve_GetData_ChoiceCallback(
	SG_context*         pCtx,
	SG_resolve__choice* pChoice,
	void*               pUserData,
	SG_bool*            pContinue
	)
{
	vvVerbs::WcResolveChoiceData * pData = (vvVerbs::WcResolveChoiceData*)pUserData;
	vvVerbs::ResolveChoice        cChoice;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	SG_ERR_CHECK(  _Resolve_GetData_FillChoice(pCtx, pData->sWorkingCopyRoot, pChoice, cChoice)  );
	pData->cChoices.insert(vvVerbs::stlResolveChoiceMap::value_type(cChoice.eState, cChoice));

fail:
	*pContinue = SG_TRUE;
	return;
}

static void _Resolve_GetData_FillItem(
	SG_context*		  pCtx,
	wxString			  sWorkingCopyRoot,
	SG_resolve__item*     pItem,
	vvVerbs::ResolveItem& cItem
	)
{
	//SG_bool                bValue  = SG_FALSE;
	//TODO: figure out if this is necessary. -jeremy
	//SG_bool                bValue2 = SG_FALSE;
	const char*            szValue = NULL;
	SG_treenode_entry_type eType;
	SG_wc_status_flags    eStatus;
	//TODO: restore this
	//SG_uint32              uIndex = 0u;
	SG_string *				psgstr_value = NULL;
	SG_pathname*		   pPathname = NULL;
	vvFlags cResolvedFlags;
	SG_bool bShowInParens;
	vvContext cContext = pCtx;
	vvVerbs::WcResolveChoiceData cChoiceData;
	SG_NULLARGCHECK(pItem);

	SG_ERR_CHECK(  SG_resolve__item__get_gid(pCtx, pItem, &szValue)  );
	cItem.sGid = vvSgHelpers::Convert_sglibString_wxString(szValue);

	SG_ERR_CHECK(  SG_resolve__item__get_type(pCtx, pItem, &eType)  );
	SG_ERR_CHECK(  _TranslateItemType(pCtx, eType, &cItem.eType)  );

	/* TODO Restore these.
	SG_ERR_CHECK(  SG_resolve__item__get_ancestor(pCtx, pItem, &szValue)  );
	cItem.sAncestor = vvSgHelpers::Convert_sglibString_wxString(szValue);

	SG_ERR_CHECK(  SG_resolve__item__get_repo_path__ancestor(pCtx, pItem, &szValue)  );
	cItem.sRepoPath_Ancestor = vvSgHelpers::Convert_sglibString_wxString(szValue, wxEmptyString);
	*/
	SG_ERR_CHECK(  SG_resolve__item__get_repo_path__working(pCtx, pItem, &psgstr_value, &bShowInParens)  );
	cItem.sRepoPath_Working = vvSgHelpers::Convert_sglibString_wxString(SG_string__sz(psgstr_value), wxEmptyString);

	SG_ERR_CHECK(  pPathname = vvSgHelpers::Convert_wxString_sgPathname(cContext, sWorkingCopyRoot)  );
	SG_ERR_CHECK(  SG_wc__get_item_status_flags(pCtx, pPathname, SG_string__sz(psgstr_value), SG_FALSE, SG_FALSE, &eStatus, NULL)  );
	SG_ERR_CHECK(  _TranslateDiffStatusFlags(pCtx, eStatus, NULL, &cItem.cStatus)  );

	// we need to retrieve baseline/others separately
	// but we want to treat baseline like the 0th parent
	cItem.sRepoPath_Parents.Clear();
	/* TODO Restore these.
	SG_ERR_CHECK(  SG_resolve__item__get_repo_path__baseline(pCtx, pItem, &szValue)  );
	do
	{
		cItem.sRepoPath_Parents.Add(vvSgHelpers::Convert_sglibString_wxString(szValue, wxEmptyString));
		++uIndex;
		SG_ERR_TRY(  SG_resolve__item__get_repo_path__other(pCtx, pItem, uIndex, &szValue)  );
		SG_ERR_CATCH_SET(SG_ERR_NOT_FOUND, uIndex, 0u);
		SG_ERR_CATCH_END;
	}
	while (uIndex > 0u);
	*/
	//TODO - 9/10/2012 investigate if this needs to care about deleted.
	//The old call, before switching to Jeff's new WC code. -jeremy
	SG_ERR_CHECK(  SG_resolve__item__get_resolved(pCtx, pItem, &eStatus )  );
	cResolvedFlags = eStatus;
	cItem.bResolved = cResolvedFlags.HasAnyFlag(SG_WC_STATUS_FLAGS__X__RESOLVED);
	cItem.bDeleted = false;
	//cItem.bDeleted= cResolvedFlags.HasAnyFlag(SG_WC_STATUS_FLAGS__X_);

	cItem.cChoices.clear();
	cChoiceData.sWorkingCopyRoot = sWorkingCopyRoot;
	SG_ERR_CHECK(  SG_resolve__item__foreach_choice(pCtx, pItem, SG_FALSE, _Resolve_GetData_ChoiceCallback, (void*)&cChoiceData)  );
	cItem.cChoices = cChoiceData.cChoices;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	SG_STRING_NULLFREE(pCtx, psgstr_value);
	return;
}

static void _Resolve_GetData_ItemCallback(
	SG_context*		  pCtx,
	SG_resolve__item* pItem,
	void*             pUserData,
	SG_bool*          pContinue
	)
{
	vvVerbs::WcResolveData * pData = (vvVerbs::WcResolveData*)pUserData;
	vvVerbs::stlResolveItemList cItems = pData->cResolveItems;
	vvVerbs::ResolveItem         cItem;
	
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	SG_ERR_CHECK(  _Resolve_GetData_FillItem(pCtx, pData->sWorkingCopyRoot, pItem, cItem)  );
	cItems.push_back(cItem);
	pData->cResolveItems = cItems;
fail:
	*pContinue = SG_TRUE;
	return;
}

wxString& vvVerbs::ResolveItem::FindBestRepoPath(
	bool* pNonExistent
	)
{
	return const_cast<wxString&>(const_cast<const ResolveItem*>(this)->FindBestRepoPath(pNonExistent));
}

const wxString& vvVerbs::ResolveItem::FindBestRepoPath(
	bool* pNonExistent
	) const
{
	if (pNonExistent != NULL)
	{
		*pNonExistent = this->sRepoPath_Working.IsEmpty();
	}

	if (!this->sRepoPath_Working.IsEmpty())
	{
		return this->sRepoPath_Working;
	}

	for (unsigned int uIndex = 0u; uIndex < sRepoPath_Parents.Count(); ++uIndex)
	{
		if (!this->sRepoPath_Parents[uIndex].IsEmpty())
		{
			return this->sRepoPath_Parents[uIndex];
		}
	}

	return this->sRepoPath_Ancestor;
}

bool vvVerbs::Resolve_GetData(
	vvContext&          pCtx,
	const wxString&     sFolder,
	stlResolveItemList& cData_out
	)
{
	SG_pathname*    pFolder      = NULL;
	SG_wc_tx *		pWcTx		 = NULL;
	SG_resolve*     pResolve     = NULL;
	WcResolveData	cData;

	SG_ERR_CHECK(  pFolder = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFolder)  );
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pFolder, SG_FALSE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );
	cData.sWorkingCopyRoot = SG_pathname__sz(pFolder);
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_ERR_CHECK(  SG_resolve__foreach_item(pCtx, pResolve, SG_FALSE, _Resolve_GetData_ItemCallback, (void*)&cData)  );
	cData_out = cData.cResolveItems;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pFolder);
//	SG_RESOLVE_NULLFREE(pCtx, pResolve);
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	return !pCtx.Error_Check();
}

bool vvVerbs::Resolve_GetData(
	vvContext&      pCtx,
	const wxString& sFolder,
	const wxString& sGid,
	ResolveItem&    cData
	)
{
	SG_pathname*      pFolder      = NULL;
	SG_wc_tx *		  pWcTx		   = NULL;
	SG_resolve*       pResolve     = NULL;
	SG_resolve__item* pItem        = NULL;

	SG_ERR_CHECK(  pFolder = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFolder)  );
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pFolder, SG_TRUE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_ERR_CHECK(  SG_resolve__get_item__gid(pCtx, pResolve, vvSgHelpers::Convert_wxString_sglibString(sGid), &pItem)  );
	SG_ERR_CHECK(  _Resolve_GetData_FillItem(pCtx, sFolder, pItem, cData)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pFolder);
	//SG_RESOLVE_NULLFREE(pCtx, pResolve);
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	return !pCtx.Error_Check();
}

bool vvVerbs::Resolve_Diff(
	vvContext&         pCtx,
	const wxString&    sFolder,
	const wxString&    sItemGid,
	ResolveChoiceState eChoice,
	const wxString&    sLeftValue,
	const wxString&    sRightValue,
	const wxString&    sTool
	)
{
	SG_pathname*        pFolder      = NULL;
	SG_wc_tx *			pWcTx		 = NULL;
	SG_resolve*         pResolve     = NULL;
	SG_resolve__item*   pItem        = NULL;
	SG_resolve__choice* pChoice      = NULL;
	SG_resolve__value*  pLeftValue   = NULL;
	SG_resolve__value*  pRightValue  = NULL;

	SG_ERR_CHECK(  pFolder = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFolder)  );
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pFolder, SG_TRUE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );

	SG_ERR_CHECK(  SG_resolve__get_item__gid(pCtx, pResolve, vvSgHelpers::Convert_wxString_sglibString(sItemGid), &pItem)  );
	SG_ERR_CHECK(  SG_resolve__item__get_choice__state(pCtx, pItem, (SG_resolve__state)eChoice, &pChoice)  );
	SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, vvSgHelpers::Convert_wxString_sglibString(sLeftValue),  0, &pLeftValue)  );
	SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, vvSgHelpers::Convert_wxString_sglibString(sRightValue), 0, &pRightValue)  );
	SG_ERR_CHECK(  SG_resolve__diff_values(pCtx, pLeftValue, pRightValue, SG_DIFFTOOL__CONTEXT__GUI, vvSgHelpers::Convert_wxString_sglibString(sTool, true), NULL)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pFolder);
	//SG_RESOLVE_NULLFREE(pCtx, pResolve);
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	return !pCtx.Error_Check();
}

bool vvVerbs::Resolve_Merge(
	vvContext&         pCtx,
	const wxString&    sFolder,
	const wxString&    sItemGid,
	ResolveChoiceState eChoice,
	const wxString&    sBaselineValue,
	const wxString&    sOtherValue,
	const wxString&    sTool,
	const wxString&    sLabel,
	int*               pStatus,
	wxString*          pMergedValue
	)
{
	SG_pathname*        pFolder        = NULL;
	SG_wc_tx *			pWcTx		   = NULL;
	SG_resolve*         pResolve       = NULL;
	SG_resolve__item*   pItem          = NULL;
	SG_resolve__choice* pChoice        = NULL;
	SG_resolve__value*  pBaselineValue = NULL;
	SG_resolve__value*  pOtherValue    = NULL;
	SG_int32            iStatus        = 0;
	SG_resolve__value*  pResultValue   = NULL;

	SG_ERR_CHECK(  pFolder = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFolder)  );
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pFolder, SG_FALSE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );

	SG_ERR_CHECK(  SG_resolve__get_item__gid(pCtx, pResolve, vvSgHelpers::Convert_wxString_sglibString(sItemGid), &pItem)  );
	SG_ERR_CHECK(  SG_resolve__item__get_choice__state(pCtx, pItem, (SG_resolve__state)eChoice, &pChoice)  );
	SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, vvSgHelpers::Convert_wxString_sglibString(sBaselineValue),  0, &pBaselineValue)  );
	SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, vvSgHelpers::Convert_wxString_sglibString(sOtherValue), 0, &pOtherValue)  );
	SG_ERR_CHECK(  SG_resolve__merge_values(pCtx, pBaselineValue, pOtherValue, vvSgHelpers::Convert_wxString_sglibString(sTool, true), vvSgHelpers::Convert_wxString_sglibString(sLabel, true), SG_FALSE, &iStatus, &pResultValue, NULL)  );
	//SG_ERR_CHECK(  SG_resolve__save(pCtx, pResolve, SG_TRUE)  );

	if (pStatus != NULL)
	{
		*pStatus = static_cast<int>(iStatus);
	}
	if (pResultValue != NULL && pMergedValue != NULL)
	{
		const char* szLabel = NULL;
		SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pResultValue, &szLabel)  );
		*pMergedValue = vvSgHelpers::Convert_sglibString_wxString(szLabel);
	}
	// save any data that changed and free the working copy/resolve
	SG_ERR_CHECK(  SG_wc_tx__apply(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);


fail:
	SG_PATHNAME_NULLFREE(pCtx, pFolder);
	//SG_RESOLVE_NULLFREE(pCtx, pResolve);
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	return !pCtx.Error_Check();
}

bool vvVerbs::Resolve_Accept(
	vvContext&         pCtx,
	const wxString&    sFolder,
	const wxString&    sItemGid,
	ResolveChoiceState eChoice,
	const wxString&    sValue
	)
{
	SG_pathname*        pFolder      = NULL;
	SG_resolve*         pResolve     = NULL;
	SG_resolve__item*   pItem        = NULL;
	SG_resolve__choice* pChoice      = NULL;
	SG_resolve__value*  pValue       = NULL;
	SG_wc_tx * pWcTx = NULL;

	SG_ERR_CHECK(  pFolder = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFolder)  );
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pFolder, SG_FALSE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );
	
	SG_ERR_CHECK(  SG_resolve__get_item__gid(pCtx, pResolve, vvSgHelpers::Convert_wxString_sglibString(sItemGid), &pItem)  );
	SG_ERR_CHECK(  SG_resolve__item__get_choice__state(pCtx, pItem, (SG_resolve__state)eChoice, &pChoice)  );
	SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, vvSgHelpers::Convert_wxString_sglibString(sValue),  0, &pValue)  );
	SG_ERR_CHECK(  SG_resolve__accept_value(pCtx, pValue)  );
	//SG_ERR_CHECK(  SG_resolve__save(pCtx, pResolve, SG_TRUE)  );

	// save any data that changed and free the working copy/resolve
	SG_ERR_CHECK(  SG_wc_tx__apply(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);

fail:
	SG_PATHNAME_NULLFREE(pCtx, pFolder);
	//SG_RESOLVE_NULLFREE(pCtx, pResolve);
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);

	return !pCtx.Error_Check();
}

bool vvVerbs::PerformBranchAttachmentAction(
		vvContext&             pCtx,                    
		const wxString&        sFolder,                 
		const vvVerbs::BranchAttachmentAction action,   
		const wxString&        sBranchName				
		)
{
	sFolder;
	sBranchName;
	SG_pathname* pPath_top = NULL;
	SG_vhash * pvh_pile = NULL;
	SG_vhash * pvh_pile_branches = NULL;
	SG_vhash * pvh_pile_closed_branches = NULL;
	SG_bool b_has = SG_FALSE;
	SG_repo * pRepo = NULL;
	SG_string * pstrRepoName = NULL;
	if (action == vvVerbs::BRANCH_ACTION__ERROR || action == vvVerbs::BRANCH_ACTION__NO_CHANGE)
		return true;
	SG_ERR_CHECK(  pPath_top = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sFolder)  );
	if (action == vvVerbs::BRANCH_ACTION__DETACH)
	{
		//Detach the working copy.
		SG_ERR_CHECK(  SG_wc__branch__detach(pCtx, pPath_top)  );
		//pCtx.Log_ReportInfo("The working copy has been detached.");
	}
	else if (action == vvVerbs::BRANCH_ACTION__ATTACH)
	{
		SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPath_top, NULL, &pstrRepoName, NULL)  );
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, SG_string__sz(pstrRepoName), &pRepo)  );
		SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_pile_branches)  );
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pile_branches, vvSgHelpers::Convert_wxString_sglibString(sBranchName), &b_has)  );
        if (!b_has)
        {
            SG_ERR_THROW(  SG_ERR_BRANCH_NOT_FOUND  );
        }

		SG_ERR_CHECK(  SG_wc__branch__attach(pCtx, pPath_top, vvSgHelpers::Convert_wxString_sglibString(sBranchName))  );
	}
	else if (action == vvVerbs::BRANCH_ACTION__CREATE)
	{
		SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPath_top, NULL, &pstrRepoName, NULL)  );
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, SG_string__sz(pstrRepoName), &pRepo)  );
		SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_pile_branches)  );
        SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pile_branches, vvSgHelpers::Convert_wxString_sglibString(sBranchName), &b_has)  );
        if (b_has)
        {
            SG_ERR_THROW(  SG_ERR_BRANCH_ALREADY_EXISTS  );
        }

		{
			SG_bool bHasClosedBranches;
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pile, "closed", &bHasClosedBranches)  );
			if (bHasClosedBranches)
			{
				SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "closed", &pvh_pile_closed_branches)  );
				SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pile_closed_branches, vvSgHelpers::Convert_wxString_sglibString(sBranchName), &b_has)  );
				if (b_has)
				{
					SG_ERR_THROW(  SG_ERR_BRANCH_ALREADY_EXISTS  );
				}
			}
		}
		SG_ERR_CHECK(  SG_wc__branch__attach_new(pCtx, pPath_top, vvSgHelpers::Convert_wxString_sglibString(sBranchName))  );
	}
	else if (action == vvVerbs::BRANCH_ACTION__CLOSE)
	{
		SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPath_top, NULL, &pstrRepoName, NULL)  );
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, SG_string__sz(pstrRepoName), &pRepo)  );
		SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );
        SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "branches", &pvh_pile_branches)  );
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pile_branches, vvSgHelpers::Convert_wxString_sglibString(sBranchName), &b_has)  );
        if (!b_has)
        {
            SG_ERR_THROW(  SG_ERR_BRANCH_NOT_FOUND  );
        }


		SG_audit pAudit;
		SG_ERR_CHECK(  SG_audit__init(pCtx, &pAudit, pRepo,
							SG_AUDIT__WHEN__NOW,
							SG_AUDIT__WHO__FROM_SETTINGS)  );
		SG_ERR_CHECK(  SG_vc_branches__close(pCtx, pRepo, vvSgHelpers::Convert_wxString_sglibString(sBranchName), &pAudit)  );

		SG_ERR_CHECK(  SG_wc__branch__attach(pCtx, pPath_top, vvSgHelpers::Convert_wxString_sglibString(sBranchName))  );
	}
	else if (action == vvVerbs::BRANCH_ACTION__REOPEN)
	{
	    SG_bool bHasClosedBranches;
	
		SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPath_top, NULL, &pstrRepoName, NULL)  );
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, SG_string__sz(pstrRepoName), &pRepo)  );
		SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );
    	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pile, "closed", &bHasClosedBranches)  );
		if (bHasClosedBranches)
		{
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "closed", &pvh_pile_closed_branches)  );
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pile_closed_branches, vvSgHelpers::Convert_wxString_sglibString(sBranchName), &b_has)  );
			if (b_has)
			{
				//The branch they specified is indeed closed.
				//Reopen it.
				SG_audit pAudit;
				SG_ERR_CHECK(  SG_audit__init(pCtx, &pAudit, pRepo,
								  SG_AUDIT__WHEN__NOW,
								  SG_AUDIT__WHO__FROM_SETTINGS)  );
				SG_ERR_CHECK(  SG_vc_branches__reopen(pCtx, pRepo, vvSgHelpers::Convert_wxString_sglibString(sBranchName), &pAudit)  );
			}
		}
		SG_ERR_CHECK(  SG_wc__branch__attach(pCtx, pPath_top, vvSgHelpers::Convert_wxString_sglibString(sBranchName))  );
	}
fail:
	SG_STRING_NULLFREE(pCtx, pstrRepoName);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_PATHNAME_NULLFREE(pCtx, pPath_top);
	SG_VHASH_NULLFREE(pCtx, pvh_pile);
	return !pCtx.Error_Check();
}

bool vvVerbs::CheckForDagConnection(
		vvContext&             pCtx,                    //< [in] [out] Error and context info.
		const wxString&        sRepoName,               //< [in] The repo that contains the change sets.
		const wxString&        sPossibleAncestor,		//< [in] The potential parent.
		const wxString&        sPossibleDescendant,		//< [in] The potential descendant.
		bool *				   bResultAreConnected		//< [out] The result of the check.
		)
{
	SG_repo * pRepo = NULL;
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepoName), &pRepo)  );
	if (sPossibleAncestor == sPossibleDescendant)
	{
		*bResultAreConnected = true;
	}
	else
	{
		SG_dagquery_relationship relationship;
		SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, vvSgHelpers::Convert_wxString_sglibString(sPossibleAncestor), vvSgHelpers::Convert_wxString_sglibString(sPossibleDescendant), SG_TRUE, SG_FALSE, &relationship)  );
		*bResultAreConnected = relationship == SG_DAGQUERY_RELATIONSHIP__ANCESTOR;
	}
fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	return !pCtx.Error_Check();
}

bool vvVerbs::DeleteToRecycleBin(
		vvContext&             pCtx,
		const wxString&        sDiskPath,
		bool				   bOkToShowGUI
		)
{
	SG_pathname * pPathToDelete = NULL;

	SG_ERR_CHECK(  pPathToDelete = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sDiskPath)  );
	SG_ERR_CHECK(  SG_fsobj__delete_to_recycle_bin(pCtx, vvSgHelpers::Convert_cppBool_sgBool(bOkToShowGUI), pPathToDelete) );
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathToDelete);
	return !pCtx.Error_Check();
}

bool vvVerbs::ListAreas(
	vvContext&             pCtx,
	const wxString&        sRepoName,
	wxArrayString&		   sAreaNames
	)
{
	SG_repo * pRepo = NULL;
	SG_vhash * pvh_areas = NULL;
	SG_varray * pva_area_names = NULL;

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepoName), &pRepo)  );

	SG_ERR_CHECK(  SG_area__list(pCtx, pRepo, &pvh_areas)  );
	SG_ERR_CHECK(  SG_vhash__get_keys(pCtx, pvh_areas, &pva_area_names)  );
	SG_ERR_CHECK( vvSgHelpers::Convert_sgVarray_wxArrayString(pCtx, pva_area_names, sAreaNames)  );
fail:
	SG_VHASH_NULLFREE(pCtx, pvh_areas);
	SG_VARRAY_NULLFREE(pCtx, pva_area_names);
	return !pCtx.Error_Check();
}

wxString vvVerbs::GetVersionControlAreaName()
{
	return wxString(SG_AREA_NAME__VERSION_CONTROL);
}

bool vvVerbs::StartWebServer(
		vvContext&             pCtx,
		struct mg_context ** ppctx,
		bool bPublic,
		wxString sWorkingFolderLocation,
		int nPortNumber
		)
{
	SG_pathname * pPathnameWorkingFolderLocation = NULL;
	if (!sWorkingFolderLocation.IsEmpty())
	{
		SG_ERR_CHECK(  pPathnameWorkingFolderLocation = vvSgHelpers::Convert_wxString_sgPathname(pCtx, sWorkingFolderLocation)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &pPathnameWorkingFolderLocation)  );
	}
	SG_ERR_CHECK(  SG_fsobj__cd__pathname(pCtx, pPathnameWorkingFolderLocation)  );
	SG_ERR_CHECK(  mg_start(pCtx, vvSgHelpers::Convert_cppBool_sgBool(bPublic), nPortNumber, ppctx)  );
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathnameWorkingFolderLocation);
	return !pCtx.Error_Check();
}

bool vvVerbs::StopWebServer(
		vvContext&             pCtx,
		struct mg_context *ctx
		)
{
	SG_ERR_IGNORE(  mg_stop(pCtx, ctx)  );
	return !pCtx.Error_Check();
}

const char* vvVerbs::GetConfigScopeName(
	vvVerbs::ConfigScope eScope
	)
{
	switch (eScope)
	{
	case vvVerbs::CONFIG_SCOPE_INSTANCE: return SG_LOCALSETTING__SCOPE__INSTANCE + 1;
	case vvVerbs::CONFIG_SCOPE_REPO:     return SG_LOCALSETTING__SCOPE__REPO     + 1;
	case vvVerbs::CONFIG_SCOPE_ADMIN:    return SG_LOCALSETTING__SCOPE__ADMIN    + 1;
	case vvVerbs::CONFIG_SCOPE_MACHINE:  return SG_LOCALSETTING__SCOPE__MACHINE  + 1;
	case vvVerbs::CONFIG_SCOPE_DEFAULT:  return SG_LOCALSETTING__SCOPE__DEFAULT  + 1;
	default:                             return NULL;
	}
}

static void _FormatConfigName(
	vvContext&           pCtx,
	vvVerbs::ConfigScope eScope,
	const wxString&      sName,
	SG_repo*             pRepo,
	wxString&            sConfig
	)
{
	char*       szValue      = NULL;
	const char* szConstValue = NULL;

	switch (eScope)
	{
	case vvVerbs::CONFIG_SCOPE_INSTANCE:
		SG_NULLARGCHECK(pRepo);
		SG_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepo, &szConstValue)  );
		sConfig = wxString::Format("%s/%s/%s", SG_LOCALSETTING__SCOPE__INSTANCE, vvSgHelpers::Convert_sglibString_wxString(szConstValue), sName);
		break;
	case vvVerbs::CONFIG_SCOPE_REPO:
		SG_NULLARGCHECK(pRepo);
		SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pRepo, &szValue)  );
		sConfig = wxString::Format("%s/%s/%s", SG_LOCALSETTING__SCOPE__REPO, vvSgHelpers::Convert_sglibString_wxString(szValue), sName);
		break;
	case vvVerbs::CONFIG_SCOPE_ADMIN:
		SG_NULLARGCHECK(pRepo);
		SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &szValue)  );
		sConfig = wxString::Format("%s/%s/%s", SG_LOCALSETTING__SCOPE__ADMIN, vvSgHelpers::Convert_sglibString_wxString(szValue), sName);
		break;
	case vvVerbs::CONFIG_SCOPE_MACHINE:
		sConfig = wxString::Format("%s/%s", SG_LOCALSETTING__SCOPE__MACHINE, sName);
		break;
	case vvVerbs::CONFIG_SCOPE_DEFAULT:
		sConfig = wxString::Format("%s/%s", SG_LOCALSETTING__SCOPE__DEFAULT, sName);
		break;
	default:
		sConfig = wxEmptyString;
		break;
	}

fail:
	SG_NULLFREE(pCtx, szValue);
	return;
}

static void _GetConfigValue(
	vvContext&      pCtx,
	vvFlags         cScope,
	const wxString& sName,
	const wxString& sRepo,
	SG_variant**    ppValue
	)
{
	SG_repo* pRepo = NULL;

	// if they specified a repo, open it
	if (sRepo != wxEmptyString)
	{
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	}

	// check if they specified any scopes
	if (cScope.FlagsEmpty())
	{
		// let the system find the setting in the most appropriate scope
		SG_ERR_CHECK(  SG_localsettings__get__variant(pCtx, vvSgHelpers::Convert_wxString_sglibString(sName), pRepo, ppValue, NULL)  );
	}
	else
	{
		// run through each specified scope and check for the setting
		for (vvFlags::enum_iterator<vvVerbs::ConfigScope> it = cScope.begin(); it != cScope.end(); ++it)
		{
			wxString sFullName;

			SG_ERR_CHECK(  _FormatConfigName(pCtx, *it, sName, pRepo, sFullName)  );
			SG_ERR_CHECK(  SG_localsettings__get__variant(pCtx, vvSgHelpers::Convert_wxString_sglibString(sFullName), pRepo, ppValue, NULL)  );
			if (*ppValue != NULL)
			{
				break;
			}
		}
	}

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	return;
}

bool vvVerbs::GetConfigValue(
	vvContext&      pCtx,
	vvFlags         cScope,
	const wxString& sName,
	const wxString& sRepo,
	wxString&       sValue,
	const wxString& sDefault
	)
{
	SG_variant* pValue = NULL;

	SG_ERR_CHECK(  _GetConfigValue(pCtx, cScope, sName, sRepo, &pValue)  );
	if (pValue == NULL)
	{
		sValue = sDefault;
	}
	else
	{
		const char* sgValue = NULL;

		SG_ERR_CHECK(  SG_variant__get__sz(pCtx, pValue, &sgValue)  );
		sValue = vvSgHelpers::Convert_sglibString_wxString(sgValue);
	}

fail:
	SG_VARIANT_NULLFREE(pCtx, pValue);
	return !pCtx.Error_Check();
}

bool vvVerbs::GetConfigValue(
	vvContext&      pCtx,
	vvFlags         cScope,
	const wxString& sName,
	const wxString& sRepo,
	wxInt64&        iValue,
	wxInt64         iDefault
	)
{
	SG_variant* pValue = NULL;

	SG_ERR_CHECK(  _GetConfigValue(pCtx, cScope, sName, sRepo, &pValue)  );
	if (pValue == NULL)
	{
		iValue = iDefault;
	}
	else
	{
		SG_int64 sgValue = NULL;

		SG_ERR_CHECK(  SG_variant__get__int64(pCtx, pValue, &sgValue)  );
		iValue = sgValue;
	}

fail:
	SG_VARIANT_NULLFREE(pCtx, pValue);
	return !pCtx.Error_Check();
}

bool vvVerbs::GetConfigValue(
	vvContext&      pCtx,
	vvFlags         cScope,
	const wxString& sName,
	const wxString& sRepo,
	wxArrayString&  cValue
	)
{
	SG_variant* pValue = NULL;

	SG_ERR_CHECK(  _GetConfigValue(pCtx, cScope, sName, sRepo, &pValue)  );
	if (pValue != NULL)
	{
		SG_varray* sgValue = NULL;

		SG_ERR_CHECK(  SG_variant__get__varray(pCtx, pValue, &sgValue)  );
		SG_ERR_CHECK(  vvSgHelpers::Convert_sgVarray_wxArrayString(pCtx, sgValue, cValue)  );
	}

fail:
	SG_VARIANT_NULLFREE(pCtx, pValue);
	return !pCtx.Error_Check();
}

void _SetConfigValueVariant(
	vvContext&        pCtx,
	const wxString&   sName,
	const SG_variant* pValue
	)
{
	switch (pValue->type)
	{
	case SG_VARIANT_TYPE_NULL:
		SG_ERR_CHECK(  SG_localsettings__reset(pCtx, vvSgHelpers::Convert_wxString_sglibString(sName))  );
		break;
	case SG_VARIANT_TYPE_SZ:
		SG_ERR_CHECK(  SG_localsettings__update__sz(pCtx, vvSgHelpers::Convert_wxString_sglibString(sName), pValue->v.val_sz)  );
		break;
	case SG_VARIANT_TYPE_INT64:
		SG_ERR_CHECK(  SG_localsettings__update__int64(pCtx, vvSgHelpers::Convert_wxString_sglibString(sName), pValue->v.val_int64)  );
		break;
	case SG_VARIANT_TYPE_VARRAY:
		SG_ERR_CHECK(  SG_localsettings__update__varray(pCtx, vvSgHelpers::Convert_wxString_sglibString(sName), pValue->v.val_varray)  );
		break;
	default:
		SG_ASSERT(false);
	}

fail:
	return;
}

void _SetConfigValue(
	vvContext&        pCtx,
	vvFlags           cScope,
	const wxString&   sName,
	const wxString&   sRepo,
	const SG_variant* pValue
	)
{
	SG_repo* pRepo = NULL;

	// if they specified a repo, open it
	if (sRepo != wxEmptyString)
	{
		SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, vvSgHelpers::Convert_wxString_sglibString(sRepo), &pRepo)  );
	}

	// check if they specified any scopes
	if (cScope.FlagsEmpty())
	{
		// let the system set the setting
		// assume that the scope is specified in the name
		// if not, it will end up set at machine scope
		SG_ERR_CHECK(  _SetConfigValueVariant(pCtx, sName, pValue)  );
	}
	else
	{
		// run through each specified scope and set the setting in it
		for (vvFlags::enum_iterator<vvVerbs::ConfigScope> it = cScope.begin(); it != cScope.end(); ++it)
		{
			wxString sFullName;

			SG_ERR_CHECK(  _FormatConfigName(pCtx, *it, sName, pRepo, sFullName)  );
			SG_ERR_CHECK(  _SetConfigValueVariant(pCtx, sFullName, pValue)  );
		}
	}

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	return;
}

bool vvVerbs::SetConfigValue(
	vvContext&      pCtx,
	vvFlags         cScope,
	const wxString& sName,
	const wxString& sRepo,
	const wxString& sValue
	)
{
	wxScopedCharBuffer szValue;
	SG_variant         cVariant;

	szValue = vvSgHelpers::Convert_wxString_sglibString(sValue);

	cVariant.type     = SG_VARIANT_TYPE_SZ;
	cVariant.v.val_sz = szValue.data();

	SG_ERR_CHECK(  _SetConfigValue(pCtx, cScope, sName, sRepo, &cVariant)  );

fail:
	return !pCtx.Error_Check();
}

bool vvVerbs::SetConfigValue(
	vvContext&      pCtx,
	vvFlags         cScope,
	const wxString& sName,
	const wxString& sRepo,
	wxInt64         iValue
	)
{
	SG_variant cVariant;

	cVariant.type        = SG_VARIANT_TYPE_INT64;
	cVariant.v.val_int64 = static_cast<SG_int64>(iValue);

	SG_ERR_CHECK(  _SetConfigValue(pCtx, cScope, sName, sRepo, &cVariant)  );

fail:
	return !pCtx.Error_Check();
}

bool vvVerbs::SetConfigValue(
	vvContext&           pCtx,
	vvFlags              cScope,
	const wxString&      sName,
	const wxString&      sRepo,
	const wxArrayString& cValue
	)
{
	SG_variant cVariant;

	cVariant.type = SG_VARIANT_TYPE_VARRAY;
	SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgVarray(pCtx, cValue, &cVariant.v.val_varray, false)  );

	SG_ERR_CHECK(  _SetConfigValue(pCtx, cScope, sName, sRepo, &cVariant)  );

fail:
	SG_VARRAY_NULLFREE(pCtx, cVariant.v.val_varray);
	return !pCtx.Error_Check();
}

bool vvVerbs::UnsetConfigValue(
	vvContext&      pCtx,
	vvFlags         cScope,
	const wxString& sName,
	const wxString& sRepo
	)
{
	SG_variant cVariant;

	cVariant.type = SG_VARIANT_TYPE_NULL;
	SG_ERR_CHECK(  _SetConfigValue(pCtx, cScope, sName, sRepo, &cVariant)  );

fail:
	return !pCtx.Error_Check();
}

bool vvVerbs::Upgrade__listOldRepos(
	vvContext&      pCtx,
	wxArrayString& aRepoNames
	)
{
	SG_vhash* pvh_old_repos = NULL;
	SG_varray* pva_keys = NULL;
	SG_ERR_CHECK( SG_vv_verbs__upgrade__find_old_repos(pCtx, &pvh_old_repos)  );
	if (pvh_old_repos)
	{
		SG_ERR_CHECK(  SG_vhash__get_keys(pCtx, pvh_old_repos, &pva_keys)  );
		SG_ERR_CHECK(  vvSgHelpers::Convert_sgVarray_wxArrayString(pCtx, pva_keys, aRepoNames)  );
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_old_repos);
	SG_VARRAY_NULLFREE(pCtx, pva_keys);
	return !pCtx.Error_Check();

}
bool vvVerbs::Upgrade(
	vvContext&      pCtx
	)
{
	SG_vhash* pvh_old_repos = NULL;
	SG_pathname* pPath_closet_backup = NULL;
	SG_ERR_CHECK( SG_vv_verbs__upgrade__find_old_repos(pCtx, &pvh_old_repos)  );

	SG_ERR_CHECK( SG_log__push_operation(pCtx, "Backing up closet", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_closet__backup(pCtx, &pPath_closet_backup)  );
	SG_ERR_CHECK( SG_log__pop_operation(pCtx)  );

	SG_ERR_CHECK(  SG_vv_verbs__upgrade_repos(pCtx, pvh_old_repos)  );
fail:
	SG_VHASH_NULLFREE(pCtx, pvh_old_repos);
	SG_PATHNAME_NULLFREE(pCtx, pPath_closet_backup);
	return !pCtx.Error_Check();
}

bool vvVerbs::GetSavedPassword(vvContext&           pCtx, 
		const wxString& sRemoteRepo,
		const wxString& sUserName,
		wxString& sPassword)
{
	const char* sgUser = vvSgHelpers::Convert_wxString_sglibString(sUserName).data();
	const char* sgRemoteRepo = vvSgHelpers::Convert_wxString_sglibString(sRemoteRepo).data();
	SG_string * pstrPassword = NULL;
	SG_ERR_CHECK(  SG_password__get(pCtx, sgRemoteRepo, sgUser, &pstrPassword)  );
	if (pstrPassword != NULL)
		sPassword = vvSgHelpers::Convert_sglibString_wxString(SG_string__sz(pstrPassword));
fail:
	SG_STRING_NULLFREE(pCtx, pstrPassword);
	return !pCtx.Error_Check();
}

bool vvVerbs::SetSavedPassword(vvContext&           pCtx, 
		const wxString& sRemoteRepo,
		const wxString& sUserName,
		const wxString& sPassword)
{
	SG_string * pstrUser = NULL;
	SG_string * pstrPassword = NULL;
	const char* sgRemoteRepo = vvSgHelpers::Convert_wxString_sglibString(sRemoteRepo).data();
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pstrUser, vvSgHelpers::Convert_wxString_sglibString(sUserName).data())  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pstrPassword, vvSgHelpers::Convert_wxString_sglibString(sPassword).data())  );
	SG_ERR_CHECK(  SG_password__set(pCtx, sgRemoteRepo, pstrUser, pstrPassword)  );
fail:
	SG_STRING_NULLFREE(pCtx, pstrUser);
	SG_STRING_NULLFREE(pCtx, pstrPassword);
	return !pCtx.Error_Check();
}


bool vvVerbs::DeleteSavedPasswords(
	vvContext& pCtx
	)
{
	SG_ERR_CHECK(  SG_password__forget_all(pCtx)  );

fail:
	return !pCtx.Error_Check();
}

bool vvVerbs::ListWorkItems(vvContext& pCtx,
	const wxString& sRepoName,
	const wxString& sSearchTerm,
	WorkItemList& workItems)
{
	SG_varray * pva_results = NULL;
	SG_uint32 uWorkItemsCount = 0;
	int lastStatusInt = 0;
	wxString lastStatusString;

	SG_ERR_CHECK (  SG_vv_verbs__work_items__text_search(pCtx, 
		vvSgHelpers::Convert_wxString_sglibString(sRepoName).data(),
		vvSgHelpers::Convert_wxString_sglibString(sSearchTerm).data()
		, &pva_results)  );

	SG_ERR_CHECK(  SG_varray__count(pCtx, pva_results, &uWorkItemsCount)  );
	
	for (SG_uint32 uIndex = 0u; uIndex < uWorkItemsCount; ++uIndex)
	{
		SG_vhash * pvhCurrent = NULL;
		const char * pszID = NULL;
		const char * pszRecID = NULL;
		const char * pszTitle = NULL;
		const char * pszStatus = NULL;

		// get the current branch name and add it to the list
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_results, uIndex, &pvhCurrent)  );
		//SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhCurrent, "name", &pszID)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhCurrent, "id", &pszID)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhCurrent, "title", &pszTitle)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhCurrent, "status", &pszStatus)  );
		WorkItemEntry wie;
		wie.sID = vvSgHelpers::Convert_sglibString_wxString(pszID);
		wie.sRecID = vvSgHelpers::Convert_sglibString_wxString(pszRecID);
		wie.sTitle = vvSgHelpers::Convert_sglibString_wxString(pszTitle);
		wie.sStatus = vvSgHelpers::Convert_sglibString_wxString(pszStatus);
		if (wie.sStatus.CompareTo(lastStatusString) != 0)
		{
			lastStatusString = wie.sStatus;
			lastStatusInt++;
		}
		wie.nStatus = lastStatusInt;
		workItems.push_back(wie);
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pva_results);
	return !pCtx.Error_Check();
}

bool vvVerbs::Lock(
	vvContext&           pCtx,
	const vvRepoLocator& cRepoLocator,
	const wxString& sLockServer,
	const wxString& sUsername,
	const wxString& sPassword,
	const wxArrayString& pPaths
	)
{
	const char* pszUptreamRepo = NULL;
	const char* pszUsername     = NULL;
	const char* pszPassword     = NULL;
	SG_stringarray * psa_paths = NULL;
	SG_pathname * pPathMyWF = NULL;

	pszUptreamRepo = vvSgHelpers::Convert_wxString_sglibString(sLockServer);
	
	pPathMyWF  = vvSgHelpers::Convert_wxString_sgPathname(pCtx, cRepoLocator.GetWorkingFolder());
	
	pszUsername     = vvSgHelpers::Convert_wxString_sglibString(sUsername, true);
	pszPassword     = vvSgHelpers::Convert_wxString_sglibString(sPassword, true);
	SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgStringArray(pCtx, pPaths, &psa_paths)  );

	SG_ERR_CHECK(  SG_wc__lock(pCtx, pPathMyWF, psa_paths, pszUsername, pszPassword, pszUptreamRepo)  );

	SG_ERR_CHECK(  RefreshWC(pCtx, cRepoLocator.GetWorkingFolder())  );
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathMyWF);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_paths);
	return !pCtx.Error_Check();
}

bool vvVerbs::Unlock(
	vvContext&           pCtx,
	const vvRepoLocator& cRepoLocator,
	const wxString& sLockServer,
	bool bForce,
	const wxString& sUsername,
	const wxString& sPassword,
	const wxArrayString& pPaths
	)
{
	const char* pszUptreamRepo = NULL;
	const char* pszUsername     = NULL;
	const char* pszPassword     = NULL;
	SG_stringarray * psa_paths = NULL;
	SG_pathname * pPathMyWF = NULL;

	pszUptreamRepo = vvSgHelpers::Convert_wxString_sglibString(sLockServer);
	pPathMyWF  = vvSgHelpers::Convert_wxString_sgPathname(pCtx, cRepoLocator.GetWorkingFolder());

	pszUsername     = vvSgHelpers::Convert_wxString_sglibString(sUsername, true);
	pszPassword     = vvSgHelpers::Convert_wxString_sglibString(sPassword, true);

	SG_ERR_CHECK(  vvSgHelpers::Convert_wxArrayString_sgStringArray(pCtx, pPaths, &psa_paths)  );
	SG_ERR_CHECK(  SG_wc__unlock(pCtx, pPathMyWF, psa_paths, bForce, pszUsername, pszPassword, pszUptreamRepo)  );

	SG_ERR_CHECK(  RefreshWC(pCtx, cRepoLocator.GetWorkingFolder())  );
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathMyWF);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_paths);
	return !pCtx.Error_Check();
}

bool vvVerbs::CheckFileForLock(
		vvContext&           pCtx,
		const vvRepoLocator& cRepoLocator,
		const wxString& sLocalFile,
		vvVerbs::LockList& cLockData)
{
	
	SG_pathname * pPathname = NULL;
	SG_varray*				  pvaStatus			 = NULL;
	SG_vhash*				  pvhLegend			 = NULL;
	DiffData            cDiff;
	_TranslateDiffObject_Data cCallbackData;

	SG_ERR_CHECK(  pPathname = vvSgHelpers::Convert_wxString_sgPathname(pCtx, cRepoLocator.GetWorkingFolder())  );
	SG_ERR_CHECK(  SG_wc__status(pCtx, pPathname, vvSgHelpers::Convert_wxString_sglibString(pCtx, sLocalFile), 0, SG_FALSE, SG_TRUE, SG_TRUE, SG_FALSE, SG_FALSE, SG_FALSE, &pvaStatus, &pvhLegend)  );
	cCallbackData.pvaStatus = pvaStatus;
	cCallbackData.pDiffData = &cDiff;
	SG_ERR_CHECK(  SG_varray__foreach(pCtx, pvaStatus, _TranslateStandardDiffObject, (void*)&cCallbackData)  );

	if (cDiff.size() > 0)
	{
		DiffEntry de = cDiff.front();
		cLockData = de.cLocks;
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
	return !pCtx.Error_Check();
}

bool vvVerbs::RefreshWC(vvContext&           pCtx,
	const wxString& sWC_Root,
	const wxArrayString * psWC_Specific_Paths)
{
	const char * psz_wc_root = vvSgHelpers::Convert_wxString_sglibString(sWC_Root, true);
	SG_stringarray * psa_specific_paths = NULL;
	if (psWC_Specific_Paths != NULL)
		vvSgHelpers::Convert_wxArrayString_sgStringArray(pCtx, *psWC_Specific_Paths, &psa_specific_paths);

	vvCacheMessaging::RequestRefreshWC(pCtx, psz_wc_root, psa_specific_paths);
	if (pCtx.Error_Check())
		pCtx.Log_ReportVerbose_CurrentError();
	pCtx.Error_Reset(); //Ignore any errors in refreshing the wc.
	return !pCtx.Error_Check();
}

bool vvVerbs::GetVersionString(vvContext&           pCtx,
	wxString& sVersionNumber)
{
	const char * psz_version = NULL;

	SG_ERR_CHECK(  SG_lib__version(pCtx, &psz_version)  );

	sVersionNumber = vvSgHelpers::Convert_sglibString_wxString(psz_version);

fail:
	return !pCtx.Error_Check();
}
