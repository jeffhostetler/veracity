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
 * @file    sg_log_console_prototypes.h
 * @details Prototypes for the SG_log_console module.
 *
 * This module implements an SG_log__handler that records received progress to SG_console.
 */

#ifndef H_SG_LOG_CONSOLE_PROTOTYPES_H
#define H_SG_LOG_CONSOLE_PROTOTYPES_H

BEGIN_EXTERN_C;

/**
 * Initializes an SG_log_console__data structure with default configuration.
 */
void SG_log_console__set_defaults(
	SG_context*        pCtx, //< [in] [out] Error and context information.
	SG_log_console__data* pData //< [in] The data to set to default values.
	);

/**
 * Convenient and type-safe wrapper around SG_log__register_handler(pCtx, gpConsoleLogHandler, pData).
 */
void SG_log_console__register(
	SG_context*           pCtx,   //< [in] [out] Error and context information.
	SG_log_console__data* pData,  //< [in] The data of the file logger to register.
	SG_log__stats*        pStats, //< [in] A structure to update with stats about the handler's usage.
	                              //<      NULL if no stats are needed.
	                              //<      Note: It's up to the caller to initialize with starting values!
	                              //<            Usually you'll want to just use a simple memset to zero.
	SG_uint32             uFlags  //< [in] Flags to associated with the registered handler.
	);

/**
 * Convenient and type-safe wrapper around SG_log__unregister_handler(pCtx, gpConsoleLogHandler, pData).
 */
void SG_log_console__unregister(
	SG_context*        pCtx, //< [in] [out] Error and context information.
	SG_log_console__data* pData //< [in] The data of the file logger to unregister.
	);

END_EXTERN_C;

#endif
