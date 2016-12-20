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
 * @file sg_wc__status__classic_format.c
 *
 * @details Format a canonical status VARRAY as a string
 * for a CLASSIC display (suitable for console).
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

#define FORMAT__PRIMARY          "%s%30s: %c%c %s%s"
#define FORMAT__PRIMARY_DELETED  "%s%30s: %c%c (%s)%s"

//                               "%s123456789012345678901234567890: !+ <path>%s"
#define FORMAT__ID               "%s                                   # Id @%s%s"
#define FORMAT__WAS_PATH         "%s                                   # %12s: %s%s"

//////////////////////////////////////////////////////////////////

struct _format_data
{
	SG_string *			pString;
	SG_bool				bVerbose;

	SG_uint32			kSection;

	const char *		pszLinePrefix;
	const char *		pszLineEOL;
};

struct _sections
{
	const char * pszPrefix;
	SG_bool bReportWasPath;
	SG_bool bOnlyIfVerbose;
};

/**
 * WARNING: If you and/modify this table, see the table
 * in "src/server_files/ui/getstatus.js" and
 * in "src/server_files/ui/getchangeset.js" and
 * make similar changes there.
 *
 * We use a prefix match using these keys so that we
 * print all of the "Added", "Added (B)", and "Added (C)"
 * items together.
 * 
 */
static struct _sections aSections[] =
{ { "Added",         SG_FALSE, SG_FALSE },
  { "Modified",      SG_FALSE, SG_FALSE },
  { "Auto-Merged",   SG_FALSE, SG_FALSE },
  { "Attributes",    SG_FALSE, SG_FALSE },
  { "Removed",       SG_FALSE, SG_FALSE },
  { "Existence",     SG_FALSE, SG_FALSE },
  { "Renamed",       SG_TRUE,  SG_FALSE },
  { "Moved",         SG_TRUE,  SG_FALSE },
  { "Lost",          SG_FALSE, SG_FALSE },
  { "Found",         SG_FALSE, SG_FALSE },
  { "Ignored",       SG_FALSE, SG_TRUE  },
  { "Locked",        SG_FALSE, SG_FALSE },
  { "Reserved",      SG_FALSE, SG_TRUE  },
  { "Sparse",        SG_FALSE, SG_FALSE },
  { "Resolved",      SG_FALSE, SG_FALSE },
  { "Unresolved",    SG_FALSE, SG_FALSE },
  { "Choice Resolved",   SG_FALSE, SG_TRUE  },
  { "Choice Unresolved", SG_FALSE, SG_TRUE  },
  { "Unchanged",     SG_FALSE, SG_FALSE },	// TODO 2012/03/07 is this item needed ?
};

//////////////////////////////////////////////////////////////////

/**
 * Format the main detail line for the item.
 *
 */
static void _format_primary(SG_context * pCtx,
							struct _format_data * pData,
							const char * pszSectionName,
							const SG_vhash * pvhItem)
{
	const char * pszPath;
	SG_int64 i64;
	SG_vhash * pvhStatus;
	SG_wc_status_flags statusFlags;
	SG_bool bHasMultipleChanges;
	SG_bool bDeleted;
	char chMultiple;
	char chConflict;

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, "status", &pvhStatus)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhStatus, "flags", &i64)  );
	statusFlags = ((SG_wc_status_flags)i64);

	bHasMultipleChanges = ((statusFlags & SG_WC_STATUS_FLAGS__A__MULTIPLE_CHANGE) != 0);
	bDeleted = ((statusFlags & SG_WC_STATUS_FLAGS__S__DELETED) != 0);
	chMultiple = ((bHasMultipleChanges) ? '+' : ' ');

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "path", &pszPath)  );

	SG_ASSERT_RELEASE_FAIL2(  (((statusFlags & SG_WC_STATUS_FLAGS__X__UNRESOLVED) == 0)
							   || ((statusFlags & SG_WC_STATUS_FLAGS__X__RESOLVED) == 0)),
							  (pCtx, "Both RESOLVED and UNRESOLVED set: %s", pszPath)  );
	if (statusFlags & SG_WC_STATUS_FLAGS__X__RESOLVED)
		chConflict = '.';
	else if (statusFlags & SG_WC_STATUS_FLAGS__X__UNRESOLVED)
		chConflict = '!';
	else
		chConflict = ' ';

	SG_ERR_CHECK(  SG_string__append__format(pCtx,
											 pData->pString,
											 ((bDeleted) ? FORMAT__PRIMARY_DELETED : FORMAT__PRIMARY),
											 pData->pszLinePrefix,
											 pszSectionName,
											 chConflict,
											 chMultiple,
											 pszPath,
											 pData->pszLineEOL)  );

