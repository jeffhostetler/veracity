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


/*
**
** get_default_class
**
*/

/**
 * A single test case for detecting the default class of a file.
 */
typedef struct
{
	const char* szFilename;
	const char* szClass;
}
u0106__get_default_class__case;

/**
 * List of test files and expected results.
 */
static const u0106__get_default_class__case u0106__get_default_class__cases[] =
{
	{ "bmp",            SG_FILETOOL__CLASS__BINARY },
	{ "doc",            SG_FILETOOL__CLASS__BINARY },
	{ "docx",           SG_FILETOOL__CLASS__BINARY },
	{ "gif",            SG_FILETOOL__CLASS__BINARY },
	{ "jpg",            SG_FILETOOL__CLASS__BINARY },
	{ "odp",            SG_FILETOOL__CLASS__BINARY },
	{ "ods",            SG_FILETOOL__CLASS__BINARY },
	{ "odt",            SG_FILETOOL__CLASS__BINARY },
	{ "png",            SG_FILETOOL__CLASS__BINARY },
	{ "ppt",            SG_FILETOOL__CLASS__BINARY },
	{ "pptx",           SG_FILETOOL__CLASS__BINARY },
	{ "tga",            SG_FILETOOL__CLASS__BINARY },
	{ "tif",            SG_FILETOOL__CLASS__BINARY },
	{ "xls",            SG_FILETOOL__CLASS__BINARY },
	{ "xlsx",           SG_FILETOOL__CLASS__BINARY },
	{ "txt",            SG_FILETOOL__CLASS__TEXT   },
	{ "utf8",           SG_FILETOOL__CLASS__TEXT   },
	{ "utf8_nul",       SG_FILETOOL__CLASS__TEXT   },
	{ "utf8_nobom",     SG_FILETOOL__CLASS__TEXT   },
	{ "utf8_nobom_nul", SG_FILETOOL__CLASS__BINARY }, // this one will fool the binary detection
	{ "utf16be",        SG_FILETOOL__CLASS__TEXT   },
	{ "utf16be_nul",    SG_FILETOOL__CLASS__TEXT   },
	{ "utf16le",        SG_FILETOOL__CLASS__TEXT   },
	{ "utf16le_nul",    SG_FILETOOL__CLASS__TEXT   },
	{ "utf32be",        SG_FILETOOL__CLASS__TEXT   },
	{ "utf32be_nul",    SG_FILETOOL__CLASS__TEXT   },
	{ "utf32le",        SG_FILETOOL__CLASS__TEXT   },
	{ "utf32le_nul",    SG_FILETOOL__CLASS__TEXT   },
};

static void u0106__get_default_class__run_test_cases(SG_context* pCtx, const SG_pathname* pDataDir)
{
	SG_pathname* pDataPath = NULL;
	SG_bool      bExists   = SG_FALSE;
	SG_uint32    uIndex    = 0u;
	SG_pathname* pPathname = NULL;

	// find the data path with all of our files
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pDataPath, pDataDir, "u0106_filetool_data")  );
	VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pDataPath, &bExists, NULL, NULL)  );
	VERIFYP_COND("Data folder doesn't exist.", bExists == SG_TRUE, ("Expected data path: %s", SG_pathname__sz(pDataPath)));

	// iterate through all of our test cases
	for (uIndex = 0u; uIndex < SG_NrElements(u0106__get_default_class__cases); ++uIndex)
	{
		const u0106__get_default_class__case* pCase = u0106__get_default_class__cases + uIndex;

		// if you need to debug through a specific case
		// then update this if statement to check for the index you care about and stick a breakpoint inside
		// failure messages include the index of the failing case for easy reference
		if (uIndex == SG_UINT32_MAX)
		{
			bExists = SG_FALSE;
		}

		// build the path to the current file
		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathname, pDataPath, pCase->szFilename)  );
		VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathname, &bExists, NULL, NULL)  );
		VERIFYP_COND("Test file doesn't exist.", bExists == SG_TRUE, ("Expected test file: %s", SG_pathname__sz(pPathname)));

		// run the test
		if (bExists == SG_TRUE)
		{
			const char* szClass = NULL;

			// get the class for this file
			VERIFY_ERR_CHECK(  SG_filetool__get_default_class(pCtx, pPathname, &szClass)  );

			// make sure we got the expected result
			VERIFYP_COND("Filename had NULL class.", szClass != NULL, ("index(%d) file(%s) class(NULL), expected(%s)", uIndex, pCase->szFilename, pCase->szClass));
			if (szClass != NULL)
			{
				VERIFYP_COND("Filename had unexpected class.", strcmp(szClass, pCase->szClass) == 0, ("index(%d) file(%s) class(%s) expected(%s)", uIndex, pCase->szFilename, szClass, pCase->szClass));
			}
		}

		// free the temporary data
		SG_PATHNAME_NULLFREE(pCtx, pPathname);
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathname);
	SG_PATHNAME_NULLFREE(pCtx, pDataPath);
	return;
}


