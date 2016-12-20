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
 * @file sg__do_cmd_resolve2.c
 *
 * @details Implements the resolve command.
 *
 */


/*
**
** Includes
**
*/

#include <sg.h>
#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

//////////////////////////////////////////////////////////////////
// TODO 2012/03/29 The contents of
// TODO            src/libraries/include/sg_resolve_typedefs.h
// TODO            src/libraries/include/sg_resolve_prototypes.h
// TODO            were moved out of sglib and into the following
// TODO            PRIVATE headers in WC during the intial conversion
// TODO            from PendingTree to WC.
// TODO
// TODO            The symbols were declared PUBLIC from the point
// TODO            of view of sglib.  But I think this file should
// TODO            be changed to refer to a returned vhash/varray/etc
// TODO            rather than having a handle to a SG_resolve as
// TODO            we convert to a WC-TX-based model.
// TODO
// TODO            So for now, I'm just going to import the private
// TODO            declarations.
#include  "wc6resolve/sg_wc_tx__resolve__private_typedefs.h"

#include  "wc6resolve/sg_wc_tx__resolve__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__choice__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__item__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__state__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__value__private_prototypes.h"
//////////////////////////////////////////////////////////////////

#include <ctype.h>
#include "sg_typedefs.h"
#include "sg_prototypes.h"


/*
**
** Types
**
*/

/**
 * Type of function that handles a resolve subcommand.
 */
typedef void _subcommand__command_handler(
	SG_context*            pCtx,     //< [in] [out] Error and context info.
	const SG_option_state* pOptions, //< [in] Options parsed from the command line.
	SG_uint32              uArgs,    //< [in] Number of elements in ppArgs.
	const char**           ppArgs    //< [in] Free arguments remaining on the command line.
	                                 //<      DOES include the subcommand name itself.
	);

/**
 * Specification of a subcommand that can be passed to the main resolve command.
 */
typedef struct _subcommand__command_spec
{
	const char*                   szName;   //< Name of the subcommand, as used on the command line.
	_subcommand__command_handler* fHandler; //< Function that implements the subcommand.
}
_subcommand__command_spec;

/**
 * Specification of a single choice that is not dependent on SG_resolve data.
 */
typedef struct _choice_spec
{
	char*             szGid;  //< The GID of the item that the choice is on.
	SG_resolve__state eState; //< The type of choice on the item.
	                          //< If SG_RESOLVE__STATE__COUNT, then the spec
	                          //< indicates the only available choice on the item.
}
_choice_spec;

/**
 * Static initializer for _choice_spec.
 */
#define _CHOICE_SPEC__INIT { NULL, SG_RESOLVE__STATE__COUNT }

/**
 * Data shared by an entire interactive prompting session.
 * Mostly options and state data used by _interactive__next and its helpers.
 */
typedef struct _interactive__session
{
	SG_vector*                 pChoices;    //< List of choices to prompt for during the session.
	                                        //< Contains _interactive__choice_spec*
	SG_uint32                  uChoices;    //< Length of pChoices, cached for easy retrieval,
	                                        //< since it won't change during the course of a session.
	SG_uint32                  uChoice;     //< Index into pChoices of the current choice.
	                                        //< If equal to uChoices, the session is ended/inactive.
	SG_bool                    bWrite;      //< Whether or not the next prompt should write the choice's details.
	SG_bool                    bAutomatic;  //< Whether or not the current conflict has used an automatic command.
	const char*                szViewTool;  //< Use this tool by default for view commands.
	                                        //< If NULL, the appropriate system-wide default is used.
	const char*                szDiffTool;  //< Use this tool by default for diff commands.
	                                        //< If NULL, the appropriate system-wide default is used.
	const char*                szMergeTool; //< Use this tool by default for merge commands.
	                                        //< If NULL, the appropriate system-wide default is used.
}
_interactive__session;

/**
 * Static initializer for _interactive__session.
 */
#define _INTERACTIVE__SESSION__INIT { NULL, 0u, 0u, SG_TRUE, SG_FALSE, NULL, NULL, NULL }

/**
 * Data about a single interactive command entered by the user.
 */
typedef struct _interactive__command
{
	const struct _interactive__command_spec* pSpec;     //< The command spec indicated by the command.
	const char*                              szValue1;  //< The first value argument, or NULL if not specified.
	const char*                              szValue2;  //< The second value argument, or NULL if not specified.
	const char*                              szTool;    //< The tool argument, or NULL if not specified.
	SG_resolve__value*                       pValue1;   //< szValue1 parsed into an actual value, or NULL if it couldn't be.
	SG_resolve__value*                       pValue2;   //< szValue2 parsed into an actual value, or NULL if it couldn't be.
}
_interactive__command;

/**
 * Specification for a single argument to an interactive command.
 */
typedef enum _interactive__command_arg
{
	ARGUMENT_TYPE_MASK = 0x0000FFFF, //< Mask of argument types.
	ARGUMENT_VALUE     = 1,          //< Argument is the name of a value.
	ARGUMENT_TOOL      = 2,          //< Argument is the name of a tool.

	ARGUMENT_FLAG_MASK = 0xFFFF0000, //< Mask of optional argument flags.
	ARGUMENT_OPTIONAL  = 1 << 16,    //< Argument is optional.
}
_interactive__command_argument;

/**
 * Type of function that handles an interactive user command.
 */
typedef void _interactive__command_handler(
	SG_context*                  pCtx,     //< [in] [out] Error and context info.
	_interactive__session*       pSession, //< [in] The interactive session data.
	SG_resolve__choice*          pChoice,  //< [in] The choice that the command targets.
	const _interactive__command* pCommand  //< [in] The entered command.
	);

/**
 * Specification of a command that can be entered at the interactive prompt.
 */
typedef struct _interactive__command_spec
{
	const char*                    szName;         //< Name of the command, as it must be entered/used.
	const char*                    szAbbreviation; //< Optional shortcut for the command, usually one letter.
	SG_bool                        bMergeableOnly; //< True if the command is only valid for mergeable choices.
	_interactive__command_handler* fHandler;       //< Function that implements the command.
	int                            aArguments[5];  //< Specification of arguments used by the command.
	                                               //< Each entry is a mask of _interactive__command_arg values.
	                                               //< Must be zero-terminated.
	const char*                    szToolType;     //< The type of filetool used by this command.
	                                               //< NULL if the command doesn't use a filetool.
	const char*                    szPrompt;       //< String to use for the command in the interactive prompt.
	const char*                    szUsage;        //< One line message to display for the command's usage.
	const char*                    szDescription;  //< One line description of what the command does.
}
_interactive__command_spec;


/*
**
** Globals
**
*/

/*
 * Forward declarations for all of the interactive command handlers.
 */
static _subcommand__command_handler _subcommand__handler__list;
static _subcommand__command_handler _subcommand__handler__next;
static _subcommand__command_handler _subcommand__handler__filename;
//static _subcommand__command_handler _subcommand__handler__view;
static _subcommand__command_handler _subcommand__handler__diff;
static _subcommand__command_handler _subcommand__handler__merge;
static _subcommand__command_handler _subcommand__handler__accept;

/**
 * Specification of all the available subcommands.
 */
static const _subcommand__command_spec gaSubcommands[] =
{
	{ "list",     _subcommand__handler__list },
	{ "next",     _subcommand__handler__next },
//	{ "view",     _subcommand__handler__view },
	{ "diff",     _subcommand__handler__diff },
	{ "merge",    _subcommand__handler__merge },
	{ "accept",   _subcommand__handler__accept },
};

/**
 * The default subcommand to try when we get an argument
 * that doesn't match any other defined subcommands.
 */
static const _subcommand__command_spec gcDefaultSubcommand = { "filename", _subcommand__handler__filename };

/*
 * Forward declarations for all of the interactive command handlers.
 */
//static _interactive__command_handler _interactive__handler__view;
static _interactive__command_handler _interactive__handler__diff;
static _interactive__command_handler _interactive__handler__merge;
static _interactive__command_handler _interactive__handler__accept;
static _interactive__command_handler _interactive__handler__repeat;
static _interactive__command_handler _interactive__handler__skip;
static _interactive__command_handler _interactive__handler__back;
static _interactive__command_handler _interactive__handler__quit;
static _interactive__command_handler _interactive__handler__help;

/**
 * Specification of all the available interactive commands.
 */
static const _interactive__command_spec gaInteractiveCommands[] =
{
/*	{
		"view", "v", SG_TRUE, _interactive__handler__view,
		{ ARGUMENT_VALUE, ARGUMENT_TOOL | ARGUMENT_OPTIONAL, 0, },
		NULL,
		"(v)iew",
		"v[iew] VALUE [TOOL]",
		"View a resolution value in a viewer tool (not implemented).",
	},
*/	{
		"diff", "d", SG_TRUE, _interactive__handler__diff,
		{ ARGUMENT_VALUE, ARGUMENT_VALUE, ARGUMENT_TOOL | ARGUMENT_OPTIONAL, 0, },
		SG_DIFFTOOL__TYPE,
		"(d)iff",
		"d[iff] VALUE1 VALUE2 [TOOL]",
		"Diff two resolution values in a diff tool.",
	},
	{
		"merge", "m", SG_TRUE, _interactive__handler__merge,
		{ ARGUMENT_TOOL | ARGUMENT_OPTIONAL, 0, },
		SG_MERGETOOL__TYPE,
		"(m)erge",
		"m[erge] [TOOL]",
		"Create a new resolution value by merging the 'baseline' and 'other' values.",
	},
	{
		"accept", "a", SG_FALSE, _interactive__handler__accept,
		{ ARGUMENT_VALUE, 0, },
		NULL,
		"(a)ccept",
		"a[ccept] VALUE",
		"Accept a value as this conflict's resolution and move to the next one.",
	},
	{
		"repeat", "r", SG_FALSE, _interactive__handler__repeat,
		{ 0, },
		NULL,
		"(r)epeat",
		"r[epeat]",
		"Display the details of the current conflict again.",
	},
	{
		"skip", "s", SG_FALSE, _interactive__handler__skip,
		{ 0, },
		NULL,
		"(s)kip",
		"s[kip]",
		"Move to the next conflict without accepting a value on this one.",
	},
	{
		"back", "b", SG_FALSE, _interactive__handler__back,
		{ 0, },
		NULL,
		"(b)ack",
		"b[ack]",
		"Return to the previous conflict.",
	},
	{
		"quit", "q", SG_FALSE, _interactive__handler__quit,
		{ 0, },
		NULL,
		"(q)uit",
		"q[uit]",
		"Stop resolving conflicts.",
	},
	{
		"help", "h", SG_FALSE, _interactive__handler__help,
		{ 0, },
		NULL,
		"(h)elp",
		"h[elp]",
		"Display this usage information.",
	},
};


/*
**
** Helpers
**
*/

/**
 * A thread-safe and SG-like implementation of strtok.
 * TODO: Move this to sg_str_utils and write tests for it.
 */
static void _SG_strtok(
	SG_context*  pCtx,         //< [in] [out] Error and context info.
	char*        szString,     //< [in] The string to tokenize.
	                           //<      If NULL, tokenization is continued on ppHandle.
	const char*  szDelimiters, //< [in] A string of characters that will delimit the next token.
	                           //<      If NULL, then whitespace delimiters will be used.
	SG_bool      bSkipEmpty,   //< [in] If true, zero-length tokens will be skipped instead of returned.
	const char** ppToken,      //< [out] The next token in the string.
	                           //<       NULL if there are no more tokens.
	void**       ppHandle      //< [in] [out] A handle to this tokenization string/session/instance.
	)
{
	static const char* szWhitespaceDelimiters = " \t\r\n";

	const char* szToken = NULL;

	SG_NULLARGCHECK(ppToken);
	SG_NULLARGCHECK(ppHandle);

	// if they didn't supply a string, start with the handle
	if (szString == NULL)
	{
		szString = (char*)*ppHandle;
	}

	// if they didn't supply delimiters, use whitespace
	if (szDelimiters == NULL)
	{
		szDelimiters = szWhitespaceDelimiters;
	}

	// if we have a string to search, do it
	// we won't if the handle is NULL, meaning we found the last token last time
	if (szString != NULL)
	{
		// if we're skipping empty tokens
		// then advance until we find a non-delimiter
		while (
			   *szString != '\0'
			&& bSkipEmpty == SG_TRUE
			&& strchr(szDelimiters, *szString) != NULL
			)
		{
			++szString;
		}

		// store off the start of the string
		// so we can return it as the token
		szToken = szString;

		// advance until we find a delimiter
		while (
			   *szString != '\0'
			&& strchr(szDelimiters, *szString) == NULL
			)
		{
			++szString;
		}

		// setup the handle to keep going with the next call
		if (*szString == '\0')
		{
			// we found the end of the string, so this is the last token
			// set the handle to NULL, so next time we'll know that we're done
			*ppHandle = NULL;
		}
		else
		{
			// set the handle to the character after the delimiter
			// that's where we'll pick up next time
			*ppHandle = (void*)(szString + 1u);
		}

		// replace the delimiter with a NULL-terminator
		*szString = '\0';
	}

	// return the token
	*ppToken = szToken;

fail:
	return;
}

