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

/*
 * sg_version.h
 *
 *  Created on: Mar 30, 2010
 *      Author: jeremy
 *
 * TODO 4/2/10 Shouldn't these have SG_ prefixes for namespace collisions?
 */

#ifndef SG_VERSION_H_
#define SG_VERSION_H_

#ifndef MAJORVERSION
#define MAJORVERSION "2"
#endif
#ifndef MINORVERSION
#define MINORVERSION "5"
#endif
#ifndef REVVERSION
#define REVVERSION "0"
#endif
#ifndef BUILDNUMBER
#define BUILDNUMBER "0"
#endif

#ifndef SG_PLATFORM
#  if defined(LINUX)
#    define SG_PLATFORM	"LINUX"
#  elif defined(SG_IOS)
#    define SG_PLATFORM	"IOS"
#  elif defined(MAC)
#    define SG_PLATFORM	"MAC"
#  elif defined(WINDOWS)
#    define SG_PLATFORM	"WINDOWS"
#  else
#    error Add ifdef for SG_PLATFORM in sg_version.h
#  endif
#endif

#ifdef DEBUG
#define DEBUGSTRING " (Debug)"
#else
#define DEBUGSTRING ""
#endif
#endif /* SG_VERSION_H_ */
