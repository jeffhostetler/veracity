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

#ifndef VV_SERVE_DIALOG_H
#define VV_SERVE_DIALOG_H

#include "precompiled.h"
#include "vvDialog.h"
#include "vvValidatorMessageBoxReporter.h"
#include "vvRepoLocator.h"
#include "wx/hyperlink.h"
#include "vvVerbs.h"
#include "wx/persist.h"



class vvPersistentServeDefaults : public wxPersistentObject
{
public:
	vvPersistentServeDefaults(vvVerbs::ServeDefaults * pDefaults) : wxPersistentObject(pDefaults)
	{
	}
	virtual wxString GetName() const
	{
		return "ServeDialogDefaults";
	}

	virtual wxString GetKind() const
	{
		return "vvServeDialogDefaults";
	}

	virtual void Save() const
	{
		vvVerbs::ServeDefaults * pcb = (vvVerbs::ServeDefaults*)GetObject();
		SaveValue("port", pcb->nPortNumber);
		SaveValue("public", pcb->bPublic);
		SaveValue("serve_all_repos", pcb->bServeAllRepos);
	}

	virtual bool Restore()
	{
		vvVerbs::ServeDefaults * pcb = (vvVerbs::ServeDefaults*)GetObject();
		if (!RestoreValue("port", &pcb->nPortNumber))
			pcb->nPortNumber = 8080;
		if (!RestoreValue("public", &pcb->bPublic))
			pcb->bPublic = false;
		if (!RestoreValue("serve_all_repos", &pcb->bServeAllRepos))
			pcb->bServeAllRepos = false;
		return true;
	}
};


/**
 * This dialog is responsible for popping up
 * a wxTextEntryDialog.
 */
class vvServeDialog : public vvDialog
{
// functionality
public:

	vvServeDialog();
	~vvServeDialog();

	/**
	 * Creates the actual dialog window.
	 */
	virtual bool Create(
		wxWindow*            pParent,                         //< [in] The new dialog's parent window, or NULL if it won't have one.
		vvRepoLocator *		 pRepoLocator,					  //< [in] The repo that will be served.  Pass NULL to serve all repos.
		class vvVerbs&       cVerbs,                          //< [in] The vvVerbs implementation to retrieve local repo and config info from.
		class vvContext&     cContext                         //< [in] Context to use with pVerbs.
		);

	virtual bool TransferDataToWindow();
	virtual bool TransferDataFromWindow();

protected:
	void OnCancel(wxCommandEvent& e);
private:
	void EnableOptions(bool bEnable);
	void StartedOrStoppedServer(bool bStarted);
	void StartServing(wxCommandEvent& event);
	void StopServing(wxCommandEvent& event);
	bool ConfirmClose();
// private data
private:
	// internal data
	wxSpinCtrl * m_wPortCtrl; 
	wxHyperlinkCtrl * m_wLinkCtrl;
	vvRepoLocator * m_pRepoLocator;
	wxRadioButton * m_wOnlyThisRepo;
	wxRadioButton * m_wAllRepos;
	wxCheckBox * m_wPrivate;
	wxButton * m_wStart;
	wxButton * m_wStop;
	wxStaticText * m_wStatusText;
	wxAnimationCtrl * m_wAnimation;
	wxAnimation m_wServingAnimation;
	wxAnimation m_wNotServingAnimation;
	bool m_bServing;
	struct mg_context * m_mg_ctx;
	vvVerbs::ServeDefaults m_serveDefaults;
	vvPersistentServeDefaults * m_pPersistentServeDefaults;


// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvServeDialog);
	DECLARE_EVENT_TABLE();
};

#endif
