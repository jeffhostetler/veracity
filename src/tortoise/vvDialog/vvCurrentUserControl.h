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

#ifndef VV_CURRENT_USER_CONTROL_H
#define VV_CURRENT_USER_CONTROL_H

#include "precompiled.h"


/**
 * A control that displays the current user.
 * If there is no current user, a link that spawns a vvConfigDialog is displayed instead.
 */
class vvCurrentUserControl : public wxPanel
{
// constants
public:
	static const wxString ssDefaultLinkText; //< The default text for the config dialog link.

// construction
public:
	/**
	 * Default constructor.
	 */
	vvCurrentUserControl();

	/**
	 * Create constructor.
	 */
	vvCurrentUserControl(
		wxWindow*                  pParent,                       //< [in] The control's parent window.
		wxWindowID                 cId,                           //< [in] The control's ID.
		const class vvRepoLocator* pSource   = NULL,              //< [in] The working copy or repo to display the current user from.
		                                                          //<      If NULL, the control will not be populated.
		class vvVerbs*             pVerbs    = NULL,              //< [in] The vvVerbs implementation to use to get the current user.
		                                                          //<      If NULL, the control will not be populated.
		class vvContext*           pContext  = NULL,              //< [in] The vvContext to use with cVerbs.
		                                                          //<      If NULL, the control will not be populated.
		const wxString&            sLinkText = ssDefaultLinkText, //< [in] The text to display for the config dialog link.
		const wxPoint&             cPosition = wxDefaultPosition, //< [in] The control's position.
		const wxSize&              cSize     = wxDefaultSize,     //< [in] The control's size.
		long                       iStyle    = 0                  //< [in] The control's style.
		);

// functionality
public:
	/**
	 * Creates and populates the control.
	 */
	bool Create(
		wxWindow*                  pParent,                       //< [in] The control's parent window.
		wxWindowID                 cId,                           //< [in] The control's ID.
		const class vvRepoLocator* pSource   = NULL,              //< [in] The working copy or repo to display the current user from.
		                                                          //<      If NULL, the control will not be populated.
		class vvVerbs*             pVerbs    = NULL,              //< [in] The vvVerbs implementation to use to get the current user.
		                                                          //<      If NULL, the control will not be populated.
		class vvContext*           pContext  = NULL,              //< [in] The vvContext to use with cVerbs.
		                                                          //<      If NULL, the control will not be populated.
		const wxString&            sLinkText = ssDefaultLinkText, //< [in] The text to display for the config dialog link.
		const wxPoint&             cPosition = wxDefaultPosition, //< [in] The control's position.
		const wxSize&              cSize     = wxDefaultSize,     //< [in] The control's size.
		long                       iStyle    = 0                  //< [in] The control's style.
		);

	/**
	 * Populates the control with the current user in a specified repo.
	 */
	bool Populate(
		const class vvRepoLocator& cSource, //< [in] The working copy or repo to display the current user from.
		class vvVerbs&             cVerbs,  //< [in] The vvVerbs implementation to use to get the current user.
		class vvContext&           cContext //< [in] The vvContext to use with cVerbs.
		);

	/**
	 * Returns whether or not there is a current user being displayed.
	 * If false, then the config dialog link is being displayed.
	 */
	bool HasCurrentUser() const;

// event handlers
protected:
	void OnLink(wxHyperlinkEvent& e);

// private data
private:
	// data
	wxString   msRepo;    //< The name of the repo that we retrieve the current user from.
	vvVerbs*   mpVerbs;   //< The vvVerbs we use to look up the current user.
	vvContext* mpContext; //< The context we use with mpVerbs.

	// widgets
	class vvSelectableStaticText* mwText; //< The text control displaying the current username.
	wxHyperlinkCtrl*              mwLink; //< The link that spawns a config dialog when there is no current user.

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvCurrentUserControl);
	DECLARE_EVENT_TABLE();
};


#endif
