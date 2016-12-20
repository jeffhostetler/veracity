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

#ifndef VV_SG_HELPERS_H
#define VV_SG_HELPERS_H

#include "precompiled.h"
#include "vvVerbs.h"

/*
 * This is a collection of general helper functions for working with sglib.
 */
class vvSgHelpers
{
// static functionality
public:
	/**
	 * Converts from an SG_bool to a C++ bool.
	 */
	static bool Convert_sgBool_cppBool(
		int bValue //< [in] The SG_bool value to convert.
		);

	/**
	 * Converts from a C++ bool to an SG_bool.
	 */
	static int Convert_cppBool_sgBool(
		bool bValue //< [in] The bool value to convert.
		);

	/**
	 * Converts from an SG_string to a wxString.
	 */
	static wxString Convert_sgString_wxString(
		const struct _SG_string* sValue //< [in] The SG_string value to convert.
		                                //<      Causes an error if NULL.
		);

	/**
	 * Converts from an SG_string to a wxString.
	 */
	static wxString Convert_sgString_wxString(
		const struct _SG_string* sValue,    //< [in] The SG_string value to convert.
		const wxString&          sNullValue //< [in] The value to return if sValue is NULL.
		);

	/**
	 * Converts from an sglib-encoded const char* to a wxString.  The given string
	 * must have been encoded by/for sglib (currently meaning it's UTF8).
	 */
	static wxString Convert_sglibString_wxString(
		const char* szValue //< [in] The const char* value to convert.
		                    //<      Causes an error if NULL.
		);

	/**
	 * Converts from an sglib-encoded const char* to a wxString.  The given string
	 * must have been encoded by/for sglib (currently meaning it's UTF8).
	 */
	static wxString Convert_sglibString_wxString(
		const char*     szValue,   //< [in] The const char* value to convert.
		const wxString& sNullValue //< [in] The value to return if szValue is NULL.
		);

	/**
	 * Converts from an SG_pathname to a wxString.
	 */
	static wxString Convert_sgPathname_wxString(
		const struct _SG_pathname* pValue //< [in] The SG_pathname value to convert.
		                                  //<      Causes an error if NULL.
		);

	/**
	 * Converts from an SG_pathname to a wxString.
	 */
	static wxString Convert_sgPathname_wxString(
		const struct _SG_pathname* pValue,    //< [in] The SG_pathname value to convert.
		const wxString&            sNullValue //< [in] The value to return if pValue is NULL.
		);

	/**
	 * Converts from a wxString to a buffer containing equivalent data that
	 * is encoded in the format that sglib expects (currently UTF8).  The owner
	 * of the returned data might be either the specified wxString OR the returned
	 * buffer.  It's therefore best to make sure you don't try to access the buffer
	 * if the wxString instance has been destroyed.  If you need your own copy of
	 * the data, create a wxCharBuffer from the returned wxScopedCharBuffer.  That
	 * wxCharBuffer will contain a pointer to your own copy of the data.
	 *
	 * Warning: Because of the memory ownership described above, you should NOT
	 *          assign the value returned from this function to a char*!  The
	 *          compiler will let you because wxScopedCharBuffer implicitly converts
	 *          to one, but if you do that and the buffer actually owned the memory
	 *          for the string, then you may end up with an invalid pointer because
	 *          the buffer might be destroyed directly after the function call and
	 *          assignment, since it's a temporary object that's never assigned to
	 *          a variable.
	 *          If you need to store the returned value to a variable for later use
	 *          (rather than pass it directly to a function), you should store it
	 *          as a wxScopedCharBuffer or wxCharBuffer and then use that variable
	 *          basically like it was a simple char*.
	 */
	static wxScopedCharBuffer Convert_wxString_sglibString(
		const wxString& sValue,              //< [in] The wxString to convert.
		bool            bEmptyToNull = false //< [in] Whether or not an empty value should be converted to NULL.
		                                     //<      If true and sValue.IsEmpty, the returned buffer contains a NULL pointer.
		                                     //<      If false and sValue.IsEmpty, the returned buffer contains a pointer to a NULL character.
		);