/**
 * Prompts the user for a line of input and returns it.
 */
static void _console__prompt(
	SG_context* pCtx,     //< [in] [out] Error and context info.
	SG_string** ppInput,  //< [out] The input entered by the user.
	const char* szPrompt, //< [in] A prompt to display to the user, or NULL to not bother.
	...                   //< [in] Data to substitute into the prompt.
	)
{
	SG_string* sInput = NULL;

	SG_NULLARGCHECK(ppInput);

	if (szPrompt != NULL)
	{
		va_list pArgs;

		va_start(pArgs, szPrompt);
		SG_console__va(pCtx, SG_CS_STDOUT, szPrompt, pArgs);
		va_end(pArgs);
		SG_ERR_CHECK_CURRENT;
	}

	SG_ERR_CHECK(  SG_console__readline_stdin(pCtx, &sInput)  );

	*ppInput = sInput;
	sInput = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sInput);
	return;
}


/*
**
** Private Functionality
**
*/


/**
 * Gets the resolve item and choice that corresond to a given spec.
 * Prints error messages and returns NULLs if the item/choice cannot be found.
 */
static void _choice_spec__find(
	SG_context*          pCtx,     //< [in] [out] Error and context info.
	const _choice_spec*  pSpec,    //< [in] Spec of the item/choice to find.
	SG_resolve*          pResolve, //< [in] Resolve data to find the item/choice in.
	SG_resolve__item**   ppItem,   //< [out] The found item, or NULL if not found.
	SG_resolve__choice** ppChoice  //< [out] The found choice, or NULL if not found.
	)
{
	SG_resolve__item*   pItem   = NULL;
	SG_resolve__choice* pChoice = NULL;

	SG_NULLARGCHECK(pResolve);
	SG_NULLARGCHECK(pSpec);

	SG_ERR_CHECK(  SG_resolve__get_item__gid(pCtx, pResolve, pSpec->szGid, &pItem)  );
	if (pSpec->eState == SG_RESOLVE__STATE__COUNT)
	{
		// they want us to find the item's only choice
		SG_int32 iState = 0u;

		// run through each possible state
		for (iState = 0u; iState < SG_RESOLVE__STATE__COUNT; ++iState)
		{
			SG_resolve__choice* pCurrentChoice = NULL;

			// see if the item has a choice for this state
			SG_ERR_CHECK(  SG_resolve__item__check_choice__state(pCtx, pItem, (SG_resolve__state)iState, &pCurrentChoice)  );
			if (pCurrentChoice != NULL)
			{
				if (pChoice == NULL)
				{
					// this is the first choice we've found on the item
					// store it for later and possible returning
					pChoice = pCurrentChoice;
				}
				else
				{
					// this is the second choice we've found on the item
					// TODO 2012/08/28 This doesn't throw, so the caller
					// TODO            won't see the error and will probably
					// TODO            keep looping.  And we won't be able
					// TODO            to exit() with non-zero status.
					// TODO
					// TODO            Looks like most of the errors in this
					// TODO            routine are like this too, but I'm
					// TODO            just documenting it here because this
					// TODO            one bit me.
					SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "The specified item has multiple conflicts, so one must be explicitly specified.\n")  );
				}
			}
		}

		// if we didn't find a choice, throw an error
		if (pChoice == NULL)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "The specified item has no conflicts.\n")  );
		}
	}
	else
	{
		// they want a specific choice on the item
		SG_ERR_CHECK(  SG_resolve__item__check_choice__state(pCtx, pItem, pSpec->eState, &pChoice)  );
		if (pChoice == NULL)
		{
			const char* szState = NULL;
			
			SG_ERR_CHECK(  SG_resolve__state__get_info(pCtx, pSpec->eState, &szState, NULL, NULL)  );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "No '%s' conflict was found on the given item.\n", szState)  );
		}
	}

	// return our data
	if (ppItem != NULL)
	{
		*ppItem   = pItem;
	}
	if (ppChoice != NULL)
	{
		*ppChoice = pChoice;
	}

fail:
	return;
}

/**
 * An SG_free_callback that frees a _choice_spec.
 */
void _choice_spec__free(
	SG_context* pCtx, //< [in] [out] Error and context info.
	void*       pData //< [in] The _interactive__choice_spec to free.
	)
{
	if (pData != NULL)
	{
		_choice_spec* pSpec = (_choice_spec*)pData;

		SG_NULLFREE(pCtx, pSpec->szGid);
		SG_NULLFREE(pCtx, pSpec);
	}
}

/**
 * User data used by _add_choice callback.
 */
typedef struct _choice_spec__append__data
{
	SG_vector*        pVector; //< [in] Vector of _interactive__choice_spec* to add to.
	SG_uint32         uCount;  //< [in] Maximum number of choices to add.
	                           //<      Zero indicates no maximum.
	SG_resolve__state eState;  //< [in] Only append choices whose state matches this one.
	                           //<      If SG_RESOLVE__STATE__COUNT, all choices are appended.
}
_choice_spec__append__data;

/**
 * An SG_resolve__choice__callback that adds each choice to a vector of choice specs.
 */
void _choice_spec__append(
	SG_context*         pCtx,      //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,   //< [in] The current choice.
	void*               pUserData, //< [in] An _action__interactive__add_choice__data*
	SG_bool*            pContinue  //< [out] Whether or not iteration should continue.
	)
{
	_choice_spec__append__data* pData  = (_choice_spec__append__data*)pUserData;
	_choice_spec*               pSpec  = NULL;
	SG_resolve__state           eState = SG_RESOLVE__STATE__COUNT;

	// only consider this choice if it matches our state filter
	SG_ERR_CHECK(  SG_resolve__choice__get_state(pCtx, pChoice, &eState)  );
	if (pData->eState == SG_RESOLVE__STATE__COUNT || pData->eState == eState)
	{
		SG_resolve__item*           pItem  = NULL;
		const char*                 szGid  = NULL;

		// get the choice's item's GID
		SG_ERR_CHECK(  SG_resolve__choice__get_item(pCtx, pChoice, &pItem)  );
		SG_ERR_CHECK(  SG_resolve__item__get_gid(pCtx, pItem, &szGid)  );

		// allocate a new choice spec and populate it
		SG_alloc1(pCtx, pSpec);
		SG_ERR_CHECK(  SG_strdup(pCtx, szGid, &pSpec->szGid)  );
		pSpec->eState = eState;

		// add the new spec to the list
		SG_ERR_CHECK(  SG_vector__append(pCtx, pData->pVector, (void*)pSpec, NULL)  );
		pSpec = NULL;

		// decrement the count
		*pContinue = SG_TRUE;
		if (pData->uCount > 0u)
		{
			pData->uCount -= 1u;
			if (pData->uCount == 0u)
			{
				*pContinue = SG_FALSE;
			}
		}
	}

fail:
	_choice_spec__free(pCtx, pSpec);
	return;
}

/**
 * Populates a vector of _choice_spec structures according to various options.
 */
static void _choice_spec__populate(
	SG_context*        pCtx,            //< [in] [out] Error and context info.
	const char*        szPath,          //< [in] Path of an item to add choices from.
	                                    //<      If NULL, choices from all items are added.
	SG_resolve__state  eState,          //< [in] State of choices to add.
	                                    //<      If SG_RESOLVE__STATE__COUNT, choices of all states are added.
	SG_bool            bOnlyUnresolved, //< [in] If true, only unresolved choices are added.
	                                    //<      If false, choices are added regardless of resolved status.
	SG_uint32          uMax,            //< [in] The maximum number of choices to add.
	                                    //<      Zero indicates no maximum.
	SG_vector**        ppVector         //< [out] The populated vector.
	)
{
	SG_wc_tx * pWcTx = NULL;
	SG_resolve*                pResolve     = NULL;		// we do not own this
	SG_vector*                 pVector      = NULL;
	_choice_spec__append__data cData;
	char*                      szGid        = NULL;


	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, NULL, SG_TRUE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );
	
	// allocate the vector
	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pVector, 10u)  );

	// populate the vector with choices
	cData.pVector = pVector;
	cData.uCount  = uMax;
	cData.eState  = eState;
	if (szPath == NULL)
	{
		// no item specified, includes choices from all of them
		SG_ERR_CHECK(  SG_resolve__foreach_choice(pCtx, pResolve, bOnlyUnresolved, SG_FALSE, _choice_spec__append, &cData)  );
	}
	else
	{
		SG_resolve__item* pItem = NULL;

		// an item was specified, only include choices from that one
		SG_ERR_CHECK(  SG_wc_tx__get_item_gid(pCtx, pWcTx, szPath, &szGid)  );
		SG_ERR_CHECK(  SG_resolve__get_item__gid(pCtx, pResolve, szGid, &pItem)  );
		SG_ERR_CHECK(  SG_resolve__item__foreach_choice(pCtx, pItem, bOnlyUnresolved, _choice_spec__append, &cData)  );
	}

	// return the vector
	*ppVector = pVector;
	pVector = NULL;

fail:
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, pVector, _choice_spec__free);
	SG_NULLFREE(pCtx, szGid);
}

/**
 * Finds a choice in the resolve data of a specified directory.
 */
static void _find_choice(
	SG_context*          pCtx,         //< [in] [out] Error and context info.
	SG_wc_tx*            pWcTx,        //< [in] The TX that owns pResolve.
	SG_resolve*          pResolve,     //< [in] The resolve data to find a choice in.
	const char*          szPath,       //< [in] Path of the item to find a choice on.
	SG_resolve__state    eState,       //< [in] State of the choice to find on the item.
	                                   //<      If SG_RESOLVE__STATE__COUNT, finds the only choice on the item (or NULL if there are multiple).
	SG_resolve__item**   ppItem,       //< [out] The item that owns the found choice, or NULL if none was found.
	SG_resolve__choice** ppChoice      //< [out] The found choice, or NULL if none was found.
	)
{
	_choice_spec cSpec = _CHOICE_SPEC__INIT;

	// populate a choice spec
	SG_ERR_CHECK(  SG_wc_tx__get_item_gid(pCtx, pWcTx, szPath, &cSpec.szGid)  );
	cSpec.eState = eState;

	// find the requested choice
	SG_ERR_CHECK(  _choice_spec__find(pCtx, &cSpec, pResolve, ppItem, ppChoice)  );

fail:
	SG_NULLFREE(pCtx, cSpec.szGid);
}

/**
 * An SG_resolve__value__callback that retrieves information about the value.
 */
static void _write_choice__verbose__get_value(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	SG_resolve__value* pValue,    //< [in] The current value.
	void*              pUserData, //< [in] SG_vhash to add the value label/summary to.
	SG_bool*           pContinue  //< [out] Whether or not iteration should continue.
	)
{
	SG_vhash*   pData    = (SG_vhash*)pUserData;
	const char* szLabel  = NULL;
	SG_string*  sSummary = NULL;
	SG_string*  sLabel   = NULL;

	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	// get data from the value
	SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pValue, &szLabel)  );
	SG_ERR_CHECK(  SG_resolve__value__get_summary(pCtx, pValue, &sSummary)  );

	// add the data to the hash
	if (sSummary == NULL)
	{
		SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pData, szLabel)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pData, szLabel, SG_string__sz(sSummary))  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, sSummary);
	SG_STRING_NULLFREE(pCtx, sLabel);
	return;
}

/**
 * An SG_resolve__item__callback that retrieves information about the related item.
 */
static void _write_choice__verbose__get_related_item(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	SG_resolve__item* pItem,     //< [in] The current related item.
	void*             pUserData, //< [in] SG_vector to add the related item to.
	SG_bool*          pContinue  //< [out] Whether or not iteration should continue.
	)
{
	SG_vector* pData = (SG_vector*)pUserData;

	SG_NULLARGCHECK(pItem);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	SG_ERR_CHECK(  SG_vector__append(pCtx, pData, (void*)pItem, NULL)  );

fail:
	return;
}

/**
 * Writes verbose information about a choice to stdout.
 */
