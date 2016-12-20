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

#ifndef VV_RESOURCE_ART_PROVIDER_H
#define VV_RESOURCE_ART_PROVIDER_H

#include "precompiled.h"


/**
 * A wxArtProvider that loads art from Win32 resources.
 */
class vvResourceArtProvider : public wxArtProvider
{
// functionality
public:
	/**
	 * Loads a bitmap from Win32 resources.
	 */
	wxBitmap CreateBitmap(
		const wxArtID&     cId,
		const wxArtClient& cClient,
		const wxSize&      cSize
		);

	/**
	 * Loads an icon from Win32 resources.
	 */
	wxIconBundle CreateIconBundle(
		const wxArtID&     cId,
		const wxArtClient& cClient
		);

	/**
	* Loads an animated GIF from Win32 resources.
	*/
	static wxAnimation CreateAnimation(
		const wxArtID&     cId
		);
};


#endif
