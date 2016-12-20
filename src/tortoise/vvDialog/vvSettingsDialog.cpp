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
#include "vvSettingsDialog.h"

#include "vvContext.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvVerbs.h"
#include "vvWxHelpers.h"


/*
**
** Globals
**
*/

// control hints
static const char* gszHint_CurrentUser = "Set current user";
static const char* gszHint_DiffTool    = "Set default diff tool";
static const char* gszHint_MergeTool   = "Set default merge tool";

// validation
static const char* gszValidationErrorMessage      = "There was an error in the information specified.  Please correct the following problems and try again.";
static const char* gszValidationField_CurrentUser = "Current User";

// setting info
static int         giSettingScope_DiffTool      = vvVerbs::CONFIG_SCOPE_MACHINE;
static const char* gszSettingName_DiffTool      = "filetoolbindings/diff/:text/gui";
static int         giSettingScope_MergeTool     = vvVerbs::CONFIG_SCOPE_MACHINE;
static const char* gszSettingName_MergeTool     = "filetoolbindings/merge/:text/resolve";
static int         giSettingScope_HideOutsideWC = vvVerbs::CONFIG_SCOPE_MACHINE;
static const char* gszSettingName_HideOutsideWC = "Tortoise/explorer/hide_menu_if_no_working_copy";


/*
**
** Internal Functions
**
*/

/**
 * Updates a config value.
 */
static bool _SetConfigValue(
	vvVerbs&        cVerbs,   //< [in] Verbs to use to set the value.
	vvContext&      cContext, //< [in] Context to use with cVerbs.
	vvFlags         cScope,   //< [in] Scope to set the setting in.
	const wxString& sName,    //< [in] Name of the setting to set.
	const wxString& sRepo,    //< [in] Name of the repo set the setting in, if cScope is repo-specific.
	const wxString& sValue    //< [in] The value to set.
	                          //<      If empty, the named setting is unset/reset/cleared instead.
	)
{
	if (sValue.IsEmpty())
	{
		if (!cVerbs.UnsetConfigValue(cContext, cScope, sName, sRepo))
		{
			cContext.Error_ResetReport("Error clearing value for setting: %s", sName);
			return false;
		}
	}
	else
	{
		if (!cVerbs.SetConfigValue(cContext, cScope, sName, sRepo, sValue))
		{
			cContext.Error_ResetReport("Error updating value for setting: %s", sName);
			return false;
		}
	}

	return true;
}


/*
**
** Public Functions
**
*/

vvSettingsDialog::vvSettingsDialog()
{
}

vvSettingsDialog::vvSettingsDialog(
	wxWindow*       wParent,
	vvVerbs&        cVerbs,
	vvContext&      cContext,
	const vvRepoLocator& cRepoLocator
	)
{
	this->Create(wParent, cVerbs, cContext, cRepoLocator);
}

