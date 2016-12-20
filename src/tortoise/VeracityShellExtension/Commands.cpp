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
#include "StdAfx.h"
#include "Commands.h"

/////////////////////////////////////////////////////////////////////////////
char * AddCommand::GetExeSwitch()
{
	return "add";
}

TCHAR * AddCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Add"));
}
LPCTSTR AddCommand::GetHelpString()
{
	return _T("Add the thing");
}
bool AddCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool AddCommand::EnableWhenSelectionCountIs(SG_uint32 selectionCount)
{
	if (selectionCount == 0)
		return false;
	else
		return true;
}

bool AddCommand::EnableWhenStatusIs(SG_wc_status_flags status)
{
	if (StatusHas(status, SG_WC_STATUS_FLAGS__U__FOUND))
		return true;
	return false;
}

int AddCommand::GetIconIdentifier()
{
	//return IDI_ICON1;
	return IDB_BITMAP_ADD;
}
/////////////////////////////////////////////////////////////////////////////

char * CreateWFCommand::GetExeSwitch()
{
	return "create";
}

TCHAR * CreateWFCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Create a New Working Copy Here..."));
}
LPCTSTR CreateWFCommand::GetHelpString()
{
	return _T("Create a new working copy in this location.");
}
bool CreateWFCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool CreateWFCommand::EnableWhenInWF(bool bInWF)
{
	return !bInWF;
}
int CreateWFCommand::GetIconIdentifier()
{
	return IDB_BITMAP_SG;
}

bool CreateWFCommand::EnableWhenSelectionCountIs(SG_uint32 selectionCount)
{
	if (selectionCount > 1)
		return false;
	else
		return true;
}

bool CreateWFCommand::EnableWhenSelectionIsFilesAndFolders()
{
	return false;
}

bool CreateWFCommand::EnableWhenSelectionIsAllFiles()
{
	return false;
}
/////////////////////////////////////////////////////////////////////////////
char * DiffCommand::GetExeSwitch()
{
	return "diff";
}

TCHAR * DiffCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Diff"));
}
LPCTSTR DiffCommand::GetHelpString()
{
	return _T("Compare with previous versions");
}
bool DiffCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool DiffCommand::EnableWhenSelectionCountIs(SG_uint32 selectionCount)
{
	if (selectionCount != 1)
		return false;
	else
		return true;
}

bool DiffCommand::EnableWhenSelectionIsAllFolders()
{
	return false;
}

bool DiffCommand::EnableWhenSelectionIsFilesAndFolders()
{
	return false;
}
bool DiffCommand::EnableWhenStatusIs(SG_wc_status_flags status)
{
	if (StatusHas(status, SG_WC_STATUS_FLAGS__U__FOUND) || StatusHas(status, SG_WC_STATUS_FLAGS__S__ADDED))
		return false;
	return true;
}
int DiffCommand::GetIconIdentifier()
{
	return IDB_BITMAP_DIFF;
}

/////////////////////////////////////////////////////////////////////////////
char * LockCommand::GetExeSwitch()
{
	return "lock";
}

TCHAR * LockCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&File Locks"));
}
LPCTSTR LockCommand::GetHelpString()
{
	return _T("Lock this file");
}
bool LockCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool LockCommand::EnableWhenSelectionCountIs(SG_uint32 selectionCount)
{
	selectionCount;
	return true;
}

bool LockCommand::EnableWhenSelectionIsAllFolders()
{
	return false;
}

bool LockCommand::EnableWhenSelectionIsFilesAndFolders()
{
	return false;
}
bool LockCommand::EnableWhenStatusIs(SG_wc_status_flags status)
{
	if (StatusHas(status, SG_WC_STATUS_FLAGS__U__FOUND) || StatusHas(status, SG_WC_STATUS_FLAGS__S__ADDED))
		return false;
	return true;
}
int LockCommand::GetIconIdentifier()
{
	return IDB_BITMAP_LOCK;
}

/////////////////////////////////////////////////////////////////////////////

char * SyncCommand::GetExeSwitch()
{
	return "sync";
}

TCHAR * SyncCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Sync"));
}
LPCTSTR SyncCommand::GetHelpString()
{
	return _T("Transfer new changesets to/from other computers");
}
bool SyncCommand::GetPutOnPrimaryMenu()
{
	return true;
}

int SyncCommand::GetIconIdentifier()
{
	return IDB_BITMAP_SYNC;
}
/////////////////////////////////////////////////////////////////////////////

char * CommitCommand::GetExeSwitch()
{
	return "commit";
}

TCHAR * CommitCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Commit"));
}
LPCTSTR CommitCommand::GetHelpString()
{
	return _T("Commit pending changes in this working copy.");
}
bool CommitCommand::GetPutOnPrimaryMenu()
{
	return true;
}
int CommitCommand::GetIconIdentifier()
{
	return IDB_BITMAP_COMMIT;
}

/////////////////////////////////////////////////////////////////////////////

char * StatusCommand::GetExeSwitch()
{
	return "status";
}

