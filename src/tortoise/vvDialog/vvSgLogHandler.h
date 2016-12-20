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

#ifndef VV_SG_LOG_HANDLER_H
#define VV_SG_LOG_HANDLER_H

#include "precompiled.h"
#include "vvContext.h"
#include "vvCppUtilities.h"
#include "vvFlags.h"
#include "vvNullable.h"
#include "vvSglibStringPool.h"


/**
 * A simple mix-in base class for classes that implement sglib log handling.
 */
class vvSgLogHandler
{
// types
public:
	/**
	 * Forward declaration for SG_log__stats.
	 */
	typedef struct _sg_log__stats sgLogStats;

	/**
	 * Wrapper around SG_LOG__FLAG__HANDLER_TYPE__*
	 */
	enum Level
	{
		LEVEL_ALL,
		LEVEL_NORMAL,
		LEVEL_QUIET,
		LEVEL_COUNT
	};

// construction
public:
	/**
	 * Default constructor.
	 */
	vvSgLogHandler();

	/**
	 * Destructor.
	 */
	virtual ~vvSgLogHandler();

// functionality
public:
	/**
	 * Configures the logger to a general "log level".
	 *
	 * Each level acts like a pre-configured set of calls to SetReceive* and
	 * possibly configuration calls on derived classes, if they override this.
	 */
	virtual void SetLevel(
		Level eLevel //< [in] The log level to configure the logger for.
		);

	/**
	 * Calls SetLevel based on the current value of the log/level key in the
	 * Veracity configuration.
	 */
	void SetLevelFromConfig(
		class vvVerbs& cVerbs,  //< [in] Verbs to use to retrieve config data.
		vvContext&     cContext //< [in] Context to use with pVerbs.
		);

	/**
	 * Configures the logger to receive or not receive messages of a specific type.
	 * By default, the logger receives all types of messages.
	 * Must be called before RegisterSgLogHandler.
	 */
	void SetReceiveMessage(
		vvContext::Log_MessageType eType,   //< [in] The type of message to configure.
		bool                       bReceive //< [in] Whether or not to receive the specified type of message.
		);

	/**
	 * Configures the logger to receive brief/standard or detailed messages.
	 * By default, the logger always receives brief messages.
	 * Must be called before RegisterSgLogHandler.
	 */
	void SetReceiveDetailedMessages(
		vvNullable<bool> nbDetailed //< [in] True to always receive detailed messages.
		                            //<      False to always receive brief messages.
		                            //<      vvNULL to receive detailed messages in debug builds and brief messages in other builds.
		);

	/**
	 * Configures the logger to receive or not receive operations of a specific type.
	 * By default, the logger receives all types of operations.
	 * Must be called before RegisterSgLogHandler.
	 */
	void SetReceiveOperation(
		bool bVerbose, //< [in] The type of operation to configure (verbose or non-verbose).
		bool bReceive  //< [in] Whether or not to receive the specified type of operation.
		);

	/**
	 * Sets whether or not the logger should maintain usage stats.
	 * Must be called before RegisterSgLogHandler.
	 */
	void EnableStatTracking(
		bool bStats = true //< [in] True to maintain stats, false to not bother.
		);

	/**
	 * Gets the number of messages received by the handler.
	 * Stat tracking must be enabled to use this function.
	 */
	unsigned int GetMessageCount(
		vvNullable<vvContext::Log_MessageType> neType = vvNULL //< [in] Type of message to get the count for.
		                                                       //<      If NULL, get the total count of all types.
		) const;

	/**
	 * Gets the number of completed operations received by the handler.
	 * Stat tracking must be enabled to use this function.
	 */
	unsigned int GetOperationCount(
		vvNullable<bool> nbSuccessful = vvNULL //< [in] True to get the successful operation count.
		                                       //<      False to get the failed operation count.
		                                       //<      NULL to get the total operation count.
		) const;

	/**
	 * Registers the handler to receive message reports from sglib.
	 * Returns true if successful or false if failed.
	 */
	bool RegisterSgLogHandler(
		vvContext& cContext,          //< [in] [out] Sglib error and context info.
		bool       bExclusive = false //< [in] If true, the main application log handler will be unregistered while this one is registered.
		                              //<      This allows the handler to take exclusive control of logging if necessary.
		);

