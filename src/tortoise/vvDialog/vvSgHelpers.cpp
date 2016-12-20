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
#include "vvSgHelpers.h"

#include "sg.h"
#include "vvContext.h"


/*
**
** Public Functions
**
*/

bool vvSgHelpers::Convert_sgBool_cppBool(
	int bValue
	)
{
	wxCOMPILE_TIME_ASSERT(sizeof(bValue) == sizeof(SG_bool), SgBoolSizeMismatch);
	wxASSERT(bValue == SG_TRUE || bValue == SG_FALSE);
	return bValue == SG_FALSE ? false : true;
}

int vvSgHelpers::Convert_cppBool_sgBool(
	bool bValue
	)
{
	wxCOMPILE_TIME_ASSERT(sizeof(int) == sizeof(SG_bool), SgBoolSizeMismatch);
	return bValue ? SG_TRUE : SG_FALSE;
}

wxString vvSgHelpers::Convert_sgString_wxString(
	const SG_string* sValue
	)
{
	return vvSgHelpers::Convert_sglibString_wxString(SG_string__sz(sValue));
}

wxString vvSgHelpers::Convert_sgString_wxString(
	const SG_string* sValue,
	const wxString&  sNullValue
	)
{
	if (sValue == NULL)
	{
		return sNullValue;
	}
	else
	{
		return vvSgHelpers::Convert_sgString_wxString(sValue);
	}
}

wxString vvSgHelpers::Convert_sglibString_wxString(
	const char* szValue
	)
{
	// create a wxString converted from sglib's UTF8 data
	wxString sValue = wxString::FromUTF8(szValue);

	// wx seems to expect strings in memory to use a platform-neutral newline.
	// It is converted to a platform-specific one as necessary (like when written to disk).
	sValue.Replace(SG_PLATFORM_NATIVE_EOL_STR, "\n", true);

	return sValue;
}

wxString vvSgHelpers::Convert_sglibString_wxString(
	const char*     szValue,
	const wxString& sNullValue
	)
{
	if (szValue == NULL)
	{
		return sNullValue;
	}
	else
	{
		return vvSgHelpers::Convert_sglibString_wxString(szValue);
	}
}

wxString vvSgHelpers::Convert_sgPathname_wxString(
	const SG_pathname* sValue
	)
{
	// run it through wxFileName to normalize the path to the current platform
	return wxFileName(vvSgHelpers::Convert_sglibString_wxString(SG_pathname__sz(sValue))).GetFullPath();
}

wxString vvSgHelpers::Convert_sgPathname_wxString(
	const SG_pathname* sValue,
	const wxString&    sNullValue
	)
{
	if (sValue == NULL)
	{
		return sNullValue;
	}
	else
	{
		return vvSgHelpers::Convert_sgPathname_wxString(sValue);
	}
}

wxScopedCharBuffer vvSgHelpers::Convert_wxString_sglibString(
	const wxString& sValue,
	bool            bEmptyToNull
	)
{
	if (bEmptyToNull && sValue.IsEmpty())
	{
		return wxScopedCharBuffer();
	}
	else
	{
		// convert the string to UTF8 and return that buffer
		return sValue.utf8_str();
	}
}

wxScopedCharBuffer vvSgHelpers::Convert_wxString_sglibString(
	const wxString* pValue,
	bool            bEmptyToNull
	)
{
	if (pValue == NULL)
	{
		return wxScopedCharBuffer();
	}
	else
	{
		return vvSgHelpers::Convert_wxString_sglibString(*pValue, bEmptyToNull);
	}
}

char* vvSgHelpers::Convert_wxString_sglibString(
	vvContext&      pCtx,
	const wxString& sValue,
	bool            bEmptyToNull
	)
{
	wxScopedCharBuffer cBuffer = vvSgHelpers::Convert_wxString_sglibString(sValue, bEmptyToNull);

	char* szValue = NULL;

	if (cBuffer.data() != NULL)
	{
		SG_ERR_CHECK(  SG_strdup(pCtx, cBuffer.data(), &szValue)  );
	}

fail:
	return szValue;
}

