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
 * @file    sg_log_text_prototypes.h
 * @details Prototypes for the SG_log_text module.
 *
 * This module implements an SG_log__handler that records received progress to a FILE*.
 */

#ifndef H_SG_LOG_TEXT_PROTOTYPES_H
#define H_SG_LOG_TEXT_PROTOTYPES_H

BEGIN_EXTERN_C;

/**
 * Initializes an SG_log_text__data structure with default configuration.
 */
void SG_log_text__set_defaults(
	SG_context*        pCtx, //< [in] [out] Error and context information.
	SG_log_text__data* pData //< [in] The data to set to default values.
	);

/**
 * Convenient and type-safe wrapper around SG_log__register_handler(pCtx, gpFileLogHandler, pData).
 */
void SG_log_text__register(
	SG_context*        pCtx,   //< [in] [out] Error and context information.
	SG_log_text__data* pData,  //< [in] The data of the file logger to register.
	SG_log__stats*     pStats, //< [in] A structure to update with stats about the handler's usage.
	                           //<      NULL if no stats are needed.
	                           //<      Note: It's up to the caller to initialize with starting values!
	                           //<            Usually you'll want to just use a simple memset to zero.
	SG_uint32          uFlags  //< [in] Flags to associated with the registered handler.
	);

/**
 * Convenient and type-safe wrapper around SG_log__unregister_handler(pCtx, gpFileLogHandler, pData).
 */
void SG_log_text__unregister(
	SG_context*        pCtx, //< [in] [out] Error and context information.
	SG_log_text__data* pData //< [in] The data of the file logger to unregister.
	);

/**
 * An SG_log_text__writer that writes the text to STDOUT/STDERR.
 * pWriterData is unused, so specifying NULL is easiest.
 */
void SG_log_text__writer__std(
	SG_context*      pCtx,
	void*            pWriterData, //< NULL
	const SG_string* pText,
	SG_bool          bError
	);

/**
 * An SG_log_text__writer that writes the text to an already open file.
 * pWriterData must be an SG_file* that is open and ready for writing.
 */
void SG_log_text__writer__file(
	SG_context*      pCtx,
	void*            pWriterData, //< SG_file*
	const SG_string* pText,
	SG_bool          bError
	);

/**
 * An SG_log_text__writer that writes the text to a file specified by a path.
 * pWriterData must be an SG_log_text__writer__path__data*.
 */
void SG_log_text__writer__path(
	SG_context*      pCtx,
	void*            pWriterData, //< SG_log_text__writer__path__data*
	const SG_string* pText,
	SG_bool          bError
	);

/**
 * Initializes an SG_log_text__writer__path__data structure.
 */
void SG_log_text__writer__path__set_defaults(
	SG_context*                      pCtx,
	SG_log_text__writer__path__data* pData
	);

/**
 * An SG_log_text__writer that writes the text to a file whose name is based on the current date.
 * pWriterData must be an SG_log_text__writer__daily_path__data*.
 */
void SG_log_text__writer__daily_path(
	SG_context*      pCtx,
	void*            pWriterData, //< SG_log_text__writer__daily_path__data*
	const SG_string* pText,
	SG_bool          bError
	);

/**
 * Initializes an SG_log_text__writer__daily_path__data structure.
 */
void SG_log_text__writer__daily_path__set_defaults(
	SG_context*                            pCtx,
	SG_log_text__writer__daily_path__data* pData
	);

END_EXTERN_C;

#endif
