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

#ifndef VV_DIFF_THREAD_H
#define VV_DIFF_THREAD_H

#include "precompiled.h"
#include "vvDiffCommand.h"
#include "wx/thread.h"

class vvDiffThread : public wxThread
{
private:
	vvDiffThread();
    vvDiffThread(const vvDiffThread &copy);
public:
	/**
	* After giving the command to this function, the thread owns it.  You don't own it any more.
	*/
	vvDiffThread(vvDiffCommand **ppCommand) : wxThread(wxTHREAD_DETACHED)
    {
		m_pCommand = *ppCommand;
		ppCommand = NULL; //NULL the pointer, so that the caller doesn't try to use it again.
		if(wxTHREAD_NO_ERROR == Create()) 
			Run();
    }
protected:
	virtual wxThread::ExitCode Entry();
	void OnExit();
private:
	vvDiffCommand * m_pCommand;
};

#endif