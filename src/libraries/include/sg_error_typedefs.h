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

/**
 * @file sg_error_typedefs.h
 *
 * @details Declarations of error codes.
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_ERROR_TYPEDEFS_H
#define H_SG_ERROR_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////
// SG_error -- an error code -- these are an extension of the platform
// error codes (for example, errno).
//
// when an error comes from the system, it will have a system-defined
// value.  when it comes from the core SG library it will have a value in
// our range.  when it comes from a thirdparty library, it may have a value
// unique to that library (see SQLite).
//
// Linux and Mac define errno values between 1 and ~200.
// Windows defines 3 (overlapping) ranges:
// [1] errno for posix-compatility routines in CRT (such as _open());
//     (these range from 1 and ~200 like Linux/Mac)
// [2] GetLastError() from windows-native routines (such as CreateFileW())
//     (these range from 1 and ~15000 (from what i've been able to find)
//     (bit 29 is reserved for us if we want to use it).
// [3] an HRESULT from a COM operation.
//     (these are full 32bit values with high bits defined on subsystem
//      and facility codes...)
//
// so we define our error type to be a 64bit value and put a domain-type
// bit in the left half.  this lets the error message formatter know
// which set of messages to use.
//

typedef SG_uint64						SG_error;

#define __SG_ERR__ERRNO__				((SG_error)0x0000000000000000ULL)	// errno domain
#define __SG_ERR__SG_LIBRARY__			((SG_error)0x0000000100000000ULL)	// SG library domain
#if defined(WINDOWS)
#define __SG_ERR__GETLASTERROR__		((SG_error)0x0000000200000000ULL)	// GetLastError() domain
#define __SG_ERR__COM__					((SG_error)0x0000000400000000ULL)	// HRESULT from COM domain
#endif
#define __SG_ERR__SQLITE__				((SG_error)0x0000000800000000ULL)	// sqlite domain
#define __SG_ERR__ICU__					((SG_error)0x0000002000000000ULL)	// ICU domain
#define __SG_ERR__ZLIB__				((SG_error)0x0000004000000000ULL)	// zlib domain
#define __SG_ERR__LIBCURL__				((SG_error)0x0000010000000000ULL)	// libcurl domain
#define __SG_ERR__OPENSSL__				((SG_error)0x0000020000000000ULL)	// openssl domain

#define __SG_ERR__TYPE__MASK__			((SG_error)0x0000ffff00000000ULL)	// domain mask
#define __SG_ERR__VALUE__MASK__			((SG_error)0x00000000ffffffffULL)	// value within error domain

#define SG_ERR_TYPE(err)				((err) & __SG_ERR__TYPE__MASK__)

#define SG_ERR_GENERIC_VALUE(err)		((err) & __SG_ERR__VALUE__MASK__)	// error value without domain indicator

#define SG_ERR_ERRNO_VALUE(err)			((int)      SG_ERR_GENERIC_VALUE(err))	// error value properly casted
#define SG_ERR_SG_LIBRARY_VALUE(err)	((SG_uint32)SG_ERR_GENERIC_VALUE(err))
#if defined(WINDOWS)
#define SG_ERR_GETLASTERROR_VALUE(err)	((DWORD)    SG_ERR_GENERIC_VALUE(err))
#define SG_ERR_COM_VALUE(err)			((HRESULT)  SG_ERR_GENERIC_VALUE(err))
#endif
#define SG_ERR_SQLITE_VALUE(err)		((int)      SG_ERR_GENERIC_VALUE(err))
#define SG_ERR_ICU_VALUE(err)			((int)		((SG_uint32)SG_ERR_GENERIC_VALUE(err)))
#define SG_ERR_ZLIB_VALUE(err)			((int)      SG_ERR_GENERIC_VALUE(err))
#define SG_ERR_LIBCURL_VALUE(err)		((int)		SG_ERR_GENERIC_VALUE(err))
#define SG_ERR_OPENSSL_VALUE(err)		((int)		SG_ERR_GENERIC_VALUE(err))

// use one of these when setting a SG_error variable with an error code
// received from the system, our core library, or from a thirdparty library.
//
// for example:
//   fd = open(...)
//   if (fd == -1)
//      err = SG_ERR_ERRNO(errno);
//
//   hFile = CreateFileW(...)
//   if (hFile == INVALID_FILE_HANDLE)
//      err = SG_ERR_GETLASTERROR(GetLastError())

#define SG_ERR_ERRNO(e)					(__SG_ERR__ERRNO__        | ((SG_error)e))
#define SG_ERR_SG_LIBRARY(e)			(__SG_ERR__SG_LIBRARY__   | ((SG_error)e))
#if defined(WINDOWS)
#define SG_ERR_GETLASTERROR(e)			(__SG_ERR__GETLASTERROR__ | ((SG_error)e))
#define SG_ERR_COM()					(__SG_ERR__COM__          | (0 /*TODO*/))
#endif
#define SG_ERR_SQLITE(e)			    (__SG_ERR__SQLITE__       | ((SG_error)e))
#define SG_ERR_ZLIB(e)			        (__SG_ERR__ZLIB__         | ((SG_error)((SG_uint32)e)))
#define SG_ERR_ICU(e)					(__SG_ERR__ICU__          | ((SG_error)((SG_uint32)e)))		// WARNING: warnings<0; errors>0; ok=0
#define SG_ERR_LIBCURL(e)				(__SG_ERR__LIBCURL__	  | ((SG_error)((SG_uint32)e)))
#define SG_ERR_OPENSSL(e)				(__SG_ERR__OPENSSL__	  | ((SG_error)((SG_uint32)e)))


