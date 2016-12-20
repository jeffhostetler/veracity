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
#include "vvResourceArtProvider.h"


/*
**
** Types
**
*/

/**
 * A single icon image entry within a Win32 RT_GROUP_ICON resource.
 * This structure isn't defined in windows.h for some reason.
 * http://msdn.microsoft.com/en-us/library/ms997538.aspx
 */
#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
   BYTE   uWidth;    // width in pixels
   BYTE   uHeight;   // height in pixels
   BYTE   uColors;   // color count (0 if uBitCount >= 8)
   BYTE   uReserved; // reserved (always 0)
   WORD   uPlanes;   // color plane count
   WORD   uBitCount; // bits per pixel
   DWORD  uBytes;    // size of the image in bytes
   WORD   uId;       // resource ID for this entry's icon (RT_ICON resource)
}
GRPICONDIRENTRY;
#pragma pack( pop )

/**
 * The binary data at the top of a Win32 RT_GROUP_ICON resource.
 * This structure isn't defined in windows.h for some reason.
 * http://msdn.microsoft.com/en-us/library/ms997538.aspx
 */
#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
	WORD            uReserved;   // reserved (must be 0)
	WORD            uType;       // type (1 for icons, 2 for cursors)
	WORD            uCount;      // number of entries
	GRPICONDIRENTRY aEntries[1]; // array of uCount entries
}
GRPICONDIR;
#pragma pack( pop )


/*
**
** Globals
**
*/

/**
 * The version number of the icon file format that we assume is in the resources.
 * I picked this one because the docs for CreateIconFromResource said it's what is generally used.
 * http://msdn.microsoft.com/en-us/library/ms648060%28VS.85%29.aspx
 */
static const DWORD guIconFileFormat = 0x30000;

/**
 * The resource type used in .rc files to specify embedded .png files.
 */
static const LPCTSTR gszPngResourceType = _T("PNG");


/*
**
** Internal Functions
**
*/

/**
 * Loads a resource icon by its HRSRC handle.
 * Returns the loaded icon, or wxNullIcon on failure (just call Ok() on the return value to check).
 */
static wxIcon _LoadIconByHandle(
	HRSRC hResource //< [in] The handle of the icon to load.
	)
{
	// get a handle to the resource's data
	HGLOBAL hData = LoadResource(wxGetInstance(), hResource);
	if (hData == NULL)
	{
		return wxNullIcon;
	}
	
	// get the actual resource data and its size
	// Note: pData is usually of type "ICONIMAGE".
	//       This type isn't in windows.h for some reason, but for more info see
	//       http://msdn.microsoft.com/en-us/library/ms997538.aspx
	//       It would appear, however, that not all icon resources use this format.
	//       The standard TortoiseOverlay icons include a 256x256x32 image
	//       that Visual Studio says is PNG instead of BMP.
	//       Trying to interpret that image's pData as an ICONIMAGE fails spectacularly.
	//       I would imagine it could be interpreted as PNG data, instead.
	//       Oddly, CreateIconFromResourceEx seems perfectly happy to build a valid icon from either format.
	//       Point is that we end up not needing to read anything pointed to by pData ourselves.
	//       Therefore, I'm just leaving it as a generic byte pointer.
	PBYTE pData = (PBYTE)LockResource(hData);
	DWORD uSize = SizeofResource(wxGetInstance(), hResource);

	// create a Windows icon from the resource data
	// Note: It's important to use the Ex version of CreateIconFromResource here.
	//       The non-Ex version always creates icons of size "System Large" (usually 32x32).
	//       However, we want the created icon to be its native size.
	//       To get the Ex version to do this, we just specify zeroes for the desired height and width.
	HICON hIcon = CreateIconFromResourceEx(pData, uSize, TRUE, guIconFileFormat, 0, 0, 0u);
	if (hIcon == NULL)
	{
		return wxNullIcon;
	}

	// create a wxIcon instance from the Windows icon handle
	// I copied this from wxICOResourceHandler::LoadIcon in gdiimage.cpp
	wxIcon cIcon;
	cIcon.SetSize(wxGetHiconSize(hIcon));
	cIcon.SetHICON((WXHICON)hIcon);
	if (!cIcon.Ok())
	{
		return wxNullIcon;
	}

	// return the icon
	return cIcon;
}

/**
 * Loads a resource icon by its resource ID.
 * Returns the loaded icon, or wxNullIcon on failure (just call Ok() on the return value to check).
 */
