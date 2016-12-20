#include "precompiled.h"
#include "vv_wxListViewComboPopup.h"

BEGIN_EVENT_TABLE(vv_wxListViewComboPopup, wxListView)
    EVT_MOTION(vv_wxListViewComboPopup::OnMouseMove)
    EVT_LEFT_UP(vv_wxListViewComboPopup::OnMouseClick)
	//EVT_LIST_ITEM_SELECTED(wxID_ANY, vv_wxListViewComboPopup::OnListItemSelected)
	EVT_LIST_BEGIN_LABEL_EDIT(wxID_ANY, vv_wxListViewComboPopup::OnListItemClicked)
	//EVT_SET_FOCUS(vv_wxListViewComboPopup::OnFocus)
END_EVENT_TABLE()