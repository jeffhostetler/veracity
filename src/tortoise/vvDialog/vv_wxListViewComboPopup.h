
#ifndef _VV_LISTVIEWCOMBOBOX_H_
#define _VV_LISTVIEWCOMBOBOX_H_
#include <wx/combo.h>
#include <wx/listctrl.h>

class vv_wxListViewComboPopup : public wxListView,
                             public wxComboPopup
							 //public wxPanel
{
public:

    // Initialize member variables
    virtual void Init()
    {
        m_value = -1;
    }

    // Create popup control
    virtual bool Create(wxWindow* parent)
    {
		if (wxListView::Create(parent,wxID_ANY,wxPoint(0,0),wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_EDIT_LABELS/* | wxLC_NO_HEADER */))
		{
			//if (! wxComboPopup::Create(parent))
			//	return false;
		}
		else
			return false;
//		wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);
		wxString empty;
		//m_listView = new wxListView(parent, 2, wxPoint(0,0), wxDefaultSize,  wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES/* | wxLC_NO_HEADER */);
		//sizer->Add(m_listView, 1, wxEXPAND);
		//sizer->SetSizeHints(this);
		//wxPanel::SetSizer(sizer);
		Connect(wxEVT_KEY_UP, wxKeyEventHandler(vv_wxListViewComboPopup::OnKeyUp), NULL, this);
		return true;
    }

    // Return pointer to the created control
    virtual wxWindow *GetControl() { return this; }

	wxListView * GetListView() { 
		return NULL;
		//return m_listView;
	}

    // Translate string into a list selection
    virtual void SetStringValue(const wxString& s)
    {
		s;
        int n = FindItem(-1,s);
        if ( n >= 0 && n < GetItemCount() )
		{
            //wxListView::Select(n, true);
			SetItemState(n, wxLIST_STATE_FOCUSED |wxLIST_STATE_SELECTED, wxLIST_STATE_FOCUSED|wxLIST_STATE_SELECTED);
		}
		m_initialValue = s;
    }

    // Get list selection as a string
    virtual wxString GetStringValue() const
    {
        if ( m_value >= 0 )
            return GetItemText(m_value);
        return m_initialValue;
    }

    // Do mouse hot-tracking (which is typical in list popups)
    void OnMouseMove(wxMouseEvent& event)
    {
		event;
		int unused;
		long index = HitTest(event.GetPosition(), unused);
		if (index >= 0)
		{
			long previousSelection = GetFirstSelected();
			if (previousSelection != index)
			{
				Select(previousSelection, false);
				SetItemState(index, wxLIST_STATE_FOCUSED |wxLIST_STATE_SELECTED, wxLIST_STATE_FOCUSED|wxLIST_STATE_SELECTED);
			}
		}
		else
			event.Skip();
    }

	// On list item selection, set the value and close the popup
    void OnListItemSelected(wxListEvent& event)
    {
		event;
		Dismiss();		
		event.Skip();
    }

    // On list item selection, set the value and close the popup
    void OnListItemClicked(wxListEvent& event)
    {
		event;
		event.Veto();
		m_value = GetFirstSelected();
		Dismiss();	
		//event.Skip();
    }

	// On mouse left up, set the value and close the popup
    void OnMouseClick(wxMouseEvent& event)
    {
		event;
		
		int unused;
		long index = HitTest(event.GetPosition(), unused);
		if (index >= 0)
		{
			m_value = GetFirstSelected();
			Dismiss();
		}
		else
			event.Skip();
    }

	// Enter chooses an item, escape exits with no change.
	void OnKeyUp(wxKeyEvent& event)
    {
		event;
		int keycode = event.GetKeyCode();
		if (keycode == WXK_RETURN)
		{
			m_value = GetFirstSelected();
			Dismiss();
		}
		else if (keycode == WXK_ESCAPE)
		{
			m_value = -1;
			this->Dismiss();
		}
		else
		{
			event.Skip();
		}
    }

	/*void OnFocus(wxFocusEvent& cEvent)
	{
		cEvent;
		SetFocus();
	}*/
	wxSize GetAdjustedSize(int minWidth, int minHeight, int maxHeight)
	{
		minHeight;
		maxHeight;
		return wxSize(minWidth, 100);
	}

protected:
	//wxListView *    m_listView;
    int             m_value; // current item index
	wxString		m_initialValue; //The original value submitted by the combo box.
private:
    DECLARE_EVENT_TABLE()
};

#endif