static void _write_choice__verbose(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to write.
	SG_resolve__item*   pItem    //< [in] The item that owns the choice, if known.
	                             //<      If NULL, this function will look it up.
	)
{
	SG_string *                      pStringRepoPathWorking = NULL;
	SG_string *                      pStringRepoPathWorking_Related = NULL;
	SG_resolve__state                eState             = SG_RESOLVE__STATE__COUNT;
	const char*                      szStateName        = NULL;
	char*                            szStateNameCase    = NULL;
	const char*                      szStateCommand     = NULL;
	SG_mrg_cset_entry_conflict_flags eConflictCauses    = SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO;
	SG_bool                          bCollisionCause    = SG_FALSE;
	SG_vhash*                        pCauseDescriptions = NULL;
	SG_vector*                       pRelatedItems      = NULL;
	SG_resolve__value*               pAcceptedValue     = NULL;
	SG_uint32                        uCount             = 0u;
	SG_uint32                        uIndex             = 0u;
	SG_vhash*                        pValues            = NULL;
	SG_bool                          bThisChoiceResolved = SG_FALSE;
	SG_bool bShowInParens = SG_FALSE;

	SG_NULLARGCHECK(pChoice);

	// if we weren't given an item, look it up
	if (pItem == NULL)
	{
		SG_ERR_CHECK(  SG_resolve__choice__get_item(pCtx, pChoice, &pItem)  );
	}

	SG_ERR_CHECK(  SG_resolve__item__get_repo_path__working(pCtx, pItem, &pStringRepoPathWorking, &bShowInParens)  );

	// get data about the choice
	SG_ERR_CHECK(  SG_resolve__choice__get_state(pCtx, pChoice, &eState)  );
	SG_ERR_CHECK(  SG_resolve__state__get_info(pCtx, eState, &szStateName, NULL, &szStateCommand)  );
	SG_ERR_CHECK(  SG_resolve__choice__get_causes(pCtx, pChoice, &eConflictCauses, &bCollisionCause, &pCauseDescriptions)  );
	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pRelatedItems, 2u)  );
	SG_ERR_CHECK(  SG_resolve__choice__foreach_colliding_item(pCtx, pChoice, _write_choice__verbose__get_related_item, (void*)pRelatedItems)  );
	SG_ERR_CHECK(  SG_resolve__choice__foreach_cycling_item(pCtx, pChoice, _write_choice__verbose__get_related_item, (void*)pRelatedItems)  );
	SG_ERR_CHECK(  SG_resolve__choice__foreach_portability_colliding_item(pCtx, pChoice, _write_choice__verbose__get_related_item, (void*)pRelatedItems)  );
	SG_ERR_CHECK(  SG_resolve__choice__get_resolved(pCtx, pChoice, &bThisChoiceResolved)  );
	// NOTE 2012/08/29 With the changes for W1849 the accepted value
	// NOTE            is only defined for file-contents.
	SG_ERR_CHECK(  SG_resolve__choice__get_value__accepted(pCtx, pChoice, &pAcceptedValue)  );
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pValues)  );
	SG_ERR_CHECK(  SG_resolve__choice__foreach_value(pCtx, pChoice, SG_FALSE, _write_choice__verbose__get_value, (void*)pValues)  );

	// capitalize the state name
	SG_ERR_CHECK(  SG_strdup(pCtx, szStateName, &szStateNameCase)  );
	szStateNameCase[0] = (unsigned char)toupper((unsigned char)szStateNameCase[0]);

	// item
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
							  ((bShowInParens)
							   ? "%-12s (%s)\n"
							   : "%-12s %s\n"),
							  "Item:",
							  SG_string__sz(pStringRepoPathWorking))  );

	// conflict
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%-12s %s\n", "Conflict:", szStateNameCase)  );
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%-12s %s\n", "", szStateCommand)  );

	// causes
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pCauseDescriptions, &uCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		const char* szCauseName = NULL;
		const char* szCauseDescription = NULL;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair__sz(pCtx, pCauseDescriptions, uIndex, &szCauseName, &szCauseDescription)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%-12s %s\n", "Cause:", szCauseName)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%-12s %s\n", "", szCauseDescription)  );
	}

	// cycle
	if ((eConflictCauses & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE) == SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE)
	{
		const char* szCyclePath = NULL;

		SG_ERR_CHECK(  SG_resolve__choice__get_cycle_info(pCtx, pChoice, &szCyclePath)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%-12s %s\n", "Cycle:", szCyclePath)  );
	}

	// related
	SG_ERR_CHECK(  SG_vector__length(pCtx, pRelatedItems, &uCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		SG_resolve__item* pItem                 = NULL;

		// get data about the related item
		SG_ERR_CHECK(  SG_vector__get(pCtx, pRelatedItems, uIndex, (void**)&pItem)  );
		SG_ERR_CHECK(  SG_resolve__item__get_repo_path__working(pCtx, pItem, &pStringRepoPathWorking_Related, &bShowInParens)  );

		// write the data out
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
								  ((bShowInParens)
								   ? "%-12s (%s)\n"
								   : "%-12s %s\n"),
								  "Related:",
								  SG_string__sz(pStringRepoPathWorking_Related))  );
		SG_STRING_NULLFREE(pCtx, pStringRepoPathWorking_Related);
	}

	// status
	if (bThisChoiceResolved == SG_FALSE)
	{
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%-12s %s\n", "Status:", "Unresolved")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%-12s %s\n", "Choices:", "Resolve this conflict by accepting a value from below by label.")  );
	}
	else if (pAcceptedValue)
	{
		const char* szAcceptedValue = NULL;

		SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pAcceptedValue, &szAcceptedValue)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%-12s %s (accepted %s)\n", "Status:", "Resolved", szAcceptedValue)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%-12s %s\n",               "Choices:", "Accept a value from below to change this conflict's resolution.")  );
	}
	else
	{
		// this can happen if the item is deleted using 'vv remove' (and maybe other ways)
		// where we implicitly marks it resolved, but there may not have been an accepted value present.
		// See W1511, W0960, and W1849.
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%-12s %s\n", "Status:", "Resolved")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%-12s %s\n", "Choices:", "Accept a value from below to change this conflict's resolution.")  );
	}

	// values
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pValues, &uCount)  );
	for (uIndex = 0u; uIndex < uCount; ++uIndex)
	{
		const char*       szLabel  = NULL;
		const SG_variant* pSummary = NULL;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pValues, uIndex, &szLabel, &pSummary)  );
		if (pSummary->type == SG_VARIANT_TYPE_NULL)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%12s\n", szLabel)  );
		}
		else
		{
			SG_ASSERT(pSummary->type == SG_VARIANT_TYPE_SZ);
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%11s: %s\n", szLabel, pSummary->v.val_sz)  );
		}
	}

	// empty line
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPathWorking);
	SG_STRING_NULLFREE(pCtx, pStringRepoPathWorking_Related);
	SG_NULLFREE(pCtx, szStateNameCase);
	SG_VHASH_NULLFREE(pCtx, pCauseDescriptions);
	SG_VECTOR_NULLFREE(pCtx, pRelatedItems);
	SG_VHASH_NULLFREE(pCtx, pValues);
	return;
}

/**
 * Writes information about a choice to stdout in a list format.
 */
static void _write_choice__list(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to write.
	SG_resolve__item*   pItem    //< [in] The item that owns the choice, if known.
	                             //<      If NULL, this function will look it up.
	)
{
	SG_string * pStringRepoPathWorking = NULL;
	SG_uint32          uConflicts         = 0u;
	SG_resolve__state  eState             = SG_RESOLVE__STATE__COUNT;
	const char*        szStateName        = NULL;
	char*              szStateNameCase    = NULL;
	SG_resolve__value* pAcceptedValue     = NULL;
	SG_bool bShowInParens;

	SG_NULLARGCHECK(pChoice);

	// if we weren't given an item, look it up
	if (pItem == NULL)
	{
		SG_ERR_CHECK(  SG_resolve__choice__get_item(pCtx, pChoice, &pItem)  );
	}

	// get data about the item
	SG_ERR_CHECK(  SG_resolve__item__get_repo_path__working(pCtx, pItem, &pStringRepoPathWorking, &bShowInParens)  );
	SG_ERR_CHECK(  SG_resolve__item__foreach_choice(pCtx, pItem, SG_FALSE, SG_resolve__choice_callback__count, &uConflicts)  );

	// get data about the choice
	SG_ERR_CHECK(  SG_resolve__choice__get_state(pCtx, pChoice, &eState)  );
	SG_ERR_CHECK(  SG_resolve__state__get_info(pCtx, eState, &szStateName, NULL, NULL)  );
	// NOTE 2012/08/29 With the changes for W1849 this value
	// NOTE            is only defined for file-contents.
	SG_ERR_CHECK(  SG_resolve__choice__get_value__accepted(pCtx, pChoice, &pAcceptedValue)  );

	// capitalize the state name
	SG_ERR_CHECK(  SG_strdup(pCtx, szStateName, &szStateNameCase)  );
	szStateNameCase[0] = (unsigned char)toupper((unsigned char)szStateNameCase[0]);

	// write the data
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
							  ((bShowInParens)
							   ? "%9s:  (%s)\n"
							   : "%9s:  %s\n"),
							  szStateNameCase,
							  SG_string__sz(pStringRepoPathWorking))  );
	if (uConflicts > 1u)
	{
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "                 # There are multiple conflicts on this item.\n")  );
	}
	if (pAcceptedValue != NULL)
	{
		const char* szAcceptedLabel = NULL;

		SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pAcceptedValue, &szAcceptedLabel)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "                 # accepted %s\n", szAcceptedLabel)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPathWorking);
	SG_NULLFREE(pCtx, szStateNameCase);
	return;
}

/**
 * User data used by _write_choice__callback.
 */
typedef struct _write_choice__callback__data
{
	SG_uint32 uMax;     //< [in] The maximum number of choices to write.
	                    //<      Zero indicates no maximum.
	SG_bool   bVerbose; //< [in] Whether or not to output the verbose format.
}
_write_choice__callback__data;

/**
 * An SG_resolve__choice__callback that writes information about the item to stdout.
 */
static void _write_choice__callback(
	SG_context*         pCtx,      //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,   //< [in] The current choice.
	void*               pUserData, //< [in] A _write_choice__callback__data*
	SG_bool*            pContinue  //< [out] Whether or not iteration should continue.
	)
{
	_write_choice__callback__data* pData = (_write_choice__callback__data*)pUserData;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pUserData);
	SG_NULLARGCHECK(pContinue);

	// write the choice out in the appropriate format
	if (pData->bVerbose == SG_FALSE)
	{
		SG_ERR_CHECK(  _write_choice__list(pCtx, pChoice, NULL)  );
	}
	else
	{
		SG_ERR_CHECK(  _write_choice__verbose(pCtx, pChoice, NULL)  );
	}

	// decrement our maximum count, if we have one
	*pContinue = SG_TRUE;
	if (pData->uMax > 0u)
	{
		pData->uMax -= 1u;

		// if we hit zero, stop iterating
		if (pData->uMax == 0u)
		{
			*pContinue = SG_FALSE;
		}
	}

fail:
	return;
}

/**
 * Writes the result of an accept to stdout.
 */
static void _write_accept(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__value*  pValue,  //< [in] The value that was accepted.
	SG_resolve__choice* pChoice, //< [in] The choice that owns the value.
	                             //<      If NULL, this function will look it up from the value.
	SG_resolve__item*   pItem    //< [in] The item that owns the choice.
	                             //<      If NULL, this function will look it up from the choice.
	)
{
	SG_treenode_entry_type eType      = SG_TREENODEENTRY_TYPE__INVALID;
	const char*            szType     = NULL;
	SG_string * pStringRepoPathWorking = NULL;
	SG_resolve__state      eState     = SG_RESOLVE__STATE__COUNT;
	const char*            szState    = NULL;
	const char*            szLabel    = NULL;
	SG_bool bShowInParens;

	SG_NULLARGCHECK(pValue);

	if (pChoice == NULL)
	{
		SG_ERR_CHECK(  SG_resolve__value__get_choice(pCtx, pValue, &pChoice)  );
	}
	if (pItem == NULL)
	{
		SG_ERR_CHECK(  SG_resolve__choice__get_item(pCtx, pChoice, &pItem)  );
	}

	SG_ERR_CHECK(  SG_resolve__item__get_type(pCtx, pItem, &eType)  );
	szType = SG_treenode_entry_type__type_to_label(eType);
	SG_ERR_CHECK(  SG_resolve__item__get_repo_path__working(pCtx, pItem, &pStringRepoPathWorking, &bShowInParens)  );
	SG_ERR_CHECK(  SG_resolve__choice__get_state(pCtx, pChoice, &eState)  );
	SG_ERR_CHECK(  SG_resolve__state__get_info(pCtx, eState, &szState, NULL, NULL)  );
	SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pValue, &szLabel)  );

	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Accepted '%s' value for '%s' conflict on %s:\n", szLabel, szState, szType)  );
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
							  ((bShowInParens)
							   ? "    (%s)\n"
							   : "    %s\n"),
							  SG_string__sz(pStringRepoPathWorking))  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPathWorking);
}

/**
 * Writes the result of a merge to stdout.
 */
