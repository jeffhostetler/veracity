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

#ifndef VV_SGLIB_STRING_POOL_H
#define VV_SGLIB_STRING_POOL_H

#include "precompiled.h"


/**
 * A class that manages a pool of strings in use by sglib.
 * When an instance is destroyed, all its strings are freed.
 */
class vvSglibStringPool
{
// functionality
public:
	/**
	 * Adds a wxString to the pool.
	 * A copy of the string is stored in the pool.
	 * The pooled copy is converted for use by sglib using vvSgHelpers.
	 * Returns a pointer to the converted sglib copy in the pool.
	 */
	const char* Add(
		const wxString& sValue //< [in] String to convert and add to the pool.
		);

	/**
	 * Removes all strings from the pool.
	 */
	void Clear();

// private types
private:
	typedef std::list<wxCharBuffer> stlCharBufferList;

// private data
private:
	stlCharBufferList mcStrings; //< Local memory for strings in the pool.
};


#endif