char* vvSgHelpers::Convert_wxString_sglibString(
	vvContext&      pCtx,
	const wxString* pValue,
	bool            bEmptyToNull
	)
{
	if (pValue == NULL)
	{
		return NULL;
	}
	else
	{
		return vvSgHelpers::Convert_wxString_sglibString(pCtx, *pValue, bEmptyToNull);
	}
}

SG_pathname* vvSgHelpers::Convert_wxString_sgPathname(
	vvContext&      pCtx,
	const wxString& sValue,
	bool            bEmptyToNull
	)
{
	SG_pathname* pValue = NULL;

	if (!bEmptyToNull || !sValue.IsEmpty())
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pValue, vvSgHelpers::Convert_wxString_sglibString(sValue))  );
	}

fail:
	return pValue;
}

SG_pathname* vvSgHelpers::Convert_wxArrayString_sgPathname(
	vvContext&      pCtx,
	const wxArrayString& sValueArray,
	bool            bEmptyToNull
	)
{
	wxString sValue;
	if (sValueArray.Count() > 0)
		sValue = sValueArray[0];
	
	return vvSgHelpers::Convert_wxString_sgPathname(pCtx, sValue, bEmptyToNull);
}

void vvSgHelpers::Convert_wxArrayString_sglibStringArrayCount(
	vvContext&           pCtx,
	const wxArrayString& cStrings,
	char***              pppStrings,
	wxUint32*            pCount
	)
{
	char** ppStrings = NULL;

	SG_ERR_CHECK(  SG_alloc(pCtx, sizeof(char*), cStrings.size(), &ppStrings)  );

	for (unsigned int uIndex = 0u; uIndex < cStrings.size(); ++uIndex)
	{
		SG_ERR_CHECK(  ppStrings[uIndex] = vvSgHelpers::Convert_wxString_sglibString(pCtx, cStrings[uIndex])  );
	}

	if (pppStrings != NULL)
	{
		*pppStrings = ppStrings;
		ppStrings = NULL;
	}

	if (pCount != NULL)
	{
		*pCount = cStrings.size();
	}

fail:
	SG_NULLFREE(pCtx, ppStrings);
	return;
}


void vvSgHelpers::Convert_listStampData_wxArrayString(
	vvContext& pCtx,
	vvVerbs::stlStampDataList stampData,
	wxArrayString& cStrings
	)
{
	pCtx;
	for (std::list<vvVerbs::StampData>::const_iterator it = stampData.begin(); it != stampData.end(); ++it)
	{
		cStrings.Add((*it).sName);
	}
	return;
}

void vvSgHelpers::Convert_listTagData_wxArrayString(
	vvContext& pCtx,
	vvVerbs::stlTagDataList tagData,
	wxArrayString& cStrings
	)
{
	pCtx;
	for (std::list<vvVerbs::TagData>::const_iterator it = tagData.begin(); it != tagData.end(); ++it)
	{
		cStrings.Add((*it).sName);
	}
	return;
}

void vvSgHelpers::Convert_rbTreeKeys_wxArrayString(
	vvContext& pCtx,
	SG_rbtree * prb,
	wxArrayString& cStrings
	)
{
	pCtx;
	SG_rbtree_iterator * pi = NULL;
	SG_bool bOK = SG_FALSE;
	const char * pszKey = NULL;
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pi, prb, &bOK, &pszKey, NULL)  );
	while(bOK)
	{
		cStrings.push_back(vvSgHelpers::Convert_sglibString_wxString(pszKey));
		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pi, &bOK, &pszKey, NULL)  );
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pi);
	return;
}