static void _write_merge(
	SG_context*         pCtx,           //< [in] [out] Error and context info.
	SG_resolve__value*  pBaselineValue, //< [in] The baseline value that was merged.
	SG_resolve__value*  pOtherValue,    //< [in] The other value that was merged.
	SG_int32            iResultCode,    //< [in] The result code from the merge.
	SG_resolve__value*  pResultValue,   //< [in] The result value from the merge.
	const char*         szTool,         //< [in] Merge tool used to create the result value.
	SG_resolve__choice* pChoice,        //< [in] The choice that owns the values.
	                                    //<      If NULL, this function will look it up from the values.
	SG_resolve__item*   pItem           //< [in] The item that owns the choice.
	                                    //<      If NULL, this function will look it up from the choice.
	)
{
	const char*        szResultLabel  = NULL;
	SG_resolve__value* pAcceptedValue = NULL;

	SG_NULLARGCHECK(pBaselineValue);
	SG_NULLARGCHECK(pOtherValue);

	// if they didn't supply a choice, look it up
	if (pChoice == NULL)
	{
		SG_resolve__choice* pBaselineChoice = NULL;
		SG_resolve__choice* pOtherChoice    = NULL;

		SG_ERR_CHECK(  SG_resolve__value__get_choice(pCtx, pBaselineValue, &pBaselineChoice)  );
		SG_ERR_CHECK(  SG_resolve__value__get_choice(pCtx, pOtherValue, &pOtherChoice)  );
		SG_ARGCHECK(pBaselineChoice == pOtherChoice, pBaselineValue|pOtherValue);
		pChoice = pBaselineChoice;
	}

	// if they didn't supply an item, look it up
	if (pItem == NULL)
	{
		SG_ERR_CHECK(  SG_resolve__choice__get_item(pCtx, pChoice, &pItem)  );
	}

	// check what happened and display appropriate output
	if (pResultValue == NULL)
	{
		// no new result value, check the result code
		switch (iResultCode)
		{
		case SG_FILETOOL__RESULT__SUCCESS:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Merge tool '%s' succeeded but did not generate output, no value created.\n", szTool)  );
			break;
		case SG_MERGETOOL__RESULT__CONFLICT:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Merge tool '%s' found conflicts and did not generate output, no value created.\n", szTool)  );
			break;
		case SG_FILETOOL__RESULT__FAILURE:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Merge tool '%s' failed, no value created.\n", szTool)  );
			break;
		case SG_MERGETOOL__RESULT__CANCEL:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Merge tool '%s' was canceled, no value created.\n", szTool)  );
			break;
		case SG_FILETOOL__RESULT__ERROR:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Merge tool '%s' was not found or caused an error, no value created.\n", szTool)  );
			break;
		default:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Merge tool '%s' returned unknown result '%i'.\n", szTool, iResultCode)  );
			break;
		}
	}
	else
	{
		// we have a result value, get some additional data from it for display
		SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pResultValue, &szResultLabel)  );

		// display the result
		switch (iResultCode)
		{
		case SG_FILETOOL__RESULT__SUCCESS:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Used merge tool '%s' to create merged value '%s'.\n", szTool, szResultLabel)  );
			break;
		case SG_MERGETOOL__RESULT__CONFLICT:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Used merge tool '%s' to create merged value '%s' with some conflicts remaining.\n", szTool, szResultLabel)  );
			break;
		case SG_FILETOOL__RESULT__FAILURE:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Merge tool '%s' failed but still created merged value '%s'.\n", szTool, szResultLabel)  );
			break;
		case SG_MERGETOOL__RESULT__CANCEL:
			// Note: Shouldn't be possible to get a merged value back along with a CANCEL result, but just in case..
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Merge tool '%s' was canceled but still created merged value '%s'.\n", szTool, szResultLabel)  );
			break;
		case SG_FILETOOL__RESULT__ERROR:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Merge tool '%s' caused an error but still created merged value '%s'.\n", szTool, szResultLabel)  );
			break;
		default:
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Merge tool '%s' returned unknown result '%i' but still created merged value '%s'.\n", szTool, iResultCode, szResultLabel)  );
			break;
		}
	}

	// if the new value was automatically accepted as the choice's resolution
	// then let the user know
	SG_ERR_CHECK(  SG_resolve__choice__get_value__accepted(pCtx, pChoice, &pAcceptedValue)  );
	if (pAcceptedValue != NULL && pAcceptedValue == pResultValue)
	{
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "New merged value automatically accepted as its choice's resolution.\n")  );
	}

fail:
	return;
}


/*
**
** Private Functionality - Command-line Resolve Extensions
**
*/

/**
 * Checks if a choice is fresh and resolvable via the obvious merge.
 *
 * This function returns true if all the following criteria are met:
 * 1) The choice is mergeable.
 * 2) The choice is unresolved.
 * 3) The choice has no existing "merge" values.
 * 4) The choice has a single working value that's unmodified relative to its parent.
 *
 * TODO: Consider checking whether or not there is a single working
 *       value that has no edits in it.  Looks like new resolve APIs
 *       might be necessary to check that.
 */
static void _resolve__choice__plain_merge(
	SG_context*         pCtx,       //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,    //< [in] The choice to check.
	SG_bool*            pPlainMerge //< [out] True if the choice is fresh and resolvable
	                                //<       via a plain merge.  False otherwise.
	)
{
	SG_bool bPlainMerge = SG_FALSE;
	SG_bool bMergeable  = SG_FALSE;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pPlainMerge);

	// check for mergeable (1)
	SG_ERR_CHECK(  SG_resolve__choice__get_mergeable(pCtx, pChoice, &bMergeable)  );
	if (bMergeable == SG_TRUE)
	{
		SG_resolve__value* pAccepted = NULL;

		// check for unresolved (2)
		SG_ERR_CHECK(  SG_resolve__choice__get_value__accepted(pCtx, pChoice, &pAccepted)  );
		if (pAccepted == NULL)
		{
			SG_resolve__value* pMerge = NULL;

			// check for no existing "merge" values (3)
			SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, SG_RESOLVE__LABEL__MERGE, 0, &pMerge)  );
			if (pMerge == NULL)
			{
				SG_resolve__value* pWorking1 = NULL;
				SG_resolve__value* pWorking2 = NULL;

				// check for a single working value (4a)
				SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, SG_RESOLVE__LABEL__WORKING, 1, &pWorking1)  );
				SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, SG_RESOLVE__LABEL__WORKING, 2, &pWorking2)  );
				if (pWorking1 != NULL && pWorking2 == NULL)
				{
					// check for the working value to be unmodified relative to its parent (4b)
					SG_bool bMergeableWorking = SG_FALSE;
					SG_bool bMWModified       = SG_FALSE;

					SG_ERR_CHECK(  SG_resolve__value__get_mergeable_working(pCtx, pWorking1, &bMergeableWorking, NULL, &bMWModified)  );
					if (bMergeableWorking == SG_TRUE && bMWModified == SG_FALSE)
					{
						// okay, it's a plain fresh merge
						bPlainMerge = SG_TRUE;
					}
				}
			}
		}
	}

	*pPlainMerge = bPlainMerge;

fail:
	return;
}

/**
 * Ensures that a value can be viewed and then does it.
 * This is the common "view" code shared by interactive and non-interactive commands.
 *//*
static void _resolve__view(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to view a value on.
	SG_resolve__value*  pValue,  //< [in] The value to view.
	const char*         szTool   //< [in] Name of the tool to use to view the value.
	)
{
	SG_bool bMergeable = SG_FALSE;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pValue);

	// make sure this is a mergeable choice
	SG_ERR_CHECK(  SG_resolve__choice__get_mergeable(pCtx, pChoice, &bMergeable)  );
	if (bMergeable == SG_FALSE)
	{
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Cannot view values on non-mergeable conflicts.\n")  );
	}
	else
	{
		SG_ERR_CHECK(  SG_resolve__view_value(pCtx, pValue, szTool)  );
	}

fail:
	return;
}
*/
/**
 * Ensures that values can be diffed and then does it.
 * This is the common "diff" code shared by interactive and non-interactive commands.
 */
static void _resolve__diff(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice to diff values on.
	SG_resolve__value*  pLeft,   //< [in] The left value to diff.
	SG_resolve__value*  pRight,  //< [in] The right value to diff.
	const char*         szTool   //< [in] Name of the tool to use to diff the values.
	)
{
	SG_bool bMergeable = SG_FALSE;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pLeft);
	SG_NULLARGCHECK(pRight);

	// make sure this is a mergeable choice
	SG_ERR_CHECK(  SG_resolve__choice__get_mergeable(pCtx, pChoice, &bMergeable)  );
	if (bMergeable == SG_FALSE)
	{
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Cannot diff values on non-mergeable conflicts.\n")  );
	}
	else
	{
		SG_ERR_CHECK(  SG_resolve__diff_values(pCtx, pLeft, pRight, SG_DIFFTOOL__CONTEXT__CONSOLE, szTool, NULL)  );
	}

fail:
	return;
}

/**
 * Ensures that values can be merged and then does it.
 * This is the common "merge" code shared by interactive and non-interactive commands.
 * Note that this function always merges the "baseline" and "other" values.
 */
static void _resolve__merge(
	SG_context*         pCtx,       //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,    //< [in] The choice to merge values on.
	const char*         szTool,     //< [in] Name of the tool to use to merge the values.
	SG_bool             bAccept,    //< [in] Whether or not the newly merged value should be automatically accepted.
	SG_resolve__item*   pItem,      //< [in] The item that owns pChoice.
	                                //<      If NULL, it will be looked up from pChoice.
	SG_resolve__value** ppResult    //< [out] The value that resulted from the merge.
	                                //<       NULL if the merge wasn't performed or failed.
	)
{
	SG_bool            bMergeable = SG_FALSE;
	SG_resolve__value* pResult    = NULL;
	char*              szToolUsed = NULL;

	SG_NULLARGCHECK(pChoice);

	// make sure this is a mergeable choice
	SG_ERR_CHECK(  SG_resolve__choice__get_mergeable(pCtx, pChoice, &bMergeable)  );
	if (bMergeable == SG_FALSE)
	{
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Cannot merge values on non-mergeable conflicts.\n")  );
	}
	else
	{
		SG_resolve__value* pLeft   = NULL;
		SG_resolve__value* pRight  = NULL;
		SG_int32           iResult = SG_FILETOOL__RESULT__COUNT;

		// find the "baseline" and "other" values that we'll be merging
		SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, SG_RESOLVE__LABEL__BASELINE, 0, &pLeft)  );
		SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, SG_RESOLVE__LABEL__OTHER, 0, &pRight)  );

		// run the merge
		SG_ERR_CHECK(  SG_resolve__merge_values(pCtx, pLeft, pRight, szTool, NULL, bAccept, &iResult, &pResult, &szToolUsed)  );
		SG_ERR_CHECK(  _write_merge(pCtx, pLeft, pRight, iResult, pResult, szToolUsed, pChoice, pItem)  );
	}

	// return whatever result we had
	*ppResult = pResult;
	pResult = NULL;

fail:
	SG_NULLFREE(pCtx, szToolUsed);
	return;
}

/**
 * Ensures that a value can be accepted and then does it.
 * This is the common "accept" code shared by interactive and non-interactive commands.
 */
static void _resolve__accept(
	SG_context*         pCtx,       //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,    //< [in] The choice to accept a value on.
	SG_resolve__value*  pValue,     //< [in] The value to accept.
	SG_int32            iOverwrite, //< [in] How to handle the choice already having an accepted value.
	                                //<      If positive, overwrite the existing resolution.
	                                //<      If zero, display an error.
	                                //<      If negative, ask the user with a y/n prompt.
	SG_resolve__item*   pItem,      //< [in] The item that owns pChoice.
	                                //<      If NULL, it will be looked up from pChoice.
	SG_bool*            pAccepted   //< [out] Whether or not the value was accepted.
	)
{
	SG_resolve__value* pAcceptedValue = NULL;
	SG_bool            bOverwrite     = SG_FALSE;
	const char*        szLabel        = NULL;
	SG_string*         sOverwrite     = NULL;
	SG_string * pStringRepoPathWorking = NULL;
	SG_bool bShowInParens;
	SG_bool bChoiceResolved;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pValue);
	SG_NULLARGCHECK(pAccepted);

	// Check if the choice already has a resolution.
	// With the changes for W1849, pAcceptedValue is
	// only defined for file-contents.  For all other
	// choices, we just know resolved/unresolved.
	// Previously, pAcceptedValue was set whenever the
	// choice was resolved.
	SG_ERR_CHECK(  SG_resolve__choice__get_resolved(pCtx, pChoice, &bChoiceResolved)  );
	if (bChoiceResolved)
	{
		SG_ERR_CHECK(  SG_resolve__choice__get_value__accepted(pCtx, pChoice, &pAcceptedValue)  );
		if (pValue == pAcceptedValue)
		{
			// already resolved with given value
			SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pValue, &szLabel)  );
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "The conflict is already resolved using the specified value: %s\n", szLabel)  );
			bOverwrite = SG_FALSE;
		}
		else
		{
			if (iOverwrite > 0)
			{
				// overwrite existing value
				bOverwrite = SG_TRUE;
			}
			else if (iOverwrite == 0)
			{
				// don't overwrite existing value
				bOverwrite = SG_FALSE;

				SG_ERR_CHECK(  SG_resolve__item__get_repo_path__working(pCtx, pItem, &pStringRepoPathWorking, &bShowInParens)  );
				SG_ERR_THROW2(  SG_ERR_RESOLVE_NEEDS_OVERWRITE,
								(pCtx, "%s", SG_string__sz(pStringRepoPathWorking))  );
			}
			else if (iOverwrite < 0)
			{
				// prompt to overwrite existing value
				if (pAcceptedValue != NULL)
				{
					SG_ERR_CHECK(  SG_resolve__value__get_label(pCtx, pAcceptedValue, &szLabel)  );
					SG_ERR_CHECK(  _console__prompt(pCtx, &sOverwrite, "Choice already resolved using value '%s', overwrite? (y/n): ", szLabel)  );
				}
				else
				{
					SG_ERR_CHECK(  _console__prompt(pCtx, &sOverwrite, "Choice already resolved, overwrite? (y/n): ")  );
				}
				bOverwrite = SG_parse_bool(SG_string__sz(sOverwrite));
			}
		}
	}

	// run the accept
	if (!bChoiceResolved || bOverwrite == SG_TRUE)
	{
		SG_ERR_CHECK(  SG_resolve__accept_value(pCtx, pValue)  );
		SG_ERR_CHECK(  _write_accept(pCtx, pValue, pChoice, pItem)  );
		*pAccepted = SG_TRUE;
	}
	else
	{
		*pAccepted = SG_FALSE;
	}

