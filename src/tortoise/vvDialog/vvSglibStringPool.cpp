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
#include "vvSglibStringPool.h"

#include "vvSgHelpers.h"


/*
**
** Public Functions
**
*/

const char* vvSglibStringPool::Add(
	const wxString& sValue
	)
{
	// Convert the string to the format required by sglib.  The returned
	// wxScopedCharBuffer may or may not actually own the pointer it contains.
	// We need to make sure we have our own copy of the string, so make a
	// separate wxCharBuffer that will always own its pointer, making a copy
	// if necessary.  After this, we'll have a converted character buffer that
	// we know we own.
	wxCharBuffer cBuffer(vvSgHelpers::Convert_wxString_sglibString(sValue));

	// Add the owned buffer to our list of owned/converted strings, ensuring that
	// we free it when we're destroyed.  Since wxCharBuffer is ref-counted, copying
	// it into the container is cheap, and since the container will have the only
	// reference after this function returns, it will be freed automatically for us
	// when the container is destroyed by the default destructor.
	this->mcStrings.push_back(cBuffer);

	// The copy in the container should have the same data pointer as our local
	// cBuffer instance, because it's ref-counted.  Return that pointer.
	wxASSERT(cBuffer.data() == this->mcStrings.back().data());
	return cBuffer.data();
}

void vvSglibStringPool::Clear()
{
	this->mcStrings.clear();
}