void vvSgHelpers::Convert_wxArrayString_sgStringArray(
	vvContext& pCtx,
	const wxArrayString& cStrings,
	SG_stringarray**     ppStrings
	)
{
	SG_stringarray* pStrings = NULL;

	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &pStrings, cStrings.size())  );
	for (wxArrayString::const_iterator it = cStrings.begin(); it != cStrings.end(); ++it)
	{
		SG_ERR_CHECK(  SG_stringarray__add(pCtx, pStrings, vvSgHelpers::Convert_wxString_sglibString(*it))  );
	}

	if (ppStrings != NULL)
	{
		*ppStrings = pStrings;
		pStrings = NULL;
	}

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, pStrings);
	return;
}

void vvSgHelpers::Convert_sgStringArray_wxArrayString(
	vvContext&            pCtx,
	const SG_stringarray* pStrings,
	wxArrayString&        cStrings
	)
{
	SG_uint32 uCount = 0u;
	SG_uint32 uIndex = 0u;

	SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStrings, &uCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		const char* szValue = NULL;

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pStrings, uIndex, &szValue)  );
		cStrings.push_back(vvSgHelpers::Convert_sglibString_wxString(szValue));
	}

fail:
	return;
}

void vvSgHelpers::Convert_wxArrayString_sgRBTree(
	vvContext& pCtx,
	const wxArrayString& cStrings,
	SG_rbtree**     pprbtree,
	bool bNullIsOK
	)
{
	SG_rbtree* prb = NULL;

	if (cStrings.Count() == 0 && bNullIsOK)
		return;
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb)  );
	for (wxArrayString::const_iterator it = cStrings.begin(); it != cStrings.end(); ++it)
	{
		SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb, vvSgHelpers::Convert_wxString_sglibString(*it), NULL)  );
	}

	if (pprbtree != NULL)
	{
		*pprbtree = prb;
		prb = NULL;
	}

fail:
	SG_RBTREE_NULLFREE(pCtx, prb);
	return;
}

void vvSgHelpers::Convert_wxArrayString_sgVhash(
	vvContext& pCtx,
	const wxArrayString& cStrings,
	SG_vhash**     ppvhash,
	bool bNullIsOK
	)
{
	SG_vhash* pvh = NULL;

	if (cStrings.Count() == 0 && bNullIsOK)
		return;
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
	for (wxArrayString::const_iterator it = cStrings.begin(); it != cStrings.end(); ++it)
	{
		SG_ERR_CHECK(  SG_vhash__update__null(pCtx, pvh, vvSgHelpers::Convert_wxString_sglibString(*it))  );
	}

	if (ppvhash != NULL)
	{
		*ppvhash = pvh;
		pvh = NULL;
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
	return;
}

void vvSgHelpers::Convert_wxArrayString_sgVarray(
	vvContext&           pCtx,
	const wxArrayString& cStrings,
	SG_varray**          ppVarray,
	bool                 bNullIfEmpty
	)
{
	SG_varray* pVarray = NULL;

	SG_NULLARGCHECK(ppVarray);

	if (cStrings.Count() > 0u || !bNullIfEmpty)
	{
		SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pVarray)  );
		for (wxArrayString::const_iterator it = cStrings.begin(); it != cStrings.end(); ++it)
		{
			SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pVarray, vvSgHelpers::Convert_wxString_sglibString(*it))  );
		}
	}

	*ppVarray = pVarray;
	pVarray   = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pVarray);
	return;
}

void vvSgHelpers::Convert_sgVarray_wxArrayString(
	vvContext& pCtx,
	const void*     pvarray,
	wxArrayString& cStrings
	)
{
	wxArrayString returnVal;
	SG_uint32 varrayCount = 0;
	const char * value = NULL;
	if (pvarray)
	{
		SG_ERR_CHECK(  SG_varray__count(pCtx, (const SG_varray *)pvarray, &varrayCount)  );
		for (SG_uint32 i = 0; i < varrayCount; i++)
		{
			SG_ERR_CHECK(  SG_varray__get__sz(pCtx, (const SG_varray *)pvarray, i, &value)  );
			returnVal.Add(vvSgHelpers::Convert_sglibString_wxString(value));
		}
	}
fail:
	cStrings = returnVal;
}


