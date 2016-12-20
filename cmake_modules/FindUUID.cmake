# - Find libuuid
# Find the native UUID headers and libraries.
#
#  UUID_INCLUDE_DIRS - where to find curl/curl.h, etc.
#  UUID_LIBRARIES    - List of libraries when using curl.
#  UUID_FOUND        - True if curl found.

# Look for the header file.
FIND_PATH(UUID_INCLUDE_DIR NAMES uuid/uuid.h)
MARK_AS_ADVANCED(UUID_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(UUID_LIBRARY NAMES uuid)
MARK_AS_ADVANCED(UUID_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(UUID DEFAULT_MSG UUID_LIBRARY UUID_INCLUDE_DIR)


IF(UUID_FOUND)
  SET(UUID_LIBRARIES ${UUID_LIBRARY})
  SET(UUID_INCLUDE_DIRS ${UUID_INCLUDE_DIR})
ELSE(UUID_FOUND)
  SET(UUID_LIBRARIES)
  SET(UUID_INCLUDE_DIRS)
ENDIF(UUID_FOUND)
