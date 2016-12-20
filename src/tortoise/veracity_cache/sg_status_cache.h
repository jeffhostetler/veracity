#pragma once
#include "stdafx.h"
#include "sg.h"

#if defined(WINDOWS) && defined(CODE_ANALYSIS)
/* Ignore static analysis warnings in WX headers. There are many. */
#include <CodeAnalysis\Warnings.h>
#pragma warning(push)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include<codeanalysis\sourceannotations.h>
#endif

#include "wx/filename.h"
#include "wx/timer.h"
#include "ReadDirectoryChanges\ReadDirectoryChanges.h"

#if defined(WINDOWS) && defined(CODE_ANALYSIS)
/* Stop ignoring static analysis warnings. */
#pragma warning(pop)
#endif

/**
**This class has a few purposes.
** 1.  Perform the status, and keep the results in memory.
** 2.  Add the file system watchers.
** 3.  search the treediff for the status of a file system object.
** 4.  Keep the list of files which are in the "found" state
** 5.  Keep a cache of gids to reduce the number of gid lookups.
*/
class sg_treediff_cache 
{
private:
	wxFileName m_sTopFolder;
	wxCriticalSection m_notify_critical_section;
	wxCriticalSection m_wc_transaction_lock;
	wxArrayString				m_aPathsToNotify;

	char *						m_pPreviousCommand;
	char *						m_pPreviousResult;
	
	bool						m_bIsWin7;
	
	bool						m_bHaveGottenNewFileChanges;
	SG_int64					m_timeLastRequest;
	SG_wc_tx *					m_pwc_tx;

	const char * sg_treediff_cache::ExplainAction( DWORD dwAction ) ;
public:
	sg_treediff_cache(SG_context * pCtx = NULL, const char * psz_wc_root = NULL);
	~sg_treediff_cache();
	bool IsRepeatQuery(const char * szBuffer);
	char * GetLastResult(SG_context * pCtx);
	void RememberLastQuery(SG_context * pCtx, const char * pszLastQuery, const char * pszLastResult);
	void GetStatusForPath(SG_context * pCtx, SG_bool bIsRetry, const SG_pathname * pPathName, SG_wc_status_flags * pStatusFlags, SG_bool * pbDirChanged);
	void clearCache(SG_context * pCtx);
	wxFileName GetTopDir();
	void NotifyShellIfNecessary(SG_context * pCtx);
	void CloseTransactionIfNecessary(SG_context * pCtx);
	void NotifyExplorerToRefreshPath(SG_context * pCtx, wxFileName path, bool bSpecific = false);
protected:
	
};
