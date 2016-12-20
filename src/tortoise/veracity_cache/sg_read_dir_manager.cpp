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

#include "sg_read_dir_manager.h"
#include "sg_status_cache.h"
#include "sg_cache_application.h"

sg_read_dir_manager::sg_read_dir_manager(SG_context * pCtx)
{
#if USE_FILESYSTEM_WATCHERS
	m_hExitNow = ::CreateSemaphore(
			NULL,		// no security attributes
			0,			// initial count
			2,	// max count
			NULL);		// anonymous
	m_pChangeWatcher = new CReadDirectoryChanges();
	//TODO:  A better idea would be to have pendingtree
	//expose the location and name of the pending tree save file.
	//As of this minute, the only function that returns something close
	// sg_pendingtree__get_save_path is private.
	//The pendingtree rewrite will include a new filename, and we'll
	//a public function to get the name of the pending tree file.
	m_sPendingTreeFile = "wd.json";
	m_sBranchFile = "branch.json";

	const char * pDrawerName = NULL;
	SG_ERR_CHECK(  SG_workingdir__get_drawer_directory_name(pCtx, &pDrawerName)  );
	m_sDrawerName = pDrawerName;

	StartBGThread(pCtx);
fail:
	return;
#endif
}
#if USE_FILESYSTEM_WATCHERS

void sg_read_dir_manager::StartBGThread(SG_context * pCtx)
{
	if (CreateThread(wxTHREAD_DETACHED) != wxTHREAD_NO_ERROR)
    {
		SG_log__report_error(pCtx, "Could not create the worker thread!");
        return;
    }

    if (GetThread()->Run() != wxTHREAD_NO_ERROR)
    {
        SG_log__report_error(pCtx, "Could not run the worker thread!");
        return;
    }

}
#endif
sg_read_dir_manager::~sg_read_dir_manager()
{
	SG_context * pCtx;
	SG_context__alloc(&pCtx);

#if USE_FILESYSTEM_WATCHERS
	::ReleaseSemaphore(m_hExitNow, 1, NULL);
	{
		wxCriticalSectionLocker lock(m_changeWatcherCriticalSection);
		delete m_pChangeWatcher;
	}
#endif
	//Free all of the treediff cache objects
	//wxCriticalSectionLocker lock(m_treediff_list_critical_section);
	for (treediff_map::iterator it = m_treediffs.begin(); it != m_treediffs.end(); ++it)
	{
		if (it->second)
		{
			it->second->clearCache(pCtx);
			delete it->second;
		}
	}
	SG_CONTEXT_NULLFREE(pCtx);
}


//Check to see if windows is asking the same question it _just_ asked.
//The way that explorer asks questions tends to be very repetitive.
bool sg_read_dir_manager::IsRepeatQuery(SG_context * pCtx, const char * szBuffer, const char * psz_wc_root, char ** ppszResult)
{
	psz_wc_root;
	//sg_treediff_cache * pCache = GetCacheForRoot(psz_wc_root);
	//wxCriticalSectionLocker lock(m_treediff_list_critical_section);
	
	sg_treediff_cache * pCache = GetCacheForwxFileName(wxFileName(), psz_wc_root);

	if (pCache != NULL && pCache->IsRepeatQuery(szBuffer))
	{
		*ppszResult = pCache->GetLastResult(pCtx);
		return true;
	}

/*	for (treediff_map::iterator it = m_treediffs.begin(); it != m_treediffs.end(); ++it)
	{
		if (it->second->IsRepeatQuery(szBuffer))
		{
			*ppszResult = it->second->GetLastResult(pCtx);
			return true;
		}
	}
	*/
	return false;
}