	/**
	 * Unregisters the dialog from receiving message reports from sglib.
	 * Returns true if successful or false if failed.
	 */
	bool UnregisterSgLogHandler(
		vvContext& cContext //< [in] [out] Sglib error and context info.
		);

// internal functionality
protected:
	/**
	 * Implemented by derived classes to do the actual logger registration.
	 */
	virtual bool DoRegister(
		vvContext&   cContext, //< [in] [out] Error and context info.
		unsigned int uFlags,   //< [in] Flags to register the logger with.
		sgLogStats*  pStats    //< [in] Stat data to maintain while registered.
		                       //<      NULL if no stats should be tracked.
		) = 0;

	/**
	 * Implemented by derived classes to do the actual logger unregistration.
	 */
	virtual bool DoUnregister(
		vvContext& cContext //< [in] [out] Error and context info.
		) = 0;

// private functionality
private:
	/**
	 * Updates the flags in mcFlags.
	 */
	void UpdateFlags(
		unsigned int uFlags, //< [in] The flag(s) to add or remove from mcFlags.
		bool         bAdd    //< [in] True to add the flag(s), false to remove it/them.
		);

// private data
private:
	vvNullable<bool> mnbRegistered; //< If NULL, we're not currently registered.
	                                //< If non-NULL, indicates whether we're registered exclusively.
	vvFlags          mcFlags;       //< Flags the logger is (or will be) registered with.
	sgLogStats*      mpStats;       //< The stat data being tracked by the logger.
	                                //< NULL if we're not maintaining stats.
	
};


/**
 * vvSgLogHandler implementation for standard log handlers.
 */
class vvSgGeneralLogHandler : public vvSgLogHandler
{
// types
public:
	/**
	 * Forward declaration for SG_log_handler.
	 */
	typedef struct _sg_log__handler sgLogHandler;

// construction
public:
	/**
	 * Constructor.
	 */
	vvSgGeneralLogHandler(
		sgLogHandler& cHandler,        //< [in] The log handler struct to wrap.
		void*         pUserData = NULL //< [in] User data to pass to the handler.
		);

// implementation
protected:
	/**
	 * See base class documentation.
	 */
	virtual bool DoRegister(
		vvContext&   cContext,
		unsigned int uFlags,
		sgLogStats*  pStats
		);

	/**
	 * See base class documentation.
	 */
	virtual bool DoUnregister(
		vvContext& cContext
		);

// private data
private:
	sgLogHandler* mpHandler;  //< The log handler that we (un)register.
	void*         mpUserData; //< User data to pass to the handler.
};


/**
 * vvSgLogHandler implementation for log_text handlers.
 */
class vvSgTextLogHandler : public vvSgLogHandler
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvSgTextLogHandler(
		class vvContext& cContext//< [in] [out] Error and context info.
		);

	/**
	 * Destructor.
	 */
	~vvSgTextLogHandler();

