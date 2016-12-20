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
#include "vvServeDialog.h"

#include "vvVerbs.h"
#include "vvContext.h"
#include "vvValidatorEmptyConstraint.h"
#include "vvValidatorRepoExistsConstraint.h"
#include "wx/spinctrl.h"
#include "wx/hyperlink.h"
#include "vvDialogHeaderControl.h"
#include "vvResourceArtProvider.h"
#include "wx/persist.h"
/*
**
** Globals
**
*/
wxString gsLocalRepoTemplate = "Only serve the \"%s\" repository";
wxString gsNoLocalRepo		 = "Not serving from a working folder";
wxString gsURL				 = "http://localhost:%d";
wxString gsNotServing		 = "Stopped";
wxString gsServing			 = "Now serving at:";

wxString gsServingBitmapName = "Serve__Serving";
wxString gsNotServingBitmapName = "Serve__Not_Serving";

vvPersistentServeDefaults *wxCreatePersistentObject(vvVerbs::ServeDefaults * p)
{
	return new vvPersistentServeDefaults(p);
}

/*
**
** Public Functions
**
*/

vvServeDialog::vvServeDialog()
{
	m_bServing = false;
}

vvServeDialog::~vvServeDialog()
{
	if (m_bServing)
		wxLog::Resume();
}

bool vvServeDialog::Create(
	wxWindow*            pParent,
	vvRepoLocator*		 pRepoLocator,
	vvVerbs&             cVerbs,
	vvContext&           cContext
	)
{
	// create the dialog window
	if (!vvDialog::Create(pParent, &cVerbs, &cContext, this->GetClassInfo()->GetClassName()))
	{
		return false;
	}

	// create our header control
	if (pRepoLocator != NULL)
		wxXmlResource::Get()->AttachUnknownControl("wHeader", new vvDialogHeaderControl(this, wxID_ANY, pRepoLocator->IsWorkingFolder() ? pRepoLocator->GetWorkingFolder() : pRepoLocator->GetRepoName(), cVerbs, cContext));
	else //We're outside a working folder, create and empty header.
		wxXmlResource::Get()->AttachUnknownControl("wHeader", new vvDialogHeaderControl(this, wxID_ANY, wxEmptyString, cVerbs, cContext));
	
	m_pRepoLocator = pRepoLocator;

	m_wPortCtrl		= XRCCTRL(*this, "wPortCtrl",                 wxSpinCtrl);
	m_wPrivate		= XRCCTRL(*this, "wPrivate",				  wxCheckBox);
	m_wLinkCtrl		= XRCCTRL(*this, "wURLLink",				  wxHyperlinkCtrl);
	m_wOnlyThisRepo	= XRCCTRL(*this, "wLocalRepoOnly",			  wxRadioButton);
	m_wAllRepos		= XRCCTRL(*this, "wAllRepos",				  wxRadioButton);
	m_wAnimation    = XRCCTRL(*this, "wStatusAnimation",		  wxAnimationCtrl);
	m_wStart		= XRCCTRL(*this, "wStart",					  wxButton);
	m_wStop			= XRCCTRL(*this, "wStop",					  wxButton);
	m_wStatusText	= XRCCTRL(*this, "wStatusText",				  wxStaticText);

	m_wNotServingAnimation = vvResourceArtProvider::CreateAnimation(gsNotServingBitmapName);
	m_wServingAnimation = vvResourceArtProvider::CreateAnimation(gsServingBitmapName);
	
	//We need to do this here, so that the dialog will use
	//the correct size for the animation control.
	m_wAnimation->SetAnimation(m_wServingAnimation);
	this->Fit();
	this->Layout();
	wxPersistenceManager::Get().RegisterAndRestore(&m_serveDefaults);
	// success
	return true;
}

void vvServeDialog::EnableOptions(bool bEnable)
{
	m_wPrivate->Enable(bEnable);
	if (m_pRepoLocator)
	{
		m_wAllRepos->Enable(bEnable);
		m_wOnlyThisRepo->Enable(bEnable);
	}
	else
	{
		m_wAllRepos->Enable(false);
		m_wOnlyThisRepo->Enable(false);
	}
	m_wPortCtrl->Enable(bEnable);
}

void vvServeDialog::StartServing(wxCommandEvent& event)
{
	event;
	
	wxString sWorkingFolderLocation = wxEmptyString;
	if (m_pRepoLocator && m_wOnlyThisRepo->GetValue())
	{
		sWorkingFolderLocation = m_pRepoLocator->GetWorkingFolder();
	}

	if ( !this->GetVerbs()->StartWebServer(*this->GetContext(), &m_mg_ctx, !m_wPrivate->GetValue(), sWorkingFolderLocation, m_wPortCtrl->GetValue()) )
	{
		this->GetContext()->Error_ResetReport("Could not start the web server");
		StartedOrStoppedServer(false);
		return;
	}

	StartedOrStoppedServer(true);

	//This line needs a bit of explanation.  The server will log LOTS of messages at
	//the error and info level.  Our default logger will elevate any error or info
	//log message to a dialog.  This gets really annoying.  A rough solution is
	//to suspend the wxLog while the server is running.  This is ok as long as
	//each dialog is in a separate process.  If we ever create a single app that loads
	//all the dialogs, we'll need to reevaluate how we do this.
	wxLog::Suspend();
	
	m_bServing = true;
}

