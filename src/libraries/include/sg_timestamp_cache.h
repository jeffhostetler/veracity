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

#ifndef SG_TIMESTAMP_CACHE_H_
#define SG_TIMESTAMP_CACHE_H_

//////////////////////////////////////////////////////////////////

/**
 * The TimeStampCache: we maintain a cache in the drawer for all
 * (regular) files in the Working Directory.  This cache contains
 * the file's last observed modification time (mtime) and the
 * last known HID.
 *
 * This cache is updated whenever we do things like STATUS or SCAN
 * or ADDREMOVE or COMMIT that cause us to walk the WD and look
 * for files that are different from the baseline.  This cache is
 * completely LOCAL and can be recreated by simply rescanning the
 * WD.
 *
 * The purpose of the cache is to avoid having to recompute the HID
 * for every file for every command, since HID computations are very
 * expensive.  Especially considering there are times (such as in
 * checkout and commit) where we already know the HID.
 *
 */
typedef struct _sg_timestamp_cache SG_timestamp_cache;

/**
 * The TimeStampData: the info for an individual file/record in
 * the cache.
 *
 */
typedef struct _sg_timestamp_data SG_timestamp_data;

//////////////////////////////////////////////////////////////////

/**
 * Allocate an in-memory TimeStampCache and load the contents
 * of the stored cache from the drawer associated with the
 * given WD.
 *
 * WARNING: YOU PROBABLY SHOULD NOT CALL THIS.  Pendingtree
 * creates one if/when it does something that should update
 * the cache.  Since you probably already have a pendingtree
 * available, you should just ask for a handle to its cache.
 *
 * WARNING: Really, I mean it.  Don't call this, use the one
 * already loaded by pendingtree.  Since pendingtree takes
 * a lock on the VFILE, we can let the on-disk timestamp cache
 * hide behind the vfile lock if everyone agrees to let
 * pendingtree only create/load/save them.
 */
void SG_timestamp_cache__allocate_and_load(SG_context * pCtx, SG_timestamp_cache ** ppTSC, const SG_pathname * pPathWorkingDirectoryTop);

/**
 * Save the current contents of the in-memory TimeStampCache
 * to disk.  This completely replaces the info stored on disk.
 */
void SG_timestamp_cache__save(SG_context * pCtx, SG_timestamp_cache * pTSC);

void SG_timestamp_cache__free(SG_context * pCtx, SG_timestamp_cache * pTSC);

/**
 * Add/Update an entry to/in the in-memory cache.  This will either
 * [] create a new record,
 * [] replace the individual fields within an existing record,
 * [] or nothing if the new values are the same.
 */
void SG_timestamp_cache__add(SG_context * pCtx,
							 SG_timestamp_cache * pTSC,
							 const char * pszGid,
							 SG_int64 mtime_ms,
							 SG_uint64 size,
							 const char * pszHid);

/**
 * Ask the TimeStampCache if it has a "valid" cache entry for
 * this GID.  And if so, return it.
 *
 * mtime_ms_now is the CURRENTLY OBSERVED mtime on the file.
 * size_now is the CURRENTLY OBSERVED size of the file.
 * 
 * So, by "valid" we mean:
 * [] do we have a cache entry for this GID?
 * [] and is the observed-mtime equal to the cached-mtime?
 * [] and is the observed-size equal to the cached-size?
 * [] and does the underlying filesystem have sufficient
 *    mtime-resolution to us to rely on the equality?
 *
 * To explain the last point. Some filesystems only have
 * 1-second (ext2) or 2-second (fat32) resolution for
 * mtimes and others have nanosecond (ext4).  Today's
 * systems are fast enough that I can write a script to
 * {} commit something (cacheing the mtime),
 * {} modify the file (which will change the file but
 *       if the clock hasn't ticked yet, it won't change
 *       the whole-second-resolution mtime on the file),
 * {} and run a status,
 * all within the same second.  To guard against using
 * a stale cache entry, we reject these.
 */
void SG_timestamp_cache__is_valid(SG_context * pCtx,
								  SG_timestamp_cache * pTSC,
								  const char * pszGid,
								  SG_int64 mtime_ms_now,
								  SG_uint64 size_now,
								  SG_bool * pbValid,
								  const SG_timestamp_data ** ppData);

/**
 * Remove an entry from the in-memory cache.
 */
void SG_timestamp_cache__remove(SG_context * pCtx,
								SG_timestamp_cache * pTSC,
								const char * pszGid);

/**
 * Remove ALL entries from the in-memory cache.
 */
void SG_timestamp_cache__remove_all(SG_context * pCtx,
									SG_timestamp_cache * pTSC);

/**
 * Return the number of entries in the in-memory cache.
 */
void SG_timestamp_cache__count(SG_context * pCtx,
							   SG_timestamp_cache * pTSC,
							   SG_uint32 * pCount);

//////////////////////////////////////////////////////////////////

/**
 * Return the cached mtime for an entry in the in-memory cache.
 */
void SG_timestamp_data__get_mtime_ms(SG_context * pCtx, const SG_timestamp_data * pThis, SG_int64 * pnMtime_ms);

/**
 * Return the cached mtime for an entry in the in-memory cache.
 */
void SG_timestamp_data__get_size(SG_context * pCtx, const SG_timestamp_data * pThis, SG_uint64 * pnSize);

/**
 * Return the cached HID for an entry in the in-memory cache.
 */
void SG_timestamp_data__get_hid__ref(SG_context * pCtx, const SG_timestamp_data * pThis, const char ** ppszHid);

void SG_timestamp_data__is_equal_to(SG_context * pCtx,
									const SG_timestamp_data * pThis,
									SG_int64 mtime_ms, SG_uint64 size, const char * pszHid,
									SG_bool * pbEqual);

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void SG_timestamp_cache__dump_to_console(SG_context * pCtx,
										 const SG_timestamp_cache * pTSC,
										 const char * pszMessage);

void SG_timestamp_data__dump_to_console(SG_context * pCtx,
										const SG_timestamp_data * pData,
										const char * pszKey,
										const char * pszMessage);
#endif

//////////////////////////////////////////////////////////////////

#endif /* SG_TIMESTAMP_CACHE_H_ */
