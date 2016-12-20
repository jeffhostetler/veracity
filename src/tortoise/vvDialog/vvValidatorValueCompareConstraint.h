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

#ifndef VV_VALIDATOR_VALUE_COMPARE_CONSTRAINT_H
#define VV_VALIDATOR_VALUE_COMPARE_CONSTRAINT_H

#include "precompiled.h"
#include "vvValidator.h"


/**
 * A vvValidatorConstraint that checks if a field's value equals a given value.
 * Currently supports:
 * - wxItemContainerImmutable-derived controls
 * - wxTextEntry-derived controls
 */
class vvValidatorValueCompareConstraint : public vvValidatorConstraint
{
// construction
public:
	/**
	 * Constructor.
	 */
	vvValidatorValueCompareConstraint(
		const wxString&  sValue,                //< [in] The value to compare against the field's value.
		bool             bCaseSensitive = true, //< [in] Whether or not value matching should be case-sensitive.
		bool             bEqual         = false //< [in] True to validate that the field equals the value.
		                                        //<      False to validate that it does NOT equal the value.
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
	 * Validates the given window state.
	 */
	bool ValidateState(
		vvValidator*    pValidator, //< [in] The validator to report messages to.
		const wxString& sValue      //< [in] The current value of the control that we're validating.
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
	 * Validates a text entry widget's value.
	 */
	bool ValidateTextEntry(
		vvValidator* pValidator, //< [in] The validator to report messages to.
		wxTextEntry* wTextEntry  //< [in] The text entry window to validate.
		) const;

// private data
private:
	wxString              msValue; //< The value to compare against the field.
	wxString::caseCompare meCase;  //< Type of string comparison to use.
	bool                  mbEqual; //< True to validate that the value matches, false to validate that it doesn't.
};


#endif