fail:
	return;
}

/**
 * Format common detail lines that should follow
 * the primary and any per-section detail lines.
 *
 */
static void _format_primary_trailer(SG_context * pCtx,
									struct _format_data * pData,
									const SG_vhash * pvhItem,
									SG_bool bReportWasPath)
{
	if (bReportWasPath)
	{
		char * aszSub[] = { SG_WC__STATUS_SUBSECTION__B,
							SG_WC__STATUS_SUBSECTION__C,
							SG_WC__STATUS_SUBSECTION__A };
		SG_vhash * pvhSub = NULL;	// we do not own this
		SG_uint32 k;
		
		for (k=0; k<SG_NrElements(aszSub); k++)
		{
			SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvhItem, aszSub[k], &pvhSub)  );
			if (pvhSub)
			{
				const char * pszRepoPath = NULL;
				const char * pszWasLabel = NULL;

				SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhSub, "was_label", &pszWasLabel)  );
				SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhSub, "path", &pszRepoPath)  );
				SG_ASSERT_RELEASE_FAIL( (pszRepoPath) );
				SG_ERR_CHECK(  SG_string__append__format(pCtx,
														 pData->pString,
														 FORMAT__WAS_PATH,
														 pData->pszLinePrefix,
														 pszWasLabel,
														 pszRepoPath,
														 pData->pszLineEOL)  );
			}
		}
	}

	if (pData->bVerbose)
	{
		const char * pszGid;

		// We only print permanent GIDs (ones with a 'g').
		// We don't bother printing temporary GIDs (ones
		// with a 't').
		// 
		// For found/ignored items, sg_wc__status__append()
		// changed the gid to have a 't' prefix because they
		// are temporary ids and only valid until the end of
		// that transaction.  so don't bother printing them
		// since they will be gone by the time you can do
		// anything with them using the printed output.

		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );
		if (pszGid[0] == 'g')
			SG_ERR_CHECK(  SG_string__append__format(pCtx,
													 pData->pString,
													 FORMAT__ID,
													 pData->pszLinePrefix,
													 pszGid,
													 pData->pszLineEOL)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _do_section_item_headings(SG_context * pCtx,
									  struct _format_data * pData,
									  const SG_vhash * pvhItem)
{
	SG_varray * pvaSectionLabels = NULL;	// we do not own this
	SG_uint32 j, jLimit;
	const char * pszPrefix_k;
	SG_uint32 lenPrefix_k;

	pszPrefix_k = aSections[pData->kSection].pszPrefix;
	lenPrefix_k = SG_STRLEN(pszPrefix_k);
									  
	SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvhItem, "headings", &pvaSectionLabels)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaSectionLabels, &jLimit)  );
	for (j=0; j<jLimit; j++)
	{
		const char * pszLabel_j;

		SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pvaSectionLabels, j, &pszLabel_j)  );
		if (strncmp(pszLabel_j, pszPrefix_k, lenPrefix_k) == 0)
		{
			SG_ERR_CHECK(  _format_primary(pCtx, pData, pszLabel_j, pvhItem)  );
			SG_ERR_CHECK(  _format_primary_trailer(pCtx, pData, pvhItem,
												   aSections[pData->kSection].bReportWasPath)  );
		}
	}

fail:
	return;
}


/**
 * We get called once for (section k, dirty item x).
 * Item x contains an array of section labels that it
 * should appear under.
 * 
 */
static SG_varray_foreach_callback _do_section_item_cb;

