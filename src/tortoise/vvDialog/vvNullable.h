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

#ifndef VV_NULLABLE_H
#define VV_NULLABLE_H


/**
 * This type basically exists to not be any other type.
 * It allows vvNullable to overload functions that take a tValueType,
 * and not be ambiguous no matter the type of tValueType.
 * Its actual data doesn't matter, so long as it remains small enough to pass by value.
 */
struct vvNull
{
	int iDummy; //< A dummy value, should always be zero.
};


/**
 * A value that vvNullable recognizes as null.
 * This value can be used with vvNullable's member functions to indicate a null value.
 * For example, it can be passed to the constructor, assignment operator, and SetValue.
 */
extern const vvNull vvNULL;


/**
 * A utility class for storing values that might be of a given type or "null".
 * Allows value types that don't normally have an invalid value to have one.
 * i.e. vvNullable<bool> can store true, false, and "null".
 */
template <typename tValueType>
class vvNullable
{
// construction
public:
	/**
	 * Default constructor.
	 */
	vvNullable()
		: mbNull(true)
	{
	}

	/**
	 * Initialization constructor.
	 */
	vvNullable(
		const tValueType& tValue
		)
		: mbNull(false)
		, mtValue(tValue)
	{
	}

	/**
	 * Overload of initialization constructor for null.
	 */
	vvNullable(
		vvNull //< [in] The vvNULL constant.
		)
		: mbNull(true)
	{
	}

// functionality
public:
	/**
	 * Checks if the value is currently null.
	 */
	bool IsNull() const
	{
		return this->mbNull;
	}

	/**
	 * Checks if the value is either null or equal to a given value.
	 */
	bool IsNullOrEqual(
		const tValueType& tValue //< The value to compare against, if this is non-null.
		) const
	{
		return this->IsNull() || (this->GetValue() == tValue);
	}

	/**
	 * Checks if the value is currently non-null.
	 * This is equivalent to !IsNull().
	 * It's just for readability in complex conditional statements.
	 */
	bool IsValid() const
	{
		return !this->mbNull;
	}

	/**
	 * Checks if the value is both valid and equal to a given value.
	 */
	bool IsValidAndEqual(
		const tValueType& tValue
		) const
	{
		return this->IsValid() && (this->GetValue() == tValue);
	}

	/**
	 * Sets the value to null.
	 */
	void SetNull()
	{
		this->mbNull = true;
	}

	/**
	 * Gets the actual value.
	 * Asserts that it's currently non-null.
	 */
	const tValueType& GetValue() const
	{
		// if you trip this, it was the equivalent of dereferencing NULL
		wxASSERT(this->IsValid());
		return this->mtValue;
	}

	/**
	 * Gets the actual value, if non-null.
	 * Returns true if the parameter was filled in or false if the value is null.
	 */
	bool GetValue(
		tValueType& tValue //< [out] The retrieved value, if true is returned.
		                   //<       If false is returned, the parameter is unmodified.
		) const
	{
		if (this->mbNull)
		{
			return false;
		}
		else
		{
			tValue = this->mtValue;
			return true;
		}
	}

	/**
	 * Sets the value to a non-null value.
	 */
	void SetValue(
		const tValueType& tValue //< [in] The value to store.
		)
	{
		this->mbNull = false;
		this->mtValue = tValue;
	}

	/**
	 * Sets the value to null.
	 */
	void SetValue(
		vvNull //< [in] The vvNULL constant.
		)
	{
		this->mbNull = true;
	}

// operators
public:
	/**
	 * Convenience cast to the underlying type.
	 * Wrapper around GetValue.
	 */
	operator tValueType() const
	{
		return this->GetValue();
	}

	/**
	 * Convenience assignment from the underlying type.
	 * Wrapper around SetValue.
	 */
	vvNullable<tValueType>& operator=(
		const tValueType& that
		)
	{
		this->SetValue(that);
		return *this;
	}

