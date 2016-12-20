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

#ifndef VV_FLAGS_H
#define VV_FLAGS_H

#include "precompiled.h"


/**
 * A mix-in base class for classes that contain a set of flags.
 */
class vvHasFlags
{
// construction
public:
	/**
	 * Constructs a new set of flags.
	 */
	vvHasFlags(
		unsigned __int64 uFlags = 0u //< [in] Initial value for the flags.
		)
		: muFlags(uFlags)
	{
	}

// functionality
public:
	/**
	 * Gets the raw value of the flags.
	 */
	unsigned __int64 GetFlags() const
	{
		return this->muFlags;
	}

	/**
	 * A convenience wrapper around GetFlags that does a static_cast to
	 * the templated type.  Handy if you need the flags as some other type
	 * like a signed __int64.
	 */
	template <
		typename T //< The type to return the flags value as.
	>
	T GetFlagsAs() const
	{
		return static_cast<T>(this->GetFlags());
	}

	/**
	 * Specialization of GetFlagsAs that's intended for use with enum types
	 * whose values are powers of two.  It ensures that only a single flag is
	 * set (asserting otherwise) and then casts the flag back to the enum's type.
	 */
	template <
		typename T //< The enum type to return the single flag as.
	>
	T GetFlagAsEnum() const
	{
		wxASSERT(this->HasOneFlag());
		return this->GetFlagsAs<T>();
	}

	/**
	 * Gets the bit index of the only set flag.
	 * Asserts if multiple flags are set.
	 *
	 * This is useful for converting flags into corresponding array indices.
	 * Mathematically speaking, this gets you the log base 2 of the flag value.
	 *
	 * For example, a flag value of 32 (or 1 << 5) will return 5.  This maps
	 * the flag value from its power-of-2 space (how many powers of 2 is this
	 * number?) to a normal integer space (how far away from zero is this number?)
	 */
	unsigned __int64 GetFlagAsIndex() const
	{
		wxASSERT(this->HasOneFlag());

		unsigned __int64 uValue = this->GetFlags();
		unsigned __int64 uBit   = 0u;

		while (uValue >>= 1u)
		{
			uBit += 1u;
		}

		return uBit;
	}

	/**
	 * Checks if the flags are empty.
	 */
	bool FlagsEmpty() const
	{
		return this->muFlags == 0u;
	}

	/**
	 * Checks if the flags contain exactly one flag.
	 */
	bool HasOneFlag() const
	{
		// This amounts to checking if the flags are a power of 2.
		// http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
		return !this->FlagsEmpty() && ((this->muFlags & (this->muFlags - 1u)) == 0u);
	}

	/**
	 * Checks if the flags contain more than one flag.
	 */
	bool HasMultipleFlags() const
	{
		return !this->FlagsEmpty() && !this->HasOneFlag();
	}

	/**
	 * Checks if the flags contain any of the given flags.
	 */
	bool HasAnyFlag(
		unsigned __int64 uFlags //< [in] Flags to check for.
		) const
	{
		return ((this->muFlags & uFlags) != 0);
	}

	/**
	 * Checks if the flags contain all of the given flags.
	 */
	bool HasAllFlags(
		unsigned __int64 uFlags //< [in] Flags to check for.
		) const
	{
		return ((this->muFlags & uFlags) == uFlags);
	}

	/**
	 * Checks if the flags contain any flags that are NOT among the given flags.
	 * Equivalent to HasAnyFlag(~uFlags).
	 */
	bool HasAnyFlagExcept(
		unsigned __int64 uFlags //< [in] Flags to check for.
		) const
	{
		return this->HasAnyFlag(~uFlags);
	}

	/**
	 * Checks if the flags contain all flags that are NOT among the given flags.
	 * Equivalent to HasAllFlags(~uFlags);
	 */
	bool HasAllFlagsExcept(
		unsigned __int64 uFlags //< [in] Flags to check for.
		) const
	{
		return this->HasAllFlags(~uFlags);
	}

	/**
	 * Sets the flags to an entirely new value.
	 * Returns the new value.
	 */
	unsigned __int64 SetFlags(
		unsigned __int64 uFlags //< [in] New value for the flags.
		)
	{
		this->muFlags = uFlags;
		return this->muFlags;
	}

	/**
	 * Adds one or more flags.
	 * Returns the new value.
	 */
	unsigned __int64 AddFlags(
		unsigned __int64 uFlags //< [in] Flags to add.
		)
	{
		this->muFlags |= uFlags;
		return this->muFlags;
	}

	/**
	 * Removes one or more flags.
	 * Returns the new value.
	 */
	unsigned __int64 RemoveFlags(
		unsigned __int64 uFlags //< [in] Flags to remove.
		)
	{
		this->muFlags &= ~uFlags;
		return this->muFlags;
	}

	/**
	 * Simultaneously adds and removes flags.
	 * Returns the new value.
	 */
	unsigned __int64 ModifyFlags(
		unsigned __int64 uAddFlags,   //< [in] Flags to add.
		unsigned __int64 uRemoveFlags //< [in] Flags to remove.
		)
	{
		this->AddFlags(uAddFlags);
		this->RemoveFlags(uRemoveFlags);
		return this->muFlags;
	}

// private data
private:
	unsigned __int64 muFlags; //< Current flags value.
};