//////////////////////////////////////////////////////////////////

#define SG_ERR_OK                                           SG_ERR_SG_LIBRARY(0)
#define SG_ERR_UNSPECIFIED                                  SG_ERR_SG_LIBRARY(1)
#define SG_ERR_INVALIDARG                                   SG_ERR_SG_LIBRARY(2)
#define SG_ERR_NOTIMPLEMENTED                               SG_ERR_SG_LIBRARY(3)
#define SG_ERR_UNINITIALIZED                                SG_ERR_SG_LIBRARY(4)
#define SG_ERR_MALLOCFAILED                                 SG_ERR_SG_LIBRARY(5)
#define SG_ERR_INCOMPLETEWRITE                              SG_ERR_SG_LIBRARY(6)
#define SG_ERR_HTTP_400_BAD_REQUEST                         SG_ERR_SG_LIBRARY(7)
#define SG_ERR_HTTP_413_REQUEST_ENTITY_TOO_LARGE            SG_ERR_SG_LIBRARY(8)
#define SG_ERR_REPO_ALREADY_OPEN                            SG_ERR_SG_LIBRARY(9)
#define SG_ERR_NOTAREPOSITORY                               SG_ERR_SG_LIBRARY(10)
#define SG_ERR_NOTAFILE                                     SG_ERR_SG_LIBRARY(11)
#define SG_ERR_COLLECTIONFULL                               SG_ERR_SG_LIBRARY(12)
#define SG_ERR_USERNOTFOUND                                 SG_ERR_SG_LIBRARY(13)	// TODO see SG_ERR_USER_NOT_FOUND and consolidate
#define SG_ERR_INVALIDHOMEDIRECTORY                         SG_ERR_SG_LIBRARY(14)
#define SG_ERR_CANNOTTRIMROOTDIRECTORY                      SG_ERR_SG_LIBRARY(15)
#define SG_ERR_LIMIT_EXCEEDED                               SG_ERR_SG_LIBRARY(16)
#define SG_ERR_OVERLAPPINGBUFFERS                           SG_ERR_SG_LIBRARY(17)
#define SG_ERR_REPO_NOT_OPEN                                SG_ERR_SG_LIBRARY(18)
#define SG_ERR_NOMOREFILES                                  SG_ERR_SG_LIBRARY(19)
#define SG_ERR_BUFFERTOOSMALL                               SG_ERR_SG_LIBRARY(20)
#define SG_ERR_INVALIDBLOBOPERATION                         SG_ERR_SG_LIBRARY(21)
#define SG_ERR_BLOBVERIFYFAILED                             SG_ERR_SG_LIBRARY(22)
#define SG_ERR_DBRECORDFIELDNAMEDUPLICATE                   SG_ERR_SG_LIBRARY(23)
#define SG_ERR_DBRECORDFIELDNAMEINVALID                     SG_ERR_SG_LIBRARY(24)
#define SG_ERR_INDEXOUTOFRANGE                              SG_ERR_SG_LIBRARY(25)
#define SG_ERR_DBRECORDFIELDNAMENOTFOUND                    SG_ERR_SG_LIBRARY(26)
#define SG_ERR_UTF8INVALID                                  SG_ERR_SG_LIBRARY(27)
#define SG_ERR_XMLWRITERNOTWELLFORMED                       SG_ERR_SG_LIBRARY(28)
#define SG_ERR_EOF                                          SG_ERR_SG_LIBRARY(29)
#define SG_ERR_INCOMPLETEREAD                               SG_ERR_SG_LIBRARY(30)
#define SG_ERR_VCDIFF_NUMBER_ENCODING                       SG_ERR_SG_LIBRARY(31)
#define SG_ERR_VCDIFF_UNSUPPORTED                           SG_ERR_SG_LIBRARY(32)
#define SG_ERR_VCDIFF_INVALID_FORMAT                        SG_ERR_SG_LIBRARY(33)
#define SG_ERR_DAGLCA_LEAF_IS_ANCESTOR                      SG_ERR_SG_LIBRARY(34)
#define SG_ERR_CANNOT_DO_FF_MERGE                           SG_ERR_SG_LIBRARY(35)
#define SG_ERR_ASSERT                                       SG_ERR_SG_LIBRARY(36)
#define SG_ERR_CHANGESETDIRALREADYEXISTS                    SG_ERR_SG_LIBRARY(37)
#define SG_ERR_INVALID_BLOB_DIRECTORY                       SG_ERR_SG_LIBRARY(38)
#define SG_ERR_COMPRESSION_NOT_HELPFUL                      SG_ERR_SG_LIBRARY(39)
#define SG_ERR_INVALID_BLOB_HEADER                          SG_ERR_SG_LIBRARY(40)
#define SG_ERR_UNSUPPORTED_BLOB_VERSION                     SG_ERR_SG_LIBRARY(41)
#define SG_ERR_BLOB_NOT_VERIFIED_INCOMPLETE                 SG_ERR_SG_LIBRARY(42)
#define SG_ERR_BLOB_NOT_VERIFIED_MISMATCH                   SG_ERR_SG_LIBRARY(43)
#define SG_ERR_JSONWRITERNOTWELLFORMED                      SG_ERR_SG_LIBRARY(44)
#define SG_ERR_JSONPARSER_SYNTAX                            SG_ERR_SG_LIBRARY(45)
#define SG_ERR_VHASH_DUPLICATEKEY                           SG_ERR_SG_LIBRARY(46)
#define SG_ERR_VHASH_KEYNOTFOUND                            SG_ERR_SG_LIBRARY(47)
#define SG_ERR_VARIANT_INVALIDTYPE                          SG_ERR_SG_LIBRARY(48)
#define SG_ERR_NOT_A_DIRECTORY                              SG_ERR_SG_LIBRARY(49)
#define SG_ERR_VARRAY_INDEX_OUT_OF_RANGE                    SG_ERR_SG_LIBRARY(50)
#define SG_ERR_TREENODE_ENTRY_VALIDATION_FAILED             SG_ERR_SG_LIBRARY(51)
#define SG_ERR_BLOB_NOT_FOUND                               SG_ERR_SG_LIBRARY(52)
#define SG_ERR_INVALID_WHILE_FROZEN                         SG_ERR_SG_LIBRARY(53)
#define SG_ERR_INVALID_UNLESS_FROZEN                        SG_ERR_SG_LIBRARY(54)
#define SG_ERR_ARGUMENT_OUT_OF_RANGE                        SG_ERR_SG_LIBRARY(55)
#define SG_ERR_CHANGESET_VALIDATION_FAILED                  SG_ERR_SG_LIBRARY(56)
#define SG_ERR_TREENODE_VALIDATION_FAILED                   SG_ERR_SG_LIBRARY(57)
#define SG_ERR_TRIENODE_VALIDATION_FAILED                   SG_ERR_SG_LIBRARY(58)
#define SG_ERR_INVALID_DBCRITERIA                           SG_ERR_SG_LIBRARY(59)
#define SG_ERR_DBRECORD_VALIDATION_FAILED                   SG_ERR_SG_LIBRARY(60)
#define SG_ERR_DAGNODE_ALREADY_EXISTS                       SG_ERR_SG_LIBRARY(61)
#define SG_ERR_NOT_FOUND                                    SG_ERR_SG_LIBRARY(62)
#define SG_ERR_CANNOT_NEST_DB_TX                            SG_ERR_SG_LIBRARY(63) // TODO: Remove when DB cache is reworked?
#define SG_ERR_NOT_IN_DB_TX                                 SG_ERR_SG_LIBRARY(64) // TODO: Remove when DB cache is reworked?
#define SG_ERR_VTABLE_NOT_BOUND                             SG_ERR_SG_LIBRARY(65)
#define SG_ERR_REPO_DB_NOT_OPEN                             SG_ERR_SG_LIBRARY(66)
#define SG_ERR_DAG_NOT_CONSISTENT                           SG_ERR_SG_LIBRARY(67)
#define SG_ERR_DB_BUSY                                      SG_ERR_SG_LIBRARY(68)
#define SG_ERR_CANNOT_CREATE_SPARSE_DAG                     SG_ERR_SG_LIBRARY(69)
#define SG_ERR_RBTREE_DUPLICATEKEY                          SG_ERR_SG_LIBRARY(70)
#define SG_ERR_RESTART_FOREACH                              SG_ERR_SG_LIBRARY(71)
#define SG_ERR_SPLIT_MOVE                                   SG_ERR_SG_LIBRARY(72)