/*
**
** replace_tokens
**
*/

/**
 * A single test case for replacing a token in a string.
 */
typedef struct
{
	const char* szName;   //< Name of the token to replace.
	SG_uint16   eType;    //< Type of the token's value (one of SG_VARIANT_TYPE_*).
	SG_bool     bValue;   //< Boolean value of the token.
	SG_int64    iValue;   //< Integer value of the token.
	const char* szValue;  //< String value of the token.
	const char* szInput;  //< String to replace the token in.
	const char* szOutput; //< Expected result of token replacement.
}
u0106__replace_tokens__case;

/**
 * List of test cases for token replacement.
 */
static const u0106__replace_tokens__case u0106__replace_tokens__cases[] =
{
	{ "FILENAME", SG_VARIANT_TYPE_SZ, SG_FALSE, 0, "file.txt", "@FILENAME@", "file.txt" },
	{ "FILENAME", SG_VARIANT_TYPE_SZ, SG_FALSE, 0, "file.txt", "--file=@FILENAME@", "--file=file.txt" },
	{ "FILENAME", SG_VARIANT_TYPE_SZ, SG_FALSE, 0, "file.txt", "@FILENAME@ whatever", "file.txt whatever" },
	{ "FILENAME", SG_VARIANT_TYPE_SZ, SG_FALSE, 0, "file.txt", "--file=@FILENAME@ whatever", "--file=file.txt whatever" },

	{ "FILENAME", SG_VARIANT_TYPE_SZ, SG_FALSE, 0, "file.txt", "FILENAME", "FILENAME" },
	{ "FILENAME", SG_VARIANT_TYPE_SZ, SG_FALSE, 0, "file.txt", "@FILENAME", "@FILENAME" },
	{ "FILENAME", SG_VARIANT_TYPE_SZ, SG_FALSE, 0, "file.txt", "FILENAME@", "FILENAME@" },
	{ "FILENAME", SG_VARIANT_TYPE_SZ, SG_FALSE, 0, "file.txt", "@FILENAME@@", "file.txt@" },
	{ "FILENAME", SG_VARIANT_TYPE_SZ, SG_FALSE, 0, "file.txt", "@FILENAME@@@", "file.txt@@" },
	{ "FILENAME", SG_VARIANT_TYPE_SZ, SG_FALSE, 0, "file.txt", "@FILENAM@", "@FILENAM@" },

	{ "INT", SG_VARIANT_TYPE_INT64, SG_FALSE, 42, NULL, "@INT@", "42" },
	{ "INT", SG_VARIANT_TYPE_INT64, SG_FALSE, 42, NULL, "--flag @INT@", "--flag 42" },
	{ "INT", SG_VARIANT_TYPE_INT64, SG_FALSE, 42, NULL, "@INT@ whatever", "42 whatever" },
	{ "INT", SG_VARIANT_TYPE_INT64, SG_FALSE, 42, NULL, "--flag @INT@ whatever", "--flag 42 whatever" },

	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_TRUE, 0, NULL, "@BOOL|true|false@", "true" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_FALSE, 0, NULL, "@BOOL|true|false@", "false" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_TRUE, 0, NULL, "@BOOL||false@", "" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_FALSE, 0, NULL, "@BOOL||false@", "false" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_TRUE, 0, NULL, "@BOOL|true|@", "true" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_FALSE, 0, NULL, "@BOOL|true|@", "" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_TRUE, 0, NULL, "@BOOL|true@", "true" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_FALSE, 0, NULL, "@BOOL|true@", "" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_TRUE, 0, NULL, "@BOOL||@", "" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_FALSE, 0, NULL, "@BOOL||@", "" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_TRUE, 0, NULL, "@BOOL|@", "" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_FALSE, 0, NULL, "@BOOL|@", "" },

	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_TRUE, 0, NULL, "--flag=@BOOL|true|false@", "--flag=true" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_TRUE, 0, NULL, "@BOOL|true|false@ whatever", "true whatever" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_TRUE, 0, NULL, "--flag=@BOOL|true|false@ whatever", "--flag=true whatever" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_FALSE, 0, NULL, "--flag=@BOOL|true|false@", "--flag=false" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_FALSE, 0, NULL, "@BOOL|true|false@ whatever", "false whatever" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_FALSE, 0, NULL, "--flag=@BOOL|true|false@ whatever", "--flag=false whatever" },

	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_TRUE, 0, NULL, "@BOOL@", "@BOOL@" },
	{ "BOOL", SG_VARIANT_TYPE_BOOL, SG_TRUE, 0, NULL, "@BOOL|", "@BOOL|" },
};

