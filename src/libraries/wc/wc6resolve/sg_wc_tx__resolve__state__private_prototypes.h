/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License"	);
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef H_SG_RESOLVE_STATE_PROTOTYPES_H
#define H_SG_RESOLVE_STATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////
// TODO 2012/03/29 The contents of this file were moved out of
// TODO            src/libraries/include/sg_resolve_prototypes.h
// TODO            during the conversion from PendingTree to WC.
// TODO
// TODO            The symbols were declared PUBLIC from the point
// TODO            of view of sglib.
// TODO
// TODO            I think all of these routines should be PRIVATE
// TODO            to WC so I've moved them into header files marked
// TODO            WC-private, but I haven't taken time to rename
// TODO            with "sg_" or "_sg_" prefixes.
// TODO
//////////////////////////////////////////////////////////////////

/**
 * Gets information about a state.
 */
void SG_resolve__state__get_info(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	SG_resolve__state eState,    //< [in] The state to get info about.
	const char**      ppName,    //< [out] The name of the state.
	const char**      ppProblem, //< [out] A brief summary of the problem that causes choices on this state.
	const char**      ppCommand  //< [out] A brief summary of what must be done to resolve choices on this state.
	);

/**
 * Parses a state's name back into its enumerated value.
 * Returns SG_RESOLVE__STATE__COUNT if the name is invalid.
 */
void SG_resolve__state__check_name(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	const char*        szName, //< [in] The name to parse into a state.
	SG_resolve__state* pState  //< [out] The parsed state.
	                           //<       SG_RESOLVE__STATE__COUNT if it could not be parsed.
	);

/**
 * Parses a state's name back into its enumerated value.
 * Throws SG_ERR_NOT_FOUND if the name is invalid.
 */
void SG_resolve__state__parse_name(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	const char*        szName, //< [in] The name to parse into a state.
	SG_resolve__state* pState  //< [out] The parsed state.
	);

END_EXTERN_C;

#endif