fail:
	SG_STRING_NULLFREE(pCtx, sOverwrite);
	SG_STRING_NULLFREE(pCtx, pStringRepoPathWorking);
}


/*
**
** Private Functionality - Interactive Helpers and Command Handlers
**
*/

/**
 * Builds the user prompt for an interactive session.
 */
static void _interactive__build_prompt(
	SG_context*            pCtx,     //< [in] [out] Error and context info.
	_interactive__session* pSession, //< [in] Data for the current interactive session.
	SG_resolve__choice*    pChoice,  //< [in] The choice being prompted for.
	SG_string**            ppPrompt  //< [out] The built prompt.
	)
{
	SG_bool    bMergeable = SG_FALSE;
	SG_string* sPrompt    = NULL;
	SG_uint32  uIndex     = 0u;
	SG_bool    bSeparator = SG_FALSE;

	SG_NULLARGCHECK(pSession);
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppPrompt);

	// check if this is a mergeable choice
	SG_ERR_CHECK(  SG_resolve__choice__get_mergeable(pCtx, pChoice, &bMergeable)  );

	// start with an empty prompt
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sPrompt)  );

	// generate a line of available commands
	for (uIndex = 0u; uIndex < SG_NrElements(gaInteractiveCommands); ++uIndex)
	{
		const _interactive__command_spec* pSpec = gaInteractiveCommands + uIndex;

		// only include the command if its "mergeability" matches the current choice
		if (pSpec->bMergeableOnly == SG_FALSE || bMergeable == SG_TRUE)
		{
			// if we need a separator, add one
			if (bSeparator == SG_TRUE)
			{
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, sPrompt, " ")  );
			}

			// append the command's prompt string
			SG_ERR_CHECK(  SG_string__append__sz(pCtx, sPrompt, pSpec->szPrompt)  );
			bSeparator = SG_TRUE;
		}
	}
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, sPrompt, SG_PLATFORM_NATIVE_EOL_STR)  );

	// generate the end of the prompt, including current position
	SG_ERR_CHECK(  SG_string__append__format(pCtx, sPrompt, "Conflict %u of %u: ", pSession->uChoice + 1u, pSession->uChoices)  );

	// return the prompt
	*ppPrompt = sPrompt;
	sPrompt = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sPrompt);
	return;
}

/**
 * Builds an automatic command to use for an interactive prompt, if one is relevent.
 */
static void _interactive__build_automatic_command(
	SG_context*            pCtx,      //< [in] [out] Error and context info.
	_interactive__session* pSession,  //< [in] Data for the current interactive session.
	SG_resolve__choice*    pChoice,   //< [in] The choice being prompted for.
	SG_string**            ppCommand, //< [out] The built command.
	                                  //<       Set to NULL if no automatic command is relevent.
	SG_string**            ppMessage  //< [out] An explanatory message to display.
	                                  //<       Set to NULL if none is necessary.
	)
{
	SG_bool    bAutoMerge = SG_FALSE;
	SG_string* sCommand   = NULL;
	SG_string* sMessage   = NULL;

	SG_NULLARGCHECK(pSession);
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(ppCommand);

	// we'll run an automatic merge if the choice looks fresh and easily mergeable
	SG_ERR_CHECK(  _resolve__choice__plain_merge(pCtx, pChoice, &bAutoMerge)  );
	if (bAutoMerge == SG_TRUE)
	{
		SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sCommand, "merge")  );
		SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &sMessage, "Automatically running manual merge, close tool without saving to cancel.")  );
	}

	// return whatever we built
	*ppCommand = sCommand;
	sCommand = NULL;
	*ppMessage = sMessage;
	sMessage = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sCommand);
	SG_STRING_NULLFREE(pCtx, sMessage);
	return;
}

/**
 * Writes the usage of a command spec to the console.
 */
static void _interactive__write_usage(
	SG_context*                       pCtx, //< [in] [out] Error and context info.
	const _interactive__command_spec* pSpec //< [in] The command spec to write usage for.
	)
{
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", pSpec->szUsage)  );
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "    %s\n", pSpec->szDescription)  );

fail:
	return;
}

/**
 * Parses a command entered by the user at the interactive prompt.
 *
 * If pParsed returns false, then this function printed an appropriate message.
 */
static void _interactive__parse_command(
	SG_context*            pCtx,      //< [in] [out] Error and context info.
	char*                  szCommand, //< [in] The command entered by the user.
	                                  //<      Will be tokenized during parsing.
	SG_resolve__choice*    pChoice,   //< [in] The choice to use to validate value names and filetools.
	                                  //<      If NULL, validation is skipped.
	_interactive__command* pCommand,  //< [out] The parsed command.
	                                  //<       Data points to tokens within szCommand.
	SG_bool*               pParsed    //< [out] True if the command successfully parsed,
	                                  //<       or false if there was a problem.
	)
{
	const char* szToken = NULL;
	void*       pHandle = NULL;
	SG_uint32   uIndex  = 0u;
	SG_bool     bParsed = SG_TRUE;

	SG_NULLARGCHECK(szCommand);
	SG_NULLARGCHECK(pCommand);

	// initialize the command with default data
	pCommand->pSpec    = NULL;
	pCommand->szValue1 = NULL;
	pCommand->szValue2 = NULL;
	pCommand->szTool   = NULL;
	pCommand->pValue1  = NULL;
	pCommand->pValue2  = NULL;

	// get the first token, which will be the command being invoked, and find its spec
	SG_ERR_CHECK(  _SG_strtok(pCtx, szCommand, NULL, SG_TRUE, &szToken, &pHandle)  );
	for (uIndex = 0u; uIndex < SG_NrElements(gaInteractiveCommands); ++uIndex)
	{
		const _interactive__command_spec* pSpec = gaInteractiveCommands + uIndex;

		if (
			   SG_stricmp(szToken, pSpec->szName) == 0
			|| (
				   pSpec->szAbbreviation != NULL
				&& SG_stricmp(szToken, pSpec->szAbbreviation) == 0
				)
			)
		{
			pCommand->pSpec = pSpec;
			break;
		}
	}

	// make sure we found a spec
	if (pCommand->pSpec == NULL)
	{
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Unknown command: %s\n", szToken)  );
		bParsed = SG_FALSE;
	}
	else
	{
		// run through the tokens and expected arguments simultaneously
		SG_ERR_CHECK(  _SG_strtok(pCtx, NULL, NULL, SG_TRUE, &szToken, &pHandle)  );
		for (uIndex = 0u; pCommand->pSpec->aArguments[uIndex] != 0 && szToken != NULL; ++uIndex)
		{
			// check what type of argument we're expecting here
			switch (pCommand->pSpec->aArguments[uIndex] & ARGUMENT_TYPE_MASK)
			{
			case ARGUMENT_VALUE:
				if (pCommand->szValue1 == NULL)
				{
					pCommand->szValue1 = szToken;
				}
				else if (pCommand->szValue2 == NULL)
				{
					pCommand->szValue2 = szToken;
				}
				else
				{
					SG_ASSERT(SG_FALSE && "Too many ARGUMENT_VALUE arguments on one command.");
					bParsed = SG_FALSE;
				}
				break;
			case ARGUMENT_TOOL:
				if (pCommand->szTool == NULL)
				{
					pCommand->szTool = szToken;
				}
				else
				{
					SG_ASSERT(SG_FALSE && "Too many ARGUMENT_TOOL arguments on one command.");
					bParsed = SG_FALSE;
				}
				break;
			default:
				SG_ASSERT(SG_FALSE && "Unknown ARGUMENT_* type value.");
				bParsed = SG_FALSE;
				break;
			}

			// get the next token
			SG_ERR_CHECK(  _SG_strtok(pCtx, NULL, NULL, SG_TRUE, &szToken, &pHandle)  );
		}

		// make sure any remaining arguments are optional
		if (bParsed == SG_TRUE)
		{
			for (/* nothing */; pCommand->pSpec->aArguments[uIndex] != 0; ++uIndex)
			{
				if ((pCommand->pSpec->aArguments[uIndex] & ARGUMENT_OPTIONAL) == 0)
				{
					// not enough arguments
					SG_ERR_CHECK(  _interactive__write_usage(pCtx, pCommand->pSpec)  );
					bParsed = SG_FALSE;
					break;
				}
			}
		}

		// make sure there aren't any leftover tokens
		if (bParsed == SG_TRUE && szToken != NULL)
		{
			// too many arguments
			SG_ERR_CHECK(  _interactive__write_usage(pCtx, pCommand->pSpec)  );
			bParsed = SG_FALSE;
		}

		// try parsing non-NULL value names into values
		if (bParsed == SG_TRUE && pChoice != NULL)
		{
			if (pCommand->szValue1 != NULL)
			{
				SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, pCommand->szValue1, 0u, &pCommand->pValue1)  );
				if (pCommand->pValue1 == NULL)
				{
					SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Unknown value: %s\n", pCommand->szValue1)  );
					bParsed = SG_FALSE;
				}
			}
			if (pCommand->szValue2 != NULL)
			{
				SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, pCommand->szValue2, 0u, &pCommand->pValue2)  );
				if (pCommand->pValue2 == NULL)
				{
					SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Unknown value: %s\n", pCommand->szValue2)  );
					bParsed = SG_FALSE;
				}
			}
		}

		// if a tool was specified, check if it exists
		if (bParsed == SG_TRUE && pCommand->szTool != NULL)
		{
			SG_resolve__item* pItem        = NULL;
			SG_resolve*       pResolve     = NULL; // we do not own this
			SG_repo*          pRepo        = NULL; // we do not own this

			// get the repo associated with this choice
			SG_ERR_CHECK(  SG_resolve__choice__get_item(pCtx, pChoice, &pItem)  );
			SG_ERR_CHECK(  SG_resolve__item__get_resolve(pCtx, pItem, &pResolve)  );
			SG_ERR_CHECK(  SG_resolve__hack__get_repo(pCtx, pResolve, &pRepo)  );

			// check for the appropriate type of tool
			if (strcmp(pCommand->pSpec->szToolType, SG_DIFFTOOL__TYPE) == 0)
			{
				SG_bool bExists = SG_FALSE;
				SG_ERR_CHECK(  SG_difftool__exists(pCtx, pCommand->szTool, pRepo, &bExists)  );
				if (bExists == SG_FALSE)
				{
					SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Unknown difftool: %s\n", pCommand->szTool)  );
					bParsed = SG_FALSE;
				}
			}
			else if (strcmp(pCommand->pSpec->szToolType, SG_MERGETOOL__TYPE) == 0)
			{
				SG_bool bExists = SG_FALSE;
				SG_ERR_CHECK(  SG_mergetool__exists(pCtx, SG_MERGETOOL__CONTEXT__RESOLVE, pCommand->szTool, pRepo, &bExists)  );
				if (bExists == SG_FALSE)
				{
					SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Unknown mergetool: %s\n", pCommand->szTool)  );
					bParsed = SG_FALSE;
				}
			}
			else
			{
				SG_ASSERT(SG_FALSE && "Unknown filetool type value.");
				bParsed = SG_FALSE;
			}
		}
	}

	// return success/failure
	*pParsed = bParsed;

fail:
	return;
}

/**
 * Marks a session inactive so that it exits.
 */
static void _interactive__end_session(
	SG_context*            pCtx,    //< [in] [out] Error and context info.
	_interactive__session* pSession //< [in] The interactive session to end.
	)
{
	SG_NULLARGCHECK(pSession);

	pSession->uChoice = pSession->uChoices;

fail:
	return;
}

/**
 * Advances an interactive session to the next choice/conflict.
 */
