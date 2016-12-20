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
#pragma once
#include "ICommandBase.h"

 class AddCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenSelectionCountIs(SG_uint32 selectionCount);
	bool EnableWhenStatusIs(SG_wc_status_flags status);
	int GetIconIdentifier();
 };

  class CreateWFCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenInWF(bool bInWF);
	bool EnableWhenSelectionCountIs(SG_uint32 selectionCount);
	bool EnableWhenSelectionIsFilesAndFolders();
	bool EnableWhenSelectionIsAllFiles();
	int GetIconIdentifier();
 };

  class DiffCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenSelectionIsAllFolders();
	bool EnableWhenSelectionIsFilesAndFolders();
	bool EnableWhenSelectionCountIs(SG_uint32 selectionCount);
	bool EnableWhenStatusIs(SG_wc_status_flags status);
	int GetIconIdentifier();
 };

  class LockCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenSelectionIsAllFolders();
	bool EnableWhenSelectionIsFilesAndFolders();
	bool EnableWhenSelectionCountIs(SG_uint32 selectionCount);
	bool EnableWhenStatusIs(SG_wc_status_flags status);
	int GetIconIdentifier();
 };

  class SyncCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	int GetIconIdentifier();
 };

class CommitCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	int GetIconIdentifier();
 };

class StatusCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	int GetIconIdentifier();
 };


class RevertCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenStatusIs(SG_wc_status_flags status);
	int GetIconIdentifier();
 };
class MoveCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenStatusIs(SG_wc_status_flags status);
	int GetIconIdentifier();
 };
class RenameCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenSelectionCountIs(SG_uint32 selectionCount);
	bool EnableWhenStatusIs(SG_wc_status_flags status);
	int GetIconIdentifier();
 };
class DeleteCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenStatusIs(SG_wc_status_flags status);
	int GetIconIdentifier();
 };

class HistoryCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenSelectionCountIs(SG_uint32 selectionCount);
	bool EnableWhenStatusIs(SG_wc_status_flags status);
	int GetIconIdentifier();
 };
class MergeCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool DisableWhenThereIsOneHead();
	int GetIconIdentifier();
 };
class ResolveCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenThereIsNoConflict();
	int GetIconIdentifier();
 };
class BranchesCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	int GetIconIdentifier();
 };
class UpdateCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	int GetIconIdentifier();
 };


class DagCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	int GetIconIdentifier();
 };

class ServeCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenInWF(bool bInWF);
	int GetIconIdentifier();
 };

class SettingsCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenInWF(bool bInWF);
	int GetIconIdentifier();
 };

class AboutCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenInWF(bool bInWF);
	int GetIconIdentifier();
 };

class ManageReposCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenInWF(bool bInWF);
	int GetIconIdentifier();
 };

class HelpCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenInWF(bool bInWF);
	int GetIconIdentifier();
 };

class SubmenuCommand : public ICommandBase
{
public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool EnableWhenInWF(bool bInWF);
	int GetIconIdentifier();
};

class SeparatorCommand : public ICommandBase
 {
 public:
	char * GetExeSwitch();
	LPCTSTR GetHelpString();
	bool GetPutOnPrimaryMenu();
	TCHAR * GetCommandString();
	bool IsSeparator();
 };
