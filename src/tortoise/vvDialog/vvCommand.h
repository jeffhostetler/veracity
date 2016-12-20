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

#ifndef VV_COMMAND_H
#define VV_COMMAND_H

#include "precompiled.h"
#include "vvFlags.h"
#include "vvRepoLocator.h"
#include "vvNullable.h"
#include "vvVerbs.h"
#include "vvContext.h"

#include "sg.h"

/**
 * A command that creates/owns a vvDialog and can execute some logic with its output.
 * Just instantiate a global one of these to register the command with the application.
 */
class vvCommand : public vvHasFlags
{
// types
public:
	/**
	 * Flags that can modify how the application interacts with the command.
	 */
	enum Flag
	{
		FLAG_NO_DIALOG			= 1 << 0, //< This command does not use a dialog to gather data.
		FLAG_NO_EXECUTE			= 1 << 1, //< This command should not be executed when its dialog closes.
		FLAG_GLOBAL_INSTANCE	= 1 << 2, //< This command should be added to the global list of commands.
		FLAG_NO_PROGRESS		= 1 << 3, //< This command should not open a progress dialog.
	};

// static functionality
public:
	/**
	 * Gets the first instance in a linked list of all instances.
	 * Returns NULL if there are no instances.
	 * Retrieve the next instance in the list with GetNext.
	 */
	static vvCommand* GetFirst();

	/**
	 * Finds a registered command with a given name.
	 * Returns the found command, or NULL if none was found.
	 */
	static vvCommand* FindByName(
		const wxString& sName //< [in] Command name to search by.
		);

// static data
private:
	static vvCommand* spFirst; //< First instance in the global linked list.

// construction
public:
	/**
	 * Constructs a new command.
	 * Automatically registers the instance in a global list.
	 * The global list can be iterated with GetFirst and GetNext.
	 */
	vvCommand(
		const wxString& sName,            //< The name used to specify this dialog on the command line.
		unsigned int    uFlags,           //< Flags that affect how the application interacts with the command.
		const wxString& sUsageDescription //< A brief description to display for the command in the app's usage text.
		);

// properties
public:
	/**
	 * Gets the name used to specify this dialog on the command line.
	 */
	const wxString& GetName() const;

	/**
	 * Gets a description to include in the app's usage text for this dialog.
	 */
	const wxString& GetUsageDescription() const;

	wxString GetRemoteRepo() const
	{
		return m_sRemoteRepo;
	}

	void SetRemoteRepo(const wxString& sRemoteRepo)
	{
		m_sRemoteRepo = sRemoteRepo;
	}

	/**
	 * Gets the next registered instance in a linked list of all instances.
	 * Returns NULL if there are no more instances in the list.
	 */
	vvCommand* GetNext() const;

	/**
	 * Sets the vvVerbs implementation that the command should use.
	 */
	void SetVerbs(
		class vvVerbs* pVerbs //< [in] The vvVerbs implementation to use.
		);

	/**
	 * Gets the vvVerbs implementation that the command is currently using.
	 */
	class vvVerbs* GetVerbs() const;

	/**
	 * Sets the vvContext that the command should use with its vvVerbs.
	 */
	void SetContext(
		class vvContext* pContext //< [in] The vvContext to use.
		);

	/**
	 * Gets the vvContext that the command is currently using.
	 */
	class vvContext* GetContext() const;

	/**
	 * Parses arguments passed to the command on the command line.
	 * Returns true on success, or false if the arguments were invalid.
	 * This function should only parse the arguments and store them.
	 * Save other initialization for the later call to Init.
	 * GetVerbs and GetContext will return NULL during this call.
	 */
	virtual bool ParseArguments(
		const wxArrayString& cArguments //< [in] The array of arguments to parse.
		);

	/**
	 * Initializes the command and any data that it might require.
	 * Returns true on success or false if initialization failed.
	 * Called after ParseArguments, but before CreateDialog and Execute.
	 * GetVerbs and GetContext will return valid values during this call.
	 */
	virtual bool Init();

	/**
	 * Creates the dialog to display to gather the information necessary to execute the command.
	 * Returns NULL if an error occurred.
	 * Called after ParseArguments and Init.
	 * GetVerbs and GetContext will return valid values during this call.
	 */
	virtual class vvDialog* CreateDialog(
		wxWindow* wParent //< [in] The window to use as the dialog's parent.
		);

	/**
	 * Executes the command.
	 * This will be called when the user clicks the OK button on the created dialog.
	 * Returns true if execution succeeded or false if it failed.
	 * GetVerbs and GetContext will return valid values during this call.
	 * If false is returned, then the context in GetContext may also be left in an error state.
	 *
	 * WARNING: This function will NOT be called in the main thread.
	 * Therefore, do NOT try to access any GUI elements of your dialog.
	 * Implement TransferDataFromWindow (or use vvValidator[Transfer] to do it for you),
	 * then just access the member variables that the GUI data was transferred to instead.
	 * It should be safe to access most other things.
	 * While this is running, the main thread will be idle except for reporting progress.
	 */
	virtual bool Execute();

	virtual bool CanHandleError(wxUint64 sgErrrorCode) const;
	/*
	 * Try to handle an error.  Returns true if the error was handled, and the command should be retried.
	 * Returns false if the progress executor should just give up.
	 */
	virtual bool TryToHandleError(vvContext* pContext, wxWindow * parent, wxUint64 sgErrorCode);

	bool GetNeedToSavePassword() const;

