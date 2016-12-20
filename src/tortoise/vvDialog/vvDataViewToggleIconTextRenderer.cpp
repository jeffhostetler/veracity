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
#include "vvDataViewToggleIconTextRenderer.h"

#include "vvFlags.h"


/*
**
** Public Functions
**
*/

// ----- vvDataViewToggleIconText -----

vvDataViewToggleIconText::vvDataViewToggleIconText(
	const wxString&  sText,
	const wxIcon&    cIcon,
	vvNullable<bool> nbChecked,
	bool bPermanentDisable
	)
	: wxDataViewIconText(sText, cIcon)
	, mbPermanentDiasble(bPermanentDisable)
	, mnbChecked(nbChecked)
{
	if (mbPermanentDiasble && !mnbChecked.IsNull())
		this->mnbChecked.SetValue(false);
}

vvDataViewToggleIconText::vvDataViewToggleIconText(
	const vvDataViewToggleIconText& that
	)
	: wxDataViewIconText(that)
	, mbPermanentDiasble(that.mbPermanentDiasble)
	, mnbChecked(that.mbPermanentDiasble ? false : that.mnbChecked)
{
	if (mbPermanentDiasble && !mnbChecked.IsNull())
		this->mnbChecked.SetValue(false);
}

void vvDataViewToggleIconText::SetChecked(
	vvNullable<bool> nbChecked
	)
{
	if (!mbPermanentDiasble)
		this->mnbChecked = nbChecked;
	else
		this->mnbChecked = false;
}

vvNullable<bool> vvDataViewToggleIconText::GetChecked() const
{
	return this->mnbChecked;
}

bool vvDataViewToggleIconText::GetPermanentDisable() const
{
	return this->mbPermanentDiasble;
}

void vvDataViewToggleIconText::Reset()
{
	this->SetText(wxEmptyString);
	this->SetIcon(wxNullIcon);
	this->mnbChecked = false;
}

IMPLEMENT_DYNAMIC_CLASS(vvDataViewToggleIconText, wxDataViewIconText);
IMPLEMENT_VARIANT_OBJECT(vvDataViewToggleIconText);

// ----- vvDataViewToggleIconTextRenderer -----

vvDataViewToggleIconTextRenderer::vvDataViewToggleIconTextRenderer(
	const wxString&    sVariantType,
	wxDataViewCellMode eMode,
	int                iAlign
	)
	: wxDataViewCustomRenderer(sVariantType, eMode, iAlign)
{
	wxASSERT(sVariantType == "vvDataViewToggleIconText" || sVariantType == "wxDataViewIconText" || sVariantType == "bool" || sVariantType == "string" || sVariantType == "wxIcon");
}

bool vvDataViewToggleIconTextRenderer::SetValue(
	const wxVariant& cValue
	)
{
	if (cValue.GetType() == "vvDataViewToggleIconText")
	{
		this->mcValue.Reset();
		this->mcValue << cValue;
		return true;
	}
	else if (cValue.GetType() == "wxDataViewIconText")
	{
		this->mcValue.Reset();
		wxDataViewIconText cTemp;
		cTemp << cValue;
		this->mcValue.SetText(cTemp.GetText());
		this->mcValue.SetIcon(cTemp.GetIcon());
		return true;
	}
	else if (cValue.GetType() == "bool")
	{
		this->mcValue.Reset();
		this->mcValue.SetChecked(cValue.GetBool());
		return true;
	}
	else if (cValue.GetType() == "string")
	{
		this->mcValue.Reset();
		this->mcValue.SetText(cValue.GetString());
		return true;
	}
	else if (cValue.GetType() == "wxIcon")
	{
		this->mcValue.Reset();
		wxIcon cTemp;
		cTemp << cValue;
		this->mcValue.SetIcon(cTemp);
		return true;
	}
	else
	{
		return false;
	}
}

