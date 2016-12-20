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

#ifndef VV_APPLICATION_H
#define VV_APPLICATION_H

#include "precompiled.h"
#include "vvContext.h"


/**
 * Represents the application itself, and contains application-wide functionality.
 */
class vvApplication : public wxApp
{
// types
public:
	/**
	 * Exit codes.
	 */
	enum ResultCode
	{
		RESULT_SUCCESS =  0, //< Command execution succeeded.
		RESULT_ERROR   = -1, //< An error occurred (-1 because that's what wx returns if OnInit fails).
		RESULT_CANCEL  = -2, //< The dialog box was canceled/aborted.
	};

// construction
public:
	/**
	 * Default constructor.
	 */
	vvApplication();

// implementation
public:
	/**
	 * Configures the command line parser.
	 */
	virtual void OnInitCmdLine(
		wxCmdLineParser& cParser //< The parser to configure.
		);

	/**
	 * Interprets the parsed command line.
	 */
	virtual bool OnCmdLineParsed(
		wxCmdLineParser& cParser //< The parser to interpret results from.  Must have had Parse() called.
		);

	/**
	 * Initializes the program and displays the main window.
	 */
	virtual bool OnInit();

	/**
	 * Runs the application's main event loop.
	 */
	virtual int OnRun();

	/**
	 * Runs program cleanup.
	 */
	virtual int OnExit();

// functionality
public:
	/**
	 * Enables or disables the global logger that forwards sglib logging
	 * to wxWidgets logging.  It can be useful to disable this so that a
	 * window can take exclusive control of displaying logged data.
	 * Note that this has no effect on data that is logged directly to
	 * the wxWidgets system; that will still be displayed as normal.
	 *
	 * Returns success/failure.
	 */
	bool EnableGlobalSglibLogging(
		vvContext& cContext, //< [in] Error and context info.
		bool       bEnable   //< [in] Whether or not global sglib logging should be enabled.
		);

// private data
private:
	vvContext                   mcContext;  //< sglib context for the main thread
	class vvCommand*            mpCommand;  //< command invoked by the user
	class vvProgressExecutor*   mpExecutor; //< executor to invoke the command behind a progress dialog
	class vvDialog*             mwDialog;   //< top-level dialog created by the command
	class vvVerbs*              mpVerbs;    //< verbs instance being used application-wide
	class vvFileProgressStream* mpLog;      //< log that we're writing to
	class vvSgWxLogHandler*     mpWxLogger; //< sglib log handler to forward messages to the wx log system for display
};

DECLARE_APP(vvApplication);


#endif
