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
#include "vvApplication.h"

#include "vvCommand.h"
#include "vvDialog.h"
#include "vvProgressExecutor.h"
#include "vvProgressStream.h"
#include "vvResourceArtProvider.h"
#include "vvSgWxLogHandler.h"
#include "vvVerbs.h"


/*
**
** Types
**
*/

/**
 * This function is generated at build time from *.xrc by wxrc.exe.
 * It loads .xrc resources that have been "compiled" into a cpp file (wxResources.inc).
 * Note that the wxResources.inc file isn't built explicitly, it's included by wxResources.cpp.
 */
void InitXmlResource();


/*
**
** Globals
**
*/

/*
 * Identification strings.
 *
 * TODO: Move these to the manifest so they show up in the EXE file correctly.
 */
static const char* gszAppName    = "vvDialog";
static const char* gszAppVersion = "0.1";
static const char* gszVendorName = "SourceGear";

static const char* gszCmd_Help_Short        = "h";
static const char* gszCmd_Help_Long         = "help";
static const char* gszCmd_Help_Description  = "Display this help message.";

/**
 * Specification of the command line used by the application.
 */
static const wxCmdLineEntryDesc gaCommandLineDesc[] =
{
	{ wxCMD_LINE_SWITCH, gszCmd_Help_Short,  gszCmd_Help_Long,  gszCmd_Help_Description,  wxCMD_LINE_VAL_NONE,   wxCMD_LINE_OPTION_HELP },
	{ wxCMD_LINE_PARAM,  NULL,               NULL,              "dialog",                 wxCMD_LINE_VAL_NONE,   0 },
	{ wxCMD_LINE_PARAM,  NULL,               NULL,              "option",                 wxCMD_LINE_VAL_NONE,   wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE },
	{ wxCMD_LINE_NONE } // effectively a NULL terminator
};

/*
 * Various strings added to the command line usage text.
 */
static const wxString gsCommandLineHeader           = wxString::Format("%s %s by %s", gszAppName, gszAppVersion, gszVendorName);
static const wxString gsCommandLineDialogHeader     = "Dialogs:";
static const wxString gsCommandLineDialogSpecFormat = "%s - %s"; // (0) Name, (1) Description
static const wxString gsCommandLineHelp             = "Available remaining options depend on the specified dialog.  Most dialogs take either a single working copy or a list of files/folders to act on.  Exceptions are noted below.";


/*
**
** Public Functions
**
*/

vvApplication::vvApplication()
	: mcContext()
	, mpCommand(NULL)
	, mpExecutor(NULL)
	, mwDialog(NULL)
	, mpVerbs(NULL)
	, mpLog(NULL)
	, mpWxLogger(NULL)
{
	// set some application properties
	this->SetAppName(gszAppName);
	this->SetVendorName(gszVendorName);
}

void vvApplication::OnInitCmdLine(
	wxCmdLineParser& cParser
	)
{
	// tell the parser how to parse the command line
	cParser.SetDesc(gaCommandLineDesc);

	// add the "logo" header line to the usage message
	cParser.SetLogo(gsCommandLineHeader + "\n");

	// add our custom help text for below the standard usage
	cParser.AddUsageText(" ");
	cParser.AddUsageText(gsCommandLineHelp);

	// add descriptions of our known commands to the bottom of the usage
	cParser.AddUsageText(" ");
	cParser.AddUsageText(gsCommandLineDialogHeader);
	const vvCommand* pSpec = vvCommand::GetFirst();
	while (pSpec != NULL)
	{
		cParser.AddUsageText(wxString::Format(gsCommandLineDialogSpecFormat, pSpec->GetName(), pSpec->GetUsageDescription()));
		pSpec = pSpec->GetNext();
	}
}