	/**
	 * Overload of assignment operator for setting a null value.
	 * Wrapper around SetValue.
	 */
	vvNullable<tValueType>& operator=(
		vvNull that
		)
	{
		this->SetValue(that);
		return *this;
	}

	/**
	 * Equality operator.
	 * Two null values are equal.
	 * A null value never equals a non-null values.
	 * Two non-null values depend on the equivalent operator of tValueType.
	 */
	bool operator==(
		const vvNullable<tValueType>& that
		) const
	{
		if (this->mbNull && that.mbNull)
		{
			return true;
		}
		else if (this->mbNull != that.mbNull)
		{
			return false;
		}
		else
		{
			return this->mtValue == that.mtValue;
		}
	}

	/**
	 * Equality operator for tValueType.
	 */
	bool operator==(
		const tValueType& that
		) const
	{
		return (*this == vvNullable<tValueType>(that));
	}

	/**
	 * Inequality operator.
	 * The opposite of the equality operator.
	 */
	bool operator!=(
		const vvNullable<tValueType>& that
		) const
	{
		return !(*this == that);
	}

	/**
	 * Inequality operator for tValueType.
	 */
	bool operator!=(
		const tValueType& that
		) const
	{
		return (*this != vvNullable<tValueType>(that));
	}

	/**
	 * Less than operator.
	 * Null values are less than non-null values.
	 * A null value is not less than another null value.
	 * Two non-null values depend on the equivalent operator of tValueType.
	 */
	bool operator<(
		const vvNullable<tValueType>& that
		) const
	{
		if (this->mbNull && that.mbNull)
		{
			return false;
		}
		else if (this->mbNull != that.mbNull)
		{
			return this->mbNull;
		}
		else
		{
			return this->mtValue < that.mtValue;
		}
	}

	/**
	 * Less than operator for tValueType.
	 */
	bool operator<(
		const tValueType& that
		) const
	{
		return (*this < vvNullable<tValueType>(that));
	}

	/**
	 * Greater than operator.
	 * Non-null values are greater than null values.
	 * A null value is not greater than another null value.
	 * Two non-null values depend on the equivalent operator of tValueType.
	 */
	bool operator>(
		const vvNullable<tValueType>& that
		) const
	{
		if (this->mbNull && that.mbNull)
		{
			return false;
		}
		else if (this->mbNull != that.mbNull)
		{
			return that.mbNull;
		}
		else
		{
			return this->mtValue > that.mtValue;
		}
	}

	/**
	 * Greater than operator for tValueType.
	 */
	bool operator>(
		const tValueType& that
		) const
	{
		return (*this > vvNullable<tValueType>(that));
	}

	/**
	 * Less than or equal operator.
	 * Equivalent to calling operator< || operator==
	 */
	bool operator<=(
		const vvNullable<tValueType>& that
		) const
	{
		return (*this < that) || (*this == that);
	}

	/**
	 * Less than or equal operator for tValueType.
	 */
	bool operator<=(
		const tValueType& that
		) const
	{
		return (*this <= vvNullable<tValueType>(that));
	}

	/**
	 * Greater than or equal operator.
	 * Equivalent to calling operator> || operator==
	 */
	bool operator>=(
		const vvNullable<tValueType>& that
		) const
	{
		return (*this > that) || (*this == that);
	}

	/**
	 * Greater than or equal operator for tValueType.
	 */
	bool operator>=(
		const tValueType& that
		) const
	{
		return (*this >= vvNullable<tValueType>(that));
	}

// private data
private:
	bool       mbNull;  //< Whether or not the value is currently null.
	tValueType mtValue; //< The current value.  Undefined if bNull is true.
};


/**
 * A specialization of vvNullable with value type vvNull that doesn't compile.
 * It doesn't compile because it has no implementation anywhere.
 * This specialization exists to throw an error if someone ever tries to use it.
 * Not only should this never be useful, but it won't work correctly
 * because vvNull specifically exists to be different
 * than any value type used with vvNullable (see vvNull, above).
 */
template <>
class vvNullable<vvNull>;


#endif
