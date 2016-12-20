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

#ifndef VV_REVISION_LINK_CONTROL_H
#define VV_REVISION_LINK_CONTROL_H

#include "precompiled.h"
#include "vvCppUtilities.h"
#include "vvNullable.h"
#include "vvVerbs.h"

class vvRevisionLinkControl : public wxPanel
{
// types
public:

// construction
public:
	/**
	 * Default constructor.
	 */
	vvRevisionLinkControl();

	/**
	 * Create contructor.
	 */
	vvRevisionLinkControl(
		unsigned int nRevisionNum,
		wxString sRevisionHID,
		wxWindow*          pParent,
		wxWindowID         cId,
		const vvRepoLocator& cRepoLocator,
		vvVerbs&        cVerbs,
		vvContext&      cContext,
		bool			   bIgnoreClicks = true,
		bool			   bShowFullHID = false,
		const wxPoint&     cPosition  = wxDefaultPosition,
		const wxSize&      cSize      = wxDefaultSize,
		long               iStyle     = 0,
		const wxValidator&  cValidator = wxDefaultValidator
		);

// functionality
public:
	/**
	 * Creates the control.
	 */
	bool Create(
		wxWindow*          pParent,
		wxWindowID         cId,
		const wxPoint&     cPosition  = wxDefaultPosition,
		const wxSize&      cSize      = wxDefaultSize,
		long               iStyle     = 0,
		const wxValidator&  cValidator = wxDefaultValidator
		);
	
	void SetRevision(long nRevno, wxString& sChangesetHID);

// event handlers
protected:
	void vvRevisionLinkControl::OnClick(wxMouseEvent& evt);
	void vvRevisionLinkControl::OnEnter(wxMouseEvent& evt);
	void vvRevisionLinkControl::OnLeave(wxMouseEvent& evt);
// private data
private:
	wxString m_revHID;
	wxStaticText * m_pST;
	bool m_bIgnoreClicks;
	bool m_bShowFullHID;
	vvRepoLocator m_cRepoLocator;
	vvVerbs*        m_pVerbs;
	vvContext*      m_pContext;
// macro declarations
private:
	vvDISABLE_COPY(vvRevisionLinkControl);
	DECLARE_DYNAMIC_CLASS(vvRevisionLinkControl);

	DECLARE_EVENT_TABLE();
};


#endif