static void _interactive__advance_choice(
	SG_context*            pCtx,     //< [in] [out] Error and context info.
	_interactive__session* pSession, //< [in] The interactive session to advance.
	SG_int32               iCount    //< [in] Number of choices to advance by.
	                                 //<      May be negative to advance backwards.
	                                 //<      If zero, the session is refreshed on the current choice.
	                                 //<      If the new index is out of range, the session is ended.
	)
{
	SG_uint32 uCount    = (SG_uint32)abs(iCount);
	SG_bool   bBackward = iCount < 0 ? SG_TRUE : SG_FALSE;
	SG_bool   bRange    = SG_FALSE;

	SG_NULLARGCHECK(pSession);
	SG_ASSERT(pSession->uChoice <= pSession->uChoices);

	// modify the choice index
	if (bBackward == SG_TRUE)
	{
		if (uCount > pSession->uChoice)
		{
			bRange = SG_TRUE;
		}
		else
		{
			pSession->uChoice -= uCount;
		}
	}
	else
	{
		pSession->uChoice += uCount;
		if (pSession->uChoice >= pSession->uChoices)
		{
			bRange = SG_TRUE;
		}
	}

	// check what to do
	if (bRange == SG_TRUE)
	{
		// we moved out of range, end the session
		SG_ERR_CHECK(  _interactive__end_session(pCtx, pSession)  );
	}
	else if (iCount == 0)
	{
		// reset appropriate data on the current choice
		pSession->bWrite = SG_TRUE;
	}
	else
	{
		// reset choice-specific data for the next choice
		pSession->bAutomatic = SG_FALSE;
		pSession->bWrite     = SG_TRUE;
	}

	SG_ASSERT(pSession->uChoice <= pSession->uChoices);

fail:
	return;
}

/**
 * _interactive__command_handler for the "view" command.
 *//*
static void _interactive__handler__view(
	SG_context*                  pCtx,
	_interactive__session*       pSession,
	SG_resolve__choice*          pChoice,
	const _interactive__command* pCommand
	)
{
	const char* szTool = pCommand->szTool;

	SG_NULLARGCHECK(pSession);
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pCommand);

	// if no tool was specified at the prompt, fall back to our default
	if (szTool == NULL)
	{
		szTool = pSession->szViewTool;
	}

	// view the value
	SG_ERR_CHECK(  _resolve__view(pCtx, pChoice, pCommand->pValue1, szTool)  );

fail:
	return;
}
*/
/**
 * _interactive__command_handler for the "diff" command.
 */
static void _interactive__handler__diff(
	SG_context*                  pCtx,
	_interactive__session*       pSession,
	SG_resolve__choice*          pChoice,
	const _interactive__command* pCommand
	)
{
	const char* szTool = pCommand->szTool;

	SG_NULLARGCHECK(pSession);
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pCommand);

	// if no tool was specified at the prompt, fall back to our default
	if (szTool == NULL)
	{
		szTool = pSession->szDiffTool;
	}

	// diff the values
	SG_ERR_CHECK(  _resolve__diff(pCtx, pChoice, pCommand->pValue1, pCommand->pValue2, szTool)  );

fail:
	return;
}

/**
 * _interactive__command_handler for the "merge" command.
 */
static void _interactive__handler__merge(
	SG_context*                  pCtx,
	_interactive__session*       pSession,
	SG_resolve__choice*          pChoice,
	const _interactive__command* pCommand
	)
{
	const char*        szTool  = pCommand->szTool;
	SG_bool            bAccept = SG_FALSE;
	SG_resolve__value* pResult = NULL;

	SG_NULLARGCHECK(pSession);
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pCommand);

	// if no tool was specified at the prompt, fall back to our default
	if (szTool == NULL)
	{
		szTool = pSession->szMergeTool;
	}

	// configure the merge to automatically accept the result
	// if the choice appears fresh and easily resolvable with this merge
	SG_ERR_CHECK(  _resolve__choice__plain_merge(pCtx, pChoice, &bAccept)  );

	// run the merge
	SG_ERR_CHECK(  _resolve__merge(pCtx, pChoice, szTool, bAccept, NULL, &pResult)  );
	if (pResult != NULL)
	{
		SG_resolve__value* pAccepted = NULL;

		// if the result value was automatically accepted
		// then advance to the next choice
		SG_ERR_CHECK(  SG_resolve__choice__get_value__accepted(pCtx, pChoice, &pAccepted)  );
		if (pAccepted != NULL && pAccepted == pResult)
		{
			SG_ERR_CHECK(  _interactive__advance_choice(pCtx, pSession, 1)  );
		}
	}

fail:
	return;
}

/**
 * _interactive__command_handler for the "accept" command.
 */
static void _interactive__handler__accept(
	SG_context*                  pCtx,
	_interactive__session*       pSession,
	SG_resolve__choice*          pChoice,
	const _interactive__command* pCommand
	)
{
	SG_bool bAccepted = SG_FALSE;

	SG_NULLARGCHECK(pSession);
	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(pCommand);

	// accept the value
	SG_ERR_CHECK(  _resolve__accept(pCtx, pChoice, pCommand->pValue1, -1, NULL, &bAccepted)  );
	if (bAccepted == SG_TRUE)
	{
		// if the accept succeeded, advance to the next choice
		SG_ERR_CHECK(  _interactive__advance_choice(pCtx, pSession, 1)  );
	}

fail:
	return;
}

/**
 * _interactive__command_handler for the "repeat" command.
 */
static void _interactive__handler__repeat(
	SG_context*                  pCtx,
	_interactive__session*       pSession,
	SG_resolve__choice*          pChoice,
	const _interactive__command* pCommand
	)
{
	SG_NULLARGCHECK(pSession);
	SG_UNUSED(pChoice);
	SG_UNUSED(pCommand);

	// advance to the same choice, refreshing its data
	SG_ERR_CHECK(  _interactive__advance_choice(pCtx, pSession, 0)  );

fail:
	return;
}

/**
 * _interactive__command_handler for the "skip" command.
 */
static void _interactive__handler__skip(
	SG_context*                  pCtx,
	_interactive__session*       pSession,
	SG_resolve__choice*          pChoice,
	const _interactive__command* pCommand
	)
{
	SG_NULLARGCHECK(pSession);
	SG_UNUSED(pChoice);
	SG_UNUSED(pCommand);

	// advance to the next choice
	SG_ERR_CHECK(  _interactive__advance_choice(pCtx, pSession, 1)  );

fail:
	return;
}

/**
 * _interactive__command_handler for the "back" command.
 */
static void _interactive__handler__back(
	SG_context*                  pCtx,
	_interactive__session*       pSession,
	SG_resolve__choice*          pChoice,
	const _interactive__command* pCommand
	)
{
	SG_NULLARGCHECK(pSession);
	SG_UNUSED(pChoice);
	SG_UNUSED(pCommand);

	// advance to the previous choice
	SG_ERR_CHECK(  _interactive__advance_choice(pCtx, pSession, -1)  );

fail:
	return;
}

/**
 * _interactive__command_handler for the "quit" command.
 */
static void _interactive__handler__quit(
	SG_context*                  pCtx,
	_interactive__session*       pSession,
	SG_resolve__choice*          pChoice,
	const _interactive__command* pCommand
	)
{
	SG_NULLARGCHECK(pSession);
	SG_UNUSED(pChoice);
	SG_UNUSED(pCommand);

	// end the session
	SG_ERR_CHECK(  _interactive__end_session(pCtx, pSession)  );

fail:
	return;
}

/**
 * _interactive__command_handler for the "help" command.
 */
static void _interactive__handler__help(
	SG_context*                  pCtx,
	_interactive__session*       pSession,
	SG_resolve__choice*          pChoice,
	const _interactive__command* pCommand
	)
{
	SG_bool   bMergeable = SG_FALSE;
	SG_uint32 uIndex     = 0u;

	SG_UNUSED(pSession);
	SG_NULLARGCHECK(pChoice);
	SG_UNUSED(pCommand);

	// check if this is a mergeable choice
	SG_ERR_CHECK(  SG_resolve__choice__get_mergeable(pCtx, pChoice, &bMergeable)  );

	// run through each command and write its usage
	for (uIndex = 0u; uIndex < SG_NrElements(gaInteractiveCommands); ++uIndex)
	{
		const _interactive__command_spec* pSpec = gaInteractiveCommands + uIndex;

		// only include the command if its "mergeability" matches the current choice
		if (pSpec->bMergeableOnly == SG_FALSE || bMergeable == SG_TRUE)
		{
			SG_ERR_CHECK(  _interactive__write_usage(pCtx, pSpec)  );
		}
	}

fail:
	return;
}

/**
 * Displays and handles a single interactive prompt/command for a given choice.
 */
static void _interactive__run_choice(
	SG_context*            pCtx,     //< [in] [out] Error and context info.
	_interactive__session* pSession, //< [in] Data for the current interactive session.
	SG_resolve__choice*    pChoice,  //< [in] The choice being prompted about.
	SG_resolve__item*      pItem     //< [in] The item that owns pChoice, if available.
	                                 //<      If NULL, the item will be looked up as/if needed.
	)
{
	SG_string*            sPrompt   = NULL;
	SG_string*            sCommand  = NULL;
	SG_string*            sMessage  = NULL;
	char*                 szCommand = NULL;
	_interactive__command cCommand;
	SG_bool               bParsed   = SG_FALSE;

	SG_NULLARGCHECK(pSession);
	SG_NULLARGCHECK(pChoice);

	// if we need to write the current choice, do that
	if (pSession->bWrite == SG_TRUE)
	{
		SG_ERR_CHECK(  _write_choice__verbose(pCtx, pChoice, pItem)  );
		pSession->bWrite = SG_FALSE;
	}

	// build the prompt
	SG_ERR_CHECK(  _interactive__build_prompt(pCtx, pSession, pChoice, &sPrompt)  );

	// if we haven't used an automatic command for this choice
	// then build any automatic command that's relevent
	if (pSession->bAutomatic == SG_FALSE)
	{
		SG_ERR_CHECK(  _interactive__build_automatic_command(pCtx, pSession, pChoice, &sCommand, &sMessage)  );
	}

	// check if we have an automatic command
	if (sCommand == NULL)
	{
		// no automatic command, ask the user for one
		SG_ERR_CHECK(  _console__prompt(pCtx, &sCommand, "%s", SG_string__sz(sPrompt))  );
	}
	else
	{
		// automatic command, write it out with the prompt
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s%s\n", SG_string__sz(sPrompt), SG_string__sz(sCommand))  );

		// if there's a message to go with it, write that as well
		if (sMessage != NULL)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", SG_string__sz(sMessage))  );
		}

		// mark that we've used an automatic command
		pSession->bAutomatic = SG_TRUE;
	}

	// unwrap the command string, we just need the buffer
	SG_ERR_CHECK(  SG_string__sizzle(pCtx, &sCommand, (SG_byte**)&szCommand, NULL)  );

	// parse the command
	SG_ERR_CHECK(  _interactive__parse_command(pCtx, szCommand, pChoice, &cCommand, &bParsed)  );
	if (bParsed == SG_TRUE)
	{
		// run the parsed command
		SG_ERR_CHECK(  cCommand.pSpec->fHandler(pCtx, pSession, pChoice, &cCommand)  );
	}

	// add a blank line after whatever output we had
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );

fail:
	SG_STRING_NULLFREE(pCtx, sPrompt);
	SG_STRING_NULLFREE(pCtx, sCommand);
	SG_STRING_NULLFREE(pCtx, sMessage);
	SG_NULLFREE(pCtx, szCommand);
	return;
}

/**
 * Runs an interactive session.
 */
static void _interactive__run_session(
	SG_context*            pCtx,    //< [in] [out] Error and context info.
	_interactive__session* pSession //< [in] An initialized _interactive__session structure.
	)
{
	SG_wc_tx * pWcTx = NULL;
	SG_resolve*     pResolve     = NULL;	// we do not own this

	SG_NULLARGCHECK(pSession);

	// cache the size of the choice list
	SG_ERR_CHECK(  SG_vector__length(pCtx, pSession->pChoices, &pSession->uChoices)  );
	SG_ARGCHECK(pSession->uChoice <= pSession->uChoices, pSession);

	// run prompts until we get through all the choices
	while (pSession->uChoice < pSession->uChoices)
	{
		const _choice_spec* pSpec   = NULL;
		SG_resolve__item*   pItem   = NULL;
		SG_resolve__choice* pChoice = NULL;

		// allocate a fresh WC TX and resolve data
		SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, NULL, SG_FALSE)  );
		SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );

		// find the data for the current choice
		SG_ERR_CHECK(  SG_vector__get(pCtx, pSession->pChoices, pSession->uChoice, (void**)&pSpec)  );
		SG_ERR_CHECK(  _choice_spec__find(pCtx, pSpec, pResolve, &pItem, &pChoice)  );

		// display and handle a prompt for this choice
		SG_ERR_CHECK(  _interactive__run_choice(pCtx, pSession, pChoice, pItem)  );

		// save any data that changed and free the pendingtree/resolve
		SG_ERR_CHECK(  SG_wc_tx__apply(pCtx, pWcTx)  );
		SG_WC_TX__NULLFREE(pCtx, pWcTx);
	}

