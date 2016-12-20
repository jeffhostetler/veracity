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

#include <sg.h>
#include "unittests.h"

//////////////////////////////////////////////////////////////////
// we define a little trick here to prefix all global symbols (type,
// structures, functions) with our test name.  this allows all of
// the tests in the suite to be #include'd into one meta-test (without
// name collisions) when we do a GCOV run.

#define MyMain()				TEST_MAIN(u0076_time)
#define MyDcl(name)				u0076_time__##name
#define MyFn(name)				u0076_time__##name

//////////////////////////////////////////////////////////////////

#define GEN_DATA 0

#if GEN_DATA
void MyFn(test__gen_data)(SG_context * pCtx)
{
	// i used this on one platform to generate test data and
	// manually create test cases for calling test__sanity().

	const char * asz[] =
		{ "Tue, 16 Mar 2010 14:11:13 GMT",	// value from original test
		  "Sat, 06 Mar 2010 01:02:03 GMT",	// value from original test

		  "Sat, 01 Jan 2011 00:00:00 GMT",
		  "Sat, 01 Jan 2011 12:00:00 GMT",
		  "Sat, 01 Jan 2011 23:59:59 GMT",
		  "Sat, 31 Dec 2011 23:59:59 GMT",

		  "Fri, 01 Jul 2011 00:00:01 GMT",
		  "Fri, 01 Jul 2011 12:00:01 GMT",
		  "Fri, 01 Jul 2011 23:59:59 GMT",

		  "Thu, 30 Jun 2011 23:59:59 GMT",
		};
	SG_uint32 k;

	for (k=0; k<SG_NrElements(asz); k++)
	{
		SG_int64 iTime;
		char bufUTC[SG_TIME_FORMAT_LENGTH + 1];
		SG_int_to_string_buffer bufI64;

		VERIFY_ERR_CHECK(  SG_time__parseRFC850(pCtx, asz[k], &iTime)  );
		SG_int64_to_sz(iTime, bufI64);
		VERIFY_ERR_CHECK(  SG_time__format_utc__i64(pCtx, iTime, bufUTC, SG_NrElements(bufUTC))  );

		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "\tBEGIN_TEST(  MyFn(test__sanity)(pCtx, %sLL, \"%s\", \"%s\")  );\n",
								   bufI64, bufUTC, asz[k])  );
	}

fail:
	;
}
#endif

void MyFn(test__parseRfc850)(SG_context *pCtx)
{
	const char *ims = "Tue, 16 Mar 2010 14:11:13 GMT";
	SG_int32 epoch = 1268748673;
	SG_int64 sgtime = (SG_int64)epoch * 1000;
	SG_string *msg = NULL;
	SG_int64 parsed = 0;
	char buf_i64_sgtime[40];
	char buf_i64_parsed[40];

	SG_ERR_CHECK(  SG_time__parseRFC850(pCtx, ims, &parsed)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &msg)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, msg, "parsing '%s': expected '%s', got '%s'", ims, SG_int64_to_sz(sgtime, buf_i64_sgtime), SG_int64_to_sz(parsed, buf_i64_parsed))  );

	VERIFY_COND(SG_string__sz(msg), parsed == sgtime);

fail:
	SG_STRING_NULLFREE(pCtx, msg);
}


void MyFn(test__formatRfc850)(SG_context *pCtx)
{
	const char *expected = "Tue, 16 Mar 2010 14:11:13 GMT";
	SG_int32 epoch = 1268748673;
	SG_int64 sgtime = (SG_int64)epoch * 1000;
	SG_string *msg = NULL;
	char formatted[1024];
	char buf_i64[40];

	SG_ERR_CHECK(  SG_time__formatRFC850(pCtx, sgtime, formatted, sizeof(formatted))  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &msg)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, msg, "formatting '%s': expected '%s', got '%s'", SG_int64_to_sz(sgtime, buf_i64), expected, formatted)  );

	VERIFY_COND(SG_string__sz(msg), strcmp(expected, formatted) == 0);

