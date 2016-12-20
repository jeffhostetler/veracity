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

#ifndef VV_REV_SPEC_H
#define VV_REV_SPEC_H

#include "precompiled.h"


/**
 * A simple C++ wrapper around sglib's SG_rev_spec.
 *
 * Most constructors alloc the underlying SG_rev_spec.
 * Destructor does NOT free it, because doing so requires a context.
 * Callers must explicitly use Nullfree to free it.
 * Therefore, treat instances of this like pointers: explicitly document
 * ownership transfers when copying/passing/returning.
 */
class vvRevSpec
{
// types
public:
	/**
	 * Forward declaration of SG_rev_spec.
	 */
	typedef struct _sg_rev_spec_is_opaque sgRevSpec;

// construction
public:
	/**
	 * Creates an unallocated/NULL spec.
	 */
	vvRevSpec();

	/**
	 * Wraps a given already allocated spec.
	 */
	vvRevSpec(
		sgRevSpec* pRevSpec //< [in] The revspec to wrap.
		);

	/**
	 * Constructor that also allocates a blank spec.
	 */
	vvRevSpec(
		class vvContext& cContext //< [in] [out] Error and context info.
		);

	/**
	 * Constructor that also allocates a copy of a given spec.
	 */
	vvRevSpec(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const vvRevSpec& cThat     //< [in] The spec to allocate a copy of.
		);

	/**
	 * Constructor that also allocates a spec containing the given revisions, tags, and branches.
	 */
	vvRevSpec(
		class vvContext&     cContext,   //< [in] [out] Error and context info.
		const wxArrayString* pRevisions, //< [in] The revisions to add to the newly allocated spec.
		                                 //<      If NULL, none are added.
		const wxArrayString* pTags,      //< [in] The tags to add to the newly allocated spec.
		                                 //<      If NULL, none are added.
		const wxArrayString* pBranches   //< [in] The branches to add to the newly allocated spec.
		                                 //<      If NULL, none are added.
		);

// allocation
public:
	/**
	 * Allocates a blank spec.
	 */
	bool Alloc(
		class vvContext& cContext //< [in] [out] Error and context info.
		);

	/**
	 * Allocates a copy of an existing spec.
	 */
	bool Alloc(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const vvRevSpec& cThat     //< [in] The spec to allocate a copy of.
		);

	/**
	 * Allocates a spec containing the given revisions, tags, and branches.
	 */
	bool Alloc(
		class vvContext&     cContext,   //< [in] [out] Error and context info.
		const wxArrayString* pRevisions, //< [in] The revisions to add to the newly allocated spec.
		                                 //<      If NULL, none are added.
		const wxArrayString* pTags,      //< [in] The tags to add to the newly allocated spec.
		                                 //<      If NULL, none are added.
		const wxArrayString* pBranches   //< [in] The branches to add to the newly allocated spec.
		                                 //<      If NULL, none are added.
		);

	/**
	 * Wrapper around SG_REV_SPEC_NULLFREE.
	 */
	void Nullfree(
		class vvContext& cContext //< [in] [out] Error and context info.
		);

// compatibility
public:
	/**
	 * Implicit cast to the underlying revspec type.
	 */
	operator sgRevSpec*() const;

	/**
	 * Implicit dereference to the underlying context type.
	 */
	sgRevSpec* operator->() const;

// functionality
public:
	/**
	 * Returns true if the spec is allocated, or false if it is NULL.
	 * Opposite of IsNull.
	 */
	bool IsAllocated() const;

	/**
	 * Returns true if the spec is NULL, or false if it is allocated.
	 * Opposite of IsAllocated.
	 */
	bool IsNull() const;

	/**
	 * ================================================================================== *
	 * The UpdateBranch methods are not mirrored in the sg_rev_spec.  During update or merge,
	 * if a branch needs to be disambiguated, the revspec dialog will prompt to the user to pick
	 * which head to update to.  Use the SetUpdateBranch and GetUpdateBranch methods to remember
	 * the branch they selected.
	 */
	void SetUpdateBranch(wxString& sBranch);

	/**
	 * The UpdateBranch methods are not mirrored in the sg_rev_spec.  Read the comment for SetUpdateBranch.
	 * ================================================================================== *
	 */
	wxString GetUpdateBranch() const;

	/**
	 * Wrapper around SG_rev_spec__add_rev.
	 */
	bool AddRevision(
		class vvContext& cContext,
		const wxString&  sRevision
		);

	/**
	 * Wrapper around SG_rev_spec__add_tag.
	 */
	bool AddTag(
		class vvContext& cContext,
		const wxString&  sTag
		);

	/**
	 * Wrapper around SG_rev_spec__add_branch.
	 */
	bool AddBranch(
		class vvContext& cContext,
		const wxString&  sBranch
		);

	/**
	 * Wrapper around SG_rev_spec__count_revs.
	 */
	bool GetRevisionCount(
		class vvContext& cContext,
		unsigned int&    uCount
		) const;

	/**
	 * Wrapper around SG_rev_spec__count_tags.
	 */
	bool GetTagCount(
		class vvContext& cContext,
		unsigned int&    uCount
		) const;

	/**
	 * Wrapper around SG_rev_spec__count_branches.
	 */
	bool GetBranchCount(
		class vvContext& cContext,
		unsigned int&    uCount
		) const;

	/**
	 * Helper function that returns the sum of all counts returned by
	 * GetRevisionCount, GetTagCount, and GetBranchCount.
	 */
	bool GetTotalCount(
		class vvContext& cContext,
		unsigned int&    uCount
		) const;

	/**
	 * Wrapper around SG_rev_spec__revs.
	 */
	bool GetRevisions(
		class vvContext& cContext,
		wxArrayString&   cRevisions
		) const;

	/**
	 * Wrapper around SG_rev_spec__tags.
	 */
	bool GetTags(
		class vvContext& cContext,
		wxArrayString&   cTags
		) const;

	/**
	 * Wrapper around SG_rev_spec__branches.
	 */
	bool GetBranches(
		class vvContext& cContext,
		wxArrayString&   cBranches
		) const;

	/*
	 * SG_rev_spec has a fair bit more functionality that could be added here,
	 * but I don't think Tortoise will require it.  More can be added as needed.
	 */

// private data
private:
	sgRevSpec* mpRevSpec; //< The SG_rev_spec* that we're wrapping.
	wxString m_sUpdateBranch;
};

#endif