/**
 * A class that wraps an unsigned integer and provides
 * functionality for treating it like a set of bit flags.
 */
class vvFlags : public vvHasFlags
{
// constants
public:
	static const unsigned __int64 ALL     = ~0u;          //< Mask with all possible flags enabled.
	static const unsigned __int64 LOWEST  = 1u;           //< The lowest possible flag.
	static const unsigned __int64 HIGHEST = ~((~0u)>>1u); //< The highest possible flag.

// types
public:
	/**
	 * A class for iterating through each flag in a vvFlags instance, lowest to highest.
	 * Optionally only iterates flags that also match a given mask.
	 *
	 * This class cannot modify the underlying vvFlags instance, and is therefore a const_iterator.
	 * There is no non-const iterator implementation, because it doesn't really make sense
	 * to modify the individual bit in the original flags (especially not via pointer semantics),
	 * since we're only iterating enabled bits to start with.  This would amount to modifying a
	 * collection while iterating it.
	 */
	class const_iterator
	{
	// construction
	public:
		/**
		 * Constructor.
		 * The iterator is initialized to "one past the end" state.
		 * Use operator++ to advance it to point at the first flag.
		 */
		const_iterator(
			const vvFlags& cFlags,     //< [in] The flags instance to iterate through.
			unsigned __int64   uMask = ALL //< [in] The mask to match against when iterating.
			)
			: mpFlags(&cFlags)
			, muMask(uMask)
			, muValue(0u)
		{
		}

	// functionality
	public:
		/**
		 * Equality operator.
		 * Crashes if comparing iterators that point to different flag values.
		 */
		bool operator==(
			const const_iterator& that
			) const
		{
			wxASSERT(this->mpFlags == that.mpFlags);
			return (this->muValue == that.muValue && this->muMask == that.muMask);
		}

		/**
		 * Inequality operator.
		 * Opposite of equality operator.
		 */
		bool operator!=(
			const const_iterator& that
			) const
		{
			return !(*this == that);
		}

		/**
		 * Dereference operator.
		 * Crashes if the iterator has an invalid value.
		 */
		vvFlags operator*() const
		{
			wxASSERT(this->muValue != 0u);
			return this->muValue;
		}

		/**
		 * Member dereference operator.
		 * Crashes if the iterator has an invalid value.
		 *
		 * If the value returned by a member dereference operator isn't a pointer
		 * (as is the case here), then C++ will attempt to call *its* member dereference
		 * operator, and so on until it gets an actual pointer.  Therefore, this function
		 * relies on vvFlags having a member dereference operator that returns a pointer
		 * to itself.
		 */
		vvFlags operator->() const
		{
			wxASSERT(this->muValue != 0u);
			return this->muValue;
		}

		/**
		 * Pre-increment operator.
		 */
		const_iterator& operator++()
		{
			wxASSERT(this->muValue == 0u || vvFlags(this->muValue).HasOneFlag());
			for (unsigned __int64 uCurrent = this->muValue == 0u ? vvFlags::LOWEST : this->muValue << 1u; uCurrent != 0u; uCurrent <<= 1u)
			{
				if (this->mpFlags->HasAnyFlag(uCurrent) && (this->muMask & uCurrent) != 0u)
				{
					this->muValue = uCurrent;
					return *this;
				}
			}
			this->muValue = 0u;
			return *this;
		}

		/**
		 * Post-increment operator.
		 */
		const_iterator operator++(
			int
			)
		{
			const_iterator cResult = *this;
			++(*this);
			return cResult;
		}

		/**
		 * Pre-decrement operator.
		 */
		const_iterator& operator--()
		{
			// Note: It's very important that the index be unsigned!
			//       This ensures that the right-shift is logical.
			//       Right-shifts on signed numbers are compiler-dependent, but usually arithmetic.
			//       http://stackoverflow.com/questions/7622/shift-operator-in-c
			//       If we end up with an arithmetic shift, then this loop might go infinite.
			wxASSERT(this->muValue == 0u || vvFlags(this->muValue).HasOneFlag());
			for (unsigned __int64 uCurrent = this->muValue == 0u ? vvFlags::HIGHEST : this->muValue >> 1u; uCurrent != 0u; uCurrent >>= 1u)
			{
				if (this->mpFlags->HasAnyFlag(uCurrent) && (this->muMask & uCurrent) != 0u)
				{
					this->muValue = uCurrent;
					return *this;
				}
			}
			this->muValue = 0u;
			return *this;
		}

		/**
		 * Post-decrement operator.
		 */
		const_iterator operator--(
			int
			)
		{
			const_iterator cResult = *this;
			--(*this);
			return cResult;
		}

	// private data
	private:
		const vvFlags* mpFlags; //< The flags that are being iterated.
		unsigned __int64   muMask;  //< The mask of flags to iterate.
		unsigned __int64   muValue; //< The iterator's current value (zero indicates null/invalid iterator).
	};