bool vvDataViewToggleIconTextRenderer::GetValue(
	wxVariant& cValue
	) const
{
	if (this->GetVariantType() == "vvDataViewToggleIconText")
	{
		cValue << this->mcValue;
		return true;
	}
	else if (this->GetVariantType() == "wxDataViewIconText")
	{
		wxDataViewIconText cTemp;
		cTemp.SetText(this->mcValue.GetText());
		cTemp.SetIcon(this->mcValue.GetIcon());
		cValue << cTemp;
		return true;
	}
	else if (this->GetVariantType() == "bool")
	{
		cValue = this->mcValue.GetChecked();
		return true;
	}
	else if (this->GetVariantType() == "string")
	{
		cValue = this->mcValue.GetText();
		return true;
	}
	else if (this->GetVariantType() == "wxIcon")
	{
		cValue << this->mcValue.GetIcon();
		return true;
	}
	else
	{
		return false;
	}
}

wxSize vvDataViewToggleIconTextRenderer::GetSize() const
{
	wxSize cCheckBoxSize = wxRendererNative::Get().GetCheckBoxSize(NULL);
	wxSize cIconSize(0, 0);
	wxSize cTextSize(0, 0);
	int    iPadding = 0;

	if (this->mcValue.GetIcon().IsOk())
	{
		cIconSize = this->mcValue.GetIcon().GetSize();
		iPadding += siPadding;
	}
	if (!this->mcValue.GetText().IsEmpty())
	{
		cTextSize = this->GetView()->GetTextExtent(this->mcValue.GetText());
		iPadding += siPadding;
	}

	int iWidth  = cCheckBoxSize.GetWidth() + cIconSize.GetWidth() + cTextSize.GetWidth() + iPadding;
	int iHeight = wxMax(cCheckBoxSize.GetHeight(), cTextSize.GetHeight());
	return wxSize(iWidth, iHeight);
}

bool vvDataViewToggleIconTextRenderer::Render(
	wxRect cRect,
	wxDC*  pDc,
	int    iState
	)
{
	// setup appropriate state flags for the checkbox
	vvFlags cFlags = 0;
	if (this->mcValue.GetChecked().IsNull())
	{
		cFlags.AddFlags(wxCONTROL_UNDETERMINED);
	}
	else if (this->mcValue.GetChecked().GetValue())
	{
		cFlags.AddFlags(wxCONTROL_CHECKED);
	}
	/*if (this->GetMode() != wxDATAVIEW_CELL_ACTIVATABLE)
	{
		cFlags.AddFlags(wxCONTROL_DISABLED);
	}*/
	else if (this->mcValue.GetPermanentDisable())
	{
		cFlags.AddFlags(wxCONTROL_DISABLED);
	}

	// draw the checkbox
	wxRendererNative::Get().DrawCheckBox(this->GetOwner()->GetOwner(), *pDc, this->GetCheckBoxRect(cRect), cFlags);

	// draw the icon
	if (this->mcValue.GetIcon().IsOk())
	{
		pDc->DrawIcon(this->mcValue.GetIcon(), this->GetIconRect(cRect).GetPosition());
	}

	// draw the text
	this->RenderText(this->mcValue.GetText(), 0, this->GetTextRect(cRect), pDc, iState);

	// done
	return true;
}

bool vvDataViewToggleIconTextRenderer::Activate(
	wxRect                WXUNUSED(cRect),
	wxDataViewModel*      pModel,
	const wxDataViewItem& cItem,
	unsigned int          uColumn
	)
{
	bool bChecked = true;
	if (this->mcValue.GetPermanentDisable())
		return true;
	if (this->mcValue.GetChecked().GetValue(bChecked))
	{
		bChecked = !bChecked;
	}
	this->mcValue.SetChecked(bChecked);

	wxVariant cValue;
	if (!this->GetValue(cValue))
	{
		return false;
	}
	pModel->ChangeValue(cValue, cItem, uColumn);
	return true;
}

bool vvDataViewToggleIconTextRenderer::LeftClick(
	wxPoint               cPoint,
	wxRect                cRect,
	wxDataViewModel*      pModel,
	const wxDataViewItem& cItem,
	unsigned int          uColumn
	)
{
	// if the click point wasn't in the given rectangle
	// then a different renderer/cell should have been called
//	wxASSERT(cRect.Contains(cPoint));
	// This doesn't work when the view is scrolled,
	// because cPoint is scrolled and cRect is unscrolled.

	// HACK: The wxDataViewCtrl isn't giving us the same rect that we rendered to.
	//       We need that render rect in order to compare the click to the controls we drew.
	//       So we need to reconstruct it before we can find the check box rect.
	if (this->GetCheckBoxRect(this->GetRenderRect(cItem)).Contains(cPoint))
	{
		return this->Activate(cRect, pModel, cItem, uColumn);
	}
	else
	{
		return false;
	}
}

