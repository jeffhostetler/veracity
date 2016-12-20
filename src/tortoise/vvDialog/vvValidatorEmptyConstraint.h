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

#ifndef VV_VALIDATOR_EMPTY_CONSTRAINT_H
#define VV_VALIDATOR_EMPTY_CONSTRAINT_H

#include "precompiled.h"
#include "vvValidator.h"


/**
 * A vvValidatorConstraint that checks if a field is empty or not.
 * Currently supports:
 * - wxDatePickerCtrl
 * - wxItemContainerImmutable-derived controls
 * - wxTextEntry-derived controls
 * - vvCurrentUserControl
 * - vvRevSpecControl
 * - vvStatusControl
 */
class vvValidatorEmptyConstraint : public vvValidatorConstraint
{
// construction
public:
	/**
	 * Constructor.
	 */
	vvValidatorEmptyConstraint(
		bool             bEmpty   = false, //< [in] True to validate that the field is empty, false to validate that it isn't.
		class vvContext* pContext = NULL   //< [in] The context to use with controls that require it.
		                                   //<      Required by: vvRevSpecControl
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
		vvValidator* pValidator, //< [in] The validator to report messages to.
		bool         bItems,     //< [in] True if "emptiness" is in terms of item selection, rather than blank value.
		bool         bIsEmpty    //< [in] Whether or not the window we're validating is empty.
		) const;

// private functionality
private:
	/**
	 * Validates that a date picker control has a date selected.
	 */
	bool ValidateDatePickerCtrl(
		vvValidator*      pValidator, //< [in] The validator to report messages to.
		wxDatePickerCtrl* wDatePicker //< [in] The date picker window to validate.
		) const;

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
	 * Validates that a vvCurrentUserControl has a user.
	 */
	bool ValidateCurrentUserControl(
		vvValidator*                pValidator, //< [in] The validator to report messages to.
		class vvCurrentUserControl* wUser       //< [in] The vvCurrentUserControl to validate.
		) const;

	/**
	 * Validates that a vvRevSpecControl has a rev spec that contains at least one revision.
	 */
	bool ValidateRevSpecControl(
		vvValidator*            pValidator, //< [in] The validator to report messages to.
		class vvRevSpecControl* wRevSpec    //< [in] The vvRevSpecControl to validate.
		) const;

	/**
	 * Validates that a vvStatusControl has an item selected.
	 */
	bool ValidateStatusControl(
		vvValidator*           pValidator, //< [in] The validator to report messages to.
		class vvStatusControl* wStatus     //< [in] The vvStatusControl to validate.
		) const;

// private data
private:
	bool             mbEmpty;   //< True to validate that the field is empty, false to validate that it isn't.
	class vvContext* mpContext; //< Context to use with controls that require it.
};


#endif