fail:
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	return;
}


/*
**
** Private Functionality - Action Implementations
**
*/

/**
 * Lists conflicts one by one and asks the user what to do with each.
 */
void _action__interactive(
	SG_context*        pCtx,       //< [in] [out] Error and context info.
	const char*        szPath,     //< [in] If non-NULL, only list conflicts on this item (as supplied by user).
	SG_bool            bAll,       //< [in] If true, include already resolved conflicts in the list.
	SG_uint32          uMax,       //< [in] Maximum number of conflicts to list.
	const char*        szViewTool, //< [in] If non-NULL, use this viewer tool for view commands where the user doesn't specify one.
	const char*        szDiffTool, //< [in] If non-NULL, use this viewer tool for view commands where the user doesn't specify one.
	const char*        szMergeTool //< [in] If non-NULL, use this viewer tool for view commands where the user doesn't specify one.
	)
{
	_interactive__session cSession = _INTERACTIVE__SESSION__INIT;

	// configure the session
	cSession.szViewTool      = szViewTool;
	cSession.szDiffTool      = szDiffTool;
	cSession.szMergeTool     = szMergeTool;

	// allocate/populate the vector of choices to prompt for
	SG_ERR_CHECK(  _choice_spec__populate(pCtx, szPath, SG_RESOLVE__STATE__COUNT, bAll == SG_FALSE ? SG_TRUE : SG_FALSE, uMax, &cSession.pChoices)  );

	// run the session
	SG_ERR_CHECK(  _interactive__run_session(pCtx, &cSession)  );

fail:
	SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, cSession.pChoices, _choice_spec__free);
	return;
}

/**
 * Lists conflicts.
 */
void _action__list(
	SG_context*        pCtx,     //< [in] [out] Error and context info.
	const char*        szPath,   //< [in] If non-NULL, only list conflicts on this item (as supplied by user).
	SG_bool            bAll,     //< [in] If true, include already resolved conflicts in the list.
	SG_bool            bVerbose, //< [in] True to output a verbose format, false to output a list format.
	SG_uint32          uMax      //< [in] Maximum number of conflicts to list.
	                             //<      Zero indicates no maximum.
	)
{
	SG_wc_tx * pWcTx = NULL;
	SG_resolve*                   pResolve        = NULL;		// we do not own this
	SG_bool                       bOnlyUnresolved = SG_FALSE;
	char*                         szGid           = NULL;
	_write_choice__callback__data cData;

	// load the WC and resolve data
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, NULL, SG_TRUE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );

	// decide what we're listing
	if (bAll == SG_FALSE)
	{
		bOnlyUnresolved = SG_TRUE;
	}
	else
	{
		bOnlyUnresolved = SG_FALSE;
	}

	// setup the callback data
	cData.uMax     = uMax;
	cData.bVerbose = bVerbose;

	// decide which set of choices to iterate
	if (szPath == NULL)
	{
		// no item specified, iterate all of them
		SG_ERR_CHECK(  SG_resolve__foreach_choice(pCtx, pResolve, bOnlyUnresolved, bVerbose == SG_FALSE ? SG_TRUE : SG_FALSE, _write_choice__callback, (void*)&cData)  );
	}
	else
	{
		SG_resolve__item* pItem = NULL;

		// an item was specified, just iterate that one
		SG_ERR_CHECK(  SG_wc_tx__get_item_gid(pCtx, pWcTx, szPath, &szGid)  );
		SG_ERR_CHECK(  SG_resolve__check_item__gid(pCtx, pResolve, szGid, &pItem)  );
		if (!pItem)
			SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
							(pCtx, "Item '%s' does not have issues to resolve.", szPath)  );
		SG_ERR_CHECK(  SG_resolve__item__foreach_choice(pCtx, pItem, bOnlyUnresolved, _write_choice__callback, (void*)&cData)  );
	}

fail:
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	SG_NULLFREE(pCtx, szGid);
	return;
}

/**
 * Accepts a value on multiple conflicts.
 */
void _action__accept_all(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	const char*        szValue,   //< [in] Name of the value to accept.
	const char*        szPath,    //< [in] Path of the file to accept values on, as specified by the user.
	                              //<      If NULL, the accept will not be restricted by file.
	SG_resolve__state  eState,    //< [in] The conflict state/type to accept values on.
	                              //<      If SG_RESOLVE__STATE__COUNT, the accept will not be restricted on conflict state/type.
	SG_bool            bOverwrite //< [in] If true, overwrite the resolutions of matching conflicts that are already resolved.
	                              //<      If false, ignore matching conflicts that are already resolved.
	)
{
	SG_vector*      pChoices     = NULL;
	SG_uint32       uChoices     = 0u;
	SG_uint32       uChoice      = 0u;
	SG_wc_tx * pWcTx = NULL;
	SG_resolve*     pResolve     = NULL;		// we do not own this

	SG_NULLARGCHECK(szValue);

	// allocate/populate a vector of choices to accept a value on
	SG_ERR_CHECK(  _choice_spec__populate(pCtx, szPath, eState, bOverwrite == SG_FALSE ? SG_TRUE : SG_FALSE, 0u, &pChoices)  );

	// run through each of the choices in the vector
	SG_ERR_CHECK(  SG_vector__length(pCtx, pChoices, &uChoices)  );
	for (uChoice = 0u; uChoice < uChoices; ++uChoice)
	{
		_choice_spec*       pSpec   = NULL;
		SG_resolve__item*   pItem   = NULL;
		SG_resolve__choice* pChoice = NULL;
		SG_resolve__value*  pValue  = NULL;

		// get the current choice spec
		SG_ERR_CHECK(  SG_vector__get(pCtx, pChoices, uChoice, (void**)&pSpec)  );

		// allocate a fresh pendingtree and resolve data
		SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, NULL, SG_FALSE)  );
		SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );

		// find the specified choice in the resolve data
		SG_ERR_CHECK(  _choice_spec__find(pCtx, pSpec, pResolve, &pItem, &pChoice)  );

		// check if this choice has a value with the specified label
		SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, szValue, 0u, &pValue)  );
		if (pValue != NULL)
		{
			// accept the value
			SG_ERR_CHECK(  SG_resolve__accept_value(pCtx, pValue)  );
			SG_ERR_CHECK(  _write_accept(pCtx, pValue, pChoice, pItem)  );

			// save the updated resolve data
			// save is implicit when we do a TX-apply
			SG_ERR_CHECK(  SG_wc_tx__apply(pCtx, pWcTx)  );
		}
		else
		{
			SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
		}
		// free everything so we can reload it for the next choice
		SG_WC_TX__NULLFREE(pCtx, pWcTx);
	}

fail:
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, pChoices, _choice_spec__free);
	return;
}

/**
 * Accepts a value on a conflict.
 */
void _action__accept_one(
	SG_context*        pCtx,      //< [in] [out] Error and context info.
	const char*        szValue,   //< [in] Name of the value to accept.
	const char*        szPath,    //< [in] Path of the file to accept a value on, as specified by the user.
	SG_resolve__state  eState,    //< [in] The state to accept a value on.
	                              //<      May be SG_RESOLVE__STATE__COUNT if the specified item only has a single conflict.
	SG_bool            bOverwrite //< [in] If true, the item's existing resolution may be overwritten.
	                              //<      If false, an already resolved item will produce an error.
	)
{
	SG_wc_tx * pWcTx = NULL;
	SG_resolve*         pResolve     = NULL;	// we do not own this
	SG_resolve__item*   pItem        = NULL;
	SG_resolve__choice* pChoice      = NULL;

	SG_NULLARGCHECK(szValue);
	SG_NULLARGCHECK(szPath);

	// load the pendingtree and resolve data
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, NULL, SG_FALSE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );

	// find the item/choice to accept the value on
	SG_ERR_CHECK(  _find_choice(pCtx, pWcTx, pResolve, szPath, eState, &pItem, &pChoice)  );
	if (pItem != NULL && pChoice != NULL)
	{
		SG_resolve__value* pValue = NULL;

		// get the named value
		SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, szValue, 0u, &pValue)  );
		if (pValue == NULL)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Unknown value: %s\n", szValue)  );
		}
		else
		{
			SG_bool bAccepted = SG_FALSE;

			// run the accept
			SG_ERR_CHECK(  _resolve__accept(pCtx, pChoice, pValue, (SG_int32)bOverwrite, pItem, &bAccepted)  );
			if (bAccepted == SG_TRUE)
			{
				// if the accept succeeded, save the changes
				SG_ERR_CHECK(  SG_wc_tx__apply(pCtx, pWcTx)  );
				SG_WC_TX__NULLFREE(pCtx, pWcTx);
			}
		}
	}

fail:
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	return;
}

/**
 * Views a value on a conflict.
 *//*
static void _action__view(
	SG_context*        pCtx,    //< [in] [out] Error and context info.
	const char*        szValue, //< [in] Name of the value to view.
	const char*        szPath,  //< [in] Path of the file to view a value on, as specified by the user.
	SG_resolve__state  eState,  //< [in] The state to view a value on.
	                            //<      May be SG_RESOLVE__STATE__COUNT if the specified item only has a single conflict.
	const char*        szTool   //< [in] Name of a viewer tool to use.
	                            //<      May be NULL to use the default.
	)
{
	SG_wc_tx * pWcTx = NULL;
	SG_resolve*         pResolve     = NULL;
	SG_resolve__choice* pChoice      = NULL;

	SG_NULLARGCHECK(szValue);
	SG_NULLARGCHECK(szPath);

	// load the WC and resolve data
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, NULL, SG_TRUE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );

	// find the requested choice
	SG_ERR_CHECK(  _find_choice(pCtx, pWcTx, pResolve, szPath, eState, NULL, &pChoice)  );
	if (pChoice != NULL)
	{
		SG_resolve__value* pValue = NULL;

		// get the named value
		SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, szValue, 0u, &pValue)  );
		if (pValue == NULL)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Unknown value: %s\n", szValue)  );
		}
		else
		{
			// run the view
			SG_ERR_CHECK(  _resolve__view(pCtx, pChoice, pValue, szTool)  );
		}
	}

fail:
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
}
*/
/**
 * Diffs two values on a conflict.
 */
static void _action__diff(
	SG_context*        pCtx,    //< [in] [out] Error and context info.
	const char*        szLeft,  //< [in] Name of the left value to diff.
	const char*        szRight, //< [in] Name of the right value to diff.
	const char*        szPath,  //< [in] Path of the file to diff values on, as specified by the user.
	SG_resolve__state  eState,  //< [in] The state to diff values on.
	                            //<      May be SG_RESOLVE__STATE__COUNT if the specified item only has a single conflict.
	const char*        szTool   //< [in] Name of a diff tool to use.
	                            //<      May be NULL to use the default.
	)
{
	SG_wc_tx * pWcTx = NULL;
	SG_resolve*         pResolve     = NULL;	// we do not own this
	SG_resolve__choice* pChoice      = NULL;

	SG_NULLARGCHECK(szLeft);
	SG_NULLARGCHECK(szRight);
	SG_NULLARGCHECK(szPath);

	// load the TX and resolve data
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, NULL, SG_TRUE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );
	
	// find the requested choice
	SG_ERR_CHECK(  _find_choice(pCtx, pWcTx, pResolve, szPath, eState, NULL, &pChoice)  );
	if (pChoice != NULL)
	{
		SG_resolve__value* pLeft  = NULL;
		SG_resolve__value* pRight = NULL;

		// get the named values
		SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, szLeft, 0u, &pLeft)  );
		SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, szRight, 0u, &pRight)  );
		if (pLeft == NULL)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Unknown value: %s\n", szLeft)  );
		}
		else if (pRight == NULL)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Unknown value: %s\n", szRight)  );
		}
		else
		{
			// run the diff
			SG_ERR_CHECK(  _resolve__diff(pCtx, pChoice, pLeft, pRight, szTool)  );
		}
	}

fail:
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	return;
}

/**
 * Merges two values on a conflict.
 */