bool vvSettingsDialog::Create(
	wxWindow*       wParent,
	vvVerbs&        cVerbs,
	vvContext&      cContext,
	const vvRepoLocator& cRepoLocator
	)
{
	// create the dialog window
	if (!vvDialog::Create(wParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	// store initialization data
	this->mcRepoLocator = cRepoLocator;

	// setup validation
	this->mcValidatorReporter.SetHelpMessage(gszValidationErrorMessage);

	// get widgets
	this->mwCurrentUser     = XRCCTRL(*this, "wCurrentUser",     wxComboBox);
	this->mwForgetPasswords = XRCCTRL(*this, "wForgetPasswords", wxButton);
	this->mwDiffTool        = XRCCTRL(*this, "wDiffTool",        wxComboBox);
	this->mwMergeTool       = XRCCTRL(*this, "wMergeTool",       wxComboBox);
	this->mwHideOutsideWC   = XRCCTRL(*this, "wHideOutsideWC",   wxCheckBox);

	// configure the current user control
	if (this->mcRepoLocator.GetRepoName().IsEmpty())
	{
		this->mwCurrentUser->Disable();
	}
	else
	{
		if (!this->PopulateUsers())
		{
			return false;
		}
		this->mwCurrentUser->SetHint(gszHint_CurrentUser);
		this->mwCurrentUser->SetValidator(vvValidator(gszValidationField_CurrentUser)
			.SetValidatorFlags(vvValidator::VALIDATE_SUCCEED_IF_DISABLED | vvValidator::VALIDATE_SUCCEED_IF_HIDDEN)
			.SetReporter(&this->mcValidatorReporter)
			.AddConstraint(vvValidatorEmptyConstraint(false))
			// TODO: would be nice to have a max-length constraint here as well
		);
	}

	// configure the diff tool control
	wxString sDefaultDiffTool;
	if (!cVerbs.GetConfigValue(cContext, vvVerbs::CONFIG_SCOPE_DEFAULT, gszSettingName_DiffTool, wxEmptyString, sDefaultDiffTool,  gszHint_DiffTool))
	{
		cContext.Error_ResetReport("Error retrieving default diff tool.");
		return false;
	}
	wxArrayString cDiffTools;
	if (!cVerbs.GetDiffTools(cContext, this->mcRepoLocator.GetRepoName(), cDiffTools))
	{
		cContext.Error_ResetReport("Error retrieving diff tool list.");
		return false;
	}
	cDiffTools.Sort(vvSortStrings_Insensitive);
	this->mwDiffTool->Append(cDiffTools);
	this->mwDiffTool->SetHint(sDefaultDiffTool);

	// configure the merge tool control
	wxString sDefaultMergeTool;
	if (!cVerbs.GetConfigValue(cContext, vvVerbs::CONFIG_SCOPE_DEFAULT, gszSettingName_MergeTool, wxEmptyString, sDefaultMergeTool, gszHint_MergeTool))
	{
		cContext.Error_ResetReport("Error retrieving default merge tool.");
		return false;
	}
	wxArrayString cMergeTools;
	if (!cVerbs.GetMergeTools(cContext, this->mcRepoLocator.GetRepoName(), cMergeTools))
	{
		cContext.Error_ResetReport("Error retrieving merge tool list.");
		return false;
	}
	cMergeTools.Sort(vvSortStrings_Insensitive);
	this->mwMergeTool->Append(cMergeTools);
	this->mwMergeTool->SetHint(sDefaultMergeTool);
	this->Fit();
	// success
	return true;
}

bool vvSettingsDialog::TransferDataToWindow()
{
	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext& cContext = *this->GetContext();

	// Current User
	if (!this->mcRepoLocator.GetRepoName().IsEmpty())
	{
		wxString sCurrentUser;
		if (!pVerbs->GetCurrentUser(cContext, this->mcRepoLocator.GetRepoName(), sCurrentUser))
		{
			cContext.Error_ResetReport("Error retrieving current user.");
			return false;
		}
		this->mwCurrentUser->SetValue(sCurrentUser);
	}

	// Diff Tool
	wxString sDiffTool;
	if (!pVerbs->GetConfigValue(cContext, giSettingScope_DiffTool, gszSettingName_DiffTool, this->mcRepoLocator.GetRepoName(), sDiffTool, wxEmptyString))
	{
		cContext.Error_ResetReport("Error retrieving value for setting: %s", gszSettingName_DiffTool);
		return false;
	}
	this->mwDiffTool->SetValue(sDiffTool);

	// Merge Tool
	wxString sMergeTool;
	if (!pVerbs->GetConfigValue(cContext, giSettingScope_MergeTool, gszSettingName_MergeTool, this->mcRepoLocator.GetRepoName(), sMergeTool, wxEmptyString))
	{
		cContext.Error_ResetReport("Error retrieving value for setting: %s", gszSettingName_MergeTool);
		return false;
	}
	this->mwMergeTool->SetValue(sMergeTool);

	// Hide Outside Working Copy
	wxString sHideOutsideWC;
	if (!pVerbs->GetConfigValue(cContext, giSettingScope_HideOutsideWC, gszSettingName_HideOutsideWC, this->mcRepoLocator.GetRepoName(), sHideOutsideWC, wxEmptyString))
	{
		cContext.Error_ResetReport("Error retrieving value for setting: %s", gszSettingName_HideOutsideWC);
		return false;
	}
	this->mwHideOutsideWC->SetValue(sHideOutsideWC == "true");
	// Note: VeracityTortoise treats any value of this setting that's not exactly
	//       "true" as false, so we treat it the same way.

	// done
	return true;
}

bool vvSettingsDialog::TransferDataFromWindow()
{
	wxBusyCursor cCursor;

	vvVerbs*   pVerbs   = this->GetVerbs();
	vvContext& cContext = *this->GetContext();

	// Current User
	if (!this->mcRepoLocator.GetRepoName().IsEmpty())
	{
		wxString sCurrentUser = this->mwCurrentUser->GetValue();

		// check if the entered user already exists
		wxArrayString cUsers;
		if (!pVerbs->GetUsers(cContext, this->mcRepoLocator.GetRepoName(), cUsers))
		{
			cContext.Error_ResetReport("Error retrieving user list.");
			return false;
		}
		bool bNewUser = (cUsers.Index(sCurrentUser) == wxNOT_FOUND);

		// if the user doesn't exist yet, check if they really want to create it
		if (bNewUser)
		{
			wxMessageDialog cDialog(this, wxString::Format("User '%s' does not exist in the current repository.  Would you like to create this user?", sCurrentUser), "Create New User?", wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxICON_QUESTION);
			cDialog.SetOKLabel("Create User");
			if (cDialog.ShowModal() != wxID_OK)
			{
				return false;
			}
		}

		// set the current user
		if (!pVerbs->SetCurrentUser(cContext, this->mcRepoLocator.GetRepoName(), sCurrentUser, true))
		{
			cContext.Error_ResetReport("Error updating current user.");
			return false;
		}
		else
			pVerbs->RefreshWC(cContext, this->mcRepoLocator.GetWorkingFolder());

		// if we created a new user, re-populate the list
		if (bNewUser && !this->PopulateUsers())
		{
			return false;
		}
	}

	// Diff Tool
	if (!_SetConfigValue(*pVerbs, cContext, giSettingScope_DiffTool, gszSettingName_DiffTool, this->mcRepoLocator.GetRepoName(), this->mwDiffTool->GetValue()))
	{
		return false;
	}

	// Merge Tool
	if (!_SetConfigValue(*pVerbs, cContext, giSettingScope_MergeTool, gszSettingName_MergeTool, this->mcRepoLocator.GetRepoName(), this->mwMergeTool->GetValue()))
	{
		return false;
	}

	// Hide Outside Working Copy
	if (!_SetConfigValue(*pVerbs, cContext, giSettingScope_HideOutsideWC, gszSettingName_HideOutsideWC, this->mcRepoLocator.GetRepoName(), this->mwHideOutsideWC->GetValue() ? "true" : "false"))
	{
		return false;
	}

	// done
	return true;
}

void vvSettingsDialog::OnForgetPasswords(
	wxCommandEvent& WXUNUSED(e)
	)
{
	static const char* szTitle   = "Delete All Saved Passwords?";
	static const char* szMessage = "Are you sure you want to delete all your passwords saved on this machine for all Veracity repositories?  This action is applied immediately, and cannot be undone or canceled later.";
	int                iStyle    = wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxICON_EXCLAMATION | wxCENTRE;

	if (wxMessageBox(szMessage, szTitle, iStyle, this) != wxOK)
	{
		return;
	}

	if (!this->GetVerbs()->DeleteSavedPasswords(*this->GetContext()))
	{
		this->GetContext()->Error_ResetReport("Unable to delete saved passwords.");
	}
}

bool vvSettingsDialog::PopulateUsers(
	)
{
	wxArrayString cUsers;
	if (!this->GetVerbs()->GetUsers(*this->GetContext(), this->mcRepoLocator.GetRepoName(), cUsers))
	{
		this->GetContext()->Error_ResetReport("Error retrieving user list.");
		return false;
	}
	cUsers.Sort(vvSortStrings_Insensitive);

	wxString sCurrentUser = this->mwCurrentUser->GetValue();
	((wxItemContainer*)this->mwCurrentUser)->Clear();

	this->mwCurrentUser->Append(cUsers);
	this->mwCurrentUser->SetValue(sCurrentUser);

	return true;
}

IMPLEMENT_DYNAMIC_CLASS(vvSettingsDialog, vvDialog);

BEGIN_EVENT_TABLE(vvSettingsDialog, vvDialog)
	EVT_BUTTON(XRCID("wForgetPasswords"), OnForgetPasswords)
END_EVENT_TABLE()