#define SG_ERR_WC__ITEM_ALREADY_EXISTS						SG_ERR_SG_LIBRARY(73)
#define SG_ERR_PENDINGTREE_OBJECT_ALREADY_EXISTS            SG_ERR_WC__ITEM_ALREADY_EXISTS	// TODO deprecate this symbol

#define SG_ERR_FILE_LOCK_FAILED                             SG_ERR_SG_LIBRARY(74)
#define SG_ERR_SYMLINK_NOT_SUPPORTED                        SG_ERR_SG_LIBRARY(75)
#define SG_ERR_USAGE                                        SG_ERR_SG_LIBRARY(76)
#define SG_ERR_UNMAPPABLE_UNICODE_CHAR                      SG_ERR_SG_LIBRARY(77)
#define SG_ERR_ILLEGAL_CHARSET_CHAR                         SG_ERR_SG_LIBRARY(78)
#define SG_ERR_INVALID_PENDINGTREE_OBJECT_TYPE              SG_ERR_SG_LIBRARY(79)
#define SG_ERR_PORTABILITY_WARNINGS                         SG_ERR_SG_LIBRARY(80)	// TODO deprecate this -- see SG_ERR_WC_PORT_FLAGS
#define SG_ERR_CANNOT_REMOVE_SAFELY                         SG_ERR_SG_LIBRARY(81)
#define SG_ERR_TOO_MANY_BACKUP_FILES                        SG_ERR_SG_LIBRARY(82)
#define SG_ERR_TAG_ALREADY_EXISTS                           SG_ERR_SG_LIBRARY(83)
#define SG_ERR_TAG_CONFLICT                                 SG_ERR_SG_LIBRARY(84)
#define SG_ERR_AMBIGUOUS_ID_PREFIX                          SG_ERR_SG_LIBRARY(85)
#define SG_ERR_UNKNOWN_STORAGE_IMPLEMENTATION               SG_ERR_SG_LIBRARY(86)
#define SG_ERR_DBNDX_ALLOWS_QUERY_ONLY                      SG_ERR_SG_LIBRARY(87)
#define SG_ERR_ZIP_BAD_FILE                                 SG_ERR_SG_LIBRARY(88)
#define SG_ERR_ZIP_CRC                                      SG_ERR_SG_LIBRARY(89)
#define SG_ERR_DAGFRAG_DESERIALIZATION_VERSION              SG_ERR_SG_LIBRARY(90)
#define SG_ERR_DAGNODE_DESERIALIZATION_VERSION              SG_ERR_SG_LIBRARY(91)
#define SG_ERR_FRAGBALL_INVALID_VERSION                     SG_ERR_SG_LIBRARY(92)
#define SG_ERR_FRAGBALL_BLOB_HASH_MISMATCH                  SG_ERR_SG_LIBRARY(93)
#define SG_ERR_MALFORMED_SUPERROOT_TREENODE                 SG_ERR_SG_LIBRARY(94)
#define SG_ERR_ABNORMAL_TERMINATION                         SG_ERR_SG_LIBRARY(95)
#define SG_ERR_EXTERNAL_TOOL_ERROR                          SG_ERR_SG_LIBRARY(96)
#define SG_ERR_FRAGBALL_INVALID_OP                          SG_ERR_SG_LIBRARY(97)
#define SG_ERR_INCOMPLETE_BLOB_IN_REPO_TX                   SG_ERR_SG_LIBRARY(98)
#define SG_ERR_REPO_MISMATCH                                SG_ERR_SG_LIBRARY(99)
#define SG_ERR_UNKNOWN_CLIENT_PROTOCOL                      SG_ERR_SG_LIBRARY(100)
#define SG_ERR_SAFEPTR_WRONG_TYPE                           SG_ERR_SG_LIBRARY(101)
#define SG_ERR_SAFEPTR_NULL                                 SG_ERR_SG_LIBRARY(102)
#define SG_ERR_JS                                           SG_ERR_SG_LIBRARY(103)
#define SG_ERR_INTEGER_OVERFLOW                             SG_ERR_SG_LIBRARY(104)
#define SG_ERR_JS_DBRECORD_FIELD_VALUES_MUST_BE_STRINGS     SG_ERR_SG_LIBRARY(105)
#define SG_ERR_ALREADY_INITIALIZED                          SG_ERR_SG_LIBRARY(106)
#define SG_ERR_INVALID_REV_SPEC                             SG_ERR_SG_LIBRARY(107)
#define SG_ERR_SOURCE_AND_DEST_REPO_SAME                    SG_ERR_SG_LIBRARY(108)
#define SG_ERR_NOT_ALLOWED                                  SG_ERR_SG_LIBRARY(109)
#define SG_ERR_CTX_HAS_ERR                                  SG_ERR_SG_LIBRARY(110)
#define SG_ERR_CTX_NEEDS_ERR                                SG_ERR_SG_LIBRARY(111)
#define SG_ERR_LOGGING_MODULE_IN_USE                        SG_ERR_SG_LIBRARY(112)
#define SG_ERR_EXEC_FAILED                                  SG_ERR_SG_LIBRARY(113)
// gap 114
#define SG_ERR_MERGE_REQUESTED_CLEAN_WD                     SG_ERR_SG_LIBRARY(115)
#define SG_ERR_CANNOT_MOVE_INTO_CURRENT_PARENT              SG_ERR_SG_LIBRARY(116)
// gap 117
#define SG_ERR_VALUE_CANNOT_BE_SERIALIZED_TO_JSON           SG_ERR_SG_LIBRARY(118)
#define SG_ERR_BRANCH_HEAD_CHANGESET_NOT_PRESENT			SG_ERR_SG_LIBRARY(119)
#define SG_ERR_CANNOT_MOVE_INTO_SUBFOLDER		            SG_ERR_SG_LIBRARY(120)
#define SG_ERR_DIR_ALREADY_EXISTS                           SG_ERR_SG_LIBRARY(121)
#define SG_ERR_DIR_PATH_NOT_FOUND							SG_ERR_SG_LIBRARY(122)
#define SG_ERR_PENDINGTREE_PARTIAL_CHANGE_COLLISION         SG_ERR_SG_LIBRARY(123)		// TODO 2011/12/06 give this a better name and description
#define SG_ERR_REPO_BUSY                                    SG_ERR_SG_LIBRARY(124)
#define SG_ERR_PENDINGTREE_PARENT_DIR_MISSING_OR_INACTIVE   SG_ERR_SG_LIBRARY(125)
#define SG_ERR_UNKNOWN_BLOB_TYPE							SG_ERR_SG_LIBRARY(126)
#define SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL				SG_ERR_SG_LIBRARY(127)
#define SG_ERR_DIFFMERGE_EXE_NOT_FOUND						SG_ERR_SG_LIBRARY(128)
#define SG_ERR_GETOPT_NO_MORE_OPTIONS						SG_ERR_SG_LIBRARY(129)
#define SG_ERR_GETOPT_BAD_ARG								SG_ERR_SG_LIBRARY(130)
#define SG_ERR_GID_FORMAT_INVALID							SG_ERR_SG_LIBRARY(131)
#define SG_ERR_UNKNOWN_HASH_METHOD                          SG_ERR_SG_LIBRARY(132)
#define SG_ERR_TRIVIAL_HASH_DIFFERENT                       SG_ERR_SG_LIBRARY(133)
#define SG_ERR_UNSUPPORTED_DRAWER_VERSION					SG_ERR_SG_LIBRARY(134)
#define SG_ERR_SERVER_DOESNT_ACCEPT_PUSHES                  SG_ERR_SG_LIBRARY(135)
#define SG_ERR_DATE_PARSING_ERROR                           SG_ERR_SG_LIBRARY(136)
#define SG_ERR_INT_PARSE                                    SG_ERR_SG_LIBRARY(137)
#define SG_ERR_TAG_NOT_FOUND                                SG_ERR_SG_LIBRARY(138)
#define SG_ERR_STAGING_AREA_MISSING                         SG_ERR_SG_LIBRARY(139)
#define SG_ERR_NOT_A_WORKING_COPY                           SG_ERR_SG_LIBRARY(140)
#define SG_ERR_REPO_ID_MISMATCH                             SG_ERR_SG_LIBRARY(141)
#define SG_ERR_REPO_HASH_METHOD_MISMATCH                    SG_ERR_SG_LIBRARY(142)
#define SG_ERR_SERVER_HTTP_ERROR                            SG_ERR_SG_LIBRARY(143)
#define SG_ERR_JSONDB_NO_CURRENT_OBJECT						SG_ERR_SG_LIBRARY(144)
#define SG_ERR_JSONDB_INVALID_PATH							SG_ERR_SG_LIBRARY(145)
#define SG_ERR_JSONDB_OBJECT_ALREADY_EXISTS					SG_ERR_SG_LIBRARY(146)
#define SG_ERR_JSONDB_PARENT_DOESNT_EXIST					SG_ERR_SG_LIBRARY(147)
#define SG_ERR_JSONDB_NON_CONTAINER_ANCESTOR_EXISTS			SG_ERR_SG_LIBRARY(148)
#define SG_ERR_CHANGESET_BLOB_NOT_FOUND                     SG_ERR_SG_LIBRARY(149)
#define SG_ERR_CANNOT_MERGE_SYMLINK_VALUES					SG_ERR_SG_LIBRARY(150)
#define SG_ERR_PULL_INVALID_FRAGBALL_REQUEST				SG_ERR_SG_LIBRARY(151)
#define SG_ERR_NO_SUCH_DAG									SG_ERR_SG_LIBRARY(152)
#define SG_ERR_BASELINE_NOT_HEAD							SG_ERR_SG_LIBRARY(153)
#define SG_ERR_INVALID_REPO_NAME							SG_ERR_SG_LIBRARY(154)
#define SG_ERR_DUPLICATE_ISSUE								SG_ERR_SG_LIBRARY(155)
#define SG_ERR_NO_MERGE_TOOL_CONFIGURED						SG_ERR_SG_LIBRARY(156)
#define SG_ERR_UNKNOWN_DAG_STORAGE_VERSION					SG_ERR_SG_LIBRARY(157)
#define SG_ERR_TOO_MANY_HEADS								SG_ERR_SG_LIBRARY(158)
#define SG_ERR_DIR_NOT_EMPTY								SG_ERR_SG_LIBRARY(159)
#define SG_ERR_STAMP_NOT_FOUND								SG_ERR_SG_LIBRARY(160)
#define SG_ERR_VC_LOCK_FILES_ONLY							SG_ERR_SG_LIBRARY(161)
#define SG_ERR_VC_LOCK_BRANCH_MUST_BE_CURRENT				SG_ERR_SG_LIBRARY(162)
#define SG_ERR_VC_LOCK_ALREADY_LOCKED	        			SG_ERR_SG_LIBRARY(163)
#define SG_ERR_VC_LOCK_VIOLATION    	        			SG_ERR_SG_LIBRARY(164)
#define SG_ERR_VC_LOCK_DUPLICATE    	        			SG_ERR_SG_LIBRARY(165)
#define SG_ERR_VC_LOCK_NOT_FOUND    	        			SG_ERR_SG_LIBRARY(166)
#define SG_ERR_VC_LOCK_NOT_COMMITTED_YET          			SG_ERR_SG_LIBRARY(167)
#define SG_ERR_SUBMODULE_NESTED_REVERT_PROBLEM				SG_ERR_SG_LIBRARY(168)
#define SG_ERR_CANNOT_COMMIT_LOST_ENTRY              		SG_ERR_SG_LIBRARY(169)
#define SG_ERR_MALFORMED_GIT_FAST_IMPORT              		SG_ERR_SG_LIBRARY(170)
#define SG_ERR_PULL_FAILED                                  SG_ERR_SG_LIBRARY(171)
#define SG_ERR_EXTENDED_HTTP_500							SG_ERR_SG_LIBRARY(172)
#define SG_ERR_VC_LOCK_CANNOT_DELETE_ENDED         			SG_ERR_SG_LIBRARY(173)
#define SG_ERR_AUTHORIZATION_REQUIRED                       SG_ERR_SG_LIBRARY(174)
#define SG_ERR_AUTHORIZATION_TOO_MANY_ATTEMPTS              SG_ERR_SG_LIBRARY(175)

