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

#ifndef VV_VALIDATOR_REPO_EXISTS_CONSTRAINT_H
#define VV_VALIDATOR_REPO_EXISTS_CONSTRAINT_H

#include "precompiled.h"
#include "vvValidator.h"


/**
 * A vvValidatorConstraint that checks if the entered repo name exists or not.
 * Currently supports:
 * - wxItemContainerImmutable-derived controls
 * - wxTextEntry-derived controls
 */
class vvValidatorRepoExistsConstraint : public vvValidatorConstraint
{
// construction
public:
	/**
	 * Constructor.
	 */
	vvValidatorRepoExistsConstraint(
		class vvVerbs&   cVerbs,        //< [in] vvVerbs instance to query for existing repos.
		class vvContext& cContext,      //< [in] Context to use with the vvVerbs.
		bool             bExists = true //< [in] True to validate that the repo exists, false to validate that it doesn't.
		);

// functionality
public:
	/**
	 * Creates a clone/copy of the constraint.
	 * Caller owns the returned memory.
	 */
	virtual wxObject* Clone() const;

	/**
	 * Run the validation constraint against a validator.
	 * Returns true if the validation was successful, or false if it wasn't.
	 * Reports both success and error messages to the validator.
	 */
	virtual bool Validate(
		vvValidator* pValidator //< [in] Validator being validated.
		) const;

// private functionality
private:
	/**
	 * Validates that a control with items has an item selected.
	 */
	bool ValidateItemContainer(
		vvValidator*              pValidator, //< [in] The validator to report messages to.
		wxItemContainerImmutable* wContainer  //< [in] The item container window to validate.
		) const;

	/**
	 * Validates that a text entry widget isn't empty.
	 */
	bool ValidateTextEntry(
		vvValidator* pValidator, //< [in] The validator to report messages to.
		wxTextEntry* wTextEntry  //< [in] The text entry window to validate.
		) const;

	/**
	 * Validates the given window state.
	 */
	bool ValidateState(
		vvValidator*    pValidator, //< [in] The validator to report messages to.
		const wxString& sRepoName   //< [in] The repo name entered in the window we're validating.
		) const;

// private data
private:
	class vvVerbs&   mcVerbs;   //< vvVerbs to query for existing repos.
	class vvContext& mcContext; //< Context to use with mpVerbs.
	bool             mbExists;  //< True to ensure the repo exists, false to ensure it doesn't exist.

};


#endif
