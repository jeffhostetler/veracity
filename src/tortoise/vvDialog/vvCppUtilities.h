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

#ifndef VV_CPP_UTILITIES_H
#define VV_CPP_UTILITIES_H


/*
**
** Macros
**
*/

/**
 * Gets the number of elements in an array.
 * WARNING: Won't work with pointers that point to arrays.. only actual arrays.
 */
#define vvARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]))

/**
 * Used within a class declaration to disable copying of instances of the class.
 * Just declares the copy constructor and assignment operator as private.
 */
#define vvDISABLE_COPY(cppClass)              \
	private:                                  \
		cppClass(const cppClass&);            \
		cppClass& operator=(const cppClass&); \




#endif
