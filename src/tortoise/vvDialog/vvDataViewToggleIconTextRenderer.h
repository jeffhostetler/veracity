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

#ifndef VV_DATA_VIEW_TOGGLE_ICON_TEXT_RENDERER_H
#define VV_DATA_VIEW_TOGGLE_ICON_TEXT_RENDERER_H

#include "precompiled.h"

#include "vvNullable.h"


/**
 * Data displayed used by the vvDataViewToggleIconTextRenderer.
 */
class vvDataViewToggleIconText : public wxDataViewIconText
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvDataViewToggleIconText(
		const wxString&  sText     = wxEmptyString, //< [in] The value for the string.
		const wxIcon&    cIcon     = wxNullIcon,    //< [in] The value for the icon.
		vvNullable<bool> nbChecked = false,         //< [in] The value for the toggle.
		bool bPermanentDisable = false				//< [in] This cell should be rendered as disabled.
		);

	/**
	 * Copy constructor
	 */
	vvDataViewToggleIconText(
		const vvDataViewToggleIconText& that
		);

// functionality
public:
	/**
	 * Sets the value of the toggle.
	 */
	void SetChecked(
		vvNullable<bool> nbChecked //< [in] The new value for the toggle.
		);

	/**
	 * Gets the value of the toggle.
	 */
	vvNullable<bool> GetChecked() const;

	/**
	 * This cell was permanently disabled.
	 */
	bool GetPermanentDisable() const;
	
	/**
	 * Resets the data back to its default empty state.
	 */
	void Reset();

// private data
private:
	vvNullable<bool> mnbChecked; //< The value of the toggle.
	bool mbPermanentDiasble;  //<The checkbox should be disbled.
// macro declarations
private:
	DECLARE_DYNAMIC_CLASS(vvDataViewToggleIconText);
};


/**
 * Declare an object for storing vvDataViewToggleIconText instances in a wxVariant.
 */
DECLARE_VARIANT_OBJECT(vvDataViewToggleIconText);


/**
 * A custom data view renderer that renders a checkbox, icon, and text.
 * Uses data of type vvDataViewToggleIconText.
 */
class vvDataViewToggleIconTextRenderer : public wxDataViewCustomRenderer
{
// constants
public:
	/**
	 * Amount of padding between the checkbox, icon, and text (in pixels).
	 */
	static const int siPadding = 4;

// construction
public:
	/**
	 * Default constructor.
	 */
	vvDataViewToggleIconTextRenderer(
		const wxString&    sVariantType = "vvDataViewToggleIconText", //< [in] Variant type of values to render.
		wxDataViewCellMode eMode        = wxDATAVIEW_CELL_INERT,      //< [in] Mode to render.
		int                iAlign       = wxDVR_DEFAULT_ALIGNMENT     //< [in] Alignment to use.
		);

// implementation
public:
	/**
	 * Sets the value to render.
	 */
	virtual bool SetValue(
		const wxVariant& cValue //< [in] The value to set.
		);

	/**
	 * Gets the value being rendered.
	 */
	virtual bool GetValue(
		wxVariant& cValue //< [out] The retrieved value.
		) const;

	/**
	 * Gets the size necessary to render the value.
	 */
	virtual wxSize GetSize() const;

	/**
	 * Renders the value.
	 */
	virtual bool Render(
		wxRect cRect, //< [in] The rect to render the value in.
		wxDC*  pDc,   //< [in] The DC to use to render the value.
		int    iState //< [in] Current state flags for the cell (such as selected or not).
		);

	/**
	 * Called when the user double-clicks the area rendered by the renderer.
	 * Should only be called if the column is flagged as activatable.
	 */
	virtual bool Activate(
		wxRect                cRect,  //< [in] Rectangle of the activated cell, relative to ?.
		wxDataViewModel*      pModel, //< [in] The data model being rendered.
		const wxDataViewItem& cItem,  //< [in] The data item that was activated.
		unsigned int          uColumn //< [in] The model column that was activated.
		);

	/**
	 * Called when the user left-clicks the area rendererd by the renderer.
	 */
	virtual bool LeftClick(
		wxPoint               cPoint, //< [in] The point that was clicked, relative to the wxDataView's scrolled main area.
		wxRect                cRect,  //< [in] Rectangle of the activated cell, relative to the wxDataView's unscrolled main area.
		wxDataViewModel*      pModel, //< [in] The data model being rendered.
		const wxDataViewItem& cItem,  //< [in] The data item that was clicked.
		unsigned int          uColumn //< [in] The model column that was clicked.
		);

// private functionality
private:
	/**
	 * Gets the wxRect used to render a given item, relative to the wxDataView's scrolled main area.
	 * This is necessary for Activate and LeftClick,
	 * because the wxRect passed to them bounds the entire item, not just the rendered area.
	 * This is basically a HACK because wxWidgets doesn't provide the needed data.
	 */
	wxRect GetRenderRect(
		const wxDataViewItem& cItem //< [in] The item to get the render rect for.
		) const;

	/**
	 * Calculates the rect for the checkbox from the total rect we have to render in.
	 */
	wxRect GetCheckBoxRect(
		const wxRect& cRect //< [in] The render rect to get the check box portion of.
		) const;

	/**
	 * Calculates the rect for the icon from the total rect we have to render in.
	 */
	wxRect GetIconRect(
		const wxRect& cRect //< [in] The render rect to get the icon portion of.
		) const;

	/**
	 * Calculates the rect for the text from the total rect we have to render in.
	 */
	wxRect GetTextRect(
		const wxRect& cRect //< [in] The render rect to get the text portion of.
		) const;

// private data
private:
	vvDataViewToggleIconText mcValue; //< The value being rendered.

// macro declarations
private:
	DECLARE_DYNAMIC_CLASS_NO_COPY(vvDataViewToggleIconTextRenderer);
};


#endif