fail:
	SG_STRING_NULLFREE(pCtx, msg);
}


void MyFn(test__parseRfc850nonGmt)(SG_context *pCtx)
{
	const char *ims = "Tue, 16 Mar 2010 14:11:13 EDT";
	SG_int64 parsed = 0;

	SG_time__parseRFC850(pCtx, ims, &parsed);
	VERIFY_CTX_ERR_EQUALS("out-of-range if not GMT", pCtx, SG_ERR_ARGUMENT_OUT_OF_RANGE);
	SG_context__err_reset(pCtx);
}

//////////////////////////////////////////////////////////////////

void MyFn(test__sanity)(SG_context * pCtx,
						SG_int64 iTime_Input,
						const char * pszUTC_Expected,
						const char * pszRFC_Expected)

{
	// because we internally store time in csets and db records as
	// a "SG_int64 iTime" representing UTC milliseconds since the
	// epoch and we need to know that that integer value is global
	// and means the same thing on all platforms, this test is driven
	// by starting with the iTime.  yes, that iTime should map to a
	// given struct_tm and/or a formatted datestamp and these should
	// round-trip back to the identical iTime, BUT WE NEED TO VERIFY
	// THAT THE INTEGER VALUE IS THE SAME ON ALL PLATFORMS.

	SG_time timeUTC;
	SG_time timeLoc;
	SG_int_to_string_buffer bufI64_Input;
	char bufUTC_Computed[SG_TIME_FORMAT_LENGTH + 1];
	char bufRFC_Computed[SG_MAX_RFC850_LENGTH  + 1];
	char bufLoc_Computed[SG_TIME_FORMAT_LENGTH + 1];
	SG_int64 iTime_RFC_Computed;
	SG_int_to_string_buffer bufI64_RFC_Computed;
	SG_string * pStringAdHoc = NULL;
	SG_int64 iTime_AdHoc_Computed;
	SG_int_to_string_buffer bufI64_AdHoc_Computed;
	SG_int64 iTime_AdHoc_Computed_Min;
	SG_int64 iTime_AdHoc_Computed_Max;
	SG_int_to_string_buffer bufI64_AdHoc_Computed_Min;
	SG_int_to_string_buffer bufI64_AdHoc_Computed_Max;
	SG_int64 iTime_Expected_Min;
	SG_int64 iTime_Expected_Max;
	SG_int_to_string_buffer bufI64_Expected_Min;
	SG_int_to_string_buffer bufI64_Expected_Max;
	char bufLoc_Computed_Min[SG_TIME_FORMAT_LENGTH + 1];
	char bufLoc_Computed_Max[SG_TIME_FORMAT_LENGTH + 1];
	SG_time timeLoc_Min;
	SG_time timeLoc_Max;

	// we are given an absolute iTime in ms since the epoch.
	// by definition this iTime is in UTC.

	SG_int64_to_sz(iTime_Input, bufI64_Input);

	VERIFY_ERR_CHECK(  SG_time__format_utc__i64(  pCtx, iTime_Input, bufUTC_Computed, SG_NrElements(bufUTC_Computed))  );
	VERIFY_ERR_CHECK(  SG_time__formatRFC850(     pCtx, iTime_Input, bufRFC_Computed, SG_NrElements(bufRFC_Computed))  );
	VERIFY_ERR_CHECK(  SG_time__format_local__i64(pCtx, iTime_Input, bufLoc_Computed, SG_NrElements(bufLoc_Computed))  );

	memset(&timeUTC,0,sizeof(timeUTC));
	memset(&timeLoc,0,sizeof(timeLoc));

	VERIFY_ERR_CHECK(  SG_time__decode(pCtx, iTime_Input, &timeUTC)  );
	VERIFY_ERR_CHECK(  SG_time__decode__local(pCtx, iTime_Input, &timeLoc)  );

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("Input     : %s\n"
								"       UTC: %s\n"
								"       RFC: %s\n"
								"       Loc: %s\n"
								"    UTC tm: %04d/%02d/%02d %02d:%02d:%02d.%03d %d %03d %d\n"
								"    Loc tm: %04d/%02d/%02d %02d:%02d:%02d.%03d %d %03d %d\n"),
							   bufI64_Input,
							   bufUTC_Computed,
							   bufRFC_Computed,
							   bufLoc_Computed,
							   timeUTC.year,timeUTC.month,timeUTC.mday,
							   timeUTC.hour,timeUTC.min,timeUTC.sec,timeUTC.msec,
							   timeUTC.wday,timeUTC.yday,timeUTC.offsetUTC,
							   timeLoc.year,timeLoc.month,timeLoc.mday,
							   timeLoc.hour,timeLoc.min,timeLoc.sec,timeLoc.msec,
							   timeLoc.wday,timeLoc.yday,timeLoc.offsetUTC)  );

	// verify that the UTC iTime maps to the expected UTC and RFC datestamps when formatted.

	VERIFYP_COND(bufI64_Input, (strcmp(bufUTC_Computed,pszUTC_Expected)==0), ("%s: UTC [Computed %s][Expected %s]", bufI64_Input, bufUTC_Computed, pszUTC_Expected));
	VERIFYP_COND(bufI64_Input, (strcmp(bufRFC_Computed,pszRFC_Expected)==0), ("%s: RFC [Computed %s][Expected %s]", bufI64_Input, bufRFC_Computed, pszRFC_Expected));

	// we CANNOT directly verify the formatted LOCALTIME datestamp because we don't yet know the TZ + DST delta.

	// go the other way and convert the *GIVEN* RFC datestamp into UTC iTime and verify it matches the *GIVEN* iTime.

	VERIFY_ERR_CHECK(  SG_time__parseRFC850(pCtx, pszRFC_Expected, &iTime_RFC_Computed)  );
	SG_int64_to_sz(iTime_RFC_Computed, bufI64_RFC_Computed);
	VERIFYP_COND(bufI64_Input, (iTime_RFC_Computed == iTime_Input), ("%s: rev RFC [Computed %s][Expected %s] for [%s]", bufI64_Input, bufI64_RFC_Computed, bufI64_Input, pszRFC_Expected));

	VERIFYP_COND(bufI64_Input, (strcmp(bufI64_RFC_Computed,bufI64_Input)==0), ("%s: rev RFC [Computed %s][Expected %s] for [%s]", bufI64_Input, bufI64_RFC_Computed, bufI64_Input, pszRFC_Expected));

	// we have a *loose* format parser (which is used by 'vv history' to do date-ranges).
	// since this data comes from the user, its input is in LOCALTIME.  it takes input in
	// the following formats:
	//
	//  "YYYY-MM-DD hh:mm:ss"
	//  "YYYY/MM/DD hh:mm:ss"
	//
	// they can also omit the time and just give us:
	// 
	//  "YYYY-MM-DD"
	//  "YYYY/MM/DD"
	//
	// (and we assume 00:00:00 or 23:59:59 depending on the flag).
	//
	// use the *well-defined* SG_time structure that is in LOCALTIME to synthesize
	// valid and bogus *loose* strings to test the parser.  [we can't just say
	// test( "2010-03-16 14:11:13" === 1268748673000 ) because the former is in
	// localtime and the latter is in UTC (and the hard-coded test would only work
	// during winter time....)

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringAdHoc)  );

	// the full format with dashes.

	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringAdHoc, "%04d-%02d-%02d %02d:%02d:%02d",
									  timeLoc.year, timeLoc.month, timeLoc.mday, timeLoc.hour, timeLoc.min, timeLoc.sec)  );
	VERIFY_ERR_CHECK(  SG_time__parse__informal__local(pCtx, SG_string__sz(pStringAdHoc), &iTime_AdHoc_Computed, SG_FALSE)  );
	SG_int64_to_sz(iTime_AdHoc_Computed, bufI64_AdHoc_Computed);
	VERIFYP_COND(bufI64_Input, (iTime_AdHoc_Computed == iTime_Input), ("%s: adhoc1 [Computed %s][Expected %s] for [%s]", bufI64_Input, bufI64_AdHoc_Computed, bufI64_Input, SG_string__sz(pStringAdHoc)));

	// test that we parse on the '-' and ':' characters rather than fixed length fields.

	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringAdHoc, "%d-%d-%d %d:%d:%d",
									  timeLoc.year, timeLoc.month, timeLoc.mday, timeLoc.hour, timeLoc.min, timeLoc.sec)  );
	VERIFY_ERR_CHECK(  SG_time__parse__informal__local(pCtx, SG_string__sz(pStringAdHoc), &iTime_AdHoc_Computed, SG_FALSE)  );
	SG_int64_to_sz(iTime_AdHoc_Computed, bufI64_AdHoc_Computed);
	VERIFYP_COND(bufI64_Input, (iTime_AdHoc_Computed == iTime_Input), ("%s: adhoc2 [Computed %s][Expected %s] for [%s]", bufI64_Input, bufI64_AdHoc_Computed, bufI64_Input, SG_string__sz(pStringAdHoc)));

	// the full format with slashes.

	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringAdHoc, "%04d/%02d/%02d %02d:%02d:%02d",
									  timeLoc.year, timeLoc.month, timeLoc.mday, timeLoc.hour, timeLoc.min, timeLoc.sec)  );
	VERIFY_ERR_CHECK(  SG_time__parse__informal__local(pCtx, SG_string__sz(pStringAdHoc), &iTime_AdHoc_Computed, SG_FALSE)  );
	SG_int64_to_sz(iTime_AdHoc_Computed, bufI64_AdHoc_Computed);
	VERIFYP_COND(bufI64_Input, (iTime_AdHoc_Computed == iTime_Input), ("%s: adhoc3 [Computed %s][Expected %s] for [%s]", bufI64_Input, bufI64_AdHoc_Computed, bufI64_Input, SG_string__sz(pStringAdHoc)));

	// again test parse on delimiters rather than fixed length fields.

	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringAdHoc, "%d/%d/%d %d:%d:%d",
									  timeLoc.year, timeLoc.month, timeLoc.mday, timeLoc.hour, timeLoc.min, timeLoc.sec)  );
	VERIFY_ERR_CHECK(  SG_time__parse__informal__local(pCtx, SG_string__sz(pStringAdHoc), &iTime_AdHoc_Computed, SG_FALSE)  );
	SG_int64_to_sz(iTime_AdHoc_Computed, bufI64_AdHoc_Computed);
	VERIFYP_COND(bufI64_Input, (iTime_AdHoc_Computed == iTime_Input), ("%s: adhoc4 [Computed %s][Expected %s] for [%s]", bufI64_Input, bufI64_AdHoc_Computed, bufI64_Input, SG_string__sz(pStringAdHoc)));

	// try parsing the ad hoc format using just the date.

	// MIN should be equivalent to "YYYY/MM/DD 00:00:00".

	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringAdHoc, "%04d-%02d-%02d",
									  timeLoc.year, timeLoc.month, timeLoc.mday)  );
	VERIFY_ERR_CHECK(  SG_time__parse__informal__local(pCtx, SG_string__sz(pStringAdHoc), &iTime_AdHoc_Computed_Min, SG_FALSE)  );
	SG_int64_to_sz(iTime_AdHoc_Computed_Min, bufI64_AdHoc_Computed_Min);
	iTime_Expected_Min = iTime_Input - ((timeLoc.hour * 3600 + timeLoc.min * 60 + timeLoc.sec) * 1000);
	SG_int64_to_sz(iTime_Expected_Min, bufI64_Expected_Min);
	VERIFYP_COND(bufI64_Input, (iTime_AdHoc_Computed_Min == iTime_Expected_Min), ("%s: adhoc min [Computed %s][Expected %s] for [%s]", bufI64_Input, bufI64_AdHoc_Computed_Min, bufI64_Expected_Min, SG_string__sz(pStringAdHoc)));

	// MAX should be equivalent to "YYYY/MM/DD 23:59:59".

	VERIFY_ERR_CHECK(  SG_time__parse__informal__local(pCtx, SG_string__sz(pStringAdHoc), &iTime_AdHoc_Computed_Max, SG_TRUE )  );
	SG_int64_to_sz(iTime_AdHoc_Computed_Max, bufI64_AdHoc_Computed_Max);
	iTime_Expected_Max = iTime_Expected_Min + (24 * 3600000 - 1);
	SG_int64_to_sz(iTime_Expected_Max, bufI64_Expected_Max);
	VERIFYP_COND(bufI64_Input, (iTime_AdHoc_Computed_Max == iTime_Expected_Max), ("%s: adhoc max [Computed %s][Expected %s] for [%s]", bufI64_Input, bufI64_AdHoc_Computed_Max, bufI64_Expected_Max, SG_string__sz(pStringAdHoc)));

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("       Min: %s\n"
								"       Max: %s\n"),
							   bufI64_AdHoc_Computed_Min,
							   bufI64_AdHoc_Computed_Max)  );

	// Take the computed MIN/MAX iTimes and map them back to SG_times and formatted strings.

	VERIFY_ERR_CHECK(  SG_time__decode__local(pCtx, iTime_AdHoc_Computed_Min, &timeLoc_Min)  );
	VERIFY_ERR_CHECK(  SG_time__decode__local(pCtx, iTime_AdHoc_Computed_Max, &timeLoc_Max)  );

	VERIFY_ERR_CHECK(  SG_time__format_local__i64(pCtx, iTime_AdHoc_Computed_Min, bufLoc_Computed_Min, SG_NrElements(bufLoc_Computed_Min))  );
	VERIFY_ERR_CHECK(  SG_time__format_local__i64(pCtx, iTime_AdHoc_Computed_Max, bufLoc_Computed_Max, SG_NrElements(bufLoc_Computed_Max))  );

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   ("   Loc Min: %s\n"
								"   Loc Max: %s\n"
								"Loc Min tm: %04d/%02d/%02d %02d:%02d:%02d.%03d %d %03d %d\n"
								"Loc Max tm: %04d/%02d/%02d %02d:%02d:%02d.%03d %d %03d %d\n"),
							   bufLoc_Computed_Min,
							   bufLoc_Computed_Max,
							   timeLoc_Min.year,timeLoc_Min.month,timeLoc_Min.mday,
							   timeLoc_Min.hour,timeLoc_Min.min,timeLoc_Min.sec,timeLoc_Min.msec,
							   timeLoc_Min.wday,timeLoc_Min.yday,timeLoc_Min.offsetUTC,
							   timeLoc_Max.year,timeLoc_Max.month,timeLoc_Max.mday,
							   timeLoc_Max.hour,timeLoc_Max.min,timeLoc_Max.sec,timeLoc_Max.msec,
							   timeLoc_Max.wday,timeLoc_Max.yday,timeLoc_Max.offsetUTC)  );

	VERIFYP_COND(bufI64_Input, ((   timeLoc_Min.year  == timeLoc.year)
								&& (timeLoc_Min.month == timeLoc.month)
								&& (timeLoc_Min.mday  == timeLoc.mday)
								&& (timeLoc_Min.hour  == 0)
								&& (timeLoc_Min.min   == 0)
								&& (timeLoc_Min.sec   == 0)
								&& (timeLoc_Min.msec  == 0)),
				 ("%s: adhoc min check [Computed %04d/%02d/%02d %02d:%02d:%02d.%03d][Expected %04d/%02d/%02d 00:00:00.000]",
				  bufI64_Input,
				  timeLoc_Min.year,timeLoc_Min.month,timeLoc_Min.mday,
				  timeLoc_Min.hour,timeLoc_Min.min,timeLoc_Min.sec,timeLoc_Min.msec,
				  timeUTC.year,timeUTC.month,timeUTC.mday));
	VERIFYP_COND(bufI64_Input, ((   timeLoc_Max.year  == timeLoc.year)
								&& (timeLoc_Max.month == timeLoc.month)
								&& (timeLoc_Max.mday  == timeLoc.mday)
								&& (timeLoc_Max.hour  == 23)
								&& (timeLoc_Max.min   == 59)
								&& (timeLoc_Max.sec   == 59)
								&& (timeLoc_Max.msec  == 999)),
				 ("%s: adhoc min check [Computed %04d/%02d/%02d %02d:%02d:%02d.%03d][Expected %04d/%02d/%02d 23:59:59.999]",
				  bufI64_Input,
				  timeLoc_Max.year,timeLoc_Max.month,timeLoc_Max.mday,
				  timeLoc_Max.hour,timeLoc_Max.min,timeLoc_Max.sec,timeLoc_Max.msec,
				  timeUTC.year,timeUTC.month,timeUTC.mday));

