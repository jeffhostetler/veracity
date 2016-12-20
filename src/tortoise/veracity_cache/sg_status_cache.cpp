/*
** 
**
** The treediff object holds these things:
**
** 1.  The treediff.  Used by status
** 2.  The found files.  Used by conflict and status, to remember which 
**       objects are not controlled.
** 3.  The gid cache.  Conflicts are keyed by gid, so a cache
**       is kept to minimize the gid lookups.
** 4.  The previous command and result.  Since explorer is very 
**       repetitive in its question asking, we keep the previous
**       answer around.
*/

#include "sg_status_cache.h"
#include "sg_cache_application.h"
#include "wc8api\sg_wc8api__public_prototypes.h"
#include "wc7txapi\sg_wc7txapi__public_prototypes.h"
#include "wc4status\sg_wc__status__public_prototypes.h"

sg_treediff_cache::sg_treediff_cache(SG_context * pCtx, const char * psz_wc_root)
{
	m_pPreviousCommand = NULL;
	m_pPreviousResult = NULL;
	m_sTopFolder = psz_wc_root;
	m_timeLastRequest = 0;
	m_pwc_tx = NULL;
	int major = 0, minor = 0;
	wxGetOsVersion(&major, &minor);
			
	SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "OS version is: %d.%d", major, minor)  );
	m_bIsWin7 = major > 6  || (major == 6 && minor >= 1);
	
fail:
	return;
}
sg_treediff_cache::~sg_treediff_cache()
{
	SG_context * pCtx;
	SG_context__alloc(&pCtx);
	SG_NULLFREE(pCtx, m_pPreviousCommand);
	SG_NULLFREE(pCtx, m_pPreviousResult);
	m_timeLastRequest = 0;
	CloseTransactionIfNecessary(pCtx);
	SG_CONTEXT_NULLFREE(pCtx);
}
void sg_treediff_cache::clearCache(SG_context * pCtx)
{
	if (pCtx)
	{
		m_timeLastRequest = 0;
		CloseTransactionIfNecessary(pCtx);
	}
}

//Check to see if windows is asking the same question it _just_ asked.
//The way that explorer asks questions tends to be very repetitive.
bool sg_treediff_cache::IsRepeatQuery(const char * szBuffer)
{
	if (szBuffer != NULL && m_pPreviousCommand != NULL && strcmp(szBuffer, m_pPreviousCommand) == 0)
	{
		return true;
	}
	return false;
}

char *  sg_treediff_cache::GetLastResult(SG_context * pCtx)
{
	char * pszNewCopy = NULL;
	SG_STRDUP(pCtx, m_pPreviousResult, &pszNewCopy);
	return pszNewCopy;
}

void sg_treediff_cache::RememberLastQuery(SG_context * pCtx, const char * pszLastQuery, const char * pszLastResult)
{
	//Free the previous results.
	SG_NULLFREE(pCtx, m_pPreviousResult);
	SG_NULLFREE(  pCtx, m_pPreviousCommand);

	//Save the current  command and result to reuse if necessary
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszLastResult, &m_pPreviousResult )  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszLastQuery, &m_pPreviousCommand)  );
fail:
	return;
}

void sg_treediff_cache::GetStatusForPath(SG_context * pCtx, SG_bool bIsRetry, const SG_pathname * pPathName, SG_wc_status_flags * pStatusFlags, SG_bool * pbDirChanged)
{
	wxCriticalSectionLocker lock(m_wc_transaction_lock);
	SG_wc_status_flags statusFlags = SG_WC_STATUS_FLAGS__U__FOUND;
	SG_string * psgstr_statusFlagsDetails = NULL;
	if (m_pwc_tx == NULL)
	{
		//Start a read-only transaction.
		SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &m_pwc_tx, ((pPathName)), SG_TRUE)  );
	}

	SG_ERR_CHECK(  SG_wc_tx__get_item_status_flags(pCtx, m_pwc_tx, SG_pathname__sz(pPathName), SG_FALSE, SG_FALSE, &statusFlags, NULL)  );
	if (pbDirChanged != NULL && (statusFlags & SG_WC_STATUS_FLAGS__T__DIRECTORY))
	{
		//It's a directory, so we need to check to see if there are modifications under it.
		SG_wc_status_flags dirStatusFlags = SG_WC_STATUS_FLAGS__U__FOUND;
		SG_ERR_CHECK(  SG_wc_tx__get_item_dirstatus_flags(pCtx, m_pwc_tx, SG_pathname__sz(pPathName), &dirStatusFlags, NULL, pbDirChanged)   );
	}
#if DEBUG
	SG_ERR_CHECK(  SG_wc_debug__status__dump_flags(pCtx, statusFlags, "status flags", &psgstr_statusFlagsDetails)  );
	SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "%s", SG_string__sz(psgstr_statusFlagsDetails))  );
#endif
	//Don't recurse if this is our second try. 
	if ((bIsRetry == SG_FALSE) 
		&& ((statusFlags & SG_WC_STATUS_FLAGS__T__BOGUS) == SG_WC_STATUS_FLAGS__T__BOGUS) )
	{
		//This is a item that is new in the directory since we started
		//our transaction.  Close the transaction, and start a new one.
		SG_ERR_CHECK(  SG_log__report_verbose(pCtx, "Got a bogus status.  Restarting transaction")  );
		clearCache(pCtx);
		GetStatusForPath(pCtx, SG_TRUE, pPathName, &statusFlags, pbDirChanged);
	}
	
	SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &m_timeLastRequest)  );
	*pStatusFlags = statusFlags;
