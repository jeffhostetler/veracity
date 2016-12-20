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

#ifndef VV_BRANCH_CONTROL_H
#define VV_BRANCH_CONTROL_H

#include "precompiled.h"
#include "vvVerbs.h"

/**
 * A control for selecting the branch to attach to.
 *
 */
class vvBranchControl : public wxPanel
{
// types
public:
// construction
public:
	/**
	 * Default constructor.
	 */
	vvBranchControl();

	/**
	 * Create constructor.
	 */
	vvBranchControl(
		wxWindow*        pParent,                   //< [in] The control's parent window.
		const wxString&  sRepo     = wxEmptyString, 
		wxArrayString*	 pWFParents = NULL,			//< [in] The HIDs of the parents of the working copy
													//			Used to warn the user if they are increasing the head count.
													//			of a branch.  Use NULL to disable the "will this commit 
													//			increase the head count" check
		class vvVerbs*   pVerbs    = NULL,          //< [in] Verbs to use
		class vvContext* pContext  = NULL          //< [in] Context to use
		);

// functionality
public:
	/**
	 * Creates the control empty.
	 */
	virtual bool Create(
		wxWindow*        pParent,                   //< [in] The control's parent window.
		const wxString&  sRepo     = wxEmptyString, 
		wxArrayString*	 pWFParents = NULL,			
													
													
		class vvVerbs*   pVerbs    = NULL,          
		class vvContext* pContext  = NULL          
		);

	
	void TransferDataToWindow(wxString& sBranch);
	bool TransferDataFromWindow();
		
	//Returns true if the user's branch selection has been changed from the default
	bool vvBranchControl::IsDirty();

	/**
	Get the name of the branch the user requested for creation/attachment.
	*/
	wxString GetBranchName() const;

	/**
	Get the BranchAttachmentAction the user requested.
	*/
	vvVerbs::BranchAttachmentAction GetBranchAction() const;

	/**
	Get the name of the branch that was originally associated with the working copy.
	*/
	wxString GetOriginalBranchName() const;
	
	/**
	Get the BranchAttachmentAction needed to restore the original branch.
	*/
	vvVerbs::BranchAttachmentAction GetBranchRestoreAction() const;

// internal functionality
protected:
	/**
	 * Update the visible/enabled state of all controls based on current values.
	 */
	void UpdateDisplay();
	vvVerbs::BranchAttachmentAction GetActionChoice();

// event handlers
protected:
	void OnControlValueChange(wxCommandEvent& cEvent);
	void OnBranchChange(wxCommandEvent& cEvent);
	void OnBrowseBranch(wxCommandEvent& cEvent);

// private data
private:
	// repo data
	wxString   msRepo;    //< The repo to use
	vvVerbs*   mpVerbs;   //< Verbs to use
	vvContext* mpContext; //< Context to use
	wxArrayString* mpWFParents; //<The HIDs of the parents of the working copy.

	wxString      msBranch;     //< The value of the branch to associate with
	wxString      msOrigBranch; //< The original value of the branch to associate with

	vvVerbs::BranchAttachmentAction meAction; //<Save the action they chose.
	vvVerbs::BranchAttachmentAction meRestoreAction; //<This is the action necessary to restore the original branch.

	wxTextCtrl * mwBranchText;
	wxCheckBox * mwAttachCheckBox;
	wxTextCtrl * mwHeadCountWarning;
	wxButton *   mwBrowseButton;

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvBranchControl);
	DECLARE_EVENT_TABLE();
};

#endif