TCHAR * StatusCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Status"));
}
LPCTSTR StatusCommand::GetHelpString()
{
	return _T("Show the pending changes in the working copy.");
}
bool StatusCommand::GetPutOnPrimaryMenu()
{
	return false;
}
int StatusCommand::GetIconIdentifier()
{
	return IDB_BITMAP_STATUS;
}

/////////////////////////////////////////////////////////////////////////////
char * RevertCommand::GetExeSwitch()
{
	return "revert";
}

TCHAR * RevertCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Revert"));
}
LPCTSTR RevertCommand::GetHelpString()
{
	return _T("Revert pending changes in the working copy.");
}
bool RevertCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool RevertCommand::EnableWhenStatusIs(SG_wc_status_flags status)
{
	if (StatusHas(status, SG_WC_STATUS_FLAGS__U__FOUND) || status == SG_WC_STATUS_FLAGS__ZERO)
		return false;
	return true;
}

int RevertCommand::GetIconIdentifier()
{
	return IDB_BITMAP_REVERT;
}

/////////////////////////////////////////////////////////////////////////////
char * DeleteCommand::GetExeSwitch()
{
	return "remove";
}

TCHAR * DeleteCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Delete"));
}
LPCTSTR DeleteCommand::GetHelpString()
{
	return _T("Delete items from Veracity");
}
bool DeleteCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool DeleteCommand::EnableWhenStatusIs(SG_wc_status_flags status)
{
	if (status != SG_WC_STATUS_FLAGS__ZERO)
		return false;
	return true;
}
int DeleteCommand::GetIconIdentifier()
{
	return IDB_BITMAP_DELETE;
}

/////////////////////////////////////////////////////////////////////////////
char * MoveCommand::GetExeSwitch()
{
	return "move";
}

TCHAR * MoveCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Move"));
}
LPCTSTR MoveCommand::GetHelpString()
{
	return _T("Move to a new folder.");
}
bool MoveCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool MoveCommand::EnableWhenStatusIs(SG_wc_status_flags status)
{
	if (StatusHas(status, SG_WC_STATUS_FLAGS__U__FOUND))
		return false;
	return true;
}
int MoveCommand::GetIconIdentifier()
{
	return IDB_BITMAP_MOVE;
}

/////////////////////////////////////////////////////////////////////////////

char * RenameCommand::GetExeSwitch()
{
	return "rename";
}

TCHAR * RenameCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Rename"));
}
LPCTSTR RenameCommand::GetHelpString()
{
	return _T("Change the name of an item.");
}
bool RenameCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool RenameCommand::EnableWhenStatusIs(SG_wc_status_flags status)
{
	if (StatusHas(status, SG_WC_STATUS_FLAGS__U__FOUND) || StatusHas(status, SG_WC_STATUS_FLAGS__S__ADDED))
		return false;
	return true;
}
bool RenameCommand::EnableWhenSelectionCountIs(SG_uint32 selectionCount)
{
	if (selectionCount != 1)
		return false;
	else
		return true;
}
int RenameCommand::GetIconIdentifier()
{
	return IDB_BITMAP_RENAME;
}

/////////////////////////////////////////////////////////////////////////////

char * SeparatorCommand::GetExeSwitch()
{
	return "error";
}

TCHAR * SeparatorCommand::GetCommandString()
{
	return _T("separator");
}
LPCTSTR SeparatorCommand::GetHelpString()
{
	return _T("This should not be called.");
}
bool SeparatorCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool SeparatorCommand::IsSeparator()
{
	return true;
}
/////////////////////////////////////////////////////////////////////////////

char * HistoryCommand::GetExeSwitch()
{
	return "history";
}

TCHAR * HistoryCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&History"));
}
LPCTSTR HistoryCommand::GetHelpString()
{
	return _T("View the history of an item.");
}
bool HistoryCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool HistoryCommand::EnableWhenStatusIs(SG_wc_status_flags status)
{
	if (StatusHas(status, SG_WC_STATUS_FLAGS__U__FOUND) || StatusHas(status, SG_WC_STATUS_FLAGS__S__ADDED))
		return false;
	return true;
}
bool HistoryCommand::EnableWhenSelectionCountIs(SG_uint32 selectionCount)
{
	if (selectionCount != 1)
		return false;
	else
		return true;
}
int HistoryCommand::GetIconIdentifier()
{
	return IDB_BITMAP_HISTORY;
}

/////////////////////////////////////////////////////////////////////////////

char * MergeCommand::GetExeSwitch()
{
	return "merge";
}

TCHAR * MergeCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Merge"));
}
LPCTSTR MergeCommand::GetHelpString()
{
	return _T("Merge with changes from another changeset.");
}
bool MergeCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool MergeCommand::DisableWhenThereIsOneHead()
{
	return true;
}
int MergeCommand::GetIconIdentifier()
{
	return IDB_BITMAP_MERGE;
}

/////////////////////////////////////////////////////////////////////////////

char * ResolveCommand::GetExeSwitch()
{
	return "resolve";
}

TCHAR * ResolveCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Resolve"));
}
LPCTSTR ResolveCommand::GetHelpString()
{
	return _T("Resolve merge conflicts.");
}
bool ResolveCommand::GetPutOnPrimaryMenu()
{
	return false;
}