void vvSgHelpers::Convert_sgVarray_wxArrayInt(
	vvContext& pCtx,
	const void*     pvarray,
	wxArrayInt& cInts
	)
{
	wxArrayInt returnVal;
	SG_uint32 varrayCount = 0;
	SG_int64 value = 0;
	if (pvarray)
	{
		SG_ERR_CHECK(  SG_varray__count(pCtx, (const SG_varray *)pvarray, &varrayCount)  );
		for (SG_uint32 i = 0; i < varrayCount; i++)
		{
			SG_ERR_CHECK(  SG_varray__get__int64(pCtx, (const SG_varray *)pvarray, i, &value)  );
			returnVal.Add(value);
		}
	}
fail:
	cInts = returnVal;
}

void vvSgHelpers::Convert_wxArrayInt_sgVarray(
	vvContext& pCtx,
	const wxArrayInt& cInts,
	void**     ppvarray,
	bool bNullIsOK
	)
{
	SG_varray* pva = NULL;

	if (cInts.Count() == 0 && bNullIsOK)
		return;
	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );
	for (wxArrayInt::const_iterator it = cInts.begin(); it != cInts.end(); ++it)
	{
		SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pva, *it)  );
	}

	if (ppvarray != NULL)
	{
		*ppvarray = (void*)pva;
		pva = NULL;
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
	return;
}

wxDateTime vvSgHelpers::Convert_sgTime_wxDateTime(
	wxInt64 iValue
	)
{
	wxCOMPILE_TIME_ASSERT(sizeof(iValue) == sizeof(SG_int64), SgTimeSizeMismatch);
	wxDateTime cWxTime;
	cWxTime.Set(iValue / 1000u);
	cWxTime.SetMillisecond(iValue % 1000u);
	wxASSERT(cWxTime.IsValid());
	return cWxTime;
}

wxInt64 vvSgHelpers::Convert_wxDateTime_sgTime(
	const wxDateTime& cValue
	)
{
	wxCOMPILE_TIME_ASSERT(sizeof(wxInt64) == sizeof(SG_int64), SgTimeSizeMismatch2);
	wxASSERT(cValue.IsValid());
	wxInt64 iValue = cValue.GetTicks() * 1000u;
	iValue += cValue.GetMillisecond(wxDateTime::UTC);
	return iValue;
}

wxInt64 vvSgHelpers::Convert_wxDateTime_sgTime(
	const wxDateTime& cValue,
	wxInt64           iInvalidValue
	)
{
	if (cValue.IsValid())
	{
		return vvSgHelpers::Convert_wxDateTime_sgTime(cValue);
	}
	else
	{
		return iInvalidValue;
	}
}

