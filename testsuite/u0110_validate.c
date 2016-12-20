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


/**
 * Unit tests for SG_validate.
 */


/*
 * Helpers for generating failure messages.
 */
#define U0110_CASE_FORMAT "value(%s) invalids(%s) controls(%s) min(%u) max(%u)"
#define U0110_CASE_VALUES(pCase) pCase->szValue, pCase->szInvalids, pCase->bControls == SG_FALSE ? "false" : "true", pCase->uMin, pCase->uMax

/*
 * Helpers for making the test cases more readable.
 */
#define MAX     SG_UINT32_MAX
#define VALID   SG_VALIDATE__RESULT__VALID
#define SHORT   SG_VALIDATE__RESULT__TOO_SHORT
#define LONG    SG_VALIDATE__RESULT__TOO_LONG
#define INVALID SG_VALIDATE__RESULT__INVALID_CHARACTER
#define CONTROL SG_VALIDATE__RESULT__CONTROL_CHARACTER

/**
 * A single test case.
 */
typedef struct
{
	const char*     szValue;      //< The value to validate.
	const char*     szInvalids;   //< Invalid characters.
	SG_bool         bControls;    //< Whether or not control characters are invalid.
	SG_uint32       uMin;         //< Shortest valid length.
	SG_uint32       uMax;         //< Longest valid length.
	const char*     szReplace;    //< String to replace invalid characters with with sanitizing.
	const char*     szAdd;        //< String to add when sanitizing a string that's too short.
	struct
	{
		SG_uint32   uNormal;      //< Expected result of validation.
		SG_uint32   uTrimmed;     //< Expected result of trimmed validation.
	}               cValidate;    //< Collection of validation results.
	struct
	{
		const char* szInvalids;   //< Expected result of sanitizing invalid characters.
		const char* szControls;   //< Expected result of sanitizing control characters.
		const char* szMin;        //< Expected result of sanitizing minimum length.
		const char* szMax;        //< Expected result of sanitizing maximum length.
		const char* szAll;        //< Expected result of sanitizing everything.
		const char* szTrimmedAll; //< Expected result of trimming and sanitizing everything.
	}               cSanitize;    //< Collection of sanitization results.
}
u0110__case;

/**
 * List of test cases to run.
 */
