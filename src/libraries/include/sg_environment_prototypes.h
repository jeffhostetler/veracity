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
 * @file sg_environment_prototypes.h
 */

#ifndef H_SG_ENVIRONMENT_PROTOTYPES_H
#define H_SG_ENVIRONMENT_PROTOTYPES_H

BEGIN_EXTERN_C

/**
 * Returns the value of the environment variable named in pszUtf8Name.
 * If the variable is undefined, *ppstrValue is NULL and *piLenStr is 0.
 * If the variable is defined *ppstrValue holds the value and *piLenStr is its length in bytes, 
 * making it easy to identity defined but blank variables.
 */
void SG_environment__get__str(SG_context* pCtx, const char* pszUtf8Name, SG_string** ppstrValue, SG_uint32* piLenStr);

/**
 * Sets the valule of the environment variable named in pszUtf8Name
 */
void SG_environment__set__sz(SG_context *pCtx, const char *pszUtf8Name, const char *pszUtf8Value);

END_EXTERN_C

#endif