vvVerbs::HistoryData vvSgHelpers::Convert_sg_history_result_HistoryData(
	class vvContext& pCtx,
	SG_history_result * phr_result,
	SG_vhash * pvh_pile
	)
{
	vvVerbs::HistoryData cHistoryData;
	//print the information for each
	SG_bool bFound = (phr_result != NULL);
	const char* currentInfoItem = NULL;
	SG_uint32 revno;
	SG_uint32 nCount = 0;
	SG_uint32 nIndex = 0;
	const char * pszTag = NULL;
	const char * pszComment = NULL;
	const char * pszStamp = NULL;
	const char * pszParent = NULL;
	SG_uint32 nResultCount = 0;
	SG_vhash* pvhRefBranchValues = NULL;
	SG_vhash* pvhRefClosedBranches = NULL;
	SG_vhash* pvhRefBranchNames = NULL;

	if (pvh_pile)
	{
		SG_bool bHas = SG_FALSE;
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_pile, "closed", &bHas)  );
		if (bHas)
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "closed", &pvhRefClosedBranches)  );

		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh_pile, "values", &pvhRefBranchValues)  );
	}

	if (phr_result == NULL)
		return cHistoryData;
	SG_ERR_CHECK(  SG_history_result__count(pCtx, phr_result, &nResultCount)  );
	while (nResultCount != 0 && bFound)
	{
		vvVerbs::HistoryEntry he;
		he.bIsMissing = false;

		SG_ERR_CHECK(  SG_history_result__get_revno(pCtx, phr_result, &revno)  );
		SG_ERR_CHECK(  SG_history_result__get_cshid(pCtx, phr_result, &currentInfoItem)  );
		he.nRevno = revno;
		he.sChangesetHID = vvSgHelpers::Convert_sglibString_wxString(currentInfoItem);
		pvhRefBranchNames = NULL;
		if (pvhRefBranchValues)
		{
			SG_bool b_has = SG_FALSE;

			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefBranchValues, currentInfoItem, &b_has)  );
			if (b_has)
			{
				SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhRefBranchValues, currentInfoItem, &pvhRefBranchNames)  );
			}
		}

		if (pvhRefBranchNames)
		{
			SG_uint32 count_branch_names = 0;
			SG_uint32 i;
			SG_bool bShowOnlyOpenBranchNames = SG_FALSE;

			SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhRefBranchNames, &count_branch_names)  );
			for (i=0; i<count_branch_names; i++)
			{
				const char* psz_branch_name = NULL;
				SG_bool bClosed = SG_FALSE;

				SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhRefBranchNames, i, &psz_branch_name, NULL)  );

				if (pvhRefClosedBranches)
					SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhRefClosedBranches, psz_branch_name, &bClosed)  );

				if ( !bShowOnlyOpenBranchNames || (bShowOnlyOpenBranchNames && !bClosed) )
				{
						if (bClosed)
							he.cBranches.Add(wxString::Format("(%s)", vvSgHelpers::Convert_sglibString_wxString(psz_branch_name)));
						else
							he.cBranches.Add(vvSgHelpers::Convert_sglibString_wxString(psz_branch_name));
				}
			}
		}

		SG_ERR_CHECK(  SG_history_result__get_audit__count(pCtx, phr_result, &nCount)  );
		for (nIndex = 0; nIndex < nCount; nIndex++)
		{
			SG_int64 itime = -1;
			const char * pszUser = NULL;
			vvVerbs::AuditEntry ae;

			SG_ERR_CHECK(  SG_history_result__get_audit__who(pCtx, phr_result, nIndex, &pszUser)  );
			SG_ERR_CHECK(  SG_history_result__get_audit__when(pCtx, phr_result, nIndex, &itime)  );
			ae.sWho = vvSgHelpers::Convert_sglibString_wxString(pszUser);
			ae.nWhen = vvSgHelpers::Convert_sgTime_wxDateTime(itime);
			he.cAudits.push_back(ae);
		}

		SG_ERR_CHECK(  SG_history_result__get_tag__count(pCtx, phr_result, &nCount)  );
		for (nIndex = 0; nIndex < nCount; nIndex++)
		{
			SG_ERR_CHECK(  SG_history_result__get_tag__text(pCtx, phr_result, nIndex, &pszTag)  );
			he.cTags.Add(vvSgHelpers::Convert_sglibString_wxString(pszTag));
		}

		SG_ERR_CHECK(  SG_history_result__get_comment__count(pCtx, phr_result, &nCount)  );
		for (nIndex = 0; nIndex < nCount; nIndex++)
		{
			SG_int64 itime = -1;
			const char * pszUser = NULL;
			vvVerbs::AuditEntry ae;
			vvVerbs::CommentEntry ce;
			SG_uint32 nHistoryCount = 0;
			SG_uint32 nCommentHistoryIndex = 0;
			SG_uint32 nCommentAuditCount = 0;
			SG_uint32 nCommentAuditIndex = 0;
			SG_varray * pvaCommentHistory = NULL;
			SG_vhash * pvhThisCommentHistoryItem = NULL;
			SG_varray * pvaCommentAudits = NULL;
			SG_vhash * pvhThisCommentHistoryAudit = NULL;

			SG_ERR_CHECK(  SG_history_result__get_comment__text(pCtx, phr_result, nIndex, &pszComment)  );
			SG_ERR_CHECK(  SG_history_result__get_comment__history__ref(pCtx, phr_result, nIndex, &pvaCommentHistory)  );

			SG_ERR_CHECK(  SG_varray__count(pCtx, pvaCommentHistory, &nHistoryCount)  );
			for (nCommentHistoryIndex = 0; nCommentHistoryIndex < nHistoryCount; nCommentHistoryIndex++)
			{
				SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaCommentHistory, nCommentHistoryIndex, &pvhThisCommentHistoryItem)  );
				SG_ERR_CHECK(  SG_vhash__get__varray(pCtx, pvhThisCommentHistoryItem, "audits", &pvaCommentAudits)  );
				SG_ERR_CHECK(  SG_varray__count(pCtx, pvaCommentAudits, &nCommentAuditCount)  );
				for (nCommentAuditIndex = 0; nCommentAuditIndex < nCommentAuditCount; nCommentAuditIndex++)
				{
					SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaCommentAudits, nCommentAuditIndex, &pvhThisCommentHistoryAudit)  );
					SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhThisCommentHistoryAudit, "username", &pszUser)  );
					SG_ERR_CHECK(  SG_vhash__get__int64_or_double(pCtx, pvhThisCommentHistoryAudit, "timestamp", &itime)  );
					ae.sWho = vvSgHelpers::Convert_sglibString_wxString(pszUser);
					ae.nWhen = vvSgHelpers::Convert_sgTime_wxDateTime(itime);
					ce.cAudits.push_back(ae);
				}
			}
			ce.sCommentText = vvSgHelpers::Convert_sglibString_wxString(pszComment);
			he.cComments.push_back(ce);
		}

		SG_ERR_CHECK(  SG_history_result__get_stamp__count(pCtx, phr_result, &nCount)  );
		for (nIndex = 0; nIndex < nCount; nIndex++)
		{
			SG_ERR_CHECK(  SG_history_result__get_stamp__text(pCtx, phr_result, nIndex, &pszStamp)  );
			he.cStamps.Add(vvSgHelpers::Convert_sglibString_wxString(pszStamp));
		}

		SG_ERR_CHECK(  SG_history_result__get_parent__count(pCtx, phr_result, &nCount)  );
		for (nIndex = 0; nIndex < nCount; nIndex++)
		{
			vvVerbs::RevisionEntry pe;
			SG_ERR_CHECK(  SG_history_result__get_parent(pCtx, phr_result, nIndex, &pszParent, &revno)  );
			pe.sChangesetHID = vvSgHelpers::Convert_sglibString_wxString(pszParent);
			pe.nRevno = revno;
			he.cParents.push_back(pe);
		}
		cHistoryData.push_back(he);
		SG_ERR_CHECK(  SG_history_result__next(pCtx, phr_result, &bFound)  );
	}
fail:
	return cHistoryData;
}

#if defined(DEBUG)

wxString vvSgHelpers::Dump_sgVhash(
	vvContext&      pCtx,
	const SG_vhash* pValue
	)
{
	wxString   sValue  = wxEmptyString;
	SG_string* pString = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	SG_ERR_CHECK(  SG_vhash_debug__dump(pCtx, pValue, pString)  );
	sValue = vvSgHelpers::Convert_sgString_wxString(pString);

fail:
	SG_STRING_NULLFREE(pCtx, pString);
	return sValue;
}

#endif
