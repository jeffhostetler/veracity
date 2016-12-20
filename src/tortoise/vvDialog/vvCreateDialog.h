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

#ifndef VV_CREATE_DIALOG_H
#define VV_CREATE_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvValidatorMessageBoxReporter.h"
#include "vvSyncTargetsControl.h"

/**
 * A dialog box used to create a working copy in a directory.
 * Allows the user to attach the working copy to:
 * (1) an existing repo (checkout)
 * (2) a brand new repo (init)
 * (3) a freshly cloned repo (clone, checkout)
 */
class vvCreateDialog : public vvDialog
{
// types
public:
	/**
	 * Actions the user might select to create a freshly cloned repo to create the working copy from.
	 */
	enum CloneAction
	{
		CLONE_NOTHING, //< User chose not to create a new repo.  Clone source is not valid.
		CLONE_EMPTY,   //< User chose to create a new blank repo.  Clone source is not valid.
		CLONE_LOCAL,   //< User chose to clone a local repo.  The clone source will be the name.
		CLONE_REMOTE,  //< User chose to clone a remote repo.  The clone source will be a URL.
	};

	/**
	 * Methods that the user can use to select which revision to checkout into the new working copy.
	 */
	enum CheckoutType
	{
		CHECKOUT_BRANCH,    //< User chose to checkout the head revision of a branch.  Checkout target is the branch name.
		CHECKOUT_CHANGESET, //< User chose to checkout a specific changeset.  Checkout target will be the changeset ID.
		CHECKOUT_TAG,       //< User chose to checkout a tagged revision.  Checkout target will be the tag name.
	};

// functionality
public:
	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*        pParent,                                 //< [in] The new dialog's parent window, or NULL if it won't have one.
		const wxString&  sFolder,                                 //< [in] The folder to create a working copy in.
		class vvVerbs&   cVerbs,                                  //< [in] The vvVerbs implementation to retrieve local repo and config info from.
		class vvContext& cContext,                                //< [in] Context to use with pVerbs.
		const wxString&  sRepoName       = wxEmptyString,         //< [in] The name of the repo to pre-select.
		CloneAction      eCloneAction    = CLONE_NOTHING,         //< [in] the clone action to pre-select.
		const wxString&  sCloneSource    = wxEmptyString,         //< [in] The clone source to pre-select.  Where it's placed depends on eCloneAction.
		CheckoutType     eCheckoutType   = CHECKOUT_BRANCH,       //< [in] The checkout type to pre-select.
		const wxString&  sCheckoutTarget = vvVerbs::sszMainBranch //< [in] The checkout target to pre-select.  Where it's placed depends on eCheckoutType.
		);

	/**
	 * Transfers our internal data to our widgets.
	 */
	virtual bool TransferDataToWindow();

	/**
	 * Stores data from our widgets to our internal data.
	 */
	virtual bool TransferDataFromWindow();

	/**
	 * Retrieves the name of the repo chosen by the user to create the working copy from.
	 * Might be the name of a repo that doesn't yet exist, check GetCloneAction.
	 */
	const wxString& GetRepo() const;

	/**
	 * Gets the action chosen by the user for attaching to a freshly cloned repo.
	 */
	CloneAction GetCloneAction() const;

	/**
	 * The source that the user wants to clone a repo from prior to attaching to it.
	 * Check GetCloneAction to determine whether it's a remote URL or another local repo name.
	 */
	const wxString& GetCloneSource() const;

	/**
	 * The descriptor of a repo that will be used for the shared users option to init.
	 */
	const wxString& GetSharedUsersRepo() const;

	/**
	 * Gets the type of checkout the user wants to perform to populate the new working copy.
	 * If they chose to checkout a particular revision, use GetCheckoutTarget to get the appropriate target.
	 */
	CheckoutType GetCheckoutType() const;

	/**
	 * The target that the user is checking out from the repo.
	 * Might be the name of a tag, branch, changeset, etc.  Use GetCheckoutType to tell.
	 */
	const wxString& GetCheckoutTarget() const;

	/**
	 * If the user picks a branch with multiple heads, then they will be required to 
	 * pick which head.  The result from the vvCreateDialog will be a changeset checkout,
	 * and this method will return the branch to attach to.  
	 */
	const wxString& GetBranchAttachment() const;

	/**
	 * Ask the RevSpec Control to disambiguate the branch
	 */
	bool DisambiguateBranch();

	/**
	 * 
	 */
	void vvCreateDialog::SetRepo(const wxString& sRepo);

// internal functionality
protected:
	/**
	 * Update the visible/enabled state of all controls, based on the current values.
	 */
	void UpdateDisplay();

// event handlers
protected:
	void OnControlValueChange(wxCommandEvent& cEvent);

// private data
private:
	// internal data
	wxString     msRepoName;       //< The name of the repo to create the working copy from.
	CloneAction  meCloneAction;    //< The action to take to create a freshly cloned repo.
	wxString     msCloneSource;    //< The source to create a freshly cloned repo from.
	CheckoutType meCheckoutType;   //< The method to use to checkout a revision from the repo.
	wxString     msCheckoutTarget; //< The target revision to checkout from the repo.
	wxString	 msBranchAttachment; //< A branch to attach to after the checkout.  This is necessary in some cases when a branch has multiple heads.

	// widgets
	wxChoice*               mwRepo;
	wxTextCtrl*             mwNew_Name;
	wxRadioButton*          mwNew_Remote_Select;
	vvSyncTargetsControl*   mwNew_Remote_Value;
	wxRadioButton*          mwNew_Local_Select;
	wxChoice*               mwNew_Local_Value;
	wxRadioButton*          mwNew_Empty_Select;
	wxStaticText*           mwNew_Empty_Value;

	wxChoice*               mwShareUsersChoice;
	wxCheckBox*				mwShareUsers;
	wxString				msShareUserRepo;

	class vvRevSpecControl* mwRevision;

	// misc
	vvValidatorMessageBoxReporter mcValidatorReporter; //< Reporter that will display validation errors.

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvCreateDialog);
	DECLARE_EVENT_TABLE();
};


#endif
