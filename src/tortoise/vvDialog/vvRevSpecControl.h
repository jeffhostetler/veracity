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

#ifndef VV_REV_SPEC_CONTROL_H
#define VV_REV_SPEC_CONTROL_H

#include "precompiled.h"
#include "vvRevSpec.h"


/**
 * A control for manipulating/building/choosing a vvRevSpec.
 *
 * Currently the control only supports choosing a single revision, but that
 * revision can be specified by a single branch, tag, or HID.
 */
class vvRevSpecControl : public wxPanel
{
// types
public:
	/**
	 * The possible methods of specifying a revision using the control.
	 */
	enum Type
	{
		TYPE_BRANCH,   //< The revision is specified by the single head of a branch.
		TYPE_TAG,      //< The revision is specified by a tag.
		TYPE_REVISION, //< The revision is specified directly by its HID.
		TYPE_COUNT     //< Number of values in this enum.
	};

	enum BranchAmbiguationState
	{
		BRANCH__STILL_AMBIGUOUS,   //< The user cancelled the disambiguation dialog.
		BRANCH__NO_LONGER_AMBIGUOUS,      //< The user picked the branch head to use.
		BRANCH__WAS_NOT_AMBIGUOUS, //< The branch did not need to be disambiguated
		BRANCH__COUNT     //< Number of values in this enum.
	};
// construction
public:
	/**
	 * Default constructor.
	 */
	vvRevSpecControl();

	/**
	 * Create constructor.
	 */
	vvRevSpecControl(
		wxWindow*        pParent,                   //< [in] The control's parent window.
		const wxString&  sRepo     = wxEmptyString, //< [in] Name of a repo to allow the user to browse.
		                                            //<      If empty, browsing will be disabled.
		class vvVerbs*   pVerbs    = NULL,          //< [in] Verbs to use for browsing.
		                                            //<      If NULL, browsing will be disabled.
		class vvContext* pContext  = NULL,          //< [in] Context to use with pVerbs and cRevSpec.
		                                            //<      If NULL, browsing will be disabled.
		                                            //<      If NULL, cRevSpec must be unallocated/NULL.
		const vvRevSpec& cRevSpec  = vvRevSpec()    //< [in] The control's initial value.
		                                            //<      If allocated, pContext must be non-NULL.
		                                            //<      The control does NOT take ownership.
		);

// functionality
public:
	/**
	 * Creates the control empty.
	 */
	virtual bool Create(
		wxWindow*        pParent,                   //< [in] The control's parent window.
		const wxString&  sRepo     = wxEmptyString, //< [in] Name of a repo to allow the user to browse.
		                                            //<      If empty, browsing will be disabled.
		class vvVerbs*   pVerbs    = NULL,          //< [in] Verbs to use for browsing.
		                                            //<      If NULL, browsing will be disabled.
		class vvContext* pContext  = NULL,          //< [in] Context to use with pVerbs and cRevSpec.
		                                            //<      If NULL, browsing will be disabled.
		                                            //<      If NULL, cRevSpec must be unallocated/NULL.
		const vvRevSpec& cRevSpec  = vvRevSpec()    //< [in] The control's initial value.
		                                            //<      If allocated, pContext must be non-NULL.
		                                            //<      The control does NOT take ownership.
		);

	/**
	 * Changes the repo that the control allows the user to browse from.
	 */
	void SetRepo(
		const wxString&  sRepo,   //< The repo to use.
		                          //< Pass wxEmptyString to disable browsing.
		class vvVerbs*   cVerbs,  //< Verbs to use to browse.
		                          //< Pass NULL to disable browsing.
		class vvContext* cContext //< The context to use with cVerbs.
		                          //< Pass NULL to disable browsing.
		);

	/**
	 * Sets a vvRevSpec as the control's current selection.
	 * The spec should contain a single branch, tag, or revision in it.
	 * If it has one of several of those, branches are preferred over tags, which are preferred over revisions.
	 * Returns true if the current selection was set successfully.
	 * Returns false if there was an error or if the spec did not contain a single item of any type.
	 */
	bool SetRevSpec(
		const class vvRevSpec& cRevSpec, //< [in] The vvRevSpec to display.
		class vvContext&       cContext  //< [in] The context to use with the rev spec.
		);

	/**
	 * Directly sets the control's current selection.
	 */
	void SetValue(
		Type            eType, //< [in] The type of input to set the control to.
		const wxString& sValue //< [in] The value to set for the given type of input.
		);

	/**
	 * Gets the RevSpec as currently set in the control.
	 * Caller owns the spec and must call Nullfree on it.
	 */
	vvRevSpec GetRevSpec(
		class vvContext& cContext //< [in] The context to create the rev spec with.
		) const;

	/**
	 * Gets the type of value currently selected.
	 */
	Type GetValueType() const;

	/**
	 * Gets the currently selected value.
	 */
	wxString GetValue() const;

	/**
	 * Checks to see if a branch has multiple heads, and prompts
	 * the user to pick one.
	 */
	vvRevSpecControl::BranchAmbiguationState DisambiguateBranchIfNecessary(bool bMerging, wxArrayString arBaselineHIDs, vvRevSpec& cReturnSpec);


// internal functionality
protected:
	/**
	 * Update the visible/enabled state of all controls based on current values.
	 */
	void UpdateDisplay();

// event handlers
protected:
	void OnControlValueChange(wxCommandEvent& cEvent);
	void OnBrowseBranches(wxCommandEvent& cEvent);
	void OnBrowseTags(wxCommandEvent& cEvent);
	void OnBrowseRevisions(wxCommandEvent& cEvent);

// private data
private:
	// repo data
	wxString   msRepo;    //< The repo to browse, or wxEmptyString to disable it.
	vvVerbs*   mpVerbs;   //< Verbs to use for browsing, or NULL if browsing is disabled.
	vvContext* mpContext; //< Context to use with mpVerbs, or NULL if browsing is disabled.

	// widgets
	wxRadioButton* mwBranch_Select;
	wxTextCtrl*    mwBranch_Value;
	wxButton*      mwBranch_Browse;
	wxRadioButton* mwTag_Select;
	wxTextCtrl*    mwTag_Value;
	wxButton*      mwTag_Browse;
	wxRadioButton* mwRevision_Select;
	wxTextCtrl*    mwRevision_Value;
	wxButton*      mwRevision_Browse;

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvRevSpecControl);
	DECLARE_EVENT_TABLE();
};


#endif
