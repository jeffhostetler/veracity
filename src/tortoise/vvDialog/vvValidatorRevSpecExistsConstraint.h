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
 * A vvValidatorConstraint that checks if an entered vvRevSpec references
 * revisions that actually exist in a given repo.
 *
 * Currently supports:
 * - vvRevSpecControl
 */
class vvValidatorRevSpecExistsConstraint : public vvValidatorConstraint
{
// construction
public:
	/**
	 * Constructor.
	 */
	vvValidatorRevSpecExistsConstraint(
		class vvVerbs&   cVerbs,        //< [in] Verbs to use to look up info in the repo.
		class vvContext& cContext,      //< [in] Context to use with cVerbs and the vvRevSpec being validated.
		const wxString&  sRepo,         //< [in] Name of the repo to check for the revision in.
		bool             bExists = true //< [in] True to validate that the revision(s) exists, false to validate that it doesn't.
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

// internal functionality
protected:
	/**
	 * Validates the given rev spec.
	 */
	bool ValidateState(
		vvValidator*           pValidator, //< [in] The validator to report messages to.
		const class vvRevSpec& cRevSpec    //< [in] The rev spec entered in the window we're validating.
		) const;

// private functionality
private:
	/**
	 * Validates the existence state of an individual item.
	 */
	bool ValidateItem(
		vvValidator*    pValidator, //< [in] The validator to report messages to.
		const wxString& sItemType,  //< [in] The type of the item being validated.
		const wxString& sItemName,  //< [in] The name of the item being validated.
		bool            bItemExists //< [in] Whether or not the item being validated exists.
		) const;

	/**
	 * Validates that a vvRevSpecControl meets the criteria.
	 */
	bool ValidateRevSpecControl(
		vvValidator*            pValidator, //< [in] The validator to report messages to.
		class vvRevSpecControl* wRevSpec    //< [in] The vvRevSpecControl to validate.
		) const;

// private data
private:
	class vvVerbs&   mcVerbs;   //< vvVerbs to query for existing revisions.
	class vvContext& mcContext; //< Context to use with mpVerbs and vvRevSpecs.
	wxString         msRepo;    //< The repo to check for the revision in.
	bool             mbExists;  //< True to ensure the revision exists, false to ensure it doesn't exist.

};


#endif