	/**
	 * A handy convenience iterator to use when the flags being iterated happen
	 * to correspond to values of an enum.  Pass the enum type for T and then
	 * dereferencing the iterator will return a value of that type.
	 */
	template <
		typename T //< The enum type to return when dereferencing the iterator.
	>
	class enum_iterator : public const_iterator
	{
	public:
		/**
		 * Constructor that takes a base iterator.
		 * This is required so that enum_iterators can be constructed from the
		 * output of begin/end/etc.
		 */
		enum_iterator(
			const const_iterator& it
			)
			: const_iterator(it)
		{
		}

		/**
		 * Specialized dereference operator that returns the value as the
		 * templated enum type.
		 */
		T operator*() const
		{
			return const_iterator::operator*().GetValueAsEnum<T>();
		};
	};

// construction
public:
	/**
	 * Constructs a new set of flags.
	 */
	vvFlags(
		unsigned __int64 uValue = 0u //< [in] Initial value for the flags.
		)
		: vvHasFlags(uValue)
	{
	}

// operators
public:
	/**
	 * Assignment from an unsigned long.
	 */
	vvFlags& operator=(
		unsigned __int64 uValue
		)
	{
		this->SetValue(uValue);
		return *this;
	}

	/**
	 * Implicit conversion back to an unsigned long.
	 */
	operator unsigned __int64() const
	{
		return this->GetFlags();
	}

	/**
	 * This is only necessary for our const_iterator's arrow operator to function.
	 * See the comments there for more information.
	 */
	vvFlags* operator->()
	{
		return this;
	}

	/**
	 * Bitwise NOT operator.
	 */
	vvFlags operator~() const
	{
		return vvFlags(~(this->GetFlags()));
	}

	/**
	 * Bitwise AND operator.
	 */
	vvFlags operator&(const vvFlags& that) const
	{
		return vvFlags(this->GetFlags() & that.GetFlags());
	}

	/**
	 * Bitwise OR operator.
	 */
	vvFlags operator|(const vvFlags& that) const
	{
		return vvFlags(this->GetFlags() | that.GetFlags());
	}

	/**
	 * Bitwise XOR operator.
	 */
	vvFlags operator^(const vvFlags& that) const
	{
		return vvFlags(this->GetFlags() ^ that.GetFlags());
	}

// functionality
public:
	/**
	 * Convenience wrapper around base GetFlags().
	 */
	unsigned __int64 GetValue() const
	{
		return this->GetFlags();
	}

	/**
	 * Convenience wrapper around base GetFlagsAs().
	 */
	template <typename T>
	T GetValueAs() const
	{
		return this->GetFlagsAs<T>();
	}

	/**
	 * Convenience wrapper around base GetFlagAsEnum().
	 */
	template <typename T>
	T GetValueAsEnum() const
	{
		return this->GetFlagAsEnum<T>();
	}

	/**
	 * Convenience wrapper around base GetFlagAsIndex().
	 */
	unsigned __int64 GetValueAsIndex() const
	{
		return this->GetFlagAsIndex();
	}

	/**
	 * Convenience wrapper around base SetFlags().
	 */
	unsigned __int64 SetValue(
		unsigned __int64 uValue
		)
	{
		return this->SetFlags(uValue);
	}

	/**
	 * Gets an iterator to the first (lowest) flag in the flags.
	 */
	const_iterator begin() const
	{
		const_iterator it(*this);
		++it;
		return it;
	}

	/**
	 * Gets an iterator to the first (lowest) flag in the flags
	 * that only iterates flags in the given mask.
	 */
	const_iterator begin(
		vvFlags cMask //< [in] The returned iterator will only iterate through flags in this mask.
		) const
	{
		const_iterator it(*this, cMask);
		++it;
		return it;
	}

	/**
	 * Gets an iterator to one past the last (highest) flag in the flags.
	 */
	const_iterator end() const
	{
		return const_iterator(*this, ALL);
	}

	/**
	 * Gets an iterator to one past the last (highest) flag in the flags
	 * that only iterates flags in the given mask.
	 */
	const_iterator end(
		vvFlags cMask //< [in] The returned iterator will only iterate through flags in this mask.
		) const
	{
		return const_iterator(*this, cMask);
	}
};


/**
 * Bitwise AND operator for unsigned __int64.
 */
inline vvFlags operator&(unsigned __int64 uLeft, const vvFlags& cRight)
{
	return vvFlags(uLeft & cRight.GetFlags());
}

/**
 * Bitwise OR operator for unsigned __int64.
 */
inline vvFlags operator|(unsigned __int64 uLeft, const vvFlags& cRight)
{
	return vvFlags(uLeft | cRight.GetFlags());
}

/**
 * Bitwise XOR operator for unsigned __int64.
 */
inline vvFlags operator^(unsigned __int64 uLeft, const vvFlags& cRight)
{
	return vvFlags(uLeft ^ cRight.GetFlags());
}


#endif
