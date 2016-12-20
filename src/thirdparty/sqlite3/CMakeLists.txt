# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Copyright 2010-2013 SourceGear, LLC
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSQLITE_ENABLE_FTS3 -DSQLITE_ENABLE_FTS3_PARENTHESIS")

if(WIN32)
   STRING(REGEX REPLACE "/W4" "/W3" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif() #WIN32

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")

   ## GCC 4.2.1 on MAC in release mode gives the following warnings/errors:
   ##
   ## cc1: warnings being treated as errors
   ## sqlite3.c: In function 'flockUnlock':
   ## sqlite3.c:24321: warning: ISO C90 forbids mixed declarations and code
   ## sqlite3.c: In function 'afpCheckReservedLock':
   ## sqlite3.c:24626: warning: ISO C90 forbids mixed declarations and code
   ## sqlite3.c: In function 'unixOpen':
   ## sqlite3.c:27016: warning: ISO C90 forbids mixed declarations and code
   ## sqlite3.c: In function 'proxyTakeConch':
   ## sqlite3.c:28124: warning: ISO C90 forbids mixed declarations and code

   STRING(REGEX REPLACE "-Wdeclaration-after-statement" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

   ## GCC 4.2.1 on MAC in release mode gives the following warnings/errors:
   ##
   ## cc1: warnings being treated as errors
   ## sqlite3.c: In function 'proxyConchLock':
   ## sqlite3.c:27867: warning: 'conchModTime.tv_sec' may be used uninitialized in this function
   ## sqlite3.c:27867: warning: 'conchModTime.tv_nsec' may be used uninitialized in this function

   STRING(REGEX REPLACE "-Wall" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
   STRING(REGEX REPLACE "-Wextra" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
   ## STRING(REGEX REPLACE "-Werror" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

   STRING(REGEX REPLACE "-Wconversion" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

endif()

if(CMAKE_COMPILER_IS_GNUCC)
   #todo: why doesn't this check work
   #INCLUDE(CheckCCompilerFlag)
   #CHECK_C_COMPILER_FLAG("-Wno-strict-overflow" HAVE_WNO_STRICT_OVERFLOW)
   #if(HAVE_WNO_STRICT_OVERFLOW)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-strict-overflow")
   #endif(HAVE_WNO_STRICT_OVERFLOW)
   STRING(REGEX REPLACE "-Wall" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
   STRING(REGEX REPLACE "-Wconversion" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
   STRING(REGEX REPLACE "-Wextra" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
   STRING(REGEX REPLACE "-Wunused" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
   STRING(REGEX REPLACE "-Werror" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif(CMAKE_COMPILER_IS_GNUCC)

add_library(sqlite3 STATIC sqlite3.c sqlite3.h sqlite3ext.h)
set_target_properties(sqlite3 PROPERTIES FOLDER "Libraries")

