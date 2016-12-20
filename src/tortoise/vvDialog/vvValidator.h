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

#ifndef VV_VALIDATOR_H
#define VV_VALIDATOR_H

#include "precompiled.h"
#include "vvFlags.h"


/**
 * An abstract interface that validation results can be reported to.
 * Expected to be generally implemented by controls/widgets that display the results.
 */
class vvValidatorReporter
{
// functionality
public:
	/**
	 * Starts a new session of validation reporting.
	 */
	virtual void BeginReporting(
		wxWindow* wParent //< [in] The window to use as the parent of any spawned windows.
		) = 0;

	/**
	 * Ends an existing session of validation reporting.
	 */
	virtual void EndReporting(
		wxWindow* wParent //< [in] The window to use as the parent of any spawned windows.
		) = 0;

	/**
	 * Reports a successful message from a validator.
	 */
	virtual void ReportSuccess(
		class vvValidator* pValidator, //< [in] The validator reporting the success.
		const wxString&    sMessage    //< [in] A message describing the successful validation.
		) = 0;

	/**
	 * Reports an error message from a validator.
	 */
	virtual void ReportError(
		class vvValidator* pValidator, //< [in] The validator reporting the error.
		const wxString&    sMessage    //< [in] A message describing the error.
		) = 0;
};


/**
 * An abstract interface for doing a single validation check on a control.
 * Validators use a collection of these constraints to perform full validation on a control.
 */
class vvValidatorConstraint : public wxObject
{
// functionality
public:
	/**
	 * Creates a clone/copy of the constraint.
	 * Caller owns the returned memory.
	 */
	virtual wxObject* Clone() const = 0;

	/**
	 * Run the validation constraint against a validator.
	 * Returns true if the validation was successful, or false if it wasn't.
	 * Should report both success and error messages to the validator.
	 */
	virtual bool Validate(
		vvValidator* pValidator //< [in] Validator being validated.
		                        //<      Retrieve the window being validated with GetWindow.
		                        //<      Report success/errors using ReportSuccess/ReportError.
		) const = 0;
};


/**
 * An abstract interface for a class that can transfer data between a window and a variable.
 * Validators use these to implement their transfer methods.
 */
class vvValidatorTransfer
{
// functionality
public:
	/**
	 * Creates a clone/copy of the transfer.
	 * Caller owns the returned memory.
	 */
	virtual wxObject* Clone() const = 0;

	/**
	 * Transfers data from a variable to the given window.
	 */
	virtual bool TransferToWindow(wxWindow* wWindow) const = 0;

	/**
	 * Transfers data from the given window to a variable.
	 */
	virtual bool TransferFromWindow(wxWindow* wWindow) const = 0;
};


/**
 * A validator that supports:
 * - use of smaller re-usable constraint logic components
 * - reporting errors to a vvValidatorReporter
 * - transferring data to a variable with a vvValidatorTransfer
 *
 * This is intended to decouple all of the following:
 * - controls (one vvValidator/wxValidator per control)
 * - individual validation logic (vvValidatorConstraint)
 * - reporting of validation results (vvValidatorReporter)
 * - data transfer to variables (vvValidatorTransfer)
 */
class vvValidator : public wxValidator, public vvHasFlags
{
// types
public:
	/**
	 * Flags to customize the behavior of the validator.
	 */
	enum Flags
	{
		VALIDATE_SUCCEED_IF_DISABLED = 1 << 0, //< Validator will always validate successfully if its window is disabled.
		VALIDATE_SUCCEED_IF_HIDDEN   = 1 << 1, //< Validator will always validate successfully if its window is hidden.
	};

// construction
public:
	/**
	 * Constructs a new vvValidator.
	 */
	vvValidator(
		const wxString& sLabel = wxEmptyString, //< [in] User-friendly name of the window/control being validated.
		vvFlags         cFlags = 0u             //< [in] Combination of flags the validator should use.
		);

	/**
	 * Copy constructor.
	 */
	vvValidator(
		const vvValidator& that
		);

	/**
	 * Destructor.
	 */
	~vvValidator();

	/**
	 * Assignment operator.
	 */
	vvValidator& operator=(
		const vvValidator& that //< [in] Validator to assign to this one.
		);

// functionality
public:
	/**
	 * Creates a clone/copy of the validator.
	 * Caller owns the returned memory.
	 */
	virtual wxObject* Clone() const;

	/**
	 * Gets a user-friendly label for the validator's window.
	 */
	const wxString& GetLabel() const;

	/**
	 * Convenience wrapper around SetFlags.
	 * Returns a self-reference, so that calls can be chained.
	 */
	vvValidator& SetValidatorFlags(
		unsigned int uFlags
		);

	/**
	 * Gets the reporter that this validator reports results to.
	 */
	vvValidatorReporter* GetReporter() const;

	/**
	 * Sets the reporter that this validator will report validation results to.
	 * Returns a self-reference, so that calls can be chained.
	 */
	vvValidator& SetReporter(
		vvValidatorReporter* pReporter //< [in] The reporter that the validator should report to.
		                                //<      Caller still owns this memory.
		);

	/**
	 * Gets the transfer object that this validator uses to transfer data to/from its window.
	 */
	vvValidatorTransfer* GetTransfer() const;

	/**
	 * Sets the transfer that this validator will use to transfer data between its window and a variable.
	 * Returns a self-reference, so that calls can be chained.
	 */
	vvValidator& SetTransfer(
		const vvValidatorTransfer& cTransfer //< [in] The transfer that the validator should use.
		);

	/**
	 * Adds a constraint to the validator.
	 * Returns a self-reference, so that calls can be chained.
	 */
	vvValidator& AddConstraint(
		const vvValidatorConstraint& cConstraint //< [in] The rule to add.
		);

	/**
	 * Overload of validate that doesn't require a parent window.
	 * The parent window is only necessary because wxValidator expects derived classes to spawn child windows.
	 * We don't have that expectation of our derived classes, so we just pass NULL.
	 */
	bool Validate();

	/**
	 * Main wx version of Validate.
	 */
	virtual bool Validate(
		wxWindow* pParent
		);

	/**
	 * Transfers data to the validator's window from a variable.
	 */
	virtual bool TransferToWindow();

	/**
	 * Transfers data from the validator's window to a variable.
	 */
	virtual bool TransferFromWindow();

	/**
	 * Reports a successful validation.
	 * Intended for use from derived classes.
	 */
	void ReportSuccess(
		const wxString& sMessage //< [in] A message describing the successful validation.
		);

	/**
	 * Reports a validation error.
	 * Intended for use from derived classes.
	 */
	void ReportError(
		const wxString& sMessage //< [in] The error message being reported.
		);

// private types
private:
	/**
	 * A list of validation constraints.
	 */
	typedef std::list<vvValidatorConstraint*> stlValidationConstraintList;

// private data
private:
	wxString                    msLabel;       //< User-friendly label for the validator's window.
	vvValidatorReporter*        mpReporter;    //< Reporter that the validator reports to.
	vvValidatorTransfer*        mpTransfer;    //< Object used to transfer data to/from the validator's window.
	stlValidationConstraintList mcConstraints; //< List of constraints used by the validator.
};


#endif