wxRect vvDataViewToggleIconTextRenderer::GetRenderRect(
	const wxDataViewItem& cItem
	) const
{
	// taken from PADDING_RIGHTLEFT in datavgen.cpp line ~67
	static const int iHorizontalPadding = 3;

	// start with the item's rect, as reported by our owner control
	// this rect accounts for the expander, if this is the expander column
	wxRect cItemRect = this->GetOwner()->GetOwner()->GetItemRect(cItem, this->GetOwner());

	// deflate the rect to account for padding on the left/right
	cItemRect.Deflate(iHorizontalPadding, 0);

	// align our render size in the rect, according to our alignment
	// adapted from wxDataViewCustomRendererBase::WXCallRender line ~742
	int iAlignment = this->GetAlignment();
	if (iAlignment != wxDVR_DEFAULT_ALIGNMENT)
	{
		wxSize cRenderSize = this->GetSize();

		if (0 <= cRenderSize.GetWidth() && cRenderSize.GetWidth() < cItemRect.GetWidth())
		{
			if (iAlignment & wxALIGN_CENTER_HORIZONTAL)
			{
				cItemRect.x += (cItemRect.GetWidth() - cRenderSize.GetWidth()) / 2;
			}
			else if (iAlignment & wxALIGN_RIGHT)
			{
				cItemRect.x += cItemRect.GetWidth() - cRenderSize.GetWidth();
			}
			cItemRect.width = cRenderSize.GetWidth();
		}

		if (0 <= cRenderSize.GetHeight() && cRenderSize.GetHeight() < cItemRect.GetHeight())
		{
			if (iAlignment & wxALIGN_CENTER_VERTICAL)
			{
				cItemRect.y += (cItemRect.GetHeight() - cRenderSize.GetHeight()) / 2;
			}
			else if (iAlignment & wxALIGN_BOTTOM)
			{
				cItemRect.y += cItemRect.GetHeight() - cRenderSize.GetHeight();
			}
			cItemRect.height = cRenderSize.GetHeight();
		}
	}

	// that's it
	return cItemRect;
}

wxRect vvDataViewToggleIconTextRenderer::GetCheckBoxRect(
	const wxRect& cRect
	) const
{
	wxSize cCheckBoxSize = wxRendererNative::Get().GetCheckBoxSize(NULL);
	return wxRect(
		cRect.GetX(),
		cRect.GetY() - ((cCheckBoxSize.GetHeight() - cRect.GetHeight()) / 2),
		cCheckBoxSize.GetWidth(),
		cCheckBoxSize.GetHeight()
		);
}

wxRect vvDataViewToggleIconTextRenderer::GetIconRect(
	const wxRect& cRect
	) const
{
	return wxRect(
		cRect.GetX() + this->GetCheckBoxRect(cRect).GetWidth() + siPadding,
		cRect.GetY() - ((this->mcValue.GetIcon().GetHeight() - cRect.GetHeight()) / 2),
		this->mcValue.GetIcon().IsOk() ? this->mcValue.GetIcon().GetWidth() : 0,
		cRect.GetHeight()
		);
}

wxRect vvDataViewToggleIconTextRenderer::GetTextRect(
	const wxRect& cRect
	) const
{
	int iOffset = 0;
	iOffset += this->GetCheckBoxRect(cRect).GetWidth();
	iOffset += siPadding;
	if (this->mcValue.GetIcon().IsOk())
	{
		iOffset += this->GetIconRect(cRect).GetWidth();
		iOffset += siPadding;
	}

	return wxRect(
		cRect.GetX() + iOffset,
		cRect.GetY() - ((this->GetView()->GetTextExtent(this->mcValue.GetText()).GetHeight() - cRect.GetHeight()) / 2),
		wxMax(0, cRect.GetWidth() - iOffset),
		cRect.GetHeight()
		);
}

IMPLEMENT_CLASS(vvDataViewToggleIconTextRenderer, wxDataViewCustomRenderer)
