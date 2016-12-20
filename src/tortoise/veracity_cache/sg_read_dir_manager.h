#pragma once
#include "stdafx.h"
#include "sg.h"

#include "wx/filename.h"
#include "wx/timer.h"
#include "ReadDirectoryChanges\ReadDirectoryChanges.h"
#include "sg_status_cache.h"
#include <map>

/**
**This class has a few purposes.
** 1.  Add the file system watchers.
*/
struct ci_less
{
	bool operator() (const string & s1, const string & s2) const
	{
		return _stricmp(s1.c_str(), s2.c_str()) < 0;
	}
};
typedef std::map<string, sg_treediff_cache *, ci_less> treediff_map; 


class sg_read_dir_manager 
#if USE_FILESYSTEM_WATCHERS
	: public wxThreadHelper
#endif
{
private:
	
	treediff_map m_treediffs;
#if USE_FILESYSTEM_WATCHERS
	CReadDirectoryChanges * m_pChangeWatcher;
	wxCriticalSection m_changeWatcherCriticalSection;
	HANDLE m_hExitNow;
	wxString					m_sDrawerName;
	wxString					m_sPendingTreeFile;
	wxString					m_sBranchFile;
#endif
	wxCriticalSection m_treediff_list_critical_section;


	bool IsSameAsOrInside(wxFileName parentPath, wxFileName subPath);
	sg_treediff_cache * GetCacheForwxFileName(wxFileName path, const char * psz_wc_root);
#if USE_FILESYSTEM_WATCHERS
	void sg_read_dir_manager::InitFSWatchers(wxFileName topDir, sg_treediff_cache * pCache);
#endif
public:
	sg_read_dir_manager(SG_context * pCtx);
	~sg_read_dir_manager();
	bool IsRepeatQuery(SG_context * pCtx, const char * szBuffer, const char * psz_wc_root, char ** ppszResult);
	const char * GetLastResult();
	void RefreshWF(SG_context * pCtx, const char * psz_wc_root, SG_varray* psa_paths);
	void GetStatusForPath(SG_context * pCtx, SG_pathname * pPathName, const char * psz_wc_root, SG_wc_status_flags * pStatusFlags, SG_bool * pbDirChanged, sg_treediff_cache ** ppCacheObject);
	
	void NotifyShellIfNecessary(SG_context * pCtx);
	void CloseTransactionsIfNecessary(SG_context * pCtx);
#if USE_FILESYSTEM_WATCHERS
	void CloseListeners(SG_context * pCtx, const char * psz_changed_path);
	virtual wxThread::ExitCode Entry();
protected:
	void sg_read_dir_manager::StartBGThread(SG_context * pCtx);
#endif
};