#define SG_ERR_ENTRY_ALREADY_UNDER_VERSION_CONTROL          SG_ERR_SG_LIBRARY(182)
#define SG_ERR_SUBMODULE_NESTED_UPDATE_CHECKOUT_PROBLEM		SG_ERR_SG_LIBRARY(183)
#define SG_ERR_SUBMODULE_UPDATE_CONFLICT					SG_ERR_SG_LIBRARY(184)
#define SG_ERR_CANNOT_PARTIAL_COMMIT_WITH_DIRTY_SUBMODULE	SG_ERR_SG_LIBRARY(185)
#define SG_ERR_PATH_IN_ANOTHER_SUBMODULE					SG_ERR_SG_LIBRARY(186)
#define SG_ERR_SUBMODULE_TECHNOLOGY_MISMATCH				SG_ERR_SG_LIBRARY(187)
#define SG_ERR_CANNOT_PARTIAL_REVERT_AFTER_MERGE            SG_ERR_SG_LIBRARY(188)
#define SG_ERR_CANNOT_PARTIAL_COMMIT_AFTER_MERGE            SG_ERR_SG_LIBRARY(189)
#define SG_ERR_DAGNODES_UNRELATED							SG_ERR_SG_LIBRARY(190)
#define SG_ERR_MULTIPLE_HEADS_FROM_DAGNODE					SG_ERR_SG_LIBRARY(191)
#define SG_ERR_UPDATE_CONFLICT								SG_ERR_SG_LIBRARY(192)
#define SG_ERR_ENTRYNAME_CONFLICT							SG_ERR_SG_LIBRARY(193)
#define SG_ERR_CANNOT_MAKE_RELATIVE_PATH					SG_ERR_SG_LIBRARY(194)
#define SG_ERR_CANNOT_DO_WHILE_UNCOMMITTED_MERGE			SG_ERR_SG_LIBRARY(195)
#define SG_ERR_WORKING_DIRECTORY_IN_USE						SG_ERR_SG_LIBRARY(196)
#define SG_ERR_INVALID_REPO_PATH							SG_ERR_SG_LIBRARY(197)
// gap 198
#define SG_ERR_NOTHING_TO_COMMIT							SG_ERR_SG_LIBRARY(199)
#define SG_ERR_REPO_FEATURE_NOT_SUPPORTED					SG_ERR_SG_LIBRARY(200)
#define SG_ERR_ISSUE_NOT_FOUND                              SG_ERR_SG_LIBRARY(201)
#define SG_ERR_CANNOT_COMMIT_WITH_UNRESOLVED_ISSUES         SG_ERR_SG_LIBRARY(202)
#define SG_ERR_USER_NOT_FOUND                               SG_ERR_SG_LIBRARY(203)
#define SG_ERR_ZING_RECORD_NOT_FOUND	            		SG_ERR_SG_LIBRARY(204)
#define SG_ERR_SERVER_DISALLOWED_REPO_CREATE_OR_DELETE		SG_ERR_SG_LIBRARY(205)