/**
 * Runs all the test cases for replace_tokens.
 */
static void u0106__replace_tokens__run_test_cases(SG_context* pCtx)
{
	SG_vhash*  pHash   = NULL;
	SG_uint32  uIndex  = 0u;
	SG_string* sOutput = NULL;

	// iterate through all of our test cases
	for (uIndex = 0u; uIndex < SG_NrElements(u0106__replace_tokens__cases); ++uIndex)
	{
		const u0106__replace_tokens__case* pCase = u0106__replace_tokens__cases + uIndex;
		SG_int_to_string_buffer            szBuffer;

		// if you need to debug through a specific case
		// then update this if statement to check for the index you care about and stick a breakpoint inside
		// failure messages include the index of the failing case for easy reference
		if (uIndex == SG_UINT32_MAX)
		{
			sOutput = NULL;
		}

		// build a hash with the token value
		VERIFY_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pHash)  );
		switch (pCase->eType)
		{
		case SG_VARIANT_TYPE_BOOL:
			VERIFY_ERR_CHECK(  SG_vhash__add__bool(pCtx, pHash, pCase->szName, pCase->bValue)  );
			break;
		case SG_VARIANT_TYPE_INT64:
			VERIFY_ERR_CHECK(  SG_vhash__add__int64(pCtx, pHash, pCase->szName, pCase->iValue)  );
			break;
		case SG_VARIANT_TYPE_SZ:
			VERIFY_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pHash, pCase->szName, pCase->szValue)  );
			break;
		default:
			VERIFYP_COND("Invalid token value type.", SG_FALSE, ("Variant type unsupported by this test: %d", pCase->eType));
			continue;
		}

		// run the test and verify the result
		SG_filetool__replace_tokens(pCtx, pHash, pCase->szInput, &sOutput);
		VERIFYP_CTX_IS_OK("Function threw error.", pCtx, ("TestIndex(%u) TokenName(%s) TokenType(%u) TokenBoolValue(%d) TokenIntValue(%s) TokenStringValue(%s) Input(%s) ExpectedOutput(%s)", uIndex, pCase->szName, pCase->eType, pCase->bValue, SG_int64_to_sz(pCase->iValue, szBuffer), pCase->szValue, pCase->szInput, pCase->szOutput));
		SG_context__err_reset(pCtx);
		if (sOutput != NULL)
		{
			VERIFYP_COND("Output doesn't match expectation.", strcmp(pCase->szOutput, SG_string__sz(sOutput)) == 0, ("TestIndex(%u) TokenName(%s) TokenType(%u) TokenBoolValue(%d) TokenIntValue(%s) TokenStringValue(%s) Input(%s) ExpectedOutput(%s) ActualOutput(%s)", uIndex, pCase->szName, pCase->eType, pCase->bValue, SG_int64_to_sz(pCase->iValue, szBuffer), pCase->szValue, pCase->szInput, pCase->szOutput, SG_string__sz(sOutput)));
		}

		// free the temporary data
		SG_VHASH_NULLFREE(pCtx, pHash);
		SG_STRING_NULLFREE(pCtx, sOutput);
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pHash);
	SG_STRING_NULLFREE(pCtx, sOutput);
	return;
}


/*
**
** MAIN
**
*/

TEST_MAIN(u0106_filetool)
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  u0106__get_default_class__run_test_cases(pCtx, pDataDir)  );
	BEGIN_TEST(  u0106__replace_tokens__run_test_cases(pCtx)  );

	TEMPLATE_MAIN_END;
}