void sg_read_dir_manager::RefreshWF(SG_context * pCtx, const char * psz_wc_root, SG_varray* psa_paths)
{
	SG_bool bNeedsFinalSlash = SG_FALSE;
	wxFileName requestedPath;
	SG_pathname * pPathName = NULL;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathName, psz_wc_root)  );
	SG_ERR_CHECK(  SG_pathname__remove_final_slash(pCtx, pPathName, &bNeedsFinalSlash)  );
	//Get the correct treediff_cache for the path
	requestedPath = wxFileName(SG_pathname__sz(pPathName));
	//Restore the trailing slash, as status needs it.
	if (bNeedsFinalSlash)
		SG_ERR_CHECK(  SG_pathname__add_final_slash(pCtx, pPathName)  );

	sg_treediff_cache * pCache = GetCacheForwxFileName(requestedPath, psz_wc_root);
	if (pCache == NULL)
	{
		SG_log__report_verbose(pCtx, "creating new treediff_cache object"); 
		pCache = new sg_treediff_cache(pCtx, psz_wc_root);
#if USE_FILESYSTEM_WATCHERS
		InitFSWatchers(pCache->GetTopDir(), pCache);
#endif
		//wxCriticalSectionLocker lock(m_treediff_list_critical_section);
		m_treediffs[psz_wc_root] = pCache;
	}
	pCache->NotifyExplorerToRefreshPath(pCtx, requestedPath, true);
	if (psa_paths != NULL)
	{
		//Iterate over the specific paths provided, and fire a disk event for
		//each.
		SG_uint32 nPathCount = 0, i = 0;
		const char * currentPath = NULL;
		SG_ERR_CHECK(  SG_varray__count(pCtx, psa_paths, &nPathCount)  );
		for (i = 0; i < nPathCount; i++)
		{
			SG_ERR_CHECK(  SG_varray__get__sz(pCtx, psa_paths,i, &currentPath)  );
			requestedPath.Assign(currentPath);
			pCache->NotifyExplorerToRefreshPath(pCtx, requestedPath, true);
		}
	}

	
fail:
	return;
}

void sg_read_dir_manager::GetStatusForPath(SG_context * pCtx, SG_pathname * pPathName, const char * psz_wc_root, SG_wc_status_flags * pStatusFlags, SG_bool * pbDirChanged, sg_treediff_cache ** ppCacheObject)
{
	SG_bool bNeedsFinalSlash = SG_FALSE;
	wxFileName requestedPath;
	SG_ERR_CHECK(  SG_pathname__remove_final_slash(pCtx, pPathName, &bNeedsFinalSlash)  );
	//Get the correct treediff_cache for the path
	requestedPath = wxFileName(SG_pathname__sz(pPathName));
	//Restore the trailing slash, as status needs it.
	if (bNeedsFinalSlash)
		SG_ERR_CHECK(  SG_pathname__add_final_slash(pCtx, pPathName)  );

	sg_treediff_cache * pCache = GetCacheForwxFileName(requestedPath, psz_wc_root);
	if (pCache == NULL)
	{
		SG_log__report_verbose(pCtx, "creating new treediff_cache object"); 
		pCache = new sg_treediff_cache(pCtx, psz_wc_root);
#if USE_FILESYSTEM_WATCHERS
		InitFSWatchers(pCache->GetTopDir(), pCache);
#endif
		//wxCriticalSectionLocker lock(m_treediff_list_critical_section);
		m_treediffs[psz_wc_root] = pCache;
	}
	SG_ERR_CHECK(  pCache->GetStatusForPath(pCtx, SG_FALSE, pPathName, pStatusFlags, pbDirChanged)  );
	SG_RETURN_AND_NULL(pCache, ppCacheObject);

fail:
	return;
}

#if USE_FILESYSTEM_WATCHERS
void sg_read_dir_manager::InitFSWatchers(wxFileName topDir, sg_treediff_cache * pCache)
{

	wxFileName clone(topDir.GetFullPath() + "\\");
	{
		wxCriticalSectionLocker lock(m_changeWatcherCriticalSection);
		m_pChangeWatcher->AddDirectory(clone.GetFullPath(), true, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_FILE_NAME, 16384, (void *)pCache );
	}
	
}