#define SG_ERR_ZING_TYPE_MISMATCH	            			SG_ERR_SG_LIBRARY(207)
#define SG_ERR_OLD_AUDITS       	            			SG_ERR_SG_LIBRARY(208)

#define SG_ERR_ZING_INVALID_TEMPLATE                        SG_ERR_SG_LIBRARY(210)
#define SG_ERR_ZING_UNKNOWN_ACTION_TYPE                     SG_ERR_SG_LIBRARY(211)
#define SG_ERR_ZING_UNKNOWN_BUILTIN_ACTION                  SG_ERR_SG_LIBRARY(212)
#define SG_ERR_ZING_NO_HISTORY_IF_NO_STATE                  SG_ERR_SG_LIBRARY(213)
#define SG_ERR_ZING_TYPE_CHANGE_NOT_ALLOWED                 SG_ERR_SG_LIBRARY(214)
#define SG_ERR_ZING_ILLEGAL_DURING_COMMIT                   SG_ERR_SG_LIBRARY(215)
#define SG_ERR_ZING_TEMPLATE_NOT_FOUND                      SG_ERR_SG_LIBRARY(216)
#define SG_ERR_ZING_ONE_TEMPLATE                            SG_ERR_SG_LIBRARY(217)
#define SG_ERR_ZING_CONSTRAINT                              SG_ERR_SG_LIBRARY(218)
#define SG_ERR_ZING_FIELD_NOT_FOUND	           		        SG_ERR_SG_LIBRARY(219)
#define SG_ERR_USER_ALREADY_EXISTS	           		        SG_ERR_SG_LIBRARY(220)
#define SG_ERR_ZING_MERGE_CONFLICT	           		        SG_ERR_SG_LIBRARY(221)
#define SG_ERR_ZING_WHERE_PARSE_ERROR	       		        SG_ERR_SG_LIBRARY(222)
#define SG_ERR_ZING_SORT_PARSE_ERROR	       		        SG_ERR_SG_LIBRARY(223)
#define SG_ERR_ZING_RECTYPE_NOT_FOUND	           		    SG_ERR_SG_LIBRARY(224)
#define SG_ERR_ZING_NO_RECID	                		    SG_ERR_SG_LIBRARY(225)
#define SG_ERR_NO_CURRENT_WHOAMI	               		    SG_ERR_SG_LIBRARY(226)
#define SG_ERR_GOTTA_BE_SOMEBODY	               		    SG_ERR_SG_LIBRARY(227)
#define SG_ERR_ZING_NO_ANCESTOR	               		        SG_ERR_SG_LIBRARY(228)
#define SG_ERR_INVALID_USERID	               		        SG_ERR_SG_LIBRARY(229)
#define SG_ERR_WRONG_DAG_TYPE	               		        SG_ERR_SG_LIBRARY(230)
#define SG_ERR_ZING_NO_RECTYPE	                		    SG_ERR_SG_LIBRARY(231)
#define SG_ERR_ZING_ONLY_ONE_RECTYPE_ALLOWED	            SG_ERR_SG_LIBRARY(232)
#define SG_ERR_REPO_ALREADY_EXISTS	               		    SG_ERR_SG_LIBRARY(233)
#define SG_ERR_BRANCH_NEEDS_MERGE	           		        SG_ERR_SG_LIBRARY(234)
#define SG_ERR_CHANGESET_NOT_IN_INDEX          		        SG_ERR_SG_LIBRARY(235)
#define SG_ERR_NO_RECURSIVE_GROUPS          		        SG_ERR_SG_LIBRARY(236)
#define SG_ERR_USER_ALREADY_HAS_PUBLIC_KEY     		        SG_ERR_SG_LIBRARY(237)
#define SG_ERR_PRIVATE_KEY_FILE_ALREADY_EXISTS 		        SG_ERR_SG_LIBRARY(238)
#define SG_ERR_USER_HAS_NO_PUBLIC_KEY        		        SG_ERR_SG_LIBRARY(239)
#define SG_ERR_PRIVATE_KEY_FILE_NOT_FOUND 		            SG_ERR_SG_LIBRARY(240)
#define SG_ERR_SIGNATURE_USES_OLD_KEY                       SG_ERR_SG_LIBRARY(241)
#define SG_ERR_SIGNATURE_FORMAT_UNKNOWN                     SG_ERR_SG_LIBRARY(242)
#define SG_ERR_HDB_DUPLICATE_KEY                            SG_ERR_SG_LIBRARY(243)
#define SG_ERR_HDB_CANNOT_ROLLBACK                          SG_ERR_SG_LIBRARY(244)
#define SG_ERR_NOT_TIED	                     		        SG_ERR_SG_LIBRARY(245)
#define SG_ERR_BRANCH_DOES_NOT_NEED_MERGE	   		        SG_ERR_SG_LIBRARY(246)
#define SG_ERR_TIED_BRANCH_NOT_BASELINE	     		        SG_ERR_SG_LIBRARY(247)
#define SG_ERR_JSON_WRONG_TOP_TYPE	        		        SG_ERR_SG_LIBRARY(248)
#define SG_ERR_BRANCH_NOT_FOUND	            		        SG_ERR_SG_LIBRARY(249)
// gap
#define SG_ERR_CANCEL                                       SG_ERR_SG_LIBRARY(251)
#define SG_ERR_JS_NAMED_PARAM_REQUIRED	            		SG_ERR_SG_LIBRARY(252)
#define SG_ERR_JS_NAMED_PARAM_NOT_ALLOWED	           		SG_ERR_SG_LIBRARY(253)
#define SG_ERR_JS_NAMED_PARAM_TYPE_CONVERSION	        	SG_ERR_SG_LIBRARY(254)
#define SG_ERR_MUTEX_FAILURE								SG_ERR_SG_LIBRARY(255)
#define SG_ERR_VC_HOOK_MISSING								SG_ERR_SG_LIBRARY(256)
#define SG_ERR_VC_HOOK_REFUSED								SG_ERR_SG_LIBRARY(257)
#define SG_ERR_RESOLVE_INVALID_SAVE_DATA                    SG_ERR_SG_LIBRARY(258)
#define SG_ERR_RESOLVE_TOOL_NOT_FOUND                       SG_ERR_SG_LIBRARY(259)
#define SG_ERR_RESOLVE_INVALID_VALUE                        SG_ERR_SG_LIBRARY(260)
#define SG_ERR_RESOLVE_INVALID_TARGET                       SG_ERR_SG_LIBRARY(261)
#define SG_ERR_BRANCH_ALREADY_CLOSED						SG_ERR_SG_LIBRARY(262)
#define SG_ERR_BRANCH_NOT_CLOSED	    					SG_ERR_SG_LIBRARY(263)
#define SG_ERR_BRANCH_ALREADY_EXISTS						SG_ERR_SG_LIBRARY(264)
#define SG_ERR_BRANCH_ADD_ANCESTOR		    				SG_ERR_SG_LIBRARY(265)
#define SG_ERR_BRANCH_ADD_DESCENDANT						SG_ERR_SG_LIBRARY(266)
#define SG_ERR_UNKNOWN_SYNC_PROTOCOL_VERSION				SG_ERR_SG_LIBRARY(267)
#define SG_ERR_UNKNOWN_STAGING_VERSION						SG_ERR_SG_LIBRARY(268)
#define SG_ERR_EMPTYCOMMITMESSAGE							SG_ERR_SG_LIBRARY(269)
#define SG_ERR_UNSUPPORTED_CLOSET_VERSION					SG_ERR_SG_LIBRARY(270)
#define SG_ERR_PUSH_WOULD_CREATE_NEW_HEADS					SG_ERR_SG_LIBRARY(271)
#define SG_ERR_ADMIN_ID_MISMATCH							SG_ERR_SG_LIBRARY(272)
#define SG_ERR_RESOLVE_ALREADY_RESOLVED                     SG_ERR_SG_LIBRARY(273)
#define SG_ERR_JSMIN_ERROR									SG_ERR_SG_LIBRARY(274)
#define SG_ERR_NEED_REINDEX									SG_ERR_SG_LIBRARY(275)
#define SG_ERR_AREA_NOT_FOUND								SG_ERR_SG_LIBRARY(276)
#define SG_ERR_INACTIVE_USER								SG_ERR_SG_LIBRARY(277)
#define SG_ERR_FILE_ALREADY_EXISTS							SG_ERR_SG_LIBRARY(278)
#define SG_ERR_DBNDX_FILTER_TIMEOUT							SG_ERR_SG_LIBRARY(279)
#define SG_ERR_UNSUPPORTED_USERMAP_VERSION					SG_ERR_SG_LIBRARY(280)
#define SG_ERR_INVALID_DESCRIPTOR							SG_ERR_SG_LIBRARY(290)
#define SG_ERR_REPO_UNAVAILABLE								SG_ERR_SG_LIBRARY(291)
#define SG_ERR_CLOSET_PROP_ALREADY_EXISTS					SG_ERR_SG_LIBRARY(292)
#define SG_ERR_NO_SUCH_CLOSET_PROP							SG_ERR_SG_LIBRARY(293)