bool vvApplication::OnCmdLineParsed(
	wxCmdLineParser& cParser
	)
{
	bool bSuccess = true;

	if (bSuccess)
	{
		// find the registered command that the user specified
		wxString sCommand = cParser.GetParam(0u);
		this->mpCommand = vvCommand::FindByName(sCommand);
		if (this->mpCommand == NULL)
		{
			wxLogError("Unknown command: %s", sCommand);
			bSuccess = false;
		}
	}

	if (bSuccess)
	{
		this->mpVerbs = new vvVerbs;
	}

	if (bSuccess)
	{
		// find our log file
		wxString sFilename = this->mpVerbs->GetLogFilePath(this->mcContext, "vvDialog-%d-%02d-%02d.log");
		if (!wxFile::Exists(sFilename))
		{
			wxFile f;
			wxFileName fn(sFilename);
			if (!wxDir::Exists(fn.GetPath()))
			{
				wxDir::Make(fn.GetPath(), 511, wxPATH_MKDIR_FULL);
			}
			f.Create(sFilename);
			f.Close();
		}

		// create a logger to log to the file
		this->mpLog = new vvFileProgressStream(this->mcContext, sFilename);
		if (this->mcContext.Error_Check())
		{
			this->mcContext.Error_ResetReport("Error reading/setting log level.");
			bSuccess = false;
		}
		else if (!this->mpLog->IsOk())
		{
			wxLogError("Error opening log file: %s", sFilename);
			bSuccess = false;
		}

		// set its log level based on the current config settings
		this->mpLog->SetLevelFromConfig(*this->mpVerbs, this->mcContext);
		if (this->mcContext.Error_Check())
		{
			this->mcContext.Error_ResetReport("Error reading/setting log level.");
			bSuccess = false;
		}

		// text file logger should always show detailed messages
		this->mpLog->SetReceiveDetailedMessages(true);
	}

	if (bSuccess)
	{
		// compile the command's arguments and give them to it
		// (the first parameter was the name of the command, which we'll skip)
		wxArrayString cArguments;
		for (unsigned int uIndex = 1u; uIndex < cParser.GetParamCount(); ++uIndex)
		{
			cArguments.Add(cParser.GetParam(uIndex));
		}
		if (!this->mpCommand->ParseArguments(cArguments))
		{
			//Trust the commands to log their own, more descriptive error.
			for (unsigned int uIndex = 0u; uIndex < cArguments.size(); ++uIndex)
			{
				wxLogVerbose("Argument %u: %s", uIndex, cArguments[uIndex]);
			}
			wxLogVerbose("Command '%s' failed to parse the given arguments.", this->mpCommand->GetName());
			bSuccess = false;
		}
	}

	// if something failed, cleanup
	if (!bSuccess)
	{
		if (this->mpLog != NULL)
		{
			delete this->mpLog;
			this->mpLog = NULL;
		}
		if (this->mpVerbs != NULL)
		{
			delete this->mpVerbs;
			this->mpVerbs = NULL;
		}
	}

	// done
	return bSuccess;
}

bool vvApplication::OnInit()
{
	// let the base class initialize
	if (!wxApp::OnInit())
	{
		return false;
	}

	// create and register our wx logger
	this->mpWxLogger = new vvSgWxLogHandler();
	this->mpWxLogger->RegisterSgLogHandler(this->mcContext);

	// if we have a log, register it
	if (this->mpLog != NULL)
	{
		this->mpLog->RegisterSgLogHandler(this->mcContext);
	}

	// initialize sglib
	if (!this->mpVerbs->Initialize(this->mcContext))
	{
		this->mcContext.Error_ResetReport("Failed to initialize sglib.");
		return false;
	}

	// initialize image handlers that we care about
	wxImage::AddHandler(new wxPNGHandler());

	// add our resource art provider
	wxArtProvider::Push(new vvResourceArtProvider());

	// initialize XRC handlers
	wxXmlResource::Get()->InitAllHandlers();

	// load XRC resources
	InitXmlResource();

	// initialize the command
	this->mpCommand->SetVerbs(this->mpVerbs);
	this->mpCommand->SetContext(&this->mcContext);
	if (!this->mpCommand->Init())
	{
		wxLogVerbose("Command '%s' failed to initialize.", this->mpCommand->GetName());
		return false;
	}

	// create our executor, for executing the command behind a progress window
	if (!this->mpCommand->HasAnyFlag(vvCommand::FLAG_NO_PROGRESS))
		this->mpExecutor = new vvProgressExecutor(*this->mpCommand);

	// have the command create its dialog, if it has one
	if (!this->mpCommand->HasAnyFlag(vvCommand::FLAG_NO_DIALOG))
	{
		this->mwDialog = this->mpCommand->CreateDialog(NULL);
		if (this->mwDialog == NULL)
		{
			wxLogVerbose("Command '%s' failed to create its dialog.", this->mpCommand->GetName());
			return false;
		}
	}

	// success
	return true;
}