//This function will listen for changes from the CReadDirectoryChanges class.
wxThread::ExitCode sg_read_dir_manager::Entry()
{
	SG_context * pCtx = NULL;
	SG_context__alloc(&pCtx);
	
	CStringW wstrFilename;
	wxFileName path;
	bool bTerminating = false;
	HANDLE handles[2];
	while (!bTerminating && !this->GetThread()->TestDestroy())
	{
		{
			wxCriticalSectionLocker lock(m_changeWatcherCriticalSection);
			handles[0] = m_pChangeWatcher->GetWaitHandle();
			handles[1] = m_hExitNow;
		}
		DWORD rc = ::WaitForMultipleObjectsEx(_countof(handles), handles, false, INFINITE, true);
		
		switch (rc)
		{
		case WAIT_OBJECT_0 + 0:
			// We've received a notification in the queue.
			{
				wxCriticalSectionLocker lock(m_changeWatcherCriticalSection);
				DWORD dwAction;
				if (m_pChangeWatcher->CheckOverflow())
				{
					//wprintf(L"Queue overflowed.\n");
					SG_log__report_error(pCtx, "File system change watcher error:  Queue overflowed."); 
					//Go ahead and exit.  It's the safest thing to do.  The process will 
					//automatically be restarted, and we can't really trust our file 
					//system watchers anymore, anyway.
					wxGetApp().ExitMainLoop();
				}
				else
				{
					sg_treediff_cache * pCache = NULL;
					m_pChangeWatcher->Pop(dwAction, wstrFilename, (void**)&pCache );
					//wxQueueEvent(wxTheApp, new sgFileSystemEvent(dwAction, wstrFilename) );
					path.Assign((LPCTSTR)wstrFilename);
					wxString fullName = path.GetFullName();

					//Try to eliminate all the cases we can, without finding the specific
					//treediff_cache object.
					if (fullName.CompareTo(m_sDrawerName, wxString::ignoreCase) == 0)
					{
						//If this is a change directly to .sgdrawer, ignore it.
						continue;
					}
					if (path.GetDirs().Index(m_sDrawerName, false, true) != wxNOT_FOUND)
					{
						continue;
					}
					//sg_treediff_cache * pCache = GetCacheForwxFileName(path, NULL);
					if (pCache != NULL)
					{
						//Let the treecache know to notify the shell about the change.
						pCache->ProcessDiskEvent(pCtx, dwAction, path);
						
						if (SG_CONTEXT__HAS_ERR(pCtx))
						{
							SG_log__report_error__current_error(pCtx);
							SG_context__err_reset(pCtx);
						}
					}
				}
			}
			break;
		case WAIT_OBJECT_0 + 1:
			{
				bTerminating = true;
			}
			break;
		case WAIT_IO_COMPLETION:
			// Nothing to do.
			break;
		}
	}
	
	SG_CONTEXT_NULLFREE(pCtx);
	return (wxThread::ExitCode)0;

}
#endif
void sg_read_dir_manager::NotifyShellIfNecessary(SG_context * pCtx)
{
	//wxCriticalSectionLocker lock(m_treediff_list_critical_section);
	for (treediff_map::iterator it = m_treediffs.begin(); it != m_treediffs.end(); ++it)
	{
		if (it->second != NULL)
			it->second->NotifyShellIfNecessary(pCtx);
	}
}


void sg_read_dir_manager::CloseTransactionsIfNecessary(SG_context * pCtx)
{
	//wxCriticalSectionLocker lock(m_treediff_list_critical_section);
	for (treediff_map::iterator it = m_treediffs.begin(); it != m_treediffs.end(); ++it)
	{
		if (it->second != NULL)
			it->second->CloseTransactionIfNecessary(pCtx);
	}
}

//This function will compare two wxFileNames and return true
//if they are the same, or if one is inside the other.
//This is to help find which treediff_cache to use.
bool sg_read_dir_manager::IsSameAsOrInside(wxFileName parentPath, wxFileName subPath)
{
	if (parentPath.SameAs(subPath))
		return true;
	if (parentPath.GetDirCount() > subPath.GetDirCount())
		return false;

	wxString parentName = parentPath.GetFullName();

	wxArrayString fileDirs = subPath.GetDirs();
	wxArrayString parentDirs = parentPath.GetDirs();

	if (parentDirs.GetCount() == 0)
		return false;
	parentName = parentPath.GetFullName();
	//Go backwards up the directories.
	for (wxArrayString::reverse_iterator it = fileDirs.rbegin(); it != fileDirs.rend(); ++it)
	{
		if (parentName.CompareTo(*it, wxString::ignoreCase) == 0)
		{
			//This might be the parent. loop over the dirs remaining in
			//subPath
			bool bFoundMatch = true;
			//This declaration is outside the loop header because I needed to ++ itsubPath2 before
			//the start of the loop.  Please note, that we're not changing "it".
			wxArrayString::reverse_iterator itsubPath2 = it;
			itsubPath2++;
			for (wxArrayString::reverse_iterator itParent = parentDirs.rbegin();  //Iterate over the remaining subPath items and parent items simultaneously
				itsubPath2 != fileDirs.rend() && itParent != parentDirs.rend(); //Don't fall off the end of either
				++itsubPath2, ++itParent)//Keep going.
			{
				if ((*itsubPath2).CompareTo(*itParent, wxString::ignoreCase) != 0)
				{
					//The paths are different break out of the loop.
					//We still need to check the rest of the dirs on this parent, since 
					//the same name could appear twice in the dir path
					// c:\dev is a parent of c:\dev\veracity\dev, for example. 
					bFoundMatch = false;
					break;
				}
			}
			if (bFoundMatch)
				return true;
		}
	}


	return false;
}