#define SG_ERR_SIGINT										SG_ERR_SG_LIBRARY(295) // You should never handle/discard this.
#define SG_ERR_PASSWORD_STORAGE_FAIL						SG_ERR_SG_LIBRARY(296)

#define SG_ERR_NO_REPO_TX           						SG_ERR_SG_LIBRARY(297)
#define SG_ERR_ONLY_ONE_REPO_TX        						SG_ERR_SG_LIBRARY(298)
#define SG_ERR_BAD_REPO_TX_HANDLE      						SG_ERR_SG_LIBRARY(299)
	
#define SG_ERR_VC_HOOK_AMBIGUOUS							SG_ERR_SG_LIBRARY(300)
#define SG_ERR_DB_DELTA         							SG_ERR_SG_LIBRARY(301)
#define SG_ERR_MALFORMED_SVN_DUMP                           SG_ERR_SG_LIBRARY(302)
#define SG_ERR_MALFORMED_SVN_DIFF                           SG_ERR_SG_LIBRARY(303)

#define SG_ERR_HTTP_401                                     SG_ERR_SG_LIBRARY(401)
#define SG_ERR_HTTP_404                                     SG_ERR_SG_LIBRARY(404)
#define SG_ERR_HTTP_502                                     SG_ERR_SG_LIBRARY(502)