static void _do_section_item_cb(SG_context * pCtx,
								void * pVoidData,
								const SG_varray * pva,
								SG_uint32 ndx,
								const SG_variant * pv)
{
	struct _format_data * pData = (struct _format_data *)pVoidData;
	const SG_vhash * pvhItem;

	SG_UNUSED( pva );
	SG_UNUSED( ndx );

	if (aSections[pData->kSection].bOnlyIfVerbose && !pData->bVerbose)
		return;

	SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, (/*const*/SG_vhash **)&pvhItem)  );
	SG_ERR_CHECK(  _do_section_item_headings(pCtx, pData, pvhItem)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * For each section k in { "Added", "Modified", ... }:
 *    For each dirty item:
 *        if it has a matching section label,
 *            print it.
 *
 */
static void _do_section(SG_context * pCtx,
						struct _format_data * pData,
						const SG_varray * pvaStatus)
{
	SG_ERR_CHECK_RETURN(  SG_varray__foreach(pCtx, pvaStatus, _do_section_item_cb, pData)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Generate "classic" (grouped by section) status output (suitable
 * for the console) from the canonical status info.
 *
 * The canonical status info may have more info than we actually
 * can print (such as rows for UNCHANGED items).
 *
 * This version lets you specify a line-prefix and the EOL char
 * (suitable for building the editor buffer for the changeset
 * comment).
 * 
 */
void SG_wc__status__classic_format2(SG_context * pCtx,
									const SG_varray * pvaStatus,
									SG_bool bVerbose,
									const char * pszLinePrefix,
									const char * pszLineEOL,
									SG_string ** ppStringResult)
{
	struct _format_data data;
	
	SG_NULLARGCHECK_RETURN( pvaStatus );	
	SG_NULLARGCHECK_RETURN( ppStringResult );

	memset(&data, 0, sizeof(data));
	data.bVerbose = bVerbose;
	data.pszLinePrefix = pszLinePrefix;
	data.pszLineEOL = pszLineEOL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &data.pString)  );

	// These nested loops look backwards.  We want to loop on
	// the sections and then on the items, so that we get all
	// of the ADDS reported before all of the MODIFIED items
	// and so on.

	for (data.kSection=0; data.kSection < SG_NrElements(aSections); data.kSection++)
		SG_ERR_CHECK(  _do_section(pCtx, &data, pvaStatus)  );

	*ppStringResult = data.pString;
	return;

fail:
	SG_STRING_NULLFREE(pCtx, data.pString);
}

//////////////////////////////////////////////////////////////////

/**
 * Generate "classic" status output (suitable for the console)
 * from the canonical status info.
 *
 * The canonical status info may have more info than we actually
 * can print (such as rows for UNCHANGED items).
 *
 */
void SG_wc__status__classic_format(SG_context * pCtx,
								   const SG_varray * pvaStatus,
								   SG_bool bVerbose,
								   SG_string ** ppStringResult)
{
	SG_ERR_CHECK_RETURN(  SG_wc__status__classic_format2(pCtx,
														 pvaStatus,
														 bVerbose,
														 "", SG_LF_STR,
														 ppStringResult)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Generate "classic" (grouped by section) status output (suitable
 * for the console) from the canonical status info FOR A SINGLE ITEM.
 *
 * This version lets you specify a line-prefix and the EOL char
 * (suitable for building the editor buffer for the changeset
 * comment).
 *
 * WE ASSUME THAT THE GIVEN "pvhItem" VHASH WAS CONSTRUCTED BY STATUS
 * (meaning it should be a row within a "pvaStatus").
 * 
 */
void SG_wc__status__classic_format2__item(SG_context * pCtx,
										  const SG_vhash * pvhItem,
										  const char * pszLinePrefix,
										  const char * pszLineEOL,
										  SG_string ** ppStringResult)
{
	struct _format_data data;

	SG_NULLARGCHECK_RETURN( pvhItem );	
	SG_NULLARGCHECK_RETURN( ppStringResult );

	memset(&data, 0, sizeof(data));
	data.bVerbose = SG_FALSE;
	data.pszLinePrefix = pszLinePrefix;
	data.pszLineEOL = pszLineEOL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &data.pString)  );

	for (data.kSection=0; data.kSection < SG_NrElements(aSections); data.kSection++)
		if (!aSections[data.kSection].bOnlyIfVerbose)
			SG_ERR_CHECK(  _do_section_item_headings(pCtx, &data, pvhItem)  );

	*ppStringResult = data.pString;
	return;

fail:
	SG_STRING_NULLFREE(pCtx, data.pString);
}