// functionality
public:
	/**
	 * Checks if the handler was created successfully and is able to be used.
	 */
	virtual bool IsOk() const;

	/**
	 * Sets whether or not verbose operations will be logged.
	 */
	void SetLogVerboseOperations(
		bool bLog //< [in] Whether or not to log verbose operations.
		);

	/**
	 * Sets whether or not verbose operations will be logged.
	 */
	void SetLogVerboseValues(
		bool bLog //< [in] Whether or not to log verbose operations.
		);

	/**
	 * Sets a message to be logged when the the handler is registered.
	 */
	void SetRegisterMessage(
		const wxString& sMessage //< [in] The message to log when the handler is registered.
		                         //<      wxEmptyString to not log a register message.
		);

	/**
	 * Sets a message to be logged when the the handler is unregistered.
	 */
	void SetUnregisterMessage(
		const wxString& sMessage //< [in] The message to log when the handler is unregistered.
		                         //<      wxEmptyString to not log an unregister message.
		);

	/**
	 * Sets the format string to use for displaying the current date/time with logged text.
	 */
	void SetDateTimeFormat(
		const wxString& sFormat //< [in] The format string to use for the current date/time.
		                        //<      Should contain a single '%s' placeholder for the already formatted date/time.
		                        //<      Note that wxDateTime::Format style formatting is not supported because sglib handles the formatting.
		                        //<      wxEmptyString to not log the current time.
		);

	/**
	 * Sets the format string to use for displaying process/thread IDs.
	 */
	void SetProcessThreadFormat(
		const wxString& sFormat //< [in] The format string to use for the process and thread IDs.
		                        //<      Should contain two '%s' placeholders for the process and thread IDs respectively.
		                        //<      wxEmptyString to not log process/thread IDs.
		);

	/**
	 * Sets the string used for indents.
	 */
	void SetIndent(
		const wxString& sIndent //< [in] The string to use for one level of indentation.
		);

	/**
	 * Sets the format string to use for displaying the total elapsed time with logged text.
	 */
	void SetElapsedTimeFormat(
		const wxString& sFormat //< [in] The format string to use for the elapsed time.
		                        //<      Should contain a single '%d' placeholder for the elapsed milliseconds.
		                        //<      Note that wxTimeSpan::Format style formatting is not supported because sglib handles the formatting.
		                        //<      wxEmptyString to not log the elapsed time.
		);

	/**
	 * Sets the format string to use for displaying the completion level with logged text.
	 */
	void SetCompletionFormat(
		const wxString& sFormat //< [in] The format string to use for the completion status.
		                        //<      Should contain three '%d' placeholders for the finished steps, total steps, and percentage respectively.
		                        //<      wxEmptyString to not log the completion status.
		);

	/**
	 * Sets the format string to use for operation descriptions.
	 */
	void SetOperationFormat(
		const wxString& sFormat //< [in] The format string to use for operation descriptions.
		                        //<      Should contain a single '%s' placeholder for the operation description.
		                        //<      wxEmptyString to not log operation descriptions.
		);

	/**
	 * Sets the format string to use for values.
	 */
	void SetValueFormat(
		const wxString& sFormat //< [in] The format string to use for values.
		                        //<      Should contain two '%s' placeholders for the name and value respectively.
		                        //<      wxEmptyString to not log values.
		);

	/**
	 * Sets the format string to use for step descriptions.
	 */
	void SetStepFormat(
		const wxString& sFormat //< [in] The format string to use for step descriptions.
		                        //<      Should contain a single '%s' placeholder for the step description.
		                        //<      wxEmptyString to not log step descriptions.
		);

	/**
	 * Sets the string logged when steps are finished.
	 */
	void SetStepMessage(
		const wxString& sMessage //< [in] The step message to use.
		                         //<      wxEmptyString to not log step messages.
		);

	/**
	 * Sets the format string to use for operation results.
	 */
	void SetResultFormat(
		const wxString& sFormat //< [in] The format string to use for operation result.
		                        //<      Should contain a single '%d' placeholder for the result code.
		                        //<      wxEmptyString to not log results.
		);

	/**
	 * Sets the format string to use for reported verbose messages.
	 */
	void SetVerboseFormat(
		const wxString& sFormat //< [in] The format string to use for verbose messages.
		                        //<      Should contain a single '%s' placeholder for the message.
		                        //<      wxEmptyString to not log verbose messages.
		);

	/**
	 * Sets the format string to use for reported info messages.
	 */
	void SetInfoFormat(
		const wxString& sFormat //< [in] The format string to use for info messages.
		                        //<      Should contain a single '%s' placeholder for the message.
		                        //<      wxEmptyString to not log info messages.
		);

	/**
	 * Sets the format string to use for reported warning messages.
	 */
	void SetWarningFormat(
		const wxString& sFormat //< [in] The format string to use for warning messages.
		                        //<      Should contain a single '%s' placeholder for the message.
		                        //<      wxEmptyString to not log warning messages.
		);

	/**
	 * Sets the format string to use for reported error messages.
	 */
	void SetErrorFormat(
		const wxString& sFormat //< [in] The format string to use for error messages.
		                        //<      Should contain a single '%s' placeholder for the message.
		                        //<      wxEmptyString to not log error messages.
		);

	/**
	 * Implemented by derived classes to actually write text to its destination.
	 */
	virtual void DoWrite(
		class vvContext& cContext, //< [in] [out] Error and context info.
		const wxString&  sText,    //< [in] The text to write.
		bool             bError    //< [in] Whether or not the text should be considered error text.
		) = 0;

// implementation
protected:
	/**
	 * See base class documentation.
	 */
	virtual void SetLevel(
		Level eLevel
		);

	/**
	 * See base class documentation.
	 */
	virtual bool DoRegister(
		vvContext&   cContext,
		unsigned int uFlags,
		sgLogStats*  pStats
		);

	/**
	 * See base class documentation.
	 */
	virtual bool DoUnregister(
		class vvContext& cContext
		);

// private data
private:
	struct _sg_log_text__data* mpHandler; //< The data for our log_text handler.
	vvSglibStringPool          mcStrings; //< Strings pointed to by mpHandler.

// macro declarations
private:
	vvDISABLE_COPY(vvSgTextLogHandler);
};


#endif