// NOTE: If you add an error code, update SG_error__get_message() in ut/sg_error.c

#define SG_ERR_MUST_ATTACH_OR_DETACH						SG_ERR_SG_LIBRARY(350)
#define SG_ERR_INVALID_BRANCH_NAME							SG_ERR_SG_LIBRARY(351)
#define SG_ERR_NOT_IN_A_MERGE								SG_ERR_SG_LIBRARY(352)
#define SG_ERR_NO_SERVER_SPECIFIED							SG_ERR_SG_LIBRARY(353)
#define SG_ERR_VC_LOCK_CANNOT_LOCK_DELETED_ITEM				SG_ERR_SG_LIBRARY(354)
#define SG_ERR_NOT_ALLOWED_WITH_LOST_ITEM					SG_ERR_SG_LIBRARY(355)
#define SG_ERR_NO_EFFECT									SG_ERR_SG_LIBRARY(356)
#define SG_ERR_WC_PORT_DUPLICATE							SG_ERR_SG_LIBRARY(357)
#define SG_ERR_UNRESOLVED_ITEM_WOULD_BE_AFFECTED			SG_ERR_SG_LIBRARY(358)
#define SG_ERR_RESOLVE_NEEDS_OVERWRITE						SG_ERR_SG_LIBRARY(359)
#if defined(DEBUG)
#define SG_ERR_DEBUG_1										SG_ERR_SG_LIBRARY(360)
#endif
#define SG_ERR_WC_PORT_FLAGS                                SG_ERR_SG_LIBRARY(375)
#define SG_ERR_PATH_NOT_IN_WORKING_COPY                     SG_ERR_SG_LIBRARY(376)
#define SG_ERR_WC_CANNOT_APPLY_CHANGES_IN_READONLY_TX       SG_ERR_SG_LIBRARY(377)
#define SG_ERR_WC_IS_SPARSE                                 SG_ERR_SG_LIBRARY(378)
#define SG_ERR_WC_PATH_NOT_UNIQUE                           SG_ERR_SG_LIBRARY(379)
#define SG_ERR_WC_PARTIAL_COMMIT                            SG_ERR_SG_LIBRARY(380)
#define SG_ERR_WC_ITEM_BLOCKS_REVERT                        SG_ERR_SG_LIBRARY(381)
#define SG_ERR_WC__MOVE_DESTDIR_NOT_FOUND                   SG_ERR_SG_LIBRARY(382)
#define SG_ERR_WC__MOVE_DESTDIR_UNCONTROLLED                SG_ERR_SG_LIBRARY(383)
#define SG_ERR_WC__MOVE_DESTDIR_PENDING_DELETE              SG_ERR_SG_LIBRARY(384)
#define SG_ERR_WC__REVERT_BLOCKED_BY_DESTDIR                SG_ERR_SG_LIBRARY(385)
#define SG_ERR_DAGLCA_NO_ANCESTOR                           SG_ERR_SG_LIBRARY(386)
#define SG_ERR_WC_RESERVED_ENTRYNAME                        SG_ERR_SG_LIBRARY(387)