static void _action__merge(
	SG_context*        pCtx,   //< [in] [out] Error and context info.
	const char*        szPath, //< [in] Path of the file to merge values on, as specified by the user.
	SG_resolve__state  eState, //< [in] The state to merge values on.
	                           //<      May be SG_RESOLVE__STATE__COUNT if the specified item only has a single conflict.
	const char*        szTool  //< [in] Name of a merge tool to use.
	                           //<      May be NULL to use the default.
	)
{
	SG_wc_tx * pWcTx = NULL;
	SG_resolve*         pResolve     = NULL;	// we do not own this
	SG_resolve__item*   pItem        = NULL;
	SG_resolve__choice* pChoice      = NULL;

	SG_NULLARGCHECK(szPath);

	// load the WC and resolve data
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, NULL, SG_FALSE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_resolve_info(pCtx, pWcTx, &pResolve)  );

	// find the requested choice
	SG_ERR_CHECK(  _find_choice(pCtx, pWcTx, pResolve, szPath, eState, &pItem, &pChoice)  );
	if (pItem != NULL && pChoice != NULL)
	{
		SG_bool            bAccept = SG_FALSE;
		SG_resolve__value* pResult = NULL;

		// configure the merge to automatically accept the result
		// if the choice appears fresh and easily resolvable with this merge
		SG_ERR_CHECK(  _resolve__choice__plain_merge(pCtx, pChoice, &bAccept)  );

		// run the merge
		SG_ERR_CHECK(  _resolve__merge(pCtx, pChoice, szTool, bAccept, pItem, &pResult)  );
		if (pResult != NULL)
		{
			// save the changed resolve/pendingtree data
			// save is implicit when we do a TX-apply
			SG_ERR_CHECK(  SG_wc_tx__apply(pCtx, pWcTx)  );
		}
		else
		{
			SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
		}
		SG_WC_TX__NULLFREE(pCtx, pWcTx);
	}

fail:
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	return;
}


/*
**
** Private Functionality - Subcommand Handlers
**
*/

/**
 * _subcommand__command_handler for "list" subcommand.
 */
static void _subcommand__handler__list(
	SG_context*            pCtx,
	const SG_option_state* pOptions,
	SG_uint32              uArgs,
	const char**           ppArgs
	)
{
	const char* szFilename = NULL;

	SG_NULLARGCHECK(pOptions);
	SG_NULLARGCHECK(ppArgs);

	switch (uArgs)
	{
	default:
		SG_ERR_THROW(SG_ERR_USAGE);
	case 2u:
		szFilename = ppArgs[1];
	case 1u:
		;
	}

	SG_ERR_CHECK(  _action__list(pCtx, szFilename, pOptions->bAll, pOptions->bVerbose, 0u)  );

fail:
	return;
}

/**
 * _subcommand__command_handler for "next" subcommand.
 */
static void _subcommand__handler__next(
	SG_context*            pCtx,
	const SG_option_state* pOptions,
	SG_uint32              uArgs,
	const char**           ppArgs
	)
{
	const char* szFilename = NULL;

	SG_NULLARGCHECK(pOptions);
	SG_NULLARGCHECK(ppArgs);

	switch (uArgs)
	{
	default:
		SG_ERR_THROW(SG_ERR_USAGE);
	case 2u:
		szFilename = ppArgs[1];
	case 1u:
		;
	}

	SG_ERR_CHECK(  _action__list(pCtx, szFilename, SG_FALSE, SG_TRUE, 1u)  );

fail:
	return;
}

/**
 * _subcommand__command_handler for "FILENAME" subcommand.
 */
static void _subcommand__handler__filename(
	SG_context*            pCtx,
	const SG_option_state* pOptions,
	SG_uint32              uArgs,
	const char**           ppArgs
	)
{
	SG_wc_status_flags statusFlags;
	const char*  szFilename = NULL;

	SG_NULLARGCHECK(pOptions);
	SG_NULLARGCHECK(ppArgs);

	switch (uArgs)
	{
	default:
		SG_ERR_THROW(SG_ERR_USAGE);
	case 1u:
		szFilename = ppArgs[0];
	}

	// This code originally called SG_fsobj__exists__pathname() to determine
	// if the given file/path exists before doing anything serious.
	//
	// We can't do that because it won't handle extended-prefix repo-paths.
	// So we do a quick (non-recursive) status on this item to see if it exists.
	// We could just as easily do a regular status on this item, but that's
	// a little more work -- however it would throw _NOT_FOUND and this just
	// returns __BOGUS.

	SG_ERR_CHECK(  SG_wc__get_item_status_flags(pCtx, NULL, szFilename, SG_TRUE, SG_FALSE, &statusFlags, NULL)  );
	if (statusFlags & SG_WC_STATUS_FLAGS__T__BOGUS)
	{
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "Unknown item '%s'.", szFilename)  );
	}
	else if (SG_WC_STATUS_FLAGS__IS_UNCONTROLLED(statusFlags))
	{
		SG_ERR_THROW2(  SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL,
						(pCtx, "Item '%s' not under version control.", szFilename)  );
	}
	else
	{
		SG_ERR_CHECK(  _action__interactive(pCtx, szFilename, SG_TRUE, 0u, NULL, NULL, pOptions->psz_tool)  );
	}

fail:
	return;
}

/**
 * _subcommand__command_handler for "view" subcommand.
 *//*
static void _subcommand__handler__view(
	SG_context*            pCtx,
	const SG_option_state* pOptions,
	SG_uint32              uArgs,
	const char**           ppArgs
	)
{
	const char*         szValue    = NULL;
	const char*         szFilename = NULL;
	const char*         szConflict = NULL;
	SG_resolve__state   eState     = SG_RESOLVE__STATE__COUNT;

	SG_NULLARGCHECK(pOptions);
	SG_NULLARGCHECK(ppArgs);

	switch (uArgs)
	{
	default:
		SG_ERR_THROW(SG_ERR_USAGE);
	case 4u:
		szConflict = ppArgs[3];
	case 3u:
		szFilename = ppArgs[2];
		szValue    = ppArgs[1];
	}

	if (szConflict != NULL)
	{
		SG_ERR_CHECK(  SG_resolve__state__check_name(pCtx, szConflict, &eState)  );
		if (eState == SG_RESOLVE__STATE__COUNT)
		{
			SG_ERR_THROW2(  SG_ERR_USAGE,
							(pCtx, "Unknown conflict type: %s\n", szConflict)  );
		}
	}

	SG_ERR_CHECK(  _action__view(pCtx, szValue, szFilename, eState, pOptions->psz_tool)  );

fail:
	return;
}
*/
/**
 * _subcommand__command_handler for "diff" subcommand.
 */
static void _subcommand__handler__diff(
	SG_context*            pCtx,
	const SG_option_state* pOptions,
	SG_uint32              uArgs,
	const char**           ppArgs
	)
{
	const char*         szLeft     = NULL;
	const char*         szRight    = NULL;
	const char*         szFilename = NULL;
	const char*         szConflict = NULL;
	SG_resolve__state   eState     = SG_RESOLVE__STATE__COUNT;

	SG_NULLARGCHECK(pOptions);
	SG_NULLARGCHECK(ppArgs);

	switch (uArgs)
	{
	default:
		SG_ERR_THROW(SG_ERR_USAGE);
	case 5u:
		szConflict = ppArgs[4];
	case 4u:
		szFilename = ppArgs[3];
		szRight    = ppArgs[2];
		szLeft     = ppArgs[1];
	}

	if (szConflict != NULL)
	{
		SG_ERR_CHECK(  SG_resolve__state__check_name(pCtx, szConflict, &eState)  );
		if (eState == SG_RESOLVE__STATE__COUNT)
		{
			SG_ERR_THROW2(  SG_ERR_USAGE,
							(pCtx, "Unknown conflict type: %s\n", szConflict)  );
		}
	}

	SG_ERR_CHECK(  _action__diff(pCtx, szLeft, szRight, szFilename, eState, pOptions->psz_tool)  );

fail:
	return;
}

/**
 * _subcommand__command_handler for "merge" subcommand.
 */
static void _subcommand__handler__merge(
	SG_context*            pCtx,
	const SG_option_state* pOptions,
	SG_uint32              uArgs,
	const char**           ppArgs
	)
{
	const char*       szFilename = NULL;
	const char*       szConflict = NULL;
	SG_resolve__state eState     = SG_RESOLVE__STATE__COUNT;

	SG_NULLARGCHECK(pOptions);
	SG_NULLARGCHECK(ppArgs);

	switch (uArgs)
	{
	default:
		SG_ERR_THROW(SG_ERR_USAGE);
	case 3u:
		szConflict = ppArgs[2];
	case 2u:
		szFilename = ppArgs[1];
	}

	if (szConflict != NULL)
	{
		SG_ERR_CHECK(  SG_resolve__state__check_name(pCtx, szConflict, &eState)  );
		if (eState == SG_RESOLVE__STATE__COUNT)
		{
			SG_ERR_THROW2(  SG_ERR_USAGE,
							(pCtx, "Unknown conflict type: %s\n", szConflict)  );
		}
	}

	SG_ERR_CHECK(  _action__merge(pCtx, szFilename, eState, pOptions->psz_tool)  );

fail:
	return;
}

/**
 * _subcommand__command_handler for "accept" subcommand.
 */
static void _subcommand__handler__accept(
	SG_context*            pCtx,
	const SG_option_state* pOptions,
	SG_uint32              uArgs,
	const char**           ppArgs
	)
{
	const char*       szValue    = NULL;
	const char*       szFilename = NULL;
	const char*       szConflict = NULL;
	const char*       szUnknown  = NULL;
	SG_resolve__state eState     = SG_RESOLVE__STATE__COUNT;

	SG_NULLARGCHECK(pOptions);
	SG_NULLARGCHECK(ppArgs);

	// parse the arguments
	if (uArgs < 2u || uArgs > 4u)
	{
		SG_ERR_THROW(SG_ERR_USAGE);
	}
	else if (uArgs == 2u)
	{
		szValue = ppArgs[1];
	}
	else if (uArgs == 3u)
	{
		szValue   = ppArgs[1];
		szUnknown = ppArgs[2];
	}
	else if (uArgs == 4u)
	{
		szValue    = ppArgs[1];
		szFilename = ppArgs[2];
		szConflict = ppArgs[3];
	}

	// process the parsed arguments
	if (szUnknown != NULL)
	{
		// szUnknown might be a conflict or a filename
		// try parsing a conflict out of it
		SG_ERR_CHECK(  SG_resolve__state__check_name(pCtx, szUnknown, &eState)  );
		if (eState == SG_RESOLVE__STATE__COUNT)
		{
			// didn't parse correctly, assume it's a filename
			szFilename = szUnknown;
		}
		else
		{
			// parsed fine, it's a conflict
			szConflict = szUnknown;
		}
		szUnknown = NULL;
	}
	else if (szConflict != NULL)
	{
		// parse the conflict into an enumerated value
		SG_ERR_CHECK(  SG_resolve__state__check_name(pCtx, szConflict, &eState)  );
		if (eState == SG_RESOLVE__STATE__COUNT)
		{
			SG_ERR_THROW2(  SG_ERR_USAGE,
							(pCtx, "Unknown conflict type: %s\n", szConflict)  );
		}
	}

	// check for problems
	if (szConflict != NULL && eState == SG_RESOLVE__STATE__COUNT)
	{
		// if a conflict was specified, it would be correctly parsed by now
		SG_ERR_THROW2(  SG_ERR_USAGE,
						(pCtx, "Unknown conflict type: %s\n", szConflict)  );
	}
	else
	{
		// call the appropriate command
		if (pOptions->bAll == SG_FALSE)
		{
			if (szFilename == NULL)
			{
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "To resolve one conflict specify the file (and conflict if it has several).\n")  );
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "To resolve multiple conflicts use --all.\n")  );
			}
			else
			{
				SG_ERR_CHECK(  _action__accept_one(pCtx, szValue, szFilename, eState, pOptions->bOverwrite)  );
			}
		}
		else
		{
			SG_ERR_CHECK(  _action__accept_all(pCtx, szValue, szFilename, eState, pOptions->bOverwrite)  );
		}
	}

fail:
	return;
}


/*
**
** Public Functionality
**
*/

/**
 * Entry point for resolve command implementation.
 */
void do_cmd_resolve2(
	SG_context*      pCtx,     //< [in] [out] Error and context info.
	SG_option_state* pOptions, //< [in] Parsed option state.
	SG_uint32        uArgs,    //< [in] Number of unparsed arguments.
	const char**     ppArgs    //< [in] Unparsed arguments.
	)
{
	SG_NULLARGCHECK(pOptions);
	SG_ARGCHECK(ppArgs != NULL || uArgs == 0u, ppArgs);

	// if we don't have any arguments, do the default action
	if (uArgs == 0u)
	{
		// default action is an unfiltered interactive prompt
		SG_ERR_CHECK(  _action__interactive(pCtx, NULL, pOptions->bAll, 0u, NULL, NULL, pOptions->psz_tool)  );
	}
	else
	{
		SG_uint32                        uIndex = 0u;
		const _subcommand__command_spec* pSpec  = NULL;

		// run through the defined subcommands and look for a match
		for (uIndex = 0u; uIndex < SG_NrElements(gaSubcommands); ++uIndex)
		{
			if (SG_stricmp(ppArgs[0], gaSubcommands[uIndex].szName) == 0)
			{
				pSpec = gaSubcommands + uIndex;
				break;
			}
		}

		// if we didn't find a match, use the default subcommand
		if (pSpec == NULL)
		{
			pSpec = &gcDefaultSubcommand;
		}

		// run the command's handler
		SG_ERR_CHECK(  pSpec->fHandler(pCtx, pOptions, uArgs, ppArgs)  );
	}

fail:
	return;
}
