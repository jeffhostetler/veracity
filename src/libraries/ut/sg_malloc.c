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

// sg_malloc.c
// memory allocation
//////////////////////////////////////////////////////////////////

#include <sg.h>

#ifdef DEBUG

struct _sg_mem_block
{
	void* p;
	SG_uint32 nBytes;

	const char* alloc_pszFile;		// place where SG_alloc() was called
	int alloc_iLine;

	const char* caller_pszType;
	const char* caller_pszFile;		// (optional) place where routined that called SG_alloc() was called
	int caller_iLine;
	SG_bool bForeign;				// true if we DID NOT allocate the memory, but rather adopted it for leak tracking

	struct _sg_mem_block* pNext;
};

#define NUM_BUCKETS (128*1024)

static SG_mutex g_mutex_mem;
struct _sg_mem_block* g_buckets[NUM_BUCKETS];
int _sg_mem_initalized = 0;

static SG_uint32 _sg_mem_hash(void* p)
{
    SG_uint32 hash = 0;
	SG_uint32 i;

    // this is bernstein's popular "times 33" hash algorithm as
    // implemented by engelschall (search for engelschall fast hash on google)
    for(i = 0; i < sizeof(p); i++)
	{
		hash *= 33;
		hash += (((SG_byte*) &p)[i]);
	}

    return (hash % (SG_uint32)NUM_BUCKETS);
}

#if defined(WINDOWS_CRT_LEAK_CHECK)
// Reporting hook for _CrtDumpMemoryLeaks so we can check for memory leaks without dumping info.
// (See also the comment where this is referenced in SG_mem__check_for_leaks.)
static int _ignore_all_reports_hook(int nRptType, char* szMsg, int* retVal)
{
	SG_UNUSED(nRptType);
	SG_UNUSED(szMsg);
	SG_UNUSED(retVal);
	return -1;
}
#endif

int SG_mem__check_for_leaks(void)
{
	int i;
	int ret = 0;

	for (i=0; i<NUM_BUCKETS; i++)
	{
		struct _sg_mem_block* pBlock = g_buckets[i];
		while (pBlock)
		{
			if (pBlock->p)
			{
				return -1;
			}

			pBlock = pBlock->pNext;
		}
	}

#if defined(WINDOWS_CRT_LEAK_CHECK)

	
	/* _CrtDumpMemoryLeaks() is the only way to check for leaks, but we don't want to
	dump them here because they're dumped at exit.  So we temporarily install this
	report function which just ignores all reporting hooks while we use
	_CrtDumpMemoryLeaks to determine if there is any unfreed memory, so tests will 
	fail when they should. */
	_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, _ignore_all_reports_hook);
	ret = _CrtDumpMemoryLeaks();
	_CrtSetReportHook2(_CRT_RPTHOOK_REMOVE, _ignore_all_reports_hook);

#endif // WINDOWS_CRT_LEAK_CHECK

	return ret;
}

static void _mem_dump(FILE* pf)
{
	int i;

	SG_mutex__destroy(&g_mutex_mem);
	for (i=0; i<NUM_BUCKETS; i++)
	{
		struct _sg_mem_block* pBlock = g_buckets[i];
		while (pBlock)
		{
			struct _sg_mem_block* pFreeMe = pBlock;

			if (pBlock->p)
			{
				if (pBlock->caller_pszFile)
					fprintf(pf, "LEAK: %p %5d bytes allocated at %s:%d (called from %s:%d) (%s)\n",
					pBlock->p,
					pBlock->nBytes,
					pBlock->alloc_pszFile, pBlock->alloc_iLine,
					pBlock->caller_pszFile, pBlock->caller_iLine,
					pBlock->caller_pszType);
				else
					fprintf(pf, "LEAK: %p %5d bytes allocated at %s:%d\n",
					pBlock->p,
					pBlock->nBytes,
					pBlock->alloc_pszFile, pBlock->alloc_iLine);
			}

			pBlock = pBlock->pNext;

			free(pFreeMe);
		}
		g_buckets[i] = NULL;
	}
}

static void _sg_mem_dump_atexit(void)
{
	_mem_dump(stdout);
}

#if defined(WINDOWS)
void SG__malloc__windows__mem_dump(FILE* pFile)
{
	_mem_dump(pFile);
}
#endif

static SG_uint64 g_live_bytes = 0;

void sg_mem__dump_stats(const char* psz_file, int line)
{
    SG_int_to_string_buffer buf;

    fprintf(stderr, "\ng_live_bytes = %16s -- %s:%d\n", SG_uint64_to_sz(g_live_bytes, buf), psz_file, line);
}

static struct _sg_mem_block* _sg_mem_add_block(void* p, SG_uint32 nBytes, const char* pszFile, int iLine)
{
	struct _sg_mem_block* pBlock;
	SG_uint32 iBucket;