	/**
	 * Same as the main overload, but converts a NULL string value
	 * into a buffer containing a NULL pointer.
	 */
	static wxScopedCharBuffer Convert_wxString_sglibString(
		const wxString* pValue,              //< [in] The wxString to convert.
		                                     //<      If NULL, then a buffer containing a NULL pointer is returned.
		bool            bEmptyToNull = false //< [in] Whether or not an empty value should be converted to NULL.
		                                     //<      If true and pValue->IsEmpty, the returned buffer contains a NULL pointer.
		                                     //<      If false and pValue->IsEmpty, the returned buffer contains a pointer to a NULL character.
		);

	/**
	 * Same as the main overload, except the resulting converted value is owned
	 * by sglib, and must be freed using SG_NULLFREE.
	 */
	static char* Convert_wxString_sglibString(
		class vvContext& cContext,            //< [in] [out] Error and context info.
		const wxString&  sValue,              //< [in] The wxString to convert.
		bool             bEmptyToNull = false //< [in] Whether or not an empty value should be converted to NULL.
		                                      //<      If true and pValue->IsEmpty, the returned pointer is NULL.
		                                      //<      If false and pValue->IsEmpty, the returned pointer points to a NULL character.
		);

	/**
	 * Combination of the overload that returns a value owned by sglib, and the
	 * overload that accepts a wxString pointer and converts NULL to NULL.
	 */
	static char* Convert_wxString_sglibString(
		class vvContext& cContext,            //< [in] [out] Error and context info.
		const wxString*  pValue,              //< [in] The wxString to convert.
		bool             bEmptyToNull = false //< [in] Whether or not an empty value should be converted to NULL.
		                                      //<      If true and pValue->IsEmpty, the returned pointer is NULL.
		                                      //<      If false and pValue->IsEmpty, the returned pointer points to a NULL character.
		);

	/**
	 * Converts from a wxString to an SG_pathname.
	 * Caller owns the returned pointer.
	 */
	static struct _SG_pathname* Convert_wxString_sgPathname(
		class vvContext& cContext,            //< [in] [out] Error and context info.
		const wxString&  sValue,              //< [in] The wxString value to convert.
		bool             bEmptyToNull = false //< [in] Whether or not an empty value should be converted to NULL.
		                                      //<      If true and pValue->IsEmpty, the returned pointer is NULL.
		                                      //<      If false and pValue->IsEmpty, the returned pointer points to an empty SG_pathname.
		);

	static struct _SG_pathname* vvSgHelpers::Convert_wxArrayString_sgPathname(
		class vvContext&      pCtx,            //< [in] [out] Error and context info.
		const wxArrayString& sValueArray,	   //< [in] The wxArrayString to convert.  Only the first item will be used.
		bool            bEmptyToNull = false  //< [in] Whether or not an empty value should be converted to NULL.
		                                      //<      If true and pValue.Count() == 0 || pValue[0]->IsEmpty, the returned pointer is NULL.
		                                      //<      If false and pValue.Count() == 0 || pValue[0]->IsEmpty, the returned pointer points to an empty SG_pathname.
		);

	/**
	 *Converts a list of stampdata to a wxArrayString.
	 */
	static void vvSgHelpers::Convert_listStampData_wxArrayString(
		vvContext& pCtx,
		vvVerbs::stlStampDataList stampData,
		wxArrayString& cStrings
		);
	
	/**
	 *Converts a list of tagdata to a wxArrayString.
	 */
	static void vvSgHelpers::Convert_listTagData_wxArrayString(
		vvContext& pCtx,
		vvVerbs::stlTagDataList tagData,
		wxArrayString& cStrings
		);

	/**
	* Converts a rbtree to a wxArrayString.
	*/
	static void Convert_rbTreeKeys_wxArrayString(
	vvContext& pCtx,
	struct _sg_rbtree * prb,
	wxArrayString& cStrings
	);

	/**
	 * Converts from a wxArrayString to a const char** and size/count.
	 * The output array as well as each string within it are owned by sglib
	 * and must be freed by the caller.
	 */
	static void Convert_wxArrayString_sglibStringArrayCount(
		class vvContext&     cContext,   //< [in] [out] Error and context info.
		const wxArrayString& cStrings,   //< [in] The string array to convert.
		char***              pppStrings, //< [out] The converted string array.
		                                 //<       Caller owns the array pointer and all individual string pointers.
		                                 //<       They must be freed using sglib (SG_STRINGLIST_NULLFREE).
		wxUint32*            pCount      //< [out] Size of the returned array (SG_int32).
		);

