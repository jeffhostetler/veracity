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
#include "vvProgressStream.h"

/*
**
** Public Functions
**
*/

// ----- vvProgressStream -----

vvProgressStream::vvProgressStream(
	class vvContext&    cContext,
	wxTextOutputStream* pStream
	)
	: vvSgTextLogHandler(cContext)
	, mpStream(pStream)
{
}

void vvProgressStream::SetTextStream(
	wxTextOutputStream* pStream
	)
{
	this->mpStream = pStream;
}

wxTextOutputStream* vvProgressStream::GetTextStream() const
{
	return this->mpStream;
}

void vvProgressStream::DoWrite(
	class vvContext& WXUNUSED(cContext),
	const wxString&  sText,
	bool             WXUNUSED(bError)
	)
{
	if (this->mpStream != NULL)
	{
		this->mpStream->WriteString(sText);
	}
}

// ----- vvFileProgressStream -----

vvFileProgressStream::vvFileProgressStream(
	class vvContext& cContext
	)
	: vvProgressStream(cContext)
	, mpFileStream(NULL)
	, mpTextStream(NULL)
{
}

vvFileProgressStream::vvFileProgressStream(
	class vvContext& cContext,
	const wxString&  sFilename
	)
	: vvProgressStream(cContext)
	, mpFileStream(NULL)
	, mpTextStream(NULL)
{
	this->OpenFile(sFilename);
}

vvFileProgressStream::~vvFileProgressStream()
{
	if (this->mpTextStream != NULL)
	{
		this->SetTextStream(NULL);
		delete this->mpTextStream;
		this->mpTextStream = NULL;
	}
	if (this->mpFileStream != NULL)
	{
		this->mpFileStream->Close();
		delete this->mpFileStream;
		this->mpFileStream = NULL;
	}
}

bool vvFileProgressStream::OpenFile(
	const wxString& sFilename
	)
{
	this->mpFileStream = new wxFileStream(sFilename);
	if (!this->mpFileStream->IsOk())
	{
		delete this->mpFileStream;
		this->mpFileStream = NULL;
		return false;
	}

	this->mpFileStream->SeekO(0, wxFromEnd);
	this->mpTextStream = new wxTextOutputStream(*this->mpFileStream);
	this->SetTextStream(this->mpTextStream);
	return true;
}

bool vvFileProgressStream::IsOk() const
{
	return (vvProgressStream::IsOk() && this->mpFileStream != NULL && this->mpFileStream->IsOk());
}