	if (!_sg_mem_initalized)
	{
		atexit(_sg_mem_dump_atexit);
        SG_mutex__init__bare(&g_mutex_mem);
		_sg_mem_initalized = 1;
	}

	pBlock = (struct _sg_mem_block*) calloc(1, sizeof(struct _sg_mem_block));
	if (!pBlock)
	{
		fprintf(stderr, "%s:%d: low-level calloc(%d,%d) failed (caller %s:%d)\n",
				__FILE__, __LINE__,
				1, (int)sizeof(struct _sg_mem_block),
				pszFile, iLine);
		return NULL;
	}
	else
	{
		pBlock->p = p;
		pBlock->nBytes = nBytes;
		pBlock->alloc_pszFile = pszFile;
		pBlock->alloc_iLine = iLine;
		pBlock->caller_pszFile = NULL;		// assume no caller's caller data until told.
		pBlock->caller_iLine = 0;
		pBlock->bForeign = SG_FALSE;		// assume that we allocated the memory until told otherwise.

		iBucket = _sg_mem_hash(p);

		// TODO the mutex calls return an int.  nonzero means failure
		SG_mutex__lock__bare(&g_mutex_mem);
        g_live_bytes += nBytes;
		pBlock->pNext = g_buckets[iBucket];
		g_buckets[iBucket] = pBlock;
		SG_mutex__unlock__bare(&g_mutex_mem);

		return pBlock;
	}
}

/**
 * set the file:line of the caller of the caller of SG_alloc.
 */
void _sg_mem__set_caller_data(void * p, const char * pszFile, int iLine, const char * pszType)
{
	if (p)
	{
		SG_uint32 iBucket = _sg_mem_hash(p);
		struct _sg_mem_block* pBlock = NULL;

        // TODO the mutex calls return an int.  nonzero means failure
        SG_mutex__lock__bare(&g_mutex_mem);
		pBlock = g_buckets[iBucket];
		while (pBlock)
		{
			if (pBlock->p == p)
			{
				pBlock->caller_pszFile = pszFile;
				pBlock->caller_iLine = iLine;
				pBlock->caller_pszType = pszType;
                break;
			}

			pBlock = pBlock->pNext;
		}

        if (!pBlock)
        {
            fprintf(stderr,"_sg_mem__set_caller_data: BLOCK NOT FOUND for %s:%d\n",pszFile,iLine);
            SG_ASSERT(0);
        }
        SG_mutex__unlock__bare(&g_mutex_mem);
	}
}

void* _sg_malloc_debug_no_ctx(SG_uint32 siz, const char* pszFile, int iLine)
{
	void* p = malloc(siz);
	if (!p)
	{
		fprintf(stderr, "%s:%d: low-level malloc(%d) failed (caller %s:%d)\n",
			__FILE__, __LINE__,
			siz,
			pszFile, iLine);
		return NULL;
	}
	else
	{
		struct _sg_mem_block* pBlock = _sg_mem_add_block(p, siz, pszFile, iLine);
		return pBlock->p;
	}
}

void _sg_malloc_debug(SG_context* pCtx, SG_uint32 siz, void** ppNew, const char* pszFile, int iLine)
{
	void* p = NULL;
	
	SG_NULLARGCHECK_RETURN(ppNew);

	p = _sg_malloc_debug_no_ctx(siz, pszFile, iLine);
	if (!p)
		SG_ERR_THROW_RETURN(SG_ERR_MALLOCFAILED);
	else
		*ppNew = p;
}

/**
 * The name of this routine is a bit of a lie.
 * We don't actually allocate the memory for
 * the object, rather we just adopt it into our
 * leak tracking system.
 *
 * Also, the caller may not know the acutal size
 * of the object, so we just say 1.
 */
void * _sg_alloc_foreign_debug(void * p, const char * pszFile, int iLine)
{
	if (p)
	{
		struct _sg_mem_block * pBlock = _sg_mem_add_block(p,1,pszFile,iLine);
		pBlock->bForeign = SG_TRUE;
	}

	return p;
}

void * _sg_free_foreign_debug(void * p)
{
	_sg_free_debug__no_ctx(p);

	return p;
}

// TODO deprecate this in favor of SG_alloc()....
void* _sg_calloc_debug(SG_uint32 count, SG_uint32 siz, const char* pszFile, int iLine)
{
	void* p = calloc(count, siz);
	if (!p)
	{
		fprintf(stderr, "%s:%d: low-level calloc(%d,%d) failed (caller %s:%d)\n",
				__FILE__, __LINE__,
				count, siz,
				pszFile, iLine);
		return NULL;
	}
	else
	{
		struct _sg_mem_block* pBlock = _sg_mem_add_block(p, (count*siz), pszFile, iLine);
		return pBlock->p;
	}
}

void _sg_free_debug(SG_UNUSED_PARAM(SG_context * pCtx), void* p)
{
	SG_UNUSED(pCtx);

	_sg_free_debug__no_ctx(p);
}