bool ResolveCommand::EnableWhenThereIsNoConflict()
{
	return false;
}

int ResolveCommand::GetIconIdentifier()
{
	return IDB_BITMAP_RESOLVE;
}

/////////////////////////////////////////////////////////////////////////////


char * BranchesCommand::GetExeSwitch()
{
	return "branches";
}

TCHAR * BranchesCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Branches"));
}
LPCTSTR BranchesCommand::GetHelpString()
{
	return _T("Manage Branches.");
}
bool BranchesCommand::GetPutOnPrimaryMenu()
{
	return false;
}

int BranchesCommand::GetIconIdentifier()
{
	return IDB_BITMAP_BRANCH;
}

/////////////////////////////////////////////////////////////////////////////

char * UpdateCommand::GetExeSwitch()
{
	return "update";
}

TCHAR * UpdateCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Update"));
}
LPCTSTR UpdateCommand::GetHelpString()
{
	return _T("Update the working copy to include changes.");
}
bool UpdateCommand::GetPutOnPrimaryMenu()
{
	return false;
}

int UpdateCommand::GetIconIdentifier()
{
	return IDB_BITMAP_UPDATE;
}

/////////////////////////////////////////////////////////////////////////////
char * DagCommand::GetExeSwitch()
{
	return "dag";
}

TCHAR * DagCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Dag browser"));
}
LPCTSTR DagCommand::GetHelpString()
{
	return _T("View the all changesets in the repository.");
}
bool DagCommand::GetPutOnPrimaryMenu()
{
	return false;
}

int DagCommand::GetIconIdentifier()
{
	return IDB_BITMAP_DAG;
}

/////////////////////////////////////////////////////////////////////////////
char * ServeCommand::GetExeSwitch()
{
	return "serve";
}

TCHAR * ServeCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Web Server"));
}
LPCTSTR ServeCommand::GetHelpString()
{
	return _T("Start the Veracity web server..");
}
bool ServeCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool ServeCommand::EnableWhenInWF(bool bInWF)
{
	bInWF;
	return true;
}

int ServeCommand::GetIconIdentifier()
{
	return IDB_BITMAP_SERVE;
}

/////////////////////////////////////////////////////////////////////////////
char * SettingsCommand::GetExeSwitch()
{
	return "settings";
}

TCHAR * SettingsCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Settings"));
}
LPCTSTR SettingsCommand::GetHelpString()
{
	return _T("Change Veracity settings.");
}
bool SettingsCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool SettingsCommand::EnableWhenInWF(bool bInWF)
{
	bInWF;
	return true;
}

int SettingsCommand::GetIconIdentifier()
{
	return IDB_BITMAP_SETTINGS;
}

/////////////////////////////////////////////////////////////////////////////
char * AboutCommand::GetExeSwitch()
{
	return "about";
}

TCHAR * AboutCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&About"));
}
LPCTSTR AboutCommand::GetHelpString()
{
	return _T("About Veracity.");
}
bool AboutCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool AboutCommand::EnableWhenInWF(bool bInWF)
{
	bInWF;
	return true;
}

int AboutCommand::GetIconIdentifier()
{
	return IDB_BITMAP_SG;
}

/////////////////////////////////////////////////////////////////////////////
char * ManageReposCommand::GetExeSwitch()
{
	return "managerepos";
}

TCHAR * ManageReposCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Manage Repositories"));
}
LPCTSTR ManageReposCommand::GetHelpString()
{
	return _T("Make changes to the repositories on this computer.");
}
bool ManageReposCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool ManageReposCommand::EnableWhenInWF(bool bInWF)
{
	bInWF;
	return true;
}
int ManageReposCommand::GetIconIdentifier()
{
	return IDB_BITMAP_MANAGE;
}

/////////////////////////////////////////////////////////////////////////////
char * HelpCommand::GetExeSwitch()
{
	return "help";
}

TCHAR * HelpCommand::GetCommandString()
{
	return FormatForPrimaryMenu(_T("&Help"));
}
LPCTSTR HelpCommand::GetHelpString()
{
	return _T("View the help.");
}
bool HelpCommand::GetPutOnPrimaryMenu()
{
	return false;
}
bool HelpCommand::EnableWhenInWF(bool bInWF)
{
	bInWF;
	return true;
}

int HelpCommand::GetIconIdentifier()
{
	return IDB_BITMAP_HELP;
}

/////////////////////////////////////////////////////////////////////////////
char * SubmenuCommand::GetExeSwitch()
{
	return "";
}

TCHAR * SubmenuCommand::GetCommandString()
{
	return _T("");
}
LPCTSTR SubmenuCommand::GetHelpString()
{
	return _T("Veracity Commands");
}
bool SubmenuCommand::GetPutOnPrimaryMenu()
{
	return true;
}
bool SubmenuCommand::EnableWhenInWF(bool bInWF)
{
	bInWF;
	return true;
}

int SubmenuCommand::GetIconIdentifier()
{
	return IDB_BITMAP_SG;
}

/////////////////////////////////////////////////////////////////////////////
