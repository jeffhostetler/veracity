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

#include "precompiled.h"
#include "vvValidator.h"


/*
**
** Public Functions
**
*/

vvValidator::vvValidator(
	const wxString& sLabel,
	vvFlags         cFlags
	)
	: vvHasFlags(cFlags)
	, msLabel(sLabel)
	, mpReporter(NULL)
	, mpTransfer(NULL)
{
}

vvValidator::vvValidator(
	const vvValidator& that
	)
	: vvHasFlags(that.GetFlags())
	, msLabel(that.msLabel)
	, mpReporter(that.mpReporter)
	, mpTransfer(that.mpTransfer)
{
	if (this->mpTransfer != NULL)
	{
		this->mpTransfer = (vvValidatorTransfer*)this->mpTransfer->Clone();
	}

	for (stlValidationConstraintList::const_iterator itConstraint = that.mcConstraints.begin(); itConstraint != that.mcConstraints.end(); ++itConstraint)
	{
		this->mcConstraints.push_back((vvValidatorConstraint*)(*itConstraint)->Clone());
	}
}

vvValidator::~vvValidator()
{
	if (this->mpTransfer != NULL)
	{
		delete this->mpTransfer;
	}

	for (stlValidationConstraintList::const_iterator itConstraint = this->mcConstraints.begin(); itConstraint != this->mcConstraints.end(); ++itConstraint)
	{
		delete *itConstraint;
	}
}

vvValidator& vvValidator::operator=(
	const vvValidator& that
	)
{
	// delete our old data
	if (this->mpTransfer != NULL)
	{
		delete this->mpTransfer;
	}
	for (stlValidationConstraintList::const_iterator itConstraint = this->mcConstraints.begin(); itConstraint != this->mcConstraints.end(); ++itConstraint)
	{
		delete *itConstraint;
	}
	this->mcConstraints.clear();

	// copy the new data
	this->SetFlags(that.GetFlags());
	this->msLabel    = that.msLabel;
	this->mpReporter = that.mpReporter;
	this->mpTransfer = that.mpTransfer;
	if (this->mpTransfer != NULL)
	{
		this->mpTransfer = (vvValidatorTransfer*)this->mpTransfer->Clone();
	}
	for (stlValidationConstraintList::const_iterator itConstraint = that.mcConstraints.begin(); itConstraint != that.mcConstraints.end(); ++itConstraint)
	{
		this->mcConstraints.push_back((vvValidatorConstraint*)(*itConstraint)->Clone());
	}

	// return ourselves
	return *this;
}

wxObject* vvValidator::Clone() const
{
	return new vvValidator(*this);
}

const wxString& vvValidator::GetLabel() const
{
	return this->msLabel;
}

vvValidatorReporter* vvValidator::GetReporter() const
{
	return this->mpReporter;
}

vvValidator& vvValidator::SetValidatorFlags(
	unsigned int uFlags
	)
{
	this->SetFlags(uFlags);
	return *this;
}

vvValidator& vvValidator::SetReporter(
	vvValidatorReporter* pReporter
	)
{
	this->mpReporter = pReporter;
	return *this;
}

vvValidatorTransfer* vvValidator::GetTransfer() const
{
	return this->mpTransfer;
}

vvValidator& vvValidator::SetTransfer(
	const vvValidatorTransfer& cTransfer
	)
{
	if (this->mpTransfer != NULL)
	{
		delete this->mpTransfer;
	}
	this->mpTransfer = (vvValidatorTransfer*)cTransfer.Clone();
	return *this;
}

vvValidator& vvValidator::AddConstraint(
	const vvValidatorConstraint& cConstraint
	)
{
	this->mcConstraints.push_back((vvValidatorConstraint*)cConstraint.Clone());
	return *this;
}

bool vvValidator::Validate()
{
	return this->Validate(NULL);
}

bool vvValidator::Validate(
	wxWindow *WXUNUSED(wParent)
	)
{
	wxASSERT(this->GetWindow() != NULL);

	// if our window is disabled and/or hidden, we might always successfully validate
	if (this->HasAllFlags(vvValidator::VALIDATE_SUCCEED_IF_DISABLED) && !this->GetWindow()->IsEnabled())
	{
		return true;
	}
	if (this->HasAllFlags(vvValidator::VALIDATE_SUCCEED_IF_HIDDEN) && !this->GetWindow()->IsShown())
	{
		return true;
	}

	// validate each of our constraints
	bool success = true;
	for (stlValidationConstraintList::const_iterator itConstraint = this->mcConstraints.begin(); itConstraint != this->mcConstraints.end(); ++itConstraint)
	{
		if (!(*itConstraint)->Validate(this))
		{
			success = false;
		}
	}
	return success;
}

bool vvValidator::TransferToWindow()
{
	if (this->mpTransfer == NULL)
	{
		return true;
	}
	else
	{
		return this->mpTransfer->TransferToWindow(this->GetWindow());
	}
}

bool vvValidator::TransferFromWindow()
{
	if (this->mpTransfer == NULL)
	{
		return true;
	}
	else
	{
		return this->mpTransfer->TransferFromWindow(this->GetWindow());
	}
}

void vvValidator::ReportSuccess(
	const wxString& sMessage
	)
{
	if(this->mpReporter == NULL)
	{
		return;
	}
	else
	{
		this->mpReporter->ReportSuccess(this, sMessage);
	}
}

void vvValidator::ReportError(
	const wxString& sMessage
	)
{
	if(this->mpReporter == NULL)
	{
		return;
	}
	else
	{
		this->mpReporter->ReportError(this, sMessage);
	}
}
