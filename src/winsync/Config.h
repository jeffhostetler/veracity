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

#pragma once

// If no profile is specified on the command line when launching, use this.
#define CONFIG_DEFAULT	"default"

#define CONFIG_ROOT		"/machine/winsync"
#define CONFIG_DEST		"local-dest"
#define CONFIG_SRC		"src"
#define CONFIG_INTERVAL "minutes"

#define DEFAULT_SYNC_EVERY_N_MINUTES 20

void GetProfileSettings(
	SG_context* pCtx, 
	const char* szProfile,
	char** ppszDest, 
	char** ppszSrc, 
	SG_int64* plSyncIntervalMinutes,
	LPWSTR* ppwszErr);

void SetProfileSettings(
	SG_context* pCtx, 
	const char* szProfile,
	const char* pszDest, 
	const char* pszSrc, 
	SG_int64 lSyncIntervalMinutes);