fail:
	SG_STRING_NULLFREE(pCtx, pStringAdHoc);
}



MyMain()
{
	TEMPLATE_MAIN_START;

#if GEN_DATA
	BEGIN_TEST(  MyFn(test__gen_data)(pCtx)  );
#endif

	BEGIN_TEST(  MyFn(test__sanity)(pCtx, 1268748673000LL, "2010/03/16 14:11:13.000 +0000", "Tue, 16 Mar 2010 14:11:13 GMT")  );
	BEGIN_TEST(  MyFn(test__sanity)(pCtx, 1267837323000LL, "2010/03/06 01:02:03.000 +0000", "Sat, 06 Mar 2010 01:02:03 GMT")  );
	BEGIN_TEST(  MyFn(test__sanity)(pCtx, 1293840000000LL, "2011/01/01 00:00:00.000 +0000", "Sat, 01 Jan 2011 00:00:00 GMT")  );
	BEGIN_TEST(  MyFn(test__sanity)(pCtx, 1293883200000LL, "2011/01/01 12:00:00.000 +0000", "Sat, 01 Jan 2011 12:00:00 GMT")  );
	BEGIN_TEST(  MyFn(test__sanity)(pCtx, 1293926399000LL, "2011/01/01 23:59:59.000 +0000", "Sat, 01 Jan 2011 23:59:59 GMT")  );
	BEGIN_TEST(  MyFn(test__sanity)(pCtx, 1325375999000LL, "2011/12/31 23:59:59.000 +0000", "Sat, 31 Dec 2011 23:59:59 GMT")  );
	BEGIN_TEST(  MyFn(test__sanity)(pCtx, 1309478401000LL, "2011/07/01 00:00:01.000 +0000", "Fri, 01 Jul 2011 00:00:01 GMT")  );
	BEGIN_TEST(  MyFn(test__sanity)(pCtx, 1309521601000LL, "2011/07/01 12:00:01.000 +0000", "Fri, 01 Jul 2011 12:00:01 GMT")  );
	BEGIN_TEST(  MyFn(test__sanity)(pCtx, 1309564799000LL, "2011/07/01 23:59:59.000 +0000", "Fri, 01 Jul 2011 23:59:59 GMT")  );
	BEGIN_TEST(  MyFn(test__sanity)(pCtx, 1309478399000LL, "2011/06/30 23:59:59.000 +0000", "Thu, 30 Jun 2011 23:59:59 GMT")  );


	
	BEGIN_TEST(  MyFn(test__parseRfc850)(pCtx)  );
	BEGIN_TEST(  MyFn(test__parseRfc850nonGmt)(pCtx)  );
	BEGIN_TEST(  MyFn(test__formatRfc850)(pCtx)  );

	TEMPLATE_MAIN_END;
}

#undef MyMain
#undef MyDcl
#undef MyFn