void vvServeDialog::StartedOrStoppedServer(bool bStarted)
{

	if (bStarted)
	{
		//Swap the animation.
		m_wAnimation->SetAnimation(m_wServingAnimation);	
	}
	else
		m_wAnimation->SetAnimation(m_wNotServingAnimation);	
	m_wAnimation->Play();

	if (bStarted)
	{
		//Set the link control URL
		wxString newURL;
		newURL.Printf(gsURL, m_wPortCtrl->GetValue());
		m_wLinkCtrl->SetLabel(newURL);
		m_wLinkCtrl->SetURL(newURL);
	}

	m_wLinkCtrl->Show(bStarted);
	
	//The flags need to be reset for alignment purposes.
	if (bStarted)
	{
		m_wStatusText->SetLabel(gsServing);
		m_wStatusText->SetWindowStyleFlag(wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
	}
	else
	{
		m_wStatusText->SetLabel(gsNotServing);
		m_wStatusText->SetWindowStyleFlag(wxALIGN_CENTER_HORIZONTAL | wxST_NO_AUTORESIZE);
	}
	//Swap the buttons
	m_wStart->Enable(!bStarted);
	m_wStop->Enable(bStarted);

	EnableOptions(!bStarted);
	
	//Need to relayout the line so that "stopped" is centered.
	m_wStatusText->GetParent()->Layout();
}
void vvServeDialog::StopServing(wxCommandEvent& event)
{
	event;
	if ( !this->GetVerbs()->StopWebServer(*this->GetContext(), m_mg_ctx) )
	{
		StartedOrStoppedServer(true);
		this->GetContext()->Error_ResetReport("Could not stop the web server");
		return;
	}

	StartedOrStoppedServer(false);
	
	//Read the corresponding comment in StartServing.
	wxLog::Resume();
	m_bServing = false;
}

bool vvServeDialog::TransferDataToWindow()
{
	wxString currentLabel = m_wOnlyThisRepo->GetLabelText();
	if (m_pRepoLocator == NULL)
	{
		currentLabel = gsNoLocalRepo;
		m_wAllRepos->SetValue(true);
		m_wOnlyThisRepo->SetValue(false);
		m_wOnlyThisRepo->Disable();
	}
	else
	{
		currentLabel.Printf(gsLocalRepoTemplate, m_pRepoLocator->GetRepoName());
	}
	m_wOnlyThisRepo->SetLabelText(currentLabel);

	m_wPortCtrl->SetValue(m_serveDefaults.nPortNumber);
	m_wPrivate->SetValue(!m_serveDefaults.bPublic);
	m_wAllRepos->SetValue(m_serveDefaults.bServeAllRepos);

	wxCommandEvent blah;
	StartServing(blah);
	return true;
}

bool vvServeDialog::ConfirmClose()
{
	if (m_bServing)
	{
		wxMessageDialog * msg = new wxMessageDialog(this, "Are you sure that you want to stop the web server?", "Stop Web Server?", wxYES_NO | wxNO_DEFAULT);
		if (msg->ShowModal() == wxID_YES)
		{
			wxCommandEvent blah;
			StopServing(blah);
			return true;
		}
		else
			return false;
	}
	return true;

}

void vvServeDialog::OnCancel(wxCommandEvent& e)
{
	if (ConfirmClose())
	{
		//Save the settings.
		m_serveDefaults.bPublic = !m_wPrivate->GetValue();
		m_serveDefaults.nPortNumber = m_wPortCtrl->GetValue();
		m_serveDefaults.bServeAllRepos = m_wAllRepos->GetValue();
	
		wxPersistenceManager::Get().SaveAndUnregister(&m_serveDefaults);

		e.Skip();
	}	
}

bool vvServeDialog::TransferDataFromWindow()
{
	if (!ConfirmClose())
		return false;

	m_serveDefaults.bPublic = !m_wPrivate->GetValue();
	m_serveDefaults.nPortNumber = m_wPortCtrl->GetValue();
	m_serveDefaults.bServeAllRepos = m_wAllRepos->GetValue();
	
	wxPersistenceManager::Get().SaveAndUnregister(&m_serveDefaults);
	return true;
}
IMPLEMENT_DYNAMIC_CLASS(vvServeDialog, vvDialog);
BEGIN_EVENT_TABLE(vvServeDialog, vvDialog)
	EVT_BUTTON( XRCID("wStart"),    vvServeDialog::StartServing)
	EVT_BUTTON( XRCID("wStop"),		vvServeDialog::StopServing)
	EVT_BUTTON(wxID_CANCEL,			vvServeDialog::OnCancel)
END_EVENT_TABLE()
