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
 * @file sg_password_prototypes.h
 *
 * @details Routines for storing/retrieving saved passwords.
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_PASSWORD_PROTOTYPES_H
#define H_SG_PASSWORD_PROTOTYPES_H

BEGIN_EXTERN_C;

SG_bool SG_password__supported(void);

void SG_password__set(
	SG_context *pCtx, 
	const char *szRepoSpec,
	SG_string *pstrUserName,
	SG_string *pstrPassword);

void SG_password__get(
	SG_context *pCtx, 
	const char *szRepoSpec, 
	const char *szUsername, 
	SG_string **ppstrPassword);

#if defined(WINDOWS)

void SG_password__forget_all(SG_context *pCtx);

#endif

END_EXTERN_C;

#endif//H_SG_PASSWORD_PROTOTYPES_H