#define SG_ERR_MERGE_TARGET_EQUALS_BASELINE                 SG_ERR_SG_LIBRARY(388)
#define SG_ERR_MERGE_TARGET_IS_ANCESTOR_OF_BASELINE         SG_ERR_SG_LIBRARY(389)
#define SG_ERR_MERGE_TARGET_IS_DESCENDANT_OF_BASELINE       SG_ERR_SG_LIBRARY(390)
#define SG_ERR_UPDATE_FROM_NEW_BRANCH                       SG_ERR_SG_LIBRARY(391)

#define SG_ERR_WC_DB_BUSY			                        SG_ERR_SG_LIBRARY(392)
#define SG_ERR_NO_CHANGESET_WITH_PREFIX                     SG_ERR_SG_LIBRARY(393)
#define SG_ERR_VC_LINE_HISTORY_NOT_COMMITTED_YET			SG_ERR_SG_LIBRARY(394)
#define SG_ERR_VC_LINE_HISTORY_WORKING_COPY_MODIFIED        SG_ERR_SG_LIBRARY(395)
#define SG_ERR_UNSUPPORTED_DEVICE_SPECIAL_FILE              SG_ERR_SG_LIBRARY(396)
#define SG_ERR_WC_HAS_MULTIPLE_PARENTS                      SG_ERR_SG_LIBRARY(397)
#define SG_ERR_ITEM_NOT_PRESENT_IN_PARENTS                  SG_ERR_SG_LIBRARY(398)
#define SG_ERR_DIFFTOOL_CANCELED                            SG_ERR_SG_LIBRARY(399)
// TODO watch out the 400's are taken above

END_EXTERN_C;

#endif//H_SG_ERROR_TYPEDEFS_H