	/**
	 * Converts from a wxArrayString to an SG_stringarray.
	 */
	static void Convert_wxArrayString_sgStringArray(
		class vvContext&         cContext, //< [in] [out] Error and context info.
		const wxArrayString&     cStrings, //< [in] The string array to convert.
		struct _sg_stringarray** ppStrings //< [out] The converted string array.
		                                   //<       Caller owns this pointer.
		);

	/**
	 * Converts from an SG_stringarray to a wxArrayString.
	 */
	static void Convert_sgStringArray_wxArrayString(
		class vvContext&              cContext, //< [in] [out] Error and context info.
		const struct _sg_stringarray* pStrings, //< [in] The SG_stringarray to convert.
		wxArrayString&                cStrings  //< [out] The wxStringArray to fill the strings into.
		);

	/**
	 * Converts from a wxArrayString to an SG_stringarray.
	 */
	static void Convert_wxArrayString_sgRBTree(
		class vvContext&         cContext, //< [in] [out] Error and context info.
		const wxArrayString&     cStrings, //< [in] The string array to convert.
		struct _sg_rbtree** pprbTree,		//< [out] The converted rbtree
											//<       Caller owns this pointer.
		bool bNullIsOK = false
		                                   
		);


			/**
	 * Converts from a wxArrayString to an SG_stringarray.
	 */
	static void Convert_wxArrayString_sgVhash(
		class vvContext&         cContext, //< [in] [out] Error and context info.
		const wxArrayString&     cStrings, //< [in] The string array to convert.
		struct _SG_vhash** ppvhash, //< [out] The converted vhash.
		                                   //<       Caller owns this pointer.
		bool bNullIsOK = false
		);

	/**
	 * Converts from a wxArrayString into an SG_varray containing strings.
	 */
	static void Convert_wxArrayString_sgVarray(
		class vvContext&     cContext,    //< [in] [out] Error and context info.
		const wxArrayString& cStrings,    //< [in] The string array to convert.
		struct _SG_varray**  ppVarray,    //< [in] The converted varray.
		                                  //<      Caller owns this pointer.
		bool                 bNullIfEmpty //< [in] If true, NULL is returned when cStrings is empty.
		                                  //<      If false, an allocated but empty SG_varray is returned.
		);

	static void Convert_sgVarray_wxArrayString(
		vvContext& pCtx,
		const void*     pvarray,
		wxArrayString& cStrings
		);

	static void Convert_wxArrayInt_sgVarray(
		vvContext& pCtx,
		const wxArrayInt& cInts,
		void**     ppvarray,
		bool bNullIsOK
		);

	static void Convert_sgVarray_wxArrayInt(
		vvContext& pCtx,
		const void*     pvarray,
		wxArrayInt& cInts
		);

	/**
	 * Converts from an sglib time to a wxDateTime.
	 * The sglib time is expected to be milliseconds since 1/1/1970 00:00:00.000 UTC.
	 */
	static wxDateTime Convert_sgTime_wxDateTime(
		wxInt64 iValue //< [in] The SG_int64 time value to convert.
		);

	/**
	 * Converts from a wxDateTime to an sglib time.
	 * The sglib time will be in milliseconds since 1/1/1970 00:00:00.000 UTC.
	 */
	static wxInt64 Convert_wxDateTime_sgTime(
		const wxDateTime& cValue //< [in] The wxDateTime value to convert.
		                         //<      Causes an error if !IsValid().
		);

	/**
	 * Same as other overload, but allows for invalid date values.
	 */
	static wxInt64 Convert_wxDateTime_sgTime(
		const wxDateTime& cValue,       //< [in] The wxDateTime value to convert.
		wxInt64           iInvalidValue //< [in] Value to return if !cValue.IsValid().
		);

	/**
	 * Converts from SG_history_result to HistoryData
	 **/
	static vvVerbs::HistoryData Convert_sg_history_result_HistoryData(
		class vvContext& pCtx,
		struct _opaque_history_result * phr_result,
		struct _SG_vhash * pvh_pile
		);

#if defined(DEBUG)

	/**
	 * Dumps a vhash to a string.
	 */
	static wxString Dump_sgVhash(
		class vvContext&        cContext,
		const struct _SG_vhash* pValue
		);

#endif
};


#endif