fail:
	SG_STRING_NULLFREE(pCtx, psgstr_statusFlagsDetails);
	//Log and reset any errors on the context.
	//Any error condition should report the status as
	//Found and still send a message back to the client.
	if (SG_context__has_err(pCtx) == SG_TRUE)
	{
		if (!SG_context__err_equals(pCtx, SG_ERR_WC_DB_BUSY))
		{
			SG_log__report_error__current_error(pCtx);
		}
		SG_context__err_reset(pCtx);
	}
}

wxFileName sg_treediff_cache::GetTopDir()
{
	return m_sTopFolder;
}

const char * sg_treediff_cache::ExplainAction( DWORD dwAction ) 
{
	switch (dwAction)
	{
	case FILE_ACTION_ADDED            :
		return "Added";
	case FILE_ACTION_REMOVED          :  
		return "Deleted";
	case FILE_ACTION_MODIFIED         :  
		return "Modified";
	case FILE_ACTION_RENAMED_OLD_NAME :  
		return "Renamed From";
	case FILE_ACTION_RENAMED_NEW_NAME :  
		return "Renamed To";
	default:
		return "BAD DATA";
	}
}


void sg_treediff_cache::NotifyExplorerToRefreshPath(SG_context * pCtx, wxFileName path, bool bSpecific)
{
	SG_string * pstrFullPath = NULL;
	wxString fullName = path.GetFullName();

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pstrFullPath, path.GetFullPath().ToUTF8())  );

	//Use %s to handle getting a path that has a %s in it.
	//SG_ERR_CHECK(  SG_UTF8__INTERN_FROM_OS_BUFFER(pCtx, pstrFullPath, path.GetFullPath().ToStdWstring())  );
	//SG_log__report_verbose(pCtx, "FS_CHANGE(%s):	%s", (const char *)ExplainAction(dwAction), SG_string__sz(pstrFullPath)); 

	{
		clearCache(pCtx); 
		if (!m_bIsWin7) //if we're running on XP or Vista
		{
			wxCriticalSectionLocker lock(m_notify_critical_section);
			m_bHaveGottenNewFileChanges = true;
			//For some reason, on XP and Vista, we need to notify for every folder up to the root.
			if (path.FileExists() || path.DirExists())
			{
				if (m_aPathsToNotify.Index(path.GetFullPath()) == wxNOT_FOUND)
					m_aPathsToNotify.Add(path.GetFullPath());
			}

			wxFileName currentDir = wxFileName::DirName(path.GetPath(wxPATH_GET_VOLUME));
			wxFileName topDir = GetTopDir();
			//currentDir = topDir;
			
			while (topDir.GetDirCount() < currentDir.GetDirCount())
			{
				if (m_aPathsToNotify.Index(currentDir.GetFullPath()) == wxNOT_FOUND)
					m_aPathsToNotify.Add(currentDir.GetFullPath());
				currentDir.RemoveLastDir();
			}
		}
		else
		{
			wxCriticalSectionLocker lock(m_notify_critical_section);
			m_bHaveGottenNewFileChanges = true;
			//On Win 7, just notifying for the top of the working folder 
			//will cause a recursive refresh down the tree.
			if (m_aPathsToNotify.Index(GetTopDir().GetFullPath()) == wxNOT_FOUND)
				m_aPathsToNotify.Add(GetTopDir().GetFullPath());
			if (bSpecific)
				m_aPathsToNotify.Add(path.GetFullPath());
		}
	}
fail:
	return;
}

void sg_treediff_cache::NotifyShellIfNecessary(SG_context * pCtx)
{
	wchar_t *buf = NULL;
	wxCriticalSectionLocker lock(m_notify_critical_section);
	if (m_bHaveGottenNewFileChanges)
	{
		//This reduces the number of times that we notify for shell changes
		//by making sure that it's been at least one full timer length since the last
		//time we processed a file system event.
		m_bHaveGottenNewFileChanges = false;
		return;
	}
	for (wxArrayString::const_iterator it = m_aPathsToNotify.begin(); it != m_aPathsToNotify.end(); ++it)
	{
		SG_ERR_CHECK(  SG_utf8__extern_to_os_buffer__wchar(pCtx, it->c_str(), &buf, NULL) );
		SG_log__report_verbose(pCtx, "Notifying shell change: %s", (const char *)it->c_str()); 
		SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, buf, NULL);
		SG_NULLFREE(pCtx, buf);	
	}
fail:
	m_aPathsToNotify.Clear();
	SG_NULLFREE(pCtx, buf);	
}

void sg_treediff_cache::CloseTransactionIfNecessary(SG_context * pCtx)
{
	wxCriticalSectionLocker lock(m_wc_transaction_lock);
	if (m_pwc_tx)
	{
		SG_int64 nTimeNow = 0;
		SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &nTimeNow)  );
		//Make sure that it has been at least 250 milliseconds have passed
		//since the last time we used the transaction
		if ( nTimeNow > m_timeLastRequest + (250))
		{
			SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, m_pwc_tx)  );
			SG_WC_TX__NULLFREE(pCtx, m_pwc_tx);
			SG_NULLFREE(pCtx, m_pPreviousCommand);
			SG_NULLFREE(pCtx, m_pPreviousResult);
		}
	}
fail:
	;
}