int vvApplication::OnRun()
{
	if (this->mwDialog == NULL)
	{
		// the command didn't require a dialog to gather data
		// just go ahead and execute it
		if (!this->mpCommand->HasAnyFlag(vvCommand::FLAG_NO_EXECUTE))
		{
			if (!this->mpCommand->HasAnyFlag(vvCommand::FLAG_NO_PROGRESS))
				return this->mpExecutor->Execute() ? RESULT_SUCCESS : RESULT_CANCEL;
			else
				return this->mpCommand->Execute() ? RESULT_SUCCESS : RESULT_CANCEL;
		}
		else
		{
			wxFAIL_MSG("Specified command used both NO_DIALOG and NO_EXECUTE, so there's nothing to do.");
			return RESULT_ERROR;
		}
	}
	else
	{
		// set the dialog to execute the command when it closes
		if (!this->mpCommand->HasAnyFlag(vvCommand::FLAG_NO_EXECUTE))
		{
			this->mwDialog->SetExecutor(this->mpExecutor);
		}

		// set the dialog as our top window and show it
		this->SetTopWindow(this->mwDialog);
		return this->mwDialog->ShowModal() == wxID_OK ? RESULT_SUCCESS : RESULT_CANCEL;
	}
}

int vvApplication::OnExit()
{

	if (this->mcContext.Error_Check())
	{
		//There's an error on exit.  It's possible that we didn't realize that we
		//are exiting with an error.  
#ifdef DEBUG
		this->mcContext.Log_ReportError_CurrentError();
#else
		this->mcContext.Log_ReportVerbose_CurrentError();
#endif
		this->mcContext.Error_Reset();
	}

	// if we have a dialog, destroy it
	if (this->mwDialog != NULL)
	{
		this->mwDialog->Destroy();
	}

	// destroy our command executor
	delete this->mpExecutor;
	this->mpExecutor = NULL;

	// shutdown sglib
	this->mpVerbs->Shutdown(this->mcContext);

	// if we have a log, unregister and free it
	if (this->mpLog != NULL)
	{
		if (this->mpLog->UnregisterSgLogHandler(this->mcContext))
		{
			delete this->mpLog;
		}
		else
		{
			wxFAIL_MSG("Sglib file logger failed to unregister.  The SG_context was probably left in an error state.");
			// just leak the memory rather than crash in the destructor
			// the app is about to exit anyway
		}
		this->mpLog = NULL;
	}

	// if our wx logger exists, unregister and free it
	if (this->mpWxLogger != NULL)
	{
		if (this->mpWxLogger->UnregisterSgLogHandler(this->mcContext))
		{
			delete this->mpWxLogger;
		}
		else
		{
			wxFAIL_MSG("Sglib to wxWidgets logger failed to unregister.  The SG_context was probably left in an error state.");
			// just leak the memory rather than crash in the destructor
			// the app is about to exit anyway
		}
		this->mpWxLogger = NULL;
	}

	// delete our vvVerbs implementation
	delete this->mpVerbs;
	this->mpVerbs = NULL;

	// let our base class run cleanup
	return wxApp::OnExit();
}

bool vvApplication::EnableGlobalSglibLogging(
	vvContext& cContext,
	bool       bEnable
	)
{
	if (bEnable)
	{
		return this->mpWxLogger->RegisterSgLogHandler(cContext);
	}
	else
	{
		return this->mpWxLogger->UnregisterSgLogHandler(cContext);
	}
}

IMPLEMENT_APP(vvApplication);
