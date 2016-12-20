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

#ifndef VV_AUDIT_DETAILS_CONTROL_H
#define VV_AUDIT_DETAILS_CONTROL_H

#include "precompiled.h"
#include "vvCppUtilities.h"
#include "vvNullable.h"
#include "vvVerbs.h"

/**
 * A custom wxDataViewCtrl for displaying and choosing history.
 */
class vvAuditDetailsControl : public wxPanel
{
// types
public:

// construction
public:
	/**
	 * Default constructor.
	 */
	vvAuditDetailsControl();

	/**
	 * Create contructor.
	 */
	vvAuditDetailsControl(
		vvVerbs& cVerbs,
		vvContext& cContext,
		vvVerbs::AuditEntry auditEntry,
		wxString sComment,
		wxWindow*          pParent,
		wxWindowID         cId,
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
	
// event handlers
protected:

// private data
private:
	vvContext* m_pContext;
	vvVerbs*   m_pVerbs;
// macro declarations
private:
	vvDISABLE_COPY(vvAuditDetailsControl);
	DECLARE_DYNAMIC_CLASS(vvAuditDetailsControl);

	DECLARE_EVENT_TABLE();
};


#endif
