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
#include "vvRevSpec.h"

#include "sg.h"
#include "vvContext.h"
#include "vvSgHelpers.h"


/*
**
** Public Functions
**
*/

vvRevSpec::vvRevSpec()
	: mpRevSpec(NULL)
{
}

vvRevSpec::vvRevSpec(
	sgRevSpec* pRevSpec
	)
	: mpRevSpec(pRevSpec)
{
}

vvRevSpec::vvRevSpec(
	vvContext& cContext
	)
	: mpRevSpec(NULL)
{
	this->Alloc(cContext);
}

vvRevSpec::vvRevSpec(
	vvContext&       cContext,
	const vvRevSpec& cThat
	)
	: mpRevSpec(NULL)
{
	this->Alloc(cContext, cThat);
}

vvRevSpec::vvRevSpec(
	vvContext&           cContext,
	const wxArrayString* pRevisions,
	const wxArrayString* pTags,
	const wxArrayString* pBranches
	)
	: mpRevSpec(NULL)
{
	this->Alloc(cContext, pRevisions, pTags, pBranches);
}

bool vvRevSpec::Alloc(
	vvContext& pCtx
	)
{
	this->Nullfree(pCtx);
	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &(this->mpRevSpec))  );
	
fail:
	return !pCtx.Error_Check();
}

bool vvRevSpec::Alloc(
	vvContext&       pCtx,
	const vvRevSpec& cThat
	)
{
	this->Nullfree(pCtx);
	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC__COPY(pCtx, cThat, &(this->mpRevSpec))  );

fail:
	return !pCtx.Error_Check();
}

bool vvRevSpec::Alloc(
	vvContext&           cContext,
	const wxArrayString* pRevisions,
	const wxArrayString* pTags,
	const wxArrayString* pBranches
	)
{
	if (!this->Alloc(cContext))
	{
		return false;
	}

	if (pRevisions != NULL)
	{
		for (wxArrayString::const_iterator it = pRevisions->begin(); it != pRevisions->end(); ++it)
		{
			if (!this->AddRevision(cContext, *it))
			{
				return false;
			}
		}
	}

	if (pTags != NULL)
	{
		for (wxArrayString::const_iterator it = pTags->begin(); it != pTags->end(); ++it)
		{
			if (!this->AddTag(cContext, *it))
			{
				return false;
			}
		}
	}

	if (pBranches != NULL)
	{
		for (wxArrayString::const_iterator it = pBranches->begin(); it != pBranches->end(); ++it)
		{
			if (!this->AddBranch(cContext, *it))
			{
				return false;
			}
		}
	}

	return true;
}

void vvRevSpec::Nullfree(
	vvContext& pCtx
	)
{
	SG_REV_SPEC_NULLFREE(pCtx, this->mpRevSpec);
	this->mpRevSpec = NULL;
}

vvRevSpec::operator vvRevSpec::sgRevSpec*() const
{
	return this->mpRevSpec;
}

vvRevSpec::sgRevSpec* vvRevSpec::operator->() const
{
	return this->mpRevSpec;
}

bool vvRevSpec::IsAllocated() const
{
	return this->mpRevSpec != NULL;
}

bool vvRevSpec::IsNull() const
{
	return this->mpRevSpec == NULL;
}

void vvRevSpec::SetUpdateBranch(wxString& sBranch)
{
	m_sUpdateBranch = sBranch;
}

wxString vvRevSpec::GetUpdateBranch() const
{
	return m_sUpdateBranch;
}

bool vvRevSpec::AddRevision(
	vvContext&      pCtx,
	const wxString& sRevision
	)
{
	SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, this->mpRevSpec, vvSgHelpers::Convert_wxString_sglibString(sRevision))  );

fail:
	return !pCtx.Error_Check();
}

bool vvRevSpec::AddTag(
	vvContext&      pCtx,
	const wxString& sTag
	)
{
	SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, this->mpRevSpec, vvSgHelpers::Convert_wxString_sglibString(sTag))  );

fail:
	return !pCtx.Error_Check();
}

bool vvRevSpec::AddBranch(
	vvContext&      pCtx,
	const wxString& sBranch
	)
{
	SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, this->mpRevSpec, vvSgHelpers::Convert_wxString_sglibString(sBranch))  );
	m_sUpdateBranch = sBranch;
fail:
	return !pCtx.Error_Check();
}

bool vvRevSpec::GetRevisionCount(
	vvContext&    pCtx,
	unsigned int& uCount
	) const
{
	SG_uint32 sgCount = 0u;

	SG_ERR_CHECK(  SG_rev_spec__count_revs(pCtx, this->mpRevSpec, &sgCount)  );
	uCount = sgCount;

fail:
	return !pCtx.Error_Check();
}

bool vvRevSpec::GetTagCount(
	vvContext&    pCtx,
	unsigned int& uCount
	) const
{
	SG_uint32 sgCount = 0u;

	SG_ERR_CHECK(  SG_rev_spec__count_tags(pCtx, this->mpRevSpec, &sgCount)  );
	uCount = sgCount;

fail:
	return !pCtx.Error_Check();
}

bool vvRevSpec::GetBranchCount(
	vvContext&    pCtx,
	unsigned int& uCount
	) const
{
	SG_uint32 sgCount = 0u;

	SG_ERR_CHECK(  SG_rev_spec__count_branches(pCtx, this->mpRevSpec, &sgCount)  );
	uCount = sgCount;

fail:
	return !pCtx.Error_Check();
}

bool vvRevSpec::GetTotalCount(
	vvContext&    pCtx,
	unsigned int& uCount
	) const
{
	unsigned int uRevisions = 0u;
	unsigned int uTags      = 0u;
	unsigned int uBranches  = 0u;

	if (!this->GetRevisionCount(pCtx, uRevisions))
	{
		return false;
	}
	if (!this->GetTagCount(pCtx, uTags))
	{
		return false;
	}
	if (!this->GetBranchCount(pCtx, uBranches))
	{
		return false;
	}

	uCount = uRevisions + uTags + uBranches;
	return true;
}

bool vvRevSpec::GetRevisions(
	vvContext&     pCtx,
	wxArrayString& cRevisions
	) const
{
	SG_stringarray* pValues = NULL;

	SG_ERR_CHECK(  SG_rev_spec__revs(pCtx, this->mpRevSpec, &pValues)  );
	SG_ERR_CHECK(  vvSgHelpers::Convert_sgStringArray_wxArrayString(pCtx, pValues, cRevisions)  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, pValues);
	return !pCtx.Error_Check();
}

bool vvRevSpec::GetTags(
	vvContext&     pCtx,
	wxArrayString& cTags
	) const
{
	SG_stringarray* pValues = NULL;

	SG_ERR_CHECK(  SG_rev_spec__tags(pCtx, this->mpRevSpec, &pValues)  );
	SG_ERR_CHECK(  vvSgHelpers::Convert_sgStringArray_wxArrayString(pCtx, pValues, cTags)  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, pValues);
	return !pCtx.Error_Check();
}

bool vvRevSpec::GetBranches(
	vvContext&     pCtx,
	wxArrayString& cBranches
	) const
{
	SG_stringarray* pValues = NULL;

	SG_ERR_CHECK(  SG_rev_spec__branches(pCtx, this->mpRevSpec, &pValues)  );
	if (pValues)
		SG_ERR_CHECK(  vvSgHelpers::Convert_sgStringArray_wxArrayString(pCtx, pValues, cBranches)  );

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, pValues);
	return !pCtx.Error_Check();
}