void _sg_free_debug__no_ctx(void* p)
{
	if (!p)
	{
		return ;
	}
	else
	{
		SG_uint32 iBucket = _sg_mem_hash(p);
		struct _sg_mem_block* pPrev = NULL;
		struct _sg_mem_block* pBlock = NULL;
		SG_bool bForeign = SG_FALSE;

        // TODO the mutex calls return an int.  nonzero means failure
        SG_mutex__lock__bare(&g_mutex_mem);
		pBlock = g_buckets[iBucket];
		while (pBlock)
		{
			if (pBlock->p == p)
			{
				break;
			}

			pPrev = pBlock;
			pBlock = pBlock->pNext;
		}

		if (pBlock)
		{
			/* fill the block with junk */
			memset(p, 13, pBlock->nBytes);
            g_live_bytes -= pBlock->nBytes;

			if (pPrev)
			{
				pPrev->pNext = pBlock->pNext;
			}
			else
			{
				g_buckets[iBucket] = pBlock->pNext;
			}

			bForeign = pBlock->bForeign;

			pBlock->p = NULL;
			free(pBlock);
		}
		else
		{
			SG_uint32 k;
			SG_byte * pByte = (SG_byte *)p;
			fprintf(stderr,"SG_FREE: BLOCK NOT FOUND [%p]:\n",p);
			for (k=0; k<32; k++)
				fprintf(stderr, "%02x ", pByte[k]);
			fprintf(stderr,"\n");

			// would like to throw here, but don't have a context.
			SG_ASSERT( (0) );
		}

		// if someone gives us a foreign pointer, we just remove
		// it from our leak tracking, but don't actually free it.
		// this sounds misleading, but it lets this routine be
		// used by both foreign and non-foreign memory frees since
		// we have wrapper macros for both and the caller doesn't
		// actually name this routine directly.

		if (!bForeign)
			free(p);

        SG_mutex__unlock__bare(&g_mutex_mem);
	}

	return ;
}

void _sg_alloc_debug(SG_context* pCtx,
					 SG_uint32 count,
					 SG_uint32 siz,
					 const char * pszFile,
					 int iLine,
					 void ** ppNew)
{
	void * p;

	SG_ARGCHECK_RETURN(count != 0u, count);
	SG_ARGCHECK_RETURN(siz != 0u, siz);
	SG_NULLARGCHECK_RETURN(ppNew);

	p = _sg_calloc_debug(count,siz,pszFile,iLine);
	if (!p)
		SG_ERR_THROW_RETURN(SG_ERR_MALLOCFAILED);

	*ppNew = p;
}

SG_error _sg_alloc_debug_err(SG_uint32 count,
							 SG_uint32 siz,
							 const char * pszFile,
							 int iLine,
							 void ** ppNew)
{
	void * p;

	if (!ppNew)
		return SG_ERR_INVALIDARG;

	p = _sg_calloc_debug(count,siz,pszFile,iLine);
	if (!p)
		return SG_ERR_MALLOCFAILED;

	*ppNew = p;
	return SG_ERR_OK;
}

#else

//////////////////////////////////////////////////////////////////
// THE PRODUCTION VERSION
//////////////////////////////////////////////////////////////////

void _sg_malloc(SG_context * pCtx, SG_uint32 siz, void ** ppNew)
{
	void * p = malloc(siz);
	if (!p)
		SG_ERR_THROW_RETURN(SG_ERR_MALLOCFAILED);
	*ppNew = p;
}

void _sg_free(SG_UNUSED_PARAM(SG_context * pCtx), void* p)
{
	SG_UNUSED(pCtx);

	if (!p)
		return;

	free(p);
}

void _sg_free__no_ctx(void* p)
{
	if (!p)
		return;

	free(p);
}

// TODO deprecate this in favor of SG_alloc()
void* _sg_calloc(SG_uint32 count, SG_uint32 siz)
{
	return calloc(count, siz);
}

/**
 * The normal SG_alloc() that everyone (except for SG_context.c) should use.
 */
void _sg_alloc(SG_context* pCtx, SG_uint32 count, SG_uint32 siz, void ** ppNew)
{
	void * p;

	SG_NULLARGCHECK_RETURN(ppNew);

	p = _sg_calloc(count,siz);
	if (!p)
		SG_ERR_THROW_RETURN(SG_ERR_MALLOCFAILED);

	*ppNew = p;
}

/**
 * The version of SG_alloc() that should only be used by SG_context.c
 */
SG_error _sg_alloc_err(SG_uint32 count, SG_uint32 siz, void ** ppNew)
{
	void * p;

	if (!ppNew)
		return SG_ERR_INVALIDARG;

	p = _sg_calloc(count,siz);
	if (!p)
		return SG_ERR_MALLOCFAILED;

	*ppNew = p;
	return SG_ERR_OK;
}

#endif

