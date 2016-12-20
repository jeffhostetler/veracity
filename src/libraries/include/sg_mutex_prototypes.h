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

/**
 *
 * @file sg_mutex_prototypes.h
 *
 * @details
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_MUTEX_PROTOTYPES_H
#define H_SG_MUTEX_PROTOTYPES_H

BEGIN_EXTERN_C;

/*
Clean up an existing SG_mutex.
*/
void SG_mutex__destroy(SG_mutex* pm);

/*
Initialize a new SG_mutex. 
*/
void SG_mutex__init(SG_context* pCtx, SG_mutex* pm);

/*
Acquire the lock for an existing SG_mutex. 
*/
void SG_mutex__lock(SG_context* pCtx, SG_mutex* pm);


/*
Attempt to acquire the lock for an existing SG_mutex without blocking.
*pb == SG_TRUE if you've acquired the lock, SG_FALSE if not.
*/
void SG_mutex__trylock(SG_context* pCtx, SG_mutex* pm, SG_bool* pb);

/*
Release the lock for an existing SG_mutex. 
*/
void SG_mutex__unlock(SG_context* pCtx, SG_mutex* pm);

/*
Initializes a new SG_mutex. 
If you call this version, check the return value; a non-zero return code is an error.
If you have an SG_context*, you probably want to use SG_mutex__init.
*/
int SG_mutex__init__bare(SG_mutex* pm);

/*
Acquire the lock for an existing SG_mutex. 
If you call this version, check the return value; a non-zero return code is an error.
If you have an SG_context*, you probably want to use SG_mutex__lock.
*/
int SG_mutex__lock__bare(SG_mutex* pm);

/*
Attempt to acquire the lock for an existing SG_mutex without blocking.
*pb == SG_TRUE if you've acquired the lock, SG_FALSE if not.
If you call this version, check the return value; a non-zero return code is an error.
If you have an SG_context*, you probably want to use SG_mutex__trylock.
*/
int SG_mutex__trylock__bare(SG_mutex* pm, SG_bool* pb);

/*
Release the lock for an existing SG_mutex. 
If you call this version, check the return value; a non-zero return code is an error.
If you have an SG_context*, you probably want to use SG_mutex__unlock.
*/
int SG_mutex__unlock__bare(SG_mutex* pm);

END_EXTERN_C;

#endif//H_SG_MUTEX_PROTOTYPES_H

