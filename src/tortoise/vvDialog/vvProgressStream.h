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

#ifndef VV_PROGRESS_STREAM_H
#define VV_PROGRESS_STREAM_H

#include "precompiled.h"
#include "vvSgLogHandler.h"


/**
 * An sglib log handler that records progress to a text stream.
 */
class vvProgressStream : public vvSgTextLogHandler
{
// construction
public:
	/**
	 * Default constructor.
	 * If you pass NULL, call SetTextStream later.
	 * This class does NOT take ownership of the pointer.
	 */
	vvProgressStream(
		class vvContext&    cContext,       //< [in] Sglib error and context info.
		wxTextOutputStream* pStream  = NULL //< [in] Stream to log progress to.
		);

// functionality
public:
	/**
	 * Sets the stream that progress is being logged to.
	 * This class does NOT take ownership of the pointer.
	 */
	void SetTextStream(
		wxTextOutputStream* pStream //< [in] The text stream to log progress to.
		                            //<      NULL to stop logging altogether.
		);

	/**
	 * Gets the stream that progress is being logged to.
	 * Caller does NOT own this pointer.
	 */
	wxTextOutputStream* GetTextStream() const;

// implementation
public:
	/**
	 * See base class documentation.
	 */
	virtual void DoWrite(
		class vvContext& cContext,
		const wxString&  sText,
		bool             bError
		);

// private data
private:
	wxTextOutputStream* mpStream; //< The stream that we're logging to.
};


/**
 * A helper class for a vvProgressStream that logs straight to a file.
 * This class keeps track of the wxFileOutputStream and the wxTextOutputStream for you.
 */
class vvFileProgressStream : public vvProgressStream
{
// construction
public:
	/**
	 * Constructor.
	 * Use OpenFile to begin logging to a file.
	 */
	vvFileProgressStream(
		vvContext& cContext //< [in] Sglib error and context info.
		);

	/**
	 * Constructor.
	 * Configures the stream to log to the given file.
	 * Use IsOk() to make sure the file was opened successfully.
	 */
	vvFileProgressStream(
		vvContext&      cContext, //< [in] Sglib error and context info.
		const wxString& sFilename //< [in] Filename to log progress to.
		);

	/**
	 * Destructor.
	 */
	~vvFileProgressStream();

// functionality
public:
	/**
	 * Starts logging to a given file.
	 * If already logging, the old file is closed in favor of the new one.
	 * Returns true if successful, false if failed.
	 */
	bool OpenFile(
		const wxString& sFilename
		);

// implementation
public:
	/**
	 * Checks if the file stream is okay for use.
	 */
	virtual bool IsOk() const;

// private data
private:
	wxFileOutputStream* mpFileStream; //< [in] The output stream for the file we're logging to.
	wxTextOutputStream* mpTextStream; //< [in] The text stream that wraps mpFileStream.
};


#endif