//Get the appropriate treediff_cache object for the given disk path.
sg_treediff_cache * sg_read_dir_manager::GetCacheForwxFileName(wxFileName path, const char * psz_wc_root)
{
	sg_treediff_cache * pCache = NULL;
	//wxCriticalSectionLocker lock(m_treediff_list_critical_section);
	if (psz_wc_root != NULL)
	{
		return m_treediffs[psz_wc_root];
	}
	
	for (treediff_map::const_iterator it = m_treediffs.begin(); it != m_treediffs.end(); ++it)
	{
		sg_treediff_cache * pCandidateCache = it->second;
		if (pCandidateCache != NULL)
		{
			wxFileName cacheTop = pCandidateCache->GetTopDir();
			if (cacheTop.IsOk() && IsSameAsOrInside(cacheTop, path))
				pCache = pCandidateCache;
		}
	}
	return pCache;	
}
/*
sg_treediff_cache * sg_read_dir_manager::GetCacheForRoot(const char * psz_wc_root)
{
	sg_treediff_cache * pCache = NULL;
	wxCriticalSectionLocker lock(m_treediff_list_critical_section);
	for (treediff_map::const_iterator it = m_treediffs.begin(); it != m_treediffs.end(); ++it)
	{
		sg_treediff_cache * pCandidateCache = it->second;
			wxFileName cacheTop = pCandidateCache->GetTopDir();
			if (cacheTop.IsOk() && IsSameAsOrInside(cacheTop, path))
				pCache = pCandidateCache;
	}
	return pCache;	
}*/

#if USE_FILESYSTEM_WATCHERS
void sg_read_dir_manager::CloseListeners(SG_context * pCtx, const char * psz_changed_path)
{
	SG_bool bNeedsFinalSlash = SG_FALSE;
	wxFileName requestedPath;
	SG_pathname * pPathName = NULL;
	std::list<string> cachesToClear;

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathName, psz_changed_path)  );
	SG_ERR_CHECK(  SG_pathname__remove_final_slash(pCtx, pPathName, &bNeedsFinalSlash)  );
	//Get the correct treediff_cache for the path
	requestedPath = wxFileName(SG_pathname__sz(pPathName));
	//Restore the trailing slash, as status needs it.
	if (bNeedsFinalSlash)
		SG_ERR_CHECK(  SG_pathname__add_final_slash(pCtx, pPathName)  );

	sg_treediff_cache * pCache = GetCacheForwxFileName(requestedPath, psz_changed_path);
	if (pCache != NULL)
	{
		cachesToClear.push_back(psz_changed_path);
	}
	else
	{
		//Check each cache to see if it's below the effected folder.
		for (treediff_map::const_iterator it = m_treediffs.begin(); it != m_treediffs.end(); ++it)
		{
			if (it->second != NULL)
			{
				if (IsSameAsOrInside(requestedPath, it->second->GetTopDir()))
				{
					cachesToClear.push_back(it->first);
				}
			}
		}
	}
	if (cachesToClear.size() > 0)
	{
		for (std::list<string>::const_iterator it = cachesToClear.begin();
			it != cachesToClear.end();
			it++)
		{
			string pThisCache = *it;
			treediff_map::const_iterator it2 = m_treediffs.find(pThisCache);
			
			delete it2->second;
			m_treediffs.erase(it2);
			
		}
		
		//Kill the current BG thread.
		::ReleaseSemaphore(m_hExitNow, 1, NULL);

		{
			wxCriticalSectionLocker lock(m_changeWatcherCriticalSection);
			delete m_pChangeWatcher;
			m_pChangeWatcher = new CReadDirectoryChanges();
			//Now add back the watchers that we did not delete
			for (treediff_map::const_iterator it = m_treediffs.begin(); it != m_treediffs.end(); ++it)
			{
				if (it->second != NULL)
				{
					InitFSWatchers(it->second->GetTopDir(), it->second);
				}
			}
		}
		StartBGThread(pCtx);
	}
fail:
	return;
}
#endif