static const u0110__case u0110__cases[] = {
	// edge cases
	{ "", "", SG_FALSE, 0,   0, NULL, "add", { VALID, VALID }, { "", "", "",    "", "",    "" } },
	{ "", "", SG_FALSE, 0,   1, NULL, "add", { VALID, VALID }, { "", "", "",    "", "",    "" } },
	{ "", "", SG_FALSE, 0, MAX, NULL, "add", { VALID, VALID }, { "", "", "",    "", "",    "" } },
	{ "", "", SG_FALSE, 1,   1, NULL, "add", { SHORT, SHORT }, { "", "", "add", "", "a",   "a" } },
	{ "", "", SG_FALSE, 1, MAX, NULL, "add", { SHORT, SHORT }, { "", "", "add", "", "add", "add" } },

	// edge cases, but with NULL invalids list (should work the same)
	{ "", NULL, SG_FALSE, 0,   0, NULL, "add", { VALID, VALID }, { "", "", "",    "", "",    "" } },
	{ "", NULL, SG_FALSE, 0,   1, NULL, "add", { VALID, VALID }, { "", "", "",    "", "",    "" } },
	{ "", NULL, SG_FALSE, 0, MAX, NULL, "add", { VALID, VALID }, { "", "", "",    "", "",    "" } },
	{ "", NULL, SG_FALSE, 1,   1, NULL, "add", { SHORT, SHORT }, { "", "", "add", "", "a",   "a" } },
	{ "", NULL, SG_FALSE, 1, MAX, NULL, "add", { SHORT, SHORT }, { "", "", "add", "", "add", "add" } },

	// edge cases, but with NULL value (should work the same)
	{ NULL, "", SG_FALSE, 0,   0, NULL, "add", { VALID, VALID }, { "", "", "",    "", "",    "" } },
	{ NULL, "", SG_FALSE, 0,   1, NULL, "add", { VALID, VALID }, { "", "", "",    "", "",    "" } },
	{ NULL, "", SG_FALSE, 0, MAX, NULL, "add", { VALID, VALID }, { "", "", "",    "", "",    "" } },
	{ NULL, "", SG_FALSE, 1,   1, NULL, "add", { SHORT, SHORT }, { "", "", "add", "", "a",   "a" } },
	{ NULL, "", SG_FALSE, 1, MAX, NULL, "add", { SHORT, SHORT }, { "", "", "add", "", "add", "add" } },

	// edge cases, but with NULL value and invalids list (should work the same)
	{ NULL, NULL, SG_FALSE, 0,   0, NULL, "add", { VALID, VALID }, { "", "", "",    "", "",    "" } },
	{ NULL, NULL, SG_FALSE, 0,   1, NULL, "add", { VALID, VALID }, { "", "", "",    "", "",    "" } },
	{ NULL, NULL, SG_FALSE, 0, MAX, NULL, "add", { VALID, VALID }, { "", "", "",    "", "",    "" } },
	{ NULL, NULL, SG_FALSE, 1,   1, NULL, "add", { SHORT, SHORT }, { "", "", "add", "", "a",   "a" } },
	{ NULL, NULL, SG_FALSE, 1, MAX, NULL, "add", { SHORT, SHORT }, { "", "", "add", "", "add", "add" } },

	// NULL value with any invalids list should work
	{ NULL, "abc",  SG_FALSE, 0, MAX, NULL, "add", { VALID, VALID }, { "", "", "", "", "", "" } },
	{ NULL, "123",  SG_FALSE, 0, MAX, NULL, "add", { VALID, VALID }, { "", "", "", "", "", "" } },
	{ NULL, "+-*/", SG_FALSE, 0, MAX, NULL, "add", { VALID, VALID }, { "", "", "", "", "", "" } },

	// range checking
	{ "a",          "", SG_FALSE,  0,   0, "rep", "add", { LONG,  LONG },  { "a",          "a",          "a",             "",           "",            "" } },
	{ "a",          "", SG_FALSE,  0,   1, "rep", "add", { VALID, VALID }, { "a",          "a",          "a",             "a",          "a",           "a" } },
	{ "a",          "", SG_FALSE,  1,   1, "rep", "add", { VALID, VALID }, { "a",          "a",          "a",             "a",          "a",           "a" } },
	{ "a",          "", SG_FALSE,  1,   2, "rep", "add", { VALID, VALID }, { "a",          "a",          "a",             "a",          "a",           "a" } },
	{ "a",          "", SG_FALSE,  2,   2, "rep", "add", { SHORT, SHORT }, { "a",          "a",          "aadd",          "a",          "aa",          "aa" } },
	{ "a",          "", SG_FALSE,  0, MAX, "rep", "add", { VALID, VALID }, { "a",          "a",          "a",             "a",          "a",           "a" } },
	{ "0123456789", "", SG_FALSE,  9,   9, "rep", "add", { LONG,  LONG },  { "0123456789", "0123456789", "0123456789",    "012345678",  "012345678",   "012345678" } },
	{ "0123456789", "", SG_FALSE,  9,  10, "rep", "add", { VALID, VALID }, { "0123456789", "0123456789", "0123456789",    "0123456789", "0123456789",  "0123456789" } },
	{ "0123456789", "", SG_FALSE, 10,  10, "rep", "add", { VALID, VALID }, { "0123456789", "0123456789", "0123456789",    "0123456789", "0123456789",  "0123456789" } },
	{ "0123456789", "", SG_FALSE, 10,  11, "rep", "add", { VALID, VALID }, { "0123456789", "0123456789", "0123456789",    "0123456789", "0123456789",  "0123456789" } },
	{ "0123456789", "", SG_FALSE, 11,  11, "rep", "add", { SHORT, SHORT }, { "0123456789", "0123456789", "0123456789add", "0123456789", "0123456789a", "0123456789a" } },
	{ "0123456789", "", SG_FALSE,  0, MAX, "rep", "add", { VALID, VALID }, { "0123456789", "0123456789", "0123456789",    "0123456789", "0123456789",  "0123456789" } },

	// range checking, but with NULL invalids list (should work the same)
	{ "a",          NULL, SG_FALSE,  0,   0, "rep", "add", { LONG,  LONG },  { "a",          "a",          "a",             "",            "",           "" } },
	{ "a",          NULL, SG_FALSE,  0,   1, "rep", "add", { VALID, VALID }, { "a",          "a",          "a",             "a",           "a",          "a" } },
	{ "a",          NULL, SG_FALSE,  1,   1, "rep", "add", { VALID, VALID }, { "a",          "a",          "a",             "a",           "a",          "a" } },
	{ "a",          NULL, SG_FALSE,  1,   2, "rep", "add", { VALID, VALID }, { "a",          "a",          "a",             "a",           "a",          "a" } },
	{ "a",          NULL, SG_FALSE,  2,   2, "rep", "add", { SHORT, SHORT }, { "a",          "a",          "aadd",          "a",           "aa",         "aa" } },
	{ "a",          NULL, SG_FALSE,  0, MAX, "rep", "add", { VALID, VALID }, { "a",          "a",          "a",             "a",           "a",          "a" } },
	{ "0123456789", NULL, SG_FALSE,  9,   9, "rep", "add", { LONG,  LONG },  { "0123456789", "0123456789", "0123456789",    "012345678",  "012345678",   "012345678" } },
	{ "0123456789", NULL, SG_FALSE,  9,  10, "rep", "add", { VALID, VALID }, { "0123456789", "0123456789", "0123456789",    "0123456789", "0123456789",  "0123456789" } },
	{ "0123456789", NULL, SG_FALSE, 10,  10, "rep", "add", { VALID, VALID }, { "0123456789", "0123456789", "0123456789",    "0123456789", "0123456789",  "0123456789" } },
	{ "0123456789", NULL, SG_FALSE, 10,  11, "rep", "add", { VALID, VALID }, { "0123456789", "0123456789", "0123456789",    "0123456789", "0123456789",  "0123456789" } },
	{ "0123456789", NULL, SG_FALSE, 11,  11, "rep", "add", { SHORT, SHORT }, { "0123456789", "0123456789", "0123456789add", "0123456789", "0123456789a", "0123456789a" } },
	{ "0123456789", NULL, SG_FALSE,  0, MAX, "rep", "add", { VALID, VALID }, { "0123456789", "0123456789", "0123456789",    "0123456789", "0123456789",  "0123456789" } },

	// single character checks
	{ "abc123",        "+", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc123",                      "abc123",        "abc123",        "abc123",        "abc123",                      "abc123" } },
	{ "abc+123",       "+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123",                   "abc+123",       "abc+123",       "abc+123",       "abcrep123",                   "abcrep123" } },
	{ "+abc123",       "+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123",                   "+abc123",       "+abc123",       "+abc123",       "repabc123",                   "repabc123" } },
	{ "abc123+",       "+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abc123rep",                   "abc123+",       "abc123+",       "abc123+",       "abc123rep",                   "abc123rep" } },
	{ "+abc123+",      "+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123rep",                "+abc123+",      "+abc123+",      "+abc123+",      "repabc123rep",                "repabc123rep" } },
	{ "+abc+123+",     "+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabcrep123rep",             "+abc+123+",     "+abc+123+",     "+abc+123+",     "repabcrep123rep",             "repabcrep123rep" } },
	{ "+a+b+c+1+2+3+", "+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "reparepbrepcrep1rep2rep3rep", "+a+b+c+1+2+3+", "+a+b+c+1+2+3+", "+a+b+c+1+2+3+", "reparepbrepcrep1rep2rep3rep", "reparepbrepcrep1rep2rep3rep" } },
	{ "+++++++",       "+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repreprepreprepreprep",       "+++++++",       "+++++++",       "+++++++",       "repreprepreprepreprep",       "repreprepreprepreprep" } },

	// single character checks, but with empty replacement
	{ "abc123",        "+", SG_FALSE, 0, MAX, "", "add", { VALID,   VALID },   { "abc123", "abc123",        "abc123",        "abc123",        "abc123", "abc123" } },
	{ "abc+123",       "+", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "abc+123",       "abc+123",       "abc+123",       "abc123", "abc123" } },
	{ "+abc123",       "+", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "+abc123",       "+abc123",       "+abc123",       "abc123", "abc123" } },
	{ "abc123+",       "+", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "abc123+",       "abc123+",       "abc123+",       "abc123", "abc123" } },
	{ "+abc123+",      "+", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "+abc123+",      "+abc123+",      "+abc123+",      "abc123", "abc123" } },
	{ "+abc+123+",     "+", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "+abc+123+",     "+abc+123+",     "+abc+123+",     "abc123", "abc123" } },
	{ "+a+b+c+1+2+3+", "+", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "+a+b+c+1+2+3+", "+a+b+c+1+2+3+", "+a+b+c+1+2+3+", "abc123", "abc123" } },
	{ "+++++++",       "+", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "",       "+++++++",       "+++++++",       "+++++++",       "",       "" } },

	// multiple character checks
	{ "abc123",        "+-*/", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc123",                      "abc123",        "abc123",        "abc123",        "abc123",                      "abc123" } },
	{ "abc+123",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123",                   "abc+123",       "abc+123",       "abc+123",       "abcrep123",                   "abcrep123" } },
	{ "+abc123",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123",                   "+abc123",       "+abc123",       "+abc123",       "repabc123",                   "repabc123" } },
	{ "abc123+",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abc123rep",                   "abc123+",       "abc123+",       "abc123+",       "abc123rep",                   "abc123rep" } },
	{ "+abc123+",      "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123rep",                "+abc123+",      "+abc123+",      "+abc123+",      "repabc123rep",                "repabc123rep" } },
	{ "+abc+123+",     "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabcrep123rep",             "+abc+123+",     "+abc+123+",     "+abc+123+",     "repabcrep123rep",             "repabcrep123rep" } },
	{ "+a+b+c+1+2+3+", "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "reparepbrepcrep1rep2rep3rep", "+a+b+c+1+2+3+", "+a+b+c+1+2+3+", "+a+b+c+1+2+3+", "reparepbrepcrep1rep2rep3rep", "reparepbrepcrep1rep2rep3rep" } },
	{ "+++++++",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repreprepreprepreprep",       "+++++++",       "+++++++",       "+++++++",       "repreprepreprepreprep",       "repreprepreprepreprep" } },
	{ "abc123",        "+-*/", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc123",                      "abc123",        "abc123",        "abc123",        "abc123",                      "abc123" } },
	{ "abc-123",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123",                   "abc-123",       "abc-123",       "abc-123",       "abcrep123",                   "abcrep123" } },
	{ "-abc123",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123",                   "-abc123",       "-abc123",       "-abc123",       "repabc123",                   "repabc123" } },
	{ "abc123-",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abc123rep",                   "abc123-",       "abc123-",       "abc123-",       "abc123rep",                   "abc123rep" } },
	{ "-abc123-",      "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123rep",                "-abc123-",      "-abc123-",      "-abc123-",      "repabc123rep",                "repabc123rep" } },
	{ "-abc-123-",     "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabcrep123rep",             "-abc-123-",     "-abc-123-",     "-abc-123-",     "repabcrep123rep",             "repabcrep123rep" } },
	{ "-a-b-c-1-2-3-", "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "reparepbrepcrep1rep2rep3rep", "-a-b-c-1-2-3-", "-a-b-c-1-2-3-", "-a-b-c-1-2-3-", "reparepbrepcrep1rep2rep3rep", "reparepbrepcrep1rep2rep3rep" } },
	{ "-------",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repreprepreprepreprep",       "-------",       "-------",       "-------",       "repreprepreprepreprep",       "repreprepreprepreprep" } },
	{ "abc123",        "+-*/", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc123",                      "abc123",        "abc123",        "abc123",        "abc123",                      "abc123" } },
	{ "abc*123",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123",                   "abc*123",       "abc*123",       "abc*123",       "abcrep123",                   "abcrep123" } },
	{ "*abc123",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123",                   "*abc123",       "*abc123",       "*abc123",       "repabc123",                   "repabc123" } },
	{ "abc123*",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abc123rep",                   "abc123*",       "abc123*",       "abc123*",       "abc123rep",                   "abc123rep" } },
	{ "*abc123*",      "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123rep",                "*abc123*",      "*abc123*",      "*abc123*",      "repabc123rep",                "repabc123rep" } },
	{ "*abc*123*",     "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabcrep123rep",             "*abc*123*",     "*abc*123*",     "*abc*123*",     "repabcrep123rep",             "repabcrep123rep" } },
	{ "*a*b*c*1*2*3*", "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "reparepbrepcrep1rep2rep3rep", "*a*b*c*1*2*3*", "*a*b*c*1*2*3*", "*a*b*c*1*2*3*", "reparepbrepcrep1rep2rep3rep", "reparepbrepcrep1rep2rep3rep" } },
	{ "*******",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repreprepreprepreprep",       "*******",       "*******",       "*******",       "repreprepreprepreprep",       "repreprepreprepreprep" } },
	{ "abc123",        "+-*/", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc123",                      "abc123",        "abc123",        "abc123",        "abc123",                      "abc123" } },
	{ "abc/123",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123",                   "abc/123",       "abc/123",       "abc/123",       "abcrep123",                   "abcrep123" } },
	{ "/abc123",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123",                   "/abc123",       "/abc123",       "/abc123",       "repabc123",                   "repabc123" } },
	{ "abc123/",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abc123rep",                   "abc123/",       "abc123/",       "abc123/",       "abc123rep",                   "abc123rep" } },
	{ "/abc123/",      "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123rep",                "/abc123/",      "/abc123/",      "/abc123/",      "repabc123rep",                "repabc123rep" } },
	{ "/abc/123/",     "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabcrep123rep",             "/abc/123/",     "/abc/123/",     "/abc/123/",     "repabcrep123rep",             "repabcrep123rep" } },
	{ "/a/b/c/1/2/3/", "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "reparepbrepcrep1rep2rep3rep", "/a/b/c/1/2/3/", "/a/b/c/1/2/3/", "/a/b/c/1/2/3/", "reparepbrepcrep1rep2rep3rep", "reparepbrepcrep1rep2rep3rep" } },
	{ "///////",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repreprepreprepreprep",       "///////",       "///////",       "///////",       "repreprepreprepreprep",       "repreprepreprepreprep" } },

	// multiple character checks, but with empty replacement
	{ "abc123",        "+-*/", SG_FALSE, 0, MAX, "", "add", { VALID,   VALID },   { "abc123", "abc123",        "abc123",        "abc123",        "abc123", "abc123" } },
	{ "abc+123",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "abc+123",       "abc+123",       "abc+123",       "abc123", "abc123" } },
	{ "+abc123",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "+abc123",       "+abc123",       "+abc123",       "abc123", "abc123" } },
	{ "abc123+",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "abc123+",       "abc123+",       "abc123+",       "abc123", "abc123" } },
	{ "+abc123+",      "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "+abc123+",      "+abc123+",      "+abc123+",      "abc123", "abc123" } },
	{ "+abc+123+",     "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "+abc+123+",     "+abc+123+",     "+abc+123+",     "abc123", "abc123" } },
	{ "+a+b+c+1+2+3+", "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "+a+b+c+1+2+3+", "+a+b+c+1+2+3+", "+a+b+c+1+2+3+", "abc123", "abc123" } },
	{ "+++++++",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "",       "+++++++",       "+++++++",       "+++++++",       "",       "" } },
	{ "abc123",        "+-*/", SG_FALSE, 0, MAX, "", "add", { VALID,   VALID },   { "abc123", "abc123",        "abc123",        "abc123",        "abc123", "abc123" } },
	{ "abc-123",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "abc-123",       "abc-123",       "abc-123",       "abc123", "abc123" } },
	{ "-abc123",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "-abc123",       "-abc123",       "-abc123",       "abc123", "abc123" } },
	{ "abc123-",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "abc123-",       "abc123-",       "abc123-",       "abc123", "abc123" } },
	{ "-abc123-",      "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "-abc123-",      "-abc123-",      "-abc123-",      "abc123", "abc123" } },
	{ "-abc-123-",     "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "-abc-123-",     "-abc-123-",     "-abc-123-",     "abc123", "abc123" } },
	{ "-a-b-c-1-2-3-", "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "-a-b-c-1-2-3-", "-a-b-c-1-2-3-", "-a-b-c-1-2-3-", "abc123", "abc123" } },
	{ "-------",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "",       "-------",       "-------",       "-------",       "",       "" } },
	{ "abc123",        "+-*/", SG_FALSE, 0, MAX, "", "add", { VALID,   VALID },   { "abc123", "abc123",        "abc123",        "abc123",        "abc123", "abc123" } },
	{ "abc*123",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "abc*123",       "abc*123",       "abc*123",       "abc123", "abc123" } },
	{ "*abc123",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "*abc123",       "*abc123",       "*abc123",       "abc123", "abc123" } },
	{ "abc123*",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "abc123*",       "abc123*",       "abc123*",       "abc123", "abc123" } },
	{ "*abc123*",      "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "*abc123*",      "*abc123*",      "*abc123*",      "abc123", "abc123" } },
	{ "*abc*123*",     "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "*abc*123*",     "*abc*123*",     "*abc*123*",     "abc123", "abc123" } },
	{ "*a*b*c*1*2*3*", "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "*a*b*c*1*2*3*", "*a*b*c*1*2*3*", "*a*b*c*1*2*3*", "abc123", "abc123" } },
	{ "*******",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "",       "*******",       "*******",       "*******",       "",       "" } },
	{ "abc123",        "+-*/", SG_FALSE, 0, MAX, "", "add", { VALID,   VALID },   { "abc123", "abc123",        "abc123",        "abc123",        "abc123", "abc123" } },
	{ "abc/123",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "abc/123",       "abc/123",       "abc/123",       "abc123", "abc123" } },
	{ "/abc123",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "/abc123",       "/abc123",       "/abc123",       "abc123", "abc123" } },
	{ "abc123/",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "abc123/",       "abc123/",       "abc123/",       "abc123", "abc123" } },
	{ "/abc123/",      "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "/abc123/",      "/abc123/",      "/abc123/",      "abc123", "abc123" } },
	{ "/abc/123/",     "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "/abc/123/",     "/abc/123/",     "/abc/123/",     "abc123", "abc123" } },
	{ "/a/b/c/1/2/3/", "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123", "/a/b/c/1/2/3/", "/a/b/c/1/2/3/", "/a/b/c/1/2/3/", "abc123", "abc123" } },
	{ "///////",       "+-*/", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "",       "///////",       "///////",       "///////",       "",       "" } },

	// multiple character checks, different order of invalid characters
	{ "abc123",        "/*-+", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc123",                      "abc123",        "abc123",        "abc123",        "abc123",                      "abc123" } },
	{ "abc+123",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123",                   "abc+123",       "abc+123",       "abc+123",       "abcrep123",                   "abcrep123" } },
	{ "+abc123",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123",                   "+abc123",       "+abc123",       "+abc123",       "repabc123",                   "repabc123" } },
	{ "abc123+",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abc123rep",                   "abc123+",       "abc123+",       "abc123+",       "abc123rep",                   "abc123rep" } },
	{ "+abc123+",      "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123rep",                "+abc123+",      "+abc123+",      "+abc123+",      "repabc123rep",                "repabc123rep" } },
	{ "+abc+123+",     "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabcrep123rep",             "+abc+123+",     "+abc+123+",     "+abc+123+",     "repabcrep123rep",             "repabcrep123rep" } },
	{ "+a+b+c+1+2+3+", "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "reparepbrepcrep1rep2rep3rep", "+a+b+c+1+2+3+", "+a+b+c+1+2+3+", "+a+b+c+1+2+3+", "reparepbrepcrep1rep2rep3rep", "reparepbrepcrep1rep2rep3rep" } },
	{ "+++++++",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repreprepreprepreprep",       "+++++++",       "+++++++",       "+++++++",       "repreprepreprepreprep",       "repreprepreprepreprep" } },
	{ "abc123",        "/*-+", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc123",                      "abc123",        "abc123",        "abc123",        "abc123",                      "abc123" } },
	{ "abc-123",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123",                   "abc-123",       "abc-123",       "abc-123",       "abcrep123",                   "abcrep123" } },
	{ "-abc123",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123",                   "-abc123",       "-abc123",       "-abc123",       "repabc123",                   "repabc123" } },
	{ "abc123-",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abc123rep",                   "abc123-",       "abc123-",       "abc123-",       "abc123rep",                   "abc123rep" } },
	{ "-abc123-",      "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123rep",                "-abc123-",      "-abc123-",      "-abc123-",      "repabc123rep",                "repabc123rep" } },
	{ "-abc-123-",     "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabcrep123rep",             "-abc-123-",     "-abc-123-",     "-abc-123-",     "repabcrep123rep",             "repabcrep123rep" } },
	{ "-a-b-c-1-2-3-", "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "reparepbrepcrep1rep2rep3rep", "-a-b-c-1-2-3-", "-a-b-c-1-2-3-", "-a-b-c-1-2-3-", "reparepbrepcrep1rep2rep3rep", "reparepbrepcrep1rep2rep3rep" } },
	{ "-------",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repreprepreprepreprep",       "-------",       "-------",       "-------",       "repreprepreprepreprep",       "repreprepreprepreprep" } },
	{ "abc123",        "/*-+", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc123",                      "abc123",        "abc123",        "abc123",        "abc123",                      "abc123" } },
	{ "abc*123",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123",                   "abc*123",       "abc*123",       "abc*123",       "abcrep123",                   "abcrep123" } },
	{ "*abc123",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123",                   "*abc123",       "*abc123",       "*abc123",       "repabc123",                   "repabc123" } },
	{ "abc123*",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abc123rep",                   "abc123*",       "abc123*",       "abc123*",       "abc123rep",                   "abc123rep" } },
	{ "*abc123*",      "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123rep",                "*abc123*",      "*abc123*",      "*abc123*",      "repabc123rep",                "repabc123rep" } },
	{ "*abc*123*",     "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabcrep123rep",             "*abc*123*",     "*abc*123*",     "*abc*123*",     "repabcrep123rep",             "repabcrep123rep" } },
	{ "*a*b*c*1*2*3*", "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "reparepbrepcrep1rep2rep3rep", "*a*b*c*1*2*3*", "*a*b*c*1*2*3*", "*a*b*c*1*2*3*", "reparepbrepcrep1rep2rep3rep", "reparepbrepcrep1rep2rep3rep" } },
	{ "*******",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repreprepreprepreprep",       "*******",       "*******",       "*******",       "repreprepreprepreprep",       "repreprepreprepreprep" } },
	{ "abc123",        "/*-+", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc123",                      "abc123",        "abc123",        "abc123",        "abc123",                      "abc123" } },
	{ "abc/123",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123",                   "abc/123",       "abc/123",       "abc/123",       "abcrep123",                   "abcrep123" } },
	{ "/abc123",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123",                   "/abc123",       "/abc123",       "/abc123",       "repabc123",                   "repabc123" } },
	{ "abc123/",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abc123rep",                   "abc123/",       "abc123/",       "abc123/",       "abc123rep",                   "abc123rep" } },
	{ "/abc123/",      "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabc123rep",                "/abc123/",      "/abc123/",      "/abc123/",      "repabc123rep",                "repabc123rep" } },
	{ "/abc/123/",     "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repabcrep123rep",             "/abc/123/",     "/abc/123/",     "/abc/123/",     "repabcrep123rep",             "repabcrep123rep" } },
	{ "/a/b/c/1/2/3/", "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "reparepbrepcrep1rep2rep3rep", "/a/b/c/1/2/3/", "/a/b/c/1/2/3/", "/a/b/c/1/2/3/", "reparepbrepcrep1rep2rep3rep", "reparepbrepcrep1rep2rep3rep" } },
	{ "///////",       "/*-+", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "repreprepreprepreprep",       "///////",       "///////",       "///////",       "repreprepreprepreprep",       "repreprepreprepreprep" } },

	// multiple character and range checks
	{ "abc123",       "+-*/", SG_FALSE, 8, 10, "rep", "add", { SHORT,           SHORT },           { "abc123",               "abc123",       "abc123add",    "abc123",     "abc123add",  "abc123add" } },
	{ "a+*1-/",       "+-*/", SG_FALSE, 8, 10, "rep", "add", { SHORT | INVALID, SHORT | INVALID }, { "areprep1reprep",       "a+*1-/",       "a+*1-/add",    "a+*1-/",     "areprep1re", "areprep1re" } },
	{ "abcd1234",     "+-*/", SG_FALSE, 8, 10, "rep", "add", { VALID,           VALID },           { "abcd1234",             "abcd1234",     "abcd1234",     "abcd1234",   "abcd1234",   "abcd1234" } },
	{ "ab+*12-/",     "+-*/", SG_FALSE, 8, 10, "rep", "add", { INVALID,         INVALID },         { "abreprep12reprep",     "ab+*12-/",     "ab+*12-/",     "ab+*12-/",   "abreprep12", "abreprep12" } },
	{ "abcde12345",   "+-*/", SG_FALSE, 8, 10, "rep", "add", { VALID,           VALID },           { "abcde12345",           "abcde12345",   "abcde12345",   "abcde12345", "abcde12345", "abcde12345" } },
	{ "abc+*123-/",   "+-*/", SG_FALSE, 8, 10, "rep", "add", { INVALID,         INVALID },         { "abcreprep123reprep",   "abc+*123-/",   "abc+*123-/",   "abc+*123-/", "abcreprep1", "abcreprep1" } },
	{ "abcdef123456", "+-*/", SG_FALSE, 8, 10, "rep", "add", { LONG,            LONG },            { "abcdef123456",         "abcdef123456", "abcdef123456", "abcdef1234", "abcdef1234", "abcdef1234" } },
	{ "abcd+*1234-/", "+-*/", SG_FALSE, 8, 10, "rep", "add", { LONG | INVALID,  LONG | INVALID },  { "abcdreprep1234reprep", "abcd+*1234-/", "abcd+*1234-/", "abcd+*1234", "abcdreprep", "abcdreprep" } },

	// contains valid whitespace
	{ "abc   123",          "+-*/", SG_FALSE, 0, MAX, "rep", "add", { VALID, VALID }, { "abc   123",          "abc   123",               "abc   123",          "abc   123",          "abc   123",               "abc   123" } },
	{ "abc \t\r\n\r\t 123", "+-*/", SG_FALSE, 0, MAX, "rep", "add", { VALID, VALID }, { "abc \t\r\n\r\t 123", "abc repreprepreprep 123", "abc \t\r\n\r\t 123", "abc \t\r\n\r\t 123", "abc repreprepreprep 123", "abc repreprepreprep 123" } },

	// contains invalid whitespace
	{ "abc   123",          " \t\r\n", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrepreprep123",             "abc   123",               "abc   123",          "abc   123",          "abcrepreprep123",             "abcrepreprep123" } },
	{ "abc \t\r\n\r\t 123", " \t\r\n", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrepreprepreprepreprep123", "abc repreprepreprep 123", "abc \t\r\n\r\t 123", "abc \t\r\n\r\t 123", "abcrepreprepreprepreprep123", "abcrepreprepreprepreprep123" } },

	// starts/ends with valid whitespace
	{ "   abc123   ",       "+-*/", SG_FALSE, 0, MAX, "rep", "add", { VALID, VALID }, { "   abc123   ",       "   abc123   ",             "   abc123   ",       "   abc123   ",       "   abc123   ",             "abc123" } },
	{ "\t\t\tabc123\n\n\n", "+-*/", SG_FALSE, 0, MAX, "rep", "add", { VALID, VALID }, { "\t\t\tabc123\n\n\n", "repreprepabc123repreprep", "\t\t\tabc123\n\n\n", "\t\t\tabc123\n\n\n", "repreprepabc123repreprep", "abc123" } },

	// starts/ends with invalid whitespace
	// these will fail with the non-trimming check and succeed with the trimming check
	{ "   abc123   ",       " \t\r\n", SG_FALSE, 0, MAX, "rep", "add", { INVALID, VALID }, { "repreprepabc123repreprep", "   abc123   ",             "   abc123   ",       "   abc123   ",       "repreprepabc123repreprep", "abc123" } },
	{ "\t\t\tabc123\n\n\n", " \t\r\n", SG_FALSE, 0, MAX, "rep", "add", { INVALID, VALID }, { "repreprepabc123repreprep", "repreprepabc123repreprep", "\t\t\tabc123\n\n\n", "\t\t\tabc123\n\n\n", "repreprepabc123repreprep", "abc123" } },

	// control character checks
	{ "ijkxyz",     "", SG_TRUE,  0, MAX, "rep", "add", { VALID,   VALID },   { "ijkxyz", "ijkxyz", "ijkxyz", "ijkxyz", "ijkxyz", "ijkxyz" } },
	{ "ijk\x01xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x01xyz", "ijkrepxyz","ijk\x01xyz", "ijk\x01xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x02xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x02xyz", "ijkrepxyz","ijk\x02xyz", "ijk\x02xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x03xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x03xyz", "ijkrepxyz","ijk\x03xyz", "ijk\x03xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x04xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x04xyz", "ijkrepxyz","ijk\x04xyz", "ijk\x04xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x05xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x05xyz", "ijkrepxyz","ijk\x05xyz", "ijk\x05xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x06xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x06xyz", "ijkrepxyz","ijk\x06xyz", "ijk\x06xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x07xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x07xyz", "ijkrepxyz","ijk\x07xyz", "ijk\x07xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x08xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x08xyz", "ijkrepxyz","ijk\x08xyz", "ijk\x08xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x09xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x09xyz", "ijkrepxyz","ijk\x09xyz", "ijk\x09xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x0Axyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x0Axyz", "ijkrepxyz","ijk\x0Axyz", "ijk\x0Axyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x0Bxyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x0Bxyz", "ijkrepxyz","ijk\x0Bxyz", "ijk\x0Bxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x0Cxyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x0Cxyz", "ijkrepxyz","ijk\x0Cxyz", "ijk\x0Cxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x0Dxyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x0Dxyz", "ijkrepxyz","ijk\x0Dxyz", "ijk\x0Dxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x0Exyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x0Exyz", "ijkrepxyz","ijk\x0Exyz", "ijk\x0Exyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x0Fxyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x0Fxyz", "ijkrepxyz","ijk\x0Fxyz", "ijk\x0Fxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x10xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x10xyz", "ijkrepxyz","ijk\x10xyz", "ijk\x10xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x11xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x11xyz", "ijkrepxyz","ijk\x11xyz", "ijk\x11xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x12xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x12xyz", "ijkrepxyz","ijk\x12xyz", "ijk\x12xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x13xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x13xyz", "ijkrepxyz","ijk\x13xyz", "ijk\x13xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x14xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x14xyz", "ijkrepxyz","ijk\x14xyz", "ijk\x14xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x15xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x15xyz", "ijkrepxyz","ijk\x15xyz", "ijk\x15xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x16xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x16xyz", "ijkrepxyz","ijk\x16xyz", "ijk\x16xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x17xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x17xyz", "ijkrepxyz","ijk\x17xyz", "ijk\x17xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x18xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x18xyz", "ijkrepxyz","ijk\x18xyz", "ijk\x18xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x19xyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x19xyz", "ijkrepxyz","ijk\x19xyz", "ijk\x19xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x1Axyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x1Axyz", "ijkrepxyz","ijk\x1Axyz", "ijk\x1Axyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x1Bxyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x1Bxyz", "ijkrepxyz","ijk\x1Bxyz", "ijk\x1Bxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x1Cxyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x1Cxyz", "ijkrepxyz","ijk\x1Cxyz", "ijk\x1Cxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x1Dxyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x1Dxyz", "ijkrepxyz","ijk\x1Dxyz", "ijk\x1Dxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x1Exyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x1Exyz", "ijkrepxyz","ijk\x1Exyz", "ijk\x1Exyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x1Fxyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x1Fxyz", "ijkrepxyz","ijk\x1Fxyz", "ijk\x1Fxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijkxyz\x10", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijkxyz\x10", "ijkxyzrep","ijkxyz\x10", "ijkxyz\x10", "ijkxyzrep", "ijkxyzrep" } },
	{ "\x10ijkxyz", "", SG_TRUE,  0, MAX, "rep", "add", { CONTROL, CONTROL }, { "\x10ijkxyz", "repijkxyz","\x10ijkxyz", "\x10ijkxyz", "repijkxyz", "repijkxyz" } },
	{ "ijk\x01xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x01xyz", "ijkrepxyz","ijk\x01xyz", "ijk\x01xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x02xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x02xyz", "ijkrepxyz","ijk\x02xyz", "ijk\x02xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x03xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x03xyz", "ijkrepxyz","ijk\x03xyz", "ijk\x03xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x04xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x04xyz", "ijkrepxyz","ijk\x04xyz", "ijk\x04xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x05xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x05xyz", "ijkrepxyz","ijk\x05xyz", "ijk\x05xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x06xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x06xyz", "ijkrepxyz","ijk\x06xyz", "ijk\x06xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x07xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x07xyz", "ijkrepxyz","ijk\x07xyz", "ijk\x07xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x08xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x08xyz", "ijkrepxyz","ijk\x08xyz", "ijk\x08xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x09xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x09xyz", "ijkrepxyz","ijk\x09xyz", "ijk\x09xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x0Axyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x0Axyz", "ijkrepxyz","ijk\x0Axyz", "ijk\x0Axyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x0Bxyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x0Bxyz", "ijkrepxyz","ijk\x0Bxyz", "ijk\x0Bxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x0Cxyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x0Cxyz", "ijkrepxyz","ijk\x0Cxyz", "ijk\x0Cxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x0Dxyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x0Dxyz", "ijkrepxyz","ijk\x0Dxyz", "ijk\x0Dxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x0Exyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x0Exyz", "ijkrepxyz","ijk\x0Exyz", "ijk\x0Exyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x0Fxyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x0Fxyz", "ijkrepxyz","ijk\x0Fxyz", "ijk\x0Fxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x10xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x10xyz", "ijkrepxyz","ijk\x10xyz", "ijk\x10xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x11xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x11xyz", "ijkrepxyz","ijk\x11xyz", "ijk\x11xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x12xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x12xyz", "ijkrepxyz","ijk\x12xyz", "ijk\x12xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x13xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x13xyz", "ijkrepxyz","ijk\x13xyz", "ijk\x13xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x14xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x14xyz", "ijkrepxyz","ijk\x14xyz", "ijk\x14xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x15xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x15xyz", "ijkrepxyz","ijk\x15xyz", "ijk\x15xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x16xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x16xyz", "ijkrepxyz","ijk\x16xyz", "ijk\x16xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x17xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x17xyz", "ijkrepxyz","ijk\x17xyz", "ijk\x17xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x18xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x18xyz", "ijkrepxyz","ijk\x18xyz", "ijk\x18xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x19xyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x19xyz", "ijkrepxyz","ijk\x19xyz", "ijk\x19xyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x1Axyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x1Axyz", "ijkrepxyz","ijk\x1Axyz", "ijk\x1Axyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x1Bxyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x1Bxyz", "ijkrepxyz","ijk\x1Bxyz", "ijk\x1Bxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x1Cxyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x1Cxyz", "ijkrepxyz","ijk\x1Cxyz", "ijk\x1Cxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x1Dxyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x1Dxyz", "ijkrepxyz","ijk\x1Dxyz", "ijk\x1Dxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x1Exyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x1Exyz", "ijkrepxyz","ijk\x1Exyz", "ijk\x1Exyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijk\x1Fxyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijk\x1Fxyz", "ijkrepxyz","ijk\x1Fxyz", "ijk\x1Fxyz", "ijkrepxyz", "ijkrepxyz" } },
	{ "ijkxyz\x10", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "ijkxyz\x10", "ijkxyzrep","ijkxyz\x10", "ijkxyz\x10", "ijkxyzrep", "ijkxyzrep" } },
	{ "\x10ijkxyz", "", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "\x10ijkxyz", "repijkxyz","\x10ijkxyz", "\x10ijkxyz", "repijkxyz", "repijkxyz" } },

	// control character and other character checks
	{ "ijk\x10+xyz", "+", SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijk\x10repxyz", "ijkrep+xyz", "ijk\x10+xyz", "ijk\x10+xyz", "ijkreprepxyz", "ijkreprepxyz" } },
	{ "ijk+\x10xyz", "+", SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkrep\x10xyz", "ijk+repxyz", "ijk+\x10xyz", "ijk+\x10xyz", "ijkreprepxyz", "ijkreprepxyz" } },
	{ "ijkxyz\x10+", "+", SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkxyz\x10rep", "ijkxyzrep+", "ijkxyz\x10+", "ijkxyz\x10+", "ijkxyzreprep", "ijkxyzreprep" } },
	{ "ijkxyz+\x10", "+", SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkxyzrep\x10", "ijkxyz+rep", "ijkxyz+\x10", "ijkxyz+\x10", "ijkxyzreprep", "ijkxyzreprep" } },
	{ "\x10+ijkxyz", "+", SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "\x10repijkxyz", "rep+ijkxyz", "\x10+ijkxyz", "\x10+ijkxyz", "reprepijkxyz", "reprepijkxyz" } },
	{ "+\x10ijkxyz", "+", SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "rep\x10ijkxyz", "+repijkxyz", "+\x10ijkxyz", "+\x10ijkxyz", "reprepijkxyz", "reprepijkxyz" } },

	// control character and other character checks, but with empty replacement
	{ "ijk\x10+xyz", "+", SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijk\x10xyz", "ijk+xyz", "ijk\x10+xyz", "ijk\x10+xyz", "ijkxyz", "ijkxyz" } },
	{ "ijk+\x10xyz", "+", SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijk\x10xyz", "ijk+xyz", "ijk+\x10xyz", "ijk+\x10xyz", "ijkxyz", "ijkxyz" } },
	{ "ijkxyz\x10+", "+", SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkxyz\x10", "ijkxyz+", "ijkxyz\x10+", "ijkxyz\x10+", "ijkxyz", "ijkxyz" } },
	{ "ijkxyz+\x10", "+", SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkxyz\x10", "ijkxyz+", "ijkxyz+\x10", "ijkxyz+\x10", "ijkxyz", "ijkxyz" } },
	{ "\x10+ijkxyz", "+", SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "\x10ijkxyz", "+ijkxyz", "\x10+ijkxyz", "\x10+ijkxyz", "ijkxyz", "ijkxyz" } },
	{ "+\x10ijkxyz", "+", SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "\x10ijkxyz", "+ijkxyz", "+\x10ijkxyz", "+\x10ijkxyz", "ijkxyz", "ijkxyz" } },

	// multiple control character checks
	{ "ijk\x10\x13xyz", "", SG_TRUE, 0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijk\x10\x13xyz", "ijkreprepxyz", "ijk\x10\x13xyz", "ijk\x10\x13xyz", "ijkreprepxyz", "ijkreprepxyz" } },
	{ "ijkxyz\x10\x13", "", SG_TRUE, 0, MAX, "rep", "add", { CONTROL, CONTROL }, { "ijkxyz\x10\x13", "ijkxyzreprep", "ijkxyz\x10\x13", "ijkxyz\x10\x13", "ijkxyzreprep", "ijkxyzreprep" } },
	{ "\x10\x13ijkxyz", "", SG_TRUE, 0, MAX, "rep", "add", { CONTROL, CONTROL }, { "\x10\x13ijkxyz", "reprepijkxyz", "\x10\x13ijkxyz", "\x10\x13ijkxyz", "reprepijkxyz", "reprepijkxyz" } },

	// multiple control character checks, but with empty replacement
	{ "ijk\x10\x13xyz", "", SG_TRUE, 0, MAX, "", "add", { CONTROL, CONTROL }, { "ijk\x10\x13xyz", "ijkxyz", "ijk\x10\x13xyz", "ijk\x10\x13xyz", "ijkxyz", "ijkxyz" } },
	{ "ijkxyz\x10\x13", "", SG_TRUE, 0, MAX, "", "add", { CONTROL, CONTROL }, { "ijkxyz\x10\x13", "ijkxyz", "ijkxyz\x10\x13", "ijkxyz\x10\x13", "ijkxyz", "ijkxyz" } },
	{ "\x10\x13ijkxyz", "", SG_TRUE, 0, MAX, "", "add", { CONTROL, CONTROL }, { "\x10\x13ijkxyz", "ijkxyz", "\x10\x13ijkxyz", "\x10\x13ijkxyz", "ijkxyz", "ijkxyz" } },

	// multiple control character and other character checks
	{ "ijk\x10+\x13xyz",   "+",    SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijk\x10rep\x13xyz",       "ijkrep+repxyz",   "ijk\x10+\x13xyz",   "ijk\x10+\x13xyz",   "ijkrepreprepxyz",       "ijkrepreprepxyz" } },
	{ "ijkxyz\x10+\x13",   "+",    SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkxyz\x10rep\x13",       "ijkxyzrep+rep",   "ijkxyz\x10+\x13",   "ijkxyz\x10+\x13",   "ijkxyzrepreprep",       "ijkxyzrepreprep" } },
	{ "\x10+\x13ijkxyz",   "+",    SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "\x10rep\x13ijkxyz",       "rep+repijkxyz",   "\x10+\x13ijkxyz",   "\x10+\x13ijkxyz",   "repreprepijkxyz",       "repreprepijkxyz" } },
	{ "ijk+\x10+\x13+xyz", "+",    SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkrep\x10rep\x13repxyz", "ijk+rep+rep+xyz", "ijk+\x10+\x13+xyz", "ijk+\x10+\x13+xyz", "ijkrepreprepreprepxyz", "ijkrepreprepreprepxyz" } },
	{ "ijkxyz+\x10+\x13+", "+",    SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkxyzrep\x10rep\x13rep", "ijkxyz+rep+rep+", "ijkxyz+\x10+\x13+", "ijkxyz+\x10+\x13+", "ijkxyzrepreprepreprep", "ijkxyzrepreprepreprep" } },
	{ "+\x10+\x13+ijkxyz", "+",    SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "rep\x10rep\x13repijkxyz", "+rep+rep+ijkxyz", "+\x10+\x13+ijkxyz", "+\x10+\x13+ijkxyz", "repreprepreprepijkxyz", "repreprepreprepijkxyz" } },
	{ "ijk-\x10*\x13/xyz", "+-*/", SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkrep\x10rep\x13repxyz", "ijk-rep*rep/xyz", "ijk-\x10*\x13/xyz", "ijk-\x10*\x13/xyz", "ijkrepreprepreprepxyz", "ijkrepreprepreprepxyz" } },
	{ "ijkxyz-\x10*\x13/", "+-*/", SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkxyzrep\x10rep\x13rep", "ijkxyz-rep*rep/", "ijkxyz-\x10*\x13/", "ijkxyz-\x10*\x13/", "ijkxyzrepreprepreprep", "ijkxyzrepreprepreprep" } },
	{ "-\x10*\x13/ijkxyz", "+-*/", SG_TRUE, 0, MAX, "rep", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "rep\x10rep\x13repijkxyz", "-rep*rep/ijkxyz", "-\x10*\x13/ijkxyz", "-\x10*\x13/ijkxyz", "repreprepreprepijkxyz", "repreprepreprepijkxyz" } },

	// multiple control character and other character checks, but with empty replacement
	{ "ijk\x10+\x13xyz",   "+",    SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijk\x10\x13xyz", "ijk+xyz",   "ijk\x10+\x13xyz",   "ijk\x10+\x13xyz",   "ijkxyz", "ijkxyz" } },
	{ "ijkxyz\x10+\x13",   "+",    SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkxyz\x10\x13", "ijkxyz+",   "ijkxyz\x10+\x13",   "ijkxyz\x10+\x13",   "ijkxyz", "ijkxyz" } },
	{ "\x10+\x13ijkxyz",   "+",    SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "\x10\x13ijkxyz", "+ijkxyz",   "\x10+\x13ijkxyz",   "\x10+\x13ijkxyz",   "ijkxyz", "ijkxyz" } },
	{ "ijk+\x10+\x13+xyz", "+",    SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijk\x10\x13xyz", "ijk+++xyz", "ijk+\x10+\x13+xyz", "ijk+\x10+\x13+xyz", "ijkxyz", "ijkxyz" } },
	{ "ijkxyz+\x10+\x13+", "+",    SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkxyz\x10\x13", "ijkxyz+++", "ijkxyz+\x10+\x13+", "ijkxyz+\x10+\x13+", "ijkxyz", "ijkxyz" } },
	{ "+\x10+\x13+ijkxyz", "+",    SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "\x10\x13ijkxyz", "+++ijkxyz", "+\x10+\x13+ijkxyz", "+\x10+\x13+ijkxyz", "ijkxyz", "ijkxyz" } },
	{ "ijk-\x10*\x13/xyz", "+-*/", SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijk\x10\x13xyz", "ijk-*/xyz", "ijk-\x10*\x13/xyz", "ijk-\x10*\x13/xyz", "ijkxyz", "ijkxyz" } },
	{ "ijkxyz-\x10*\x13/", "+-*/", SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "ijkxyz\x10\x13", "ijkxyz-*/", "ijkxyz-\x10*\x13/", "ijkxyz-\x10*\x13/", "ijkxyz", "ijkxyz" } },
	{ "-\x10*\x13/ijkxyz", "+-*/", SG_TRUE, 0, MAX, "", "add", { CONTROL | INVALID, CONTROL | INVALID }, { "\x10\x13ijkxyz", "-*/ijkxyz", "-\x10*\x13/ijkxyz", "-\x10*\x13/ijkxyz", "ijkxyz", "ijkxyz" } },

	// values containing multi-byte UTF8 characters
#define EH1 "\xF0\x93\x80\x80" // UTF-8 encoding of a few Egyptian hieroglyphs
#define EH2 "\xF0\x93\x80\x90"
#define EH3 "\xF0\x93\x80\xA0"
	{ "abc" EH1 "123",           "+-*/",                 SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123" } },
	{ "abc123",                  "+-" EH1 "*/",          SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc123",                  "abc123",                  "abc123",                  "abc123",                  "abc123",                  "abc123" } },
	{ "abc" EH1 "123",           "+-" EH1 "*/",          SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123",               "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abcrep123",               "abcrep123" } },
	{ "abc" EH1 "123",           "+-" EH2 "*/",          SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123" } },
	{ "abc" EH2 "123",           "+-" EH1 "*/",          SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc" EH2 "123",           "abc" EH2 "123",           "abc" EH2 "123",           "abc" EH2 "123",           "abc" EH2 "123",           "abc" EH2 "123" } },
	{ "abc" EH1 "123" EH2 "xyz", "+-" EH1 "*/",          SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123" EH2 "xyz",     "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abcrep123" EH2 "xyz",     "abcrep123" EH2 "xyz" } },
	{ "abc" EH1 "123" EH2 "xyz", "+-" EH2 "*/",          SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abc" EH1 "123repxyz",     "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123repxyz",     "abc" EH1 "123repxyz" } },
	{ "abc" EH1 "123" EH2 "xyz", "+-" EH3 "*/",          SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz" } },
	{ "abc" EH1 "123",           "+-" EH1 "*/" EH2 "#!", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123",               "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abcrep123",               "abcrep123" } },
	{ "abc" EH2 "123",           "+-" EH1 "*/" EH2 "#!", SG_FALSE, 0, MAX, "rep", "add", { INVALID, INVALID }, { "abcrep123",               "abc" EH2 "123",           "abc" EH2 "123",           "abc" EH2 "123",           "abcrep123",               "abcrep123" } },
	{ "abc" EH3 "123",           "+-" EH1 "*/" EH2 "#!", SG_FALSE, 0, MAX, "rep", "add", { VALID,   VALID },   { "abc" EH3 "123",           "abc" EH3 "123",           "abc" EH3 "123",           "abc" EH3 "123",           "abc" EH3 "123",           "abc" EH3 "123" } },
#undef EH1
#undef EH2
#undef EH3

	// values containing multi-byte UTF8 characters, but with empty replacement
#define EH1 "\xF0\x93\x80\x80" // UTF-8 encoding of a few Egyptian hieroglyphs
#define EH2 "\xF0\x93\x80\x90"
#define EH3 "\xF0\x93\x80\xA0"
	{ "abc" EH1 "123",           "+-*/",                 SG_FALSE, 0, MAX, "", "add", { VALID,   VALID },   { "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123" } },
	{ "abc123",                  "+-" EH1 "*/",          SG_FALSE, 0, MAX, "", "add", { VALID,   VALID },   { "abc123",                  "abc123",                  "abc123",                  "abc123",                  "abc123",                  "abc123" } },
	{ "abc" EH1 "123",           "+-" EH1 "*/",          SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123",                  "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc123",                  "abc123" } },
	{ "abc" EH1 "123",           "+-" EH2 "*/",          SG_FALSE, 0, MAX, "", "add", { VALID,   VALID },   { "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123" } },
	{ "abc" EH2 "123",           "+-" EH1 "*/",          SG_FALSE, 0, MAX, "", "add", { VALID,   VALID },   { "abc" EH2 "123",           "abc" EH2 "123",           "abc" EH2 "123",           "abc" EH2 "123",           "abc" EH2 "123",           "abc" EH2 "123" } },
	{ "abc" EH1 "123" EH2 "xyz", "+-" EH1 "*/",          SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123" EH2 "xyz",        "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc123" EH2 "xyz",        "abc123" EH2 "xyz" } },
	{ "abc" EH1 "123" EH2 "xyz", "+-" EH2 "*/",          SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc" EH1 "123xyz",        "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123xyz",        "abc" EH1 "123xyz" } },
	{ "abc" EH1 "123" EH2 "xyz", "+-" EH3 "*/",          SG_FALSE, 0, MAX, "", "add", { VALID,   VALID },   { "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz", "abc" EH1 "123" EH2 "xyz" } },
	{ "abc" EH1 "123",           "+-" EH1 "*/" EH2 "#!", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123",                  "abc" EH1 "123",           "abc" EH1 "123",           "abc" EH1 "123",           "abc123",                  "abc123" } },
	{ "abc" EH2 "123",           "+-" EH1 "*/" EH2 "#!", SG_FALSE, 0, MAX, "", "add", { INVALID, INVALID }, { "abc123",                  "abc" EH2 "123",           "abc" EH2 "123",           "abc" EH2 "123",           "abc123",                  "abc123" } },
	{ "abc" EH3 "123",           "+-" EH1 "*/" EH2 "#!", SG_FALSE, 0, MAX, "", "add", { VALID,   VALID },   { "abc" EH3 "123",           "abc" EH3 "123",           "abc" EH3 "123",           "abc" EH3 "123",           "abc" EH3 "123",           "abc" EH3 "123" } },
#undef EH1
#undef EH2
#undef EH3
};

/*
 * Done with these, make sure they don't interfere with anything else.
 */
#undef MAX
#undef VALID
#undef SHORT
#undef LONG
#undef INVALID
#undef CONTROL

/**
 * List of whitespace sequences to append/prepend/both when testing cases using
 * the trimming version of a function.
 */
static const char* u0110__whitespaces[] =
{
	" ",
	"    ",
	"\t",
	"\t\t\t\t",
	SG_PLATFORM_NATIVE_EOL_STR,
	SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR,
	" \t" SG_PLATFORM_NATIVE_EOL_STR,
	"    \t\t\t\t" SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR,
	SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR SG_PLATFORM_NATIVE_EOL_STR "\t\t\t\t    "
	" \t" SG_PLATFORM_NATIVE_EOL_STR " \t" SG_PLATFORM_NATIVE_EOL_STR " \t" SG_PLATFORM_NATIVE_EOL_STR " \t" SG_PLATFORM_NATIVE_EOL_STR,
	SG_PLATFORM_NATIVE_EOL_STR "\t " SG_PLATFORM_NATIVE_EOL_STR "\t " SG_PLATFORM_NATIVE_EOL_STR "\t " SG_PLATFORM_NATIVE_EOL_STR "\t ",
};

static void u0110__check__run_cases(SG_context* pCtx)
{
	SG_uint32 uIndex = 0u;

	// iterate through all of our test cases
	for (uIndex = 0u; uIndex < SG_NrElements(u0110__cases); ++uIndex)
	{
		const u0110__case* pCase   = u0110__cases + uIndex;
		SG_uint32          uResult = 0u;

		// if you need to debug through a specific case
		// then update this if statement to check for the index you care about and stick a breakpoint inside
		// failure messages include the index of the failing case for easy reference
		if (uIndex == SG_UINT32_MAX)
		{
			uResult = 0u;
		}

		// run the case
		VERIFY_ERR_CHECK(  SG_validate__check(pCtx, pCase->szValue, pCase->uMin, pCase->uMax, pCase->szInvalids, pCase->bControls, &uResult)  );

		// make sure we got the expected result
		VERIFYP_COND("check: unexpected result", uResult == pCase->cValidate.uNormal, ("index(%u) expected(%u) actual(%u) " U0110_CASE_FORMAT, uIndex, pCase->cValidate.uNormal, uResult, U0110_CASE_VALUES(pCase)));
	}

fail:
	return;
}

static void u0110__check__trim__run_cases(SG_context* pCtx)
{
	SG_uint32  uCaseIndex  = 0u;
	SG_uint32  uResult     = 0u;
	char*      szResult    = NULL;
	char*      szExpected  = NULL;
	SG_string* sPrepended  = NULL;
	SG_string* sAppended   = NULL;
	SG_string* sSurrounded = NULL;

	// iterate through all of our test cases
	for (uCaseIndex = 0u; uCaseIndex < SG_NrElements(u0110__cases); ++uCaseIndex)
	{
		const u0110__case* pCase            = u0110__cases + uCaseIndex;
		SG_uint32          uWhitespaceIndex = 0u;

		// iterate through each whitespace sequence
		for (uWhitespaceIndex = 0u; uWhitespaceIndex < SG_NrElements(u0110__whitespaces); ++uWhitespaceIndex)
		{
			const char* szWhitespace = u0110__whitespaces[uWhitespaceIndex];

			// if you need to debug through a specific case
			// then update this if statement to check for the indices you care about and stick a breakpoint inside
			// failure messages include the index of the failing case for easy reference
			if (uCaseIndex == SG_UINT32_MAX && uWhitespaceIndex == SG_UINT32_MAX)
			{
				uResult = 0u;
			}

			// calculate the expected output
			if (pCase->cValidate.uTrimmed != SG_VALIDATE__RESULT__VALID)
			{
				szExpected = NULL;
			}
			else
			{
				SG_ERR_CHECK(  SG_sz__trim(pCtx, pCase->szValue, NULL, &szExpected)  );
			}

			// run the standard value
			VERIFY_ERR_CHECK(  SG_validate__check__trim(pCtx, pCase->szValue, pCase->uMin, pCase->uMax, pCase->szInvalids, pCase->bControls, &uResult, &szResult)  );
			VERIFYP_COND("check__trim: unexpected result", uResult == pCase->cValidate.uTrimmed, ("case(%u) whitespace(%u) variant(%s) expected(%u) actual(%u) " U0110_CASE_FORMAT, uCaseIndex, uWhitespaceIndex, "standard", pCase->cValidate.uTrimmed, uResult, U0110_CASE_VALUES(pCase)));
			VERIFYP_COND("check__trim: unexpected result", SG_strcmp__null(szResult, szExpected) == 0, ("case(%u) whitespace(%u) variant(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uCaseIndex, uWhitespaceIndex, "standard", szExpected, szResult, U0110_CASE_VALUES(pCase)));
			SG_NULLFREE(pCtx, szResult);

			// run the prepended variant
			SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sPrepended,  "%s%s", szWhitespace, pCase->szValue == NULL ? "" : pCase->szValue)  );
			VERIFY_ERR_CHECK(  SG_validate__check__trim(pCtx, SG_string__sz(sPrepended), pCase->uMin, pCase->uMax, pCase->szInvalids, pCase->bControls, &uResult, &szResult)  );
			VERIFYP_COND("check__trim: unexpected result", uResult == pCase->cValidate.uTrimmed, ("case(%u) whitespace(%u) variant(%s) expected(%u) actual(%u) " U0110_CASE_FORMAT, uCaseIndex, uWhitespaceIndex, "prepended", pCase->cValidate.uTrimmed, uResult, U0110_CASE_VALUES(pCase)));
			VERIFYP_COND("check__trim: unexpected result", SG_strcmp__null(szResult, szExpected) == 0, ("case(%u) whitespace(%u) variant(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uCaseIndex, uWhitespaceIndex, "prepended", szExpected, szResult, U0110_CASE_VALUES(pCase)));
			SG_NULLFREE(pCtx, szResult);
			SG_STRING_NULLFREE(pCtx, sPrepended);

			// run the appended variant
			SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sAppended,   "%s%s", pCase->szValue == NULL ? "" : pCase->szValue, szWhitespace)  );
			VERIFY_ERR_CHECK(  SG_validate__check__trim(pCtx, SG_string__sz(sAppended), pCase->uMin, pCase->uMax, pCase->szInvalids, pCase->bControls, &uResult, &szResult)  );
			VERIFYP_COND("check__trim: unexpected result", uResult == pCase->cValidate.uTrimmed, ("case(%u) whitespace(%u) variant(%s) expected(%u) actual(%u) " U0110_CASE_FORMAT, uCaseIndex, uWhitespaceIndex, "appended", pCase->cValidate.uTrimmed, uResult, U0110_CASE_VALUES(pCase)));
			VERIFYP_COND("check__trim: unexpected result", SG_strcmp__null(szResult, szExpected) == 0, ("case(%u) whitespace(%u) variant(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uCaseIndex, uWhitespaceIndex, "appended", szExpected, szResult, U0110_CASE_VALUES(pCase)));
			SG_NULLFREE(pCtx, szResult);
			SG_STRING_NULLFREE(pCtx, sAppended);

			// run the surrounded variant
			SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sSurrounded, "%s%s%s", szWhitespace, pCase->szValue == NULL ? "" : pCase->szValue, szWhitespace)  );
			VERIFY_ERR_CHECK(  SG_validate__check__trim(pCtx, SG_string__sz(sSurrounded), pCase->uMin, pCase->uMax, pCase->szInvalids, pCase->bControls, &uResult, &szResult)  );
			VERIFYP_COND("check__trim: unexpected result", uResult == pCase->cValidate.uTrimmed, ("case(%u) whitespace(%u) variant(%s) expected(%u) actual(%u) " U0110_CASE_FORMAT, uCaseIndex, uWhitespaceIndex, "surrounded", pCase->cValidate.uTrimmed, uResult, U0110_CASE_VALUES(pCase)));
			VERIFYP_COND("check__trim: unexpected result", SG_strcmp__null(szResult, szExpected) == 0, ("case(%u) whitespace(%u) variant(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uCaseIndex, uWhitespaceIndex, "surrounded", szExpected, szResult, U0110_CASE_VALUES(pCase)));
			SG_NULLFREE(pCtx, szResult);
			SG_STRING_NULLFREE(pCtx, sSurrounded);

			// free the expected result for reuse next time
			SG_NULLFREE(pCtx, szExpected);
		}
	}

fail:
	SG_STRING_NULLFREE(pCtx, sPrepended);
	SG_STRING_NULLFREE(pCtx, sAppended);
	SG_STRING_NULLFREE(pCtx, sSurrounded);
	SG_NULLFREE(pCtx, szResult);
	SG_NULLFREE(pCtx, szExpected);
	return;
}

static void u0110__ensure__run_cases(SG_context* pCtx)
{
	SG_uint32 uIndex = 0u;

	// iterate through all of our test cases
	for (uIndex = 0u; uIndex < SG_NrElements(u0110__cases); ++uIndex)
	{
		const u0110__case* pCase  = u0110__cases + uIndex;
		SG_error           uError = SG_ERR_SG_LIBRARY((uIndex % 255) + 1); // use an arbitrary but non-constant error code

		// check if we're expecting a valid value
		if (pCase->cValidate.uNormal == SG_VALIDATE__RESULT__VALID)
		{
			// verify that we don't get a thrown error for valid values
			VERIFY_ERR_CHECK(  SG_validate__ensure(pCtx, pCase->szValue, pCase->uMin, pCase->uMax, pCase->szInvalids, pCase->bControls, uError, "test string")  );
		}
		else
		{
			// verify that we get the correct thrown error for invalid values
			VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_validate__ensure(pCtx, pCase->szValue, pCase->uMin, pCase->uMax, pCase->szInvalids, pCase->bControls, uError, "test string"), uError  );
		}
	}

fail:
	return;
}

static void u0110__ensure__trim__run_cases(SG_context* pCtx)
{
	SG_uint32 uIndex     = 0u;
	char*     szExpected = NULL;
	char*     szResult   = NULL;

	// iterate through all of our test cases
	for (uIndex = 0u; uIndex < SG_NrElements(u0110__cases); ++uIndex)
	{
		const u0110__case* pCase  = u0110__cases + uIndex;
		SG_error           uError = SG_ERR_SG_LIBRARY((uIndex % 255) + 1); // use an arbitrary but non-constant error code

		// if you need to debug through a specific case
		// then update this if statement to check for the index you care about and stick a breakpoint inside
		// failure messages include the index of the failing case for easy reference
		if (uIndex == SG_UINT32_MAX)
		{
			szResult = NULL;
		}

		// check if we're expecting a valid value
		if (pCase->cValidate.uTrimmed == SG_VALIDATE__RESULT__VALID)
		{
			// calculate the expected output
			SG_ERR_CHECK(  SG_sz__trim(pCtx, pCase->szValue, NULL, &szExpected)  );

			// verify that we don't get a thrown error
			VERIFY_ERR_CHECK(  SG_validate__ensure__trim(pCtx, pCase->szValue, pCase->uMin, pCase->uMax, pCase->szInvalids, pCase->bControls, uError, "test string", &szResult)  );

			// generate the trimmed value we expect
			VERIFYP_COND("ensure__trim: unexpected result", SG_strcmp__null(szResult, szExpected) == 0, ("index(%u) expected(%s) actual(%s) " U0110_CASE_FORMAT, uIndex, szExpected, szResult, U0110_CASE_VALUES(pCase)));

			// free up the strings for next test
			SG_NULLFREE(pCtx, szExpected);
			SG_NULLFREE(pCtx, szResult);
		}
		else
		{
			// verify that we get the correct thrown error
			VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_validate__ensure__trim(pCtx, pCase->szValue, pCase->uMin, pCase->uMax, pCase->szInvalids, pCase->bControls, uError, "test string", &szResult), uError  );

			// we shouldn't get a trimmed result back for errors
			VERIFYP_COND("ensure__trim: unexpected result", szResult == NULL, ("case(%u) expected(%s) actual(%s) " U0110_CASE_FORMAT, uIndex, szExpected, szResult, U0110_CASE_VALUES(pCase)));

			// but free it just in case we did
			SG_NULLFREE(pCtx, szResult);
		}
	}

fail:
	SG_NULLFREE(pCtx, szExpected);
	SG_NULLFREE(pCtx, szResult);
	return;
}

static void u0110__sanitize__run_cases(SG_context* pCtx)
{
	SG_uint32  uIndex  = 0u;
	SG_string* sResult = NULL;

	// iterate through all of our test cases
	for (uIndex = 0u; uIndex < SG_NrElements(u0110__cases); ++uIndex)
	{
		const u0110__case* pCase   = u0110__cases + uIndex;
		SG_uint32          uResult = 0u;

		// if you need to debug through a specific case
		// then update this if statement to check for the index you care about and stick a breakpoint inside
		// failure messages include the index of the failing case for easy reference
		if (uIndex == SG_UINT32_MAX)
		{
			sResult = NULL;
		}

		// test sanitizing TOO_SHORT
		VERIFY_ERR_CHECK(  SG_validate__sanitize(pCtx, pCase->szValue, pCase->uMin, pCase->uMax, pCase->szInvalids, SG_VALIDATE__RESULT__TOO_SHORT, pCase->szReplace, pCase->szAdd, &sResult)  );
		VERIFYP_COND("sanitize: unexpected result", strcmp(SG_string__sz(sResult), pCase->cSanitize.szMin) == 0, ("index(%u) flags(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uIndex, "TOO_SHORT", pCase->cSanitize.szMin, SG_string__sz(sResult), U0110_CASE_VALUES(pCase)));
		SG_STRING_NULLFREE(pCtx, sResult);

		// test sanitizing TOO_LONG
		VERIFY_ERR_CHECK(  SG_validate__sanitize(pCtx, pCase->szValue, pCase->uMin, pCase->uMax, pCase->szInvalids, SG_VALIDATE__RESULT__TOO_LONG, pCase->szReplace, pCase->szAdd, &sResult)  );
		VERIFYP_COND("sanitize: unexpected result", strcmp(SG_string__sz(sResult), pCase->cSanitize.szMax) == 0, ("index(%u) flags(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uIndex, "TOO_LONG", pCase->cSanitize.szMax, SG_string__sz(sResult), U0110_CASE_VALUES(pCase)));
		SG_STRING_NULLFREE(pCtx, sResult);

		// test sanitizing INVALID_CHARACTER
		VERIFY_ERR_CHECK(  SG_validate__sanitize(pCtx, pCase->szValue, pCase->uMin, pCase->uMax, pCase->szInvalids, SG_VALIDATE__RESULT__INVALID_CHARACTER, pCase->szReplace, pCase->szAdd, &sResult)  );
		VERIFYP_COND("sanitize: unexpected result", strcmp(SG_string__sz(sResult), pCase->cSanitize.szInvalids) == 0, ("index(%u) flags(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uIndex, "INVALID_CHARACTER", pCase->cSanitize.szInvalids, SG_string__sz(sResult), U0110_CASE_VALUES(pCase)));
		SG_STRING_NULLFREE(pCtx, sResult);

		// test sanitizing CONTROL_CHARACTER
		VERIFY_ERR_CHECK(  SG_validate__sanitize(pCtx, pCase->szValue, pCase->uMin, pCase->uMax, pCase->szInvalids, SG_VALIDATE__RESULT__CONTROL_CHARACTER, pCase->szReplace, pCase->szAdd, &sResult)  );
		VERIFYP_COND("sanitize: unexpected result", strcmp(SG_string__sz(sResult), pCase->cSanitize.szControls) == 0, ("index(%u) flags(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uIndex, "CONTROL_CHARACTER", pCase->cSanitize.szControls, SG_string__sz(sResult), U0110_CASE_VALUES(pCase)));
		SG_STRING_NULLFREE(pCtx, sResult);

		// test sanitizing ALL
		VERIFY_ERR_CHECK(  SG_validate__sanitize(pCtx, pCase->szValue, pCase->uMin, pCase->uMax, pCase->szInvalids, SG_VALIDATE__RESULT__ALL, pCase->szReplace, pCase->szAdd, &sResult)  );
		VERIFYP_COND("sanitize: unexpected result", strcmp(SG_string__sz(sResult), pCase->cSanitize.szAll) == 0, ("index(%u) flags(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uIndex, "ALL", pCase->cSanitize.szAll, SG_string__sz(sResult), U0110_CASE_VALUES(pCase)));
		VERIFY_ERR_CHECK(  SG_validate__check(pCtx, SG_string__sz(sResult), pCase->uMin, pCase->uMax, pCase->szInvalids, SG_TRUE, &uResult)  );
		VERIFYP_COND("sanitize: sanitized result not valid", uResult == SG_VALIDATE__RESULT__VALID, ("index(%u) sanitized(%s) invalid(%u) " U0110_CASE_FORMAT, uIndex, SG_string__sz(sResult), uResult, U0110_CASE_VALUES(pCase)));
		SG_STRING_NULLFREE(pCtx, sResult);
	}

fail:
	SG_STRING_NULLFREE(pCtx, sResult);
	return;
}

static void u0110__sanitize__trim__run_cases(SG_context* pCtx)
{
	SG_uint32  uCaseIndex  = 0u;
	SG_string* sResult     = 0u;
	SG_string* sPrepended  = NULL;
	SG_string* sAppended   = NULL;
	SG_string* sSurrounded = NULL;

	// iterate through all of our test cases
	for (uCaseIndex = 0u; uCaseIndex < SG_NrElements(u0110__cases); ++uCaseIndex)
	{
		const u0110__case* pCase            = u0110__cases + uCaseIndex;
		SG_uint32          uWhitespaceIndex = 0u;

		// iterate through each whitespace sequence
		for (uWhitespaceIndex = 0u; uWhitespaceIndex < SG_NrElements(u0110__whitespaces); ++uWhitespaceIndex)
		{
			const char* szWhitespace = u0110__whitespaces[uWhitespaceIndex];

			// if you need to debug through a specific case
			// then update this if statement to check for the indices you care about and stick a breakpoint inside
			// failure messages include the index of the failing case for easy reference
			if (uCaseIndex == SG_UINT32_MAX && uWhitespaceIndex == SG_UINT32_MAX)
			{
				sResult = NULL;
			}

			// run the standard value
			VERIFY_ERR_CHECK(  SG_validate__sanitize__trim(pCtx, pCase->szValue, pCase->uMin, pCase->uMax, pCase->szInvalids, SG_VALIDATE__RESULT__ALL, pCase->szReplace, pCase->szAdd, &sResult)  );
			VERIFYP_COND("sanitize__trim: unexpected result", strcmp(SG_string__sz(sResult), pCase->cSanitize.szTrimmedAll) == 0, ("case(%u) whitespace(%u) variant(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uCaseIndex, uWhitespaceIndex, "standard", pCase->cSanitize.szTrimmedAll, SG_string__sz(sResult), U0110_CASE_VALUES(pCase)));
			SG_STRING_NULLFREE(pCtx, sResult);

			// run the prepended variant
			SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sPrepended, "%s%s", szWhitespace, pCase->szValue == NULL ? "" : pCase->szValue)  );
			VERIFY_ERR_CHECK(  SG_validate__sanitize__trim(pCtx, SG_string__sz(sPrepended), pCase->uMin, pCase->uMax, pCase->szInvalids, SG_VALIDATE__RESULT__ALL, pCase->szReplace, pCase->szAdd, &sResult)  );
			VERIFYP_COND("sanitize__trim: unexpected result", strcmp(SG_string__sz(sResult), pCase->cSanitize.szTrimmedAll) == 0, ("case(%u) whitespace(%u) variant(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uCaseIndex, uWhitespaceIndex, "prepended", pCase->cSanitize.szTrimmedAll, SG_string__sz(sResult), U0110_CASE_VALUES(pCase)));
			SG_STRING_NULLFREE(pCtx, sResult);
			SG_STRING_NULLFREE(pCtx, sPrepended);

			// run the appended variant
			SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sAppended, "%s%s", pCase->szValue == NULL ? "" : pCase->szValue, szWhitespace)  );
			VERIFY_ERR_CHECK(  SG_validate__sanitize__trim(pCtx, SG_string__sz(sAppended), pCase->uMin, pCase->uMax, pCase->szInvalids, SG_VALIDATE__RESULT__ALL, pCase->szReplace, pCase->szAdd, &sResult)  );
			VERIFYP_COND("sanitize__trim: unexpected result", strcmp(SG_string__sz(sResult), pCase->cSanitize.szTrimmedAll) == 0, ("case(%u) whitespace(%u) variant(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uCaseIndex, uWhitespaceIndex, "appended", pCase->cSanitize.szTrimmedAll, SG_string__sz(sResult), U0110_CASE_VALUES(pCase)));
			SG_STRING_NULLFREE(pCtx, sResult);
			SG_STRING_NULLFREE(pCtx, sAppended);

			// run the surrounded variant
			SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &sSurrounded, "%s%s%s", szWhitespace, pCase->szValue == NULL ? "" : pCase->szValue, szWhitespace)  );
			VERIFY_ERR_CHECK(  SG_validate__sanitize__trim(pCtx, SG_string__sz(sSurrounded), pCase->uMin, pCase->uMax, pCase->szInvalids, SG_VALIDATE__RESULT__ALL, pCase->szReplace, pCase->szAdd, &sResult)  );
			VERIFYP_COND("sanitize__trim: unexpected result", strcmp(SG_string__sz(sResult), pCase->cSanitize.szTrimmedAll) == 0, ("case(%u) whitespace(%u) variant(%s) expected(%s) actual(%s) " U0110_CASE_FORMAT, uCaseIndex, uWhitespaceIndex, "surrounded", pCase->cSanitize.szTrimmedAll, SG_string__sz(sResult), U0110_CASE_VALUES(pCase)));
			SG_STRING_NULLFREE(pCtx, sResult);
			SG_STRING_NULLFREE(pCtx, sSurrounded);
		}
	}

fail:
	SG_STRING_NULLFREE(pCtx, sResult);
	SG_STRING_NULLFREE(pCtx, sPrepended);
	SG_STRING_NULLFREE(pCtx, sAppended);
	SG_STRING_NULLFREE(pCtx, sSurrounded);
	return;
}


/*
**
** MAIN
**
*/

TEST_MAIN(u0068_main)
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  u0110__check__run_cases(pCtx)  );
	BEGIN_TEST(  u0110__check__trim__run_cases(pCtx)  );
	BEGIN_TEST(  u0110__ensure__run_cases(pCtx)  );
	BEGIN_TEST(  u0110__ensure__trim__run_cases(pCtx)  );
	BEGIN_TEST(  u0110__sanitize__run_cases(pCtx)  );
	BEGIN_TEST(  u0110__sanitize__trim__run_cases(pCtx)  );

	TEMPLATE_MAIN_END;
}