static wxIcon _LoadIconById(
	unsigned int uId //< [in] The resource ID of the icon to load.
	)
{
	HRSRC hResource = FindResource(wxGetInstance(), MAKEINTRESOURCE(uId), RT_ICON);
	if (hResource == NULL)
	{
		return wxNullIcon;
	}
	return _LoadIconByHandle(hResource);
}

/**
 * Loads a resource icon by its resource name.
 * Returns the loaded icon, or wxNullIcon on failure (just call Ok() on the return value to check).
 */
static wxIcon _LoadIconByName(
	const wxString& sName //< [in] The resource name of the icon to load.
	)
{
	HRSRC hResource = FindResource(wxGetInstance(), sName.wc_str(), RT_ICON);
	if (hResource == NULL)
	{
		return wxNullIcon;
	}
	return _LoadIconByHandle(hResource);
}

/**
 * Loads a resource image by its resource name.
 * Returns the loaded bitmap, or wxNullBitmap on failure (just call Ok() on the return value to check).
 */
static wxBitmap _LoadImageByName(
	const wxString& sName,          //< [in] The name of the resource to load.
	LPCTSTR         szResourceType, //< [in] The type of resource, in the Win32 sense (RT_* or a string name).
	wxBitmapType    eImageType      //< [in] The wxBitmapType to load the image as.
	)
{
	HRSRC hResource = FindResource(wxGetInstance(), sName.wc_str(), szResourceType);
	if (hResource == NULL)
	{
		return wxNullBitmap;
	}

	HGLOBAL hData = LoadResource(wxGetInstance(), hResource);
	if (hData == NULL)
	{
		return wxNullBitmap;
	}

	LPVOID pData = LockResource(hData);
	DWORD  uSize = SizeofResource(wxGetInstance(), hResource);
	wxMemoryInputStream cStream(pData, uSize);
	wxImage cImage(cStream, eImageType);
	if (!cImage.Ok())
	{
		return wxNullBitmap;
	}

	return wxBitmap(cImage);
}


/*
**
** Public Functions
**
*/

wxBitmap vvResourceArtProvider::CreateBitmap(
	const wxArtID&     cId,
	const wxArtClient& WXUNUSED(cClient),
	const wxSize&      WXUNUSED(cSize)
	)
{
	wxBitmap cBitmap;

	// try loading it as a BMP
	cBitmap = _LoadImageByName(cId, RT_BITMAP, wxBITMAP_TYPE_BMP_RESOURCE);
	if (cBitmap.Ok())
	{
		return cBitmap;
	}

	// try loading it as a PNG
	cBitmap = _LoadImageByName(cId, gszPngResourceType, wxBITMAP_TYPE_PNG);
	if (cBitmap.Ok())
	{
		return cBitmap;
	}

	// I guess we don't have one
	return wxNullBitmap;
}

wxIconBundle vvResourceArtProvider::CreateIconBundle(
	const wxArtID&     cId,
	const wxArtClient& WXUNUSED(cClient)
	)
{
	wxIconBundle cIcons;

	// find the requested resource as an icon group
	HRSRC hResource = FindResource(wxGetInstance(), cId.wc_str(), RT_GROUP_ICON);
	if (hResource == NULL)
	{
		// if we didn't find a group, try loading a single icon
		wxIcon cIcon = _LoadIconByName(cId);
		if (cIcon.Ok())
		{
			cIcons.AddIcon(cIcon);
		}
		return cIcons;
	}

	// load the resource's data
	HGLOBAL hData = LoadResource(wxGetInstance(), hResource);
	if (hData == NULL)
	{
		return cIcons;
	}

	// get the resource's raw data, which will be a GRPICONDIR
	GRPICONDIR* pDirectory = (GRPICONDIR*)LockResource(hData);

	// loop over each icon in the group, load it, and add it to the bundle
	for (unsigned int uIndex = 0u; uIndex < pDirectory->uCount; ++uIndex)
	{
		GRPICONDIRENTRY* pEntry = pDirectory->aEntries + uIndex;
		wxIcon cIcon = _LoadIconById(pEntry->uId);
		if (cIcon.Ok())
		{
			cIcons.AddIcon(cIcon);
		}
	}

	// return the bundle
	return cIcons;
}

wxAnimation vvResourceArtProvider::CreateAnimation(
	const wxArtID&     cId
	)
{
	size_t resourceLen = 0;
	const void * buf;
	wxAnimation returnAnim;
	wxLoadUserResource(&buf, &resourceLen, cId, wxString("GIF"));
	wxMemoryInputStream memServing(buf, resourceLen);
	if (!returnAnim.Load(memServing))
	{
		wxLogError("Error loading GIF: %d",(int)memServing.GetLastError());
		return wxNullAnimation;
	}
	return returnAnim;
}