	/*
	 * Save the password
	 */
	bool vvCommand::SaveRememberedPassword();
protected:
	wxString		 msUserName;		 //< The user name entered into the vvLoginDialog
	wxString		 msPassword;		 //< The password entered into the vvLoginDialog
	bool			 mbRememberPassword; 
	bool mbAlreadyLoadedPassword;
	wxString		 m_sRemoteRepo;
	/*
	 * Prompt for login credentials.
	 */
	bool PromptForLoginCredentials(wxWindow * parent, vvNullable<vvRepoLocator> cRepoLocator);

	/*
	 * Prompt to upgrade the repository format.
	 */
	bool vvCommand::PromptForUpgrade(wxWindow * parent);

	/*
	 * Load the saved password.
	 */
	bool vvCommand::LoadRememberedPassword(vvNullable<vvRepoLocator> cRepoLocator);

// member data
private:
	vvCommand*       mpNext;             //< Next instance in the global linked list.
	wxString         msName;             //< Name used for this command on the command line.
	wxString         msUsageDescription; //< Description of the command for the app's usage text.
	class vvVerbs*   mpVerbs;            //< The vvVerbs implementation currently being used.
	class vvContext* mpContext;          //< The vvContext currently being used.
};

class vvRepoCommand : public vvCommand
{
protected:
	vvRepoLocator m_cRepoLocator;
	wxArrayString m_aBaselineHIDs;
public:
	vvRepoCommand(const wxString& sName,            //< The name used to specify this dialog on the command line.
		unsigned int    uFlags,           //< Flags that affect how the application interacts with the command.
		const wxString& sUsageDescription //< A brief description to display for the command in the app's usage text.
		)
		: vvCommand(sName, uFlags, sUsageDescription)
	{
	}
	bool DetermineRepo__from_workingFolder(const wxString& wfPath, bool bLogError = true, bool bLoadBaselineHIDs = false)
	{
		vvVerbs*   pVerbs   = this->GetVerbs();
		vvContext* pContext = this->GetContext();

		wxASSERT(pVerbs != NULL);
		wxASSERT(pContext != NULL);

		wxString sFolder;
		wxString sRepoName;
		if (!pVerbs->FindWorkingFolder(*pContext, wfPath, &sFolder, &sRepoName, bLoadBaselineHIDs ? &m_aBaselineHIDs : NULL))
		{
			if (bLogError)
				pContext->Error_ResetReport("Cannot find working copy for the given path.");
			else
				pContext->Error_Reset();
			return false;
		}
		wxASSERT(!sFolder.IsEmpty()); 
		m_cRepoLocator = vvRepoLocator::FromRepoNameAndWorkingFolderPath(sRepoName, sFolder);
		if (pVerbs->CheckForOldRepoFormat(*pContext, m_cRepoLocator.GetRepoName()))
		{
			return PromptForUpgrade(NULL);
		}
		return true;
	}

	bool DetermineRepo__from_workingFolder(const wxArrayString& wfPaths, bool bLogError = true, bool bLoadBaselineHIDs = false)
	{
		vvVerbs*   pVerbs   = this->GetVerbs();
		vvContext* pContext = this->GetContext();

		wxASSERT(pVerbs != NULL);
		wxASSERT(pContext != NULL);

		wxString sFolder;
		wxString sRepoName;
		if (!pVerbs->FindWorkingFolder(*pContext, wfPaths, &sFolder, &sRepoName, bLoadBaselineHIDs ? &m_aBaselineHIDs : NULL))
		{
			if (bLogError)
				pContext->Error_ResetReport("Cannot find single working copy for the given set of paths.");
			else
				pContext->Error_Reset();
			return false;
		}
		wxASSERT(!sFolder.IsEmpty()); 
		wxASSERT(!sRepoName.IsEmpty()); 
		m_cRepoLocator = vvRepoLocator::FromRepoNameAndWorkingFolderPath(sRepoName, sFolder);

		if (pVerbs->CheckForOldRepoFormat(*pContext, m_cRepoLocator.GetRepoName()))
		{
			return PromptForUpgrade(NULL);
		}
		return true;
	}

	bool DetermineRepo__from_repoName(const wxString& repoName)
	{
		vvVerbs*   pVerbs   = this->GetVerbs();
		vvContext* pContext = this->GetContext();

		wxASSERT(pVerbs != NULL);
		wxASSERT(pContext != NULL);

		m_cRepoLocator = vvRepoLocator::FromRepoName(repoName);
		if (pVerbs->CheckForOldRepoFormat(*pContext, m_cRepoLocator.GetRepoName()))
		{
			return PromptForUpgrade(NULL);
		}
		return true;
	}

	wxArrayString GetBaselineHIDs() const
	{
		return m_aBaselineHIDs;
	}

	vvRepoLocator GetRepoLocator() const
	{
		return m_cRepoLocator;
	}

	void SetRepoLocator(vvRepoLocator cRepoLocator)
	{
		m_cRepoLocator = cRepoLocator;
	}

	bool CanHandleError(wxUint64 sgErrorCode) const
	{
		if (sgErrorCode == SG_ERR_AUTHORIZATION_REQUIRED || sgErrorCode == SG_ERR_OLD_AUDITS)
			return true;
		else
			return vvCommand::CanHandleError(sgErrorCode);
	}

	bool TryToHandleError(vvContext* pContext, wxWindow * parent, wxUint64 sgErrorCode)
	{
		if (sgErrorCode == SG_ERR_AUTHORIZATION_REQUIRED)
		{
			if (!mbAlreadyLoadedPassword && LoadRememberedPassword(GetRepoLocator()))
				return true; //Loaded the password, try the command again.
			else
				return PromptForLoginCredentials(parent, GetRepoLocator());
		}
		else if (sgErrorCode == SG_ERR_OLD_AUDITS)
		{
			return PromptForUpgrade(parent);
		}
		else
			return vvCommand::TryToHandleError(pContext, parent, sgErrorCode);
	}

};
#endif
