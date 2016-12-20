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

#include <sg.h>

int SG_mutex__init__bare(SG_mutex* pm)
{
#if defined(MAC) || defined(LINUX)
    int rc;
    pthread_mutexattr_t att;

    rc = pthread_mutexattr_init(&att);
    if (rc)
    {
        return rc;
    }

    rc = pthread_mutexattr_settype(&att, PTHREAD_MUTEX_RECURSIVE);
    if (rc)
    {
        pthread_mutexattr_destroy(&att);
        return rc;
    }

    rc = pthread_mutex_init(&pm->mtx, &att);
    if (rc)
    {
        pthread_mutexattr_destroy(&att);
        return rc;
    }

    pthread_mutexattr_destroy(&att);

    return 0;
#endif

#if defined(WINDOWS)
    InitializeCriticalSection(&pm->cs);
    return 0;
#endif
}

void SG_mutex__init(SG_context* pCtx, SG_mutex* pm)
{
	int rc;

	SG_NULLARGCHECK_RETURN(pm);

	rc = SG_mutex__init__bare(pm);
	if (rc)
		SG_ERR_THROW2_RETURN(SG_ERR_MUTEX_FAILURE, (pCtx, "init: %d", rc));
}

void SG_mutex__destroy(SG_mutex* pm)
{
#if defined(MAC) || defined(LINUX)
    (void) pthread_mutex_destroy(&pm->mtx);
#endif

#if defined(WINDOWS)
    DeleteCriticalSection(&pm->cs);
#endif
}

int SG_mutex__lock__bare(SG_mutex* pm)
{
#if defined(MAC) || defined(LINUX)
    return pthread_mutex_lock(&pm->mtx);
#endif

#if defined(WINDOWS)
    EnterCriticalSection(&pm->cs);
    return 0;
#endif
}

void SG_mutex__lock(SG_context* pCtx, SG_mutex* pm)
{
	int rc;

	SG_NULLARGCHECK_RETURN(pm);

	rc = SG_mutex__lock__bare(pm);
	if (rc)
		SG_ERR_THROW2_RETURN(SG_ERR_MUTEX_FAILURE, (pCtx, "lock: %d", rc));
}


int SG_mutex__trylock__bare(SG_mutex* pm, SG_bool* pb)
{
#if defined(MAC) || defined(LINUX)
    int rc = pthread_mutex_trylock(&pm->mtx);
    if (rc)
    {
        if (EBUSY == rc)
        {
            *pb = SG_FALSE;
            return 0;
        }
        else
        {
            return rc;
        }
    }
    else
    {
        *pb = SG_TRUE;
        return 0;
    }
#endif

#if defined(WINDOWS)
    if (TryEnterCriticalSection(&pm->cs))
    {
        *pb = SG_TRUE;
        return 0;
    }
    else
    {
        *pb = SG_FALSE;
        return 0;
    }
#endif
}

void SG_mutex__trylock(SG_context* pCtx, SG_mutex* pm, SG_bool* pb)
{
	int rc;

	SG_NULLARGCHECK_RETURN(pm);
	SG_NULLARGCHECK_RETURN(pb);

	rc = SG_mutex__trylock__bare(pm, pb);
	if (rc)
		SG_ERR_THROW2_RETURN(SG_ERR_MUTEX_FAILURE, (pCtx, "trylock: %d", rc));
}

int SG_mutex__unlock__bare(SG_mutex* pm)
{
#if defined(MAC) || defined(LINUX)
    return pthread_mutex_unlock(&pm->mtx);
#endif

#if defined(WINDOWS)
    LeaveCriticalSection(&pm->cs);
    return 0;
#endif
}

void SG_mutex__unlock(SG_context* pCtx, SG_mutex* pm)
{
	int rc;

	SG_NULLARGCHECK_RETURN(pm);

	rc = SG_mutex__unlock__bare(pm);
	if (rc)
		SG_ERR_THROW2_RETURN(SG_ERR_MUTEX_FAILURE, (pCtx, "unlock: %d", rc));
}