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

#if 0
//////////////////////////////////////////////////////////////////
// TODO 2011/08/03 The following note were written for the original
// TODO            ../ut/sg_portability.c and ../ut/sg_pendingtree.c.
// TODO
// TODO            I need to read thru them for changes made in the
// TODO            ./wc/sg_wc_port.c refactor.
//////////////////////////////////////////////////////////////////

/**
 *
 * @file sg_portability.c
 *
 * We use the PORTABILITY code to detect POTENTIAL and ACTUAL problems with ENTRYNAMES and PATHNAMES.
 *
 * Detect POTENTIAL Problems:
 * ==========================
 *
 * Because we want to be able to fetch and populate a WORKING DIRECTORY on all platforms regardless
 * of where the original version of the tree was created, we need to be aware of various POTENTIAL
 * problems.  For example, a Linux user can create "README" and "ReadMe" in a directory and ADD and
 * COMMIT them; no problem.  But when a Windows or Mac user tries to CLONE/FETCH a view of the tree,
 * we won't be able to properly populate the WORKING DIRECTORY (because those platforms are not case-
 * sensitive).  In this example, it would be nice if we were able to warn the Linux user that they
 * are about to cause problems for users on other platforms and give a chance to do something different.
 * [We still allow them to do it, but we give them a chance to be nice.]
 *
 * The code in this file tries to PREDICT/GUESS these types of POTENTIAL problems without actually
 * touching the filesystem.  In a sense, we combine all of the various restictions on all of the
 * various platforms that we support and inspect the ENTRYNAMES/PATHNAMES that we are given.  This
 * tends to give warnings that are a superset of what any individual user would actually see in
 * practice when creating files and directories.  [This makes it a little difficult to determine
 * when we issue a true false-positive and something that is just not applicable to this particular
 * operating system/filesystem.]
 *
 * There are 2 types of issues:
 *
 * [1] INDIVIDUAL warnings.  These have to do with an individual ENTRYNAME or PATHNAME in isolation.
 *     For example, the ENTRYNAME "abc:def" is not valid on Windows.
 *
 * [2] COLLISION warnings.  These have to do with multiple, distinct sequences of characters being
 *     treated the same by the filesystem.
 *     For example, the ENTRYNAMES "README" and "ReadMe" on Windows and Mac systems.
 *
 * When our computation generates a warning, we record with it the reason for the warning:
 *
 * In [1] this is fairly straight-forward.  For example, an ENTRYNAME that contains a non-printable
 *        control character, such as 0x01 control-A, which isn't valid on a Windows filesystem.
 *
 * In [2] this might appear somewhat contrived and confusing.  For example, consider the ENTRYNAMES
 *        "readme", "README" and "ReadMe.".  Note the final dot on the latter.
 *        We would report that:
 *        () "readme"   (the "ORIGINAL", as given name) might collide with
 *        () "README"   when a filesystem ignores/strips "CASE" and with
 *        () "ReadMe."  when a filesystem ignores/strips "CASE" and "FINAL-DOTS".
 *
 * Most uses of the PORTABILITY code have us scan the contents of directories on the local system
 * that were independently created by the user and/or command line arguments provided by the user.
 * These names will be added to the repository in some way.  [A one-way flow of information into
 * the repository starting with something that is perfectly legal on the current system.  And where
 * problems won't be noticed until they are retrieved on another type of system.]
 *
 * We also recognize that we shouldn't badger the user about things too often.  So in the above
 * example, the user who tries to ADD "ReadMe" to the tree when "README" is already present should
 * get the warning -- right then at the time of the ADD.  Subsequent COMMITS or changes in the
 * directory should not complain about it again.  So we allow each ENTRYNAME to be marked as
 * "CHECK" or "NO-CHECK".  CHECK means that the caller wants to know about any problems with the
 * name.  NO-CHECK means that they don't *BUT* they still must provide it so that we can search
 * for POTENTIAL COLLISIONS with other (CHECKED) names.
 *
 * Since each installation may have a unique mix of system types, we allow users to turn off
 * specific warnings for things that they won't ever see.  For example, if a site will never have
 * Windows machines on the project, they can turn off the warnings about MS-DOS devices, such as COM1.
 * These are given in a MASK.
 *
 *
#if 0 // TODO
 * Detect ACTUAL Problems:
 * =======================
 *
 * Another use of the PORTABILITY is to detect ACTUAL problems with a given set of ENTRYNAMES.
 *
 * There are times when we need to restore entries into a directory from the repository, such
 * as on a "REVERT" command where we restore files that were DELETED from the WORKING DIRECTORY
 * but not COMMITTED.  Because of other changes in the directory, such as ADDED/RENAMED files
 * that are not part of the REVERT), we may not be able to complete the operation.  We would
 * like to be able to PREDICT these problems BEFORE we start modifying the WORKING DIRECTORY
 * (rather than failing half-way through the REVERT and leaving a mess for the user to clean up).
 * [This doesn't guaranteed that we won't have problems, but if we can avoid the cases where we
 * know there is a problem, the user is better off.]
 *
 * Note that these problems can only be tested using the actual filesystem supporting the
 * user's WORKING DIRECTORY.  For example, "README" and "ReadMe" don't collide on Linux systems,
 * but if the Linux user's WORKING DIRECTORY resides on a mounted network share from a Windows
 * system, then they probably do.
#endif
 *
 */
#endif

//////////////////////////////////////////////////////////////////

/*
 * TODO We need to store the user's PORTABILITY MASK settings in the LOCAL SETTINGS.
 *
 *
 * TODO See SPRAWL-54:
 * TODO
 * TODO The 8/13/2009 version of http://msdn.microsoft.com/en-us/library/aa365247%28VS.85,loband%29.aspx
 * TODO includes new information that I didn't see before.  This looks like an update for Vista and Win 7.
 * TODO
 * TODO There is discussion of \\.\ prefix to let you get into a different namespace (as in \\.\PhysicalDisk0)
 * TODO
 * TODO There is also discussion of \\?\GLOBALROOT in later versions that might need looking at.
 * TODO
 * TODO There is a comment (in the Q/A at the bottom) about leading spaces in file/directory names causing
 * TODO problems for (Vista/Win7) Windows Explorer (or rather it not respecting them when moving files).
 * TODO
 * TODO
 * TODO In http://msdn.microsoft.com/en-us/library/aa363866%28VS.85%29.aspx, it talks about how Win 7 and
 * TODO Server 2008 now have symlinks.  Don't know how they compare to Mac/Linux.
 * TODO
 * TODO
 * TODO Do we care about hard-links?
 * TODO
 * TODO In http://msdn.microsoft.com/en-us/library/aa363860%28VS.85%29.aspx, it talks about how Win 2000
 * TODO has hard-links.  So the problem is not limited to Linux/Mac.
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void _sg_wc_port_item__free(SG_context* pCtx, sg_wc_port_item * pItem)
{
	if (!pItem)
		return;

	// we do not own pItem->pConverterRepoCharSet

	SG_NULLFREE(pCtx, pItem->szUtf8EntryName);
	SG_NULLFREE(pCtx, pItem->pUtf32EntryName);

	SG_NULLFREE(pCtx, pItem->szCharSetEntryName);
	SG_NULLFREE(pCtx, pItem->szUtf8EntryNameAppleNFD);
	SG_NULLFREE(pCtx, pItem->szUtf8EntryNameAppleNFC);

	SG_STRING_NULLFREE(pCtx, pItem->pStringItemLog);
	SG_RBTREE_NULLFREE(pCtx, pItem->prbCollidedWith);	// we do not own the values

	SG_NULLFREE(pCtx, pItem);
}

/**
 * Allocate a sg_wc_port_item structure to provide workspace
 * and results for evaluating the portability of an individual
 * file/directory name.
 *
 * szEntryName is the original entryname that we are given.
 * It *MUST* be in UTF-8.
 *
 * pVoidAssocData is optional and for our callers use only;
 * we do not look at it or do anything with it.
 */
void _sg_wc_port_item__alloc(SG_context * pCtx,
							 const char * pszGid_Entry,			// optional, for logging.
							 const char * szEntryNameAsGiven,
							 SG_treenode_entry_type tneType,
							 SG_utf8_converter * pConverterRepoCharSet,
							 void * pVoidAssocData,
							 sg_wc_port_item ** ppItem)
{
	sg_wc_port_item * pItem = NULL;

	SG_NULLARGCHECK_RETURN(ppItem);
	SG_NONEMPTYCHECK_RETURN(szEntryNameAsGiven);

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pItem)  );

	pItem->flagsObserved = SG_WC_PORT_FLAGS__NONE;
	pItem->pConverterRepoCharSet = pConverterRepoCharSet;
	pItem->pVoidAssocData = pVoidAssocData;
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pItem->pStringItemLog)  );

	// The GID is optional.  If given, we will use it to
	// construct a potential collision set.
	if (pszGid_Entry && *pszGid_Entry)
		SG_ERR_CHECK(  SG_strcpy(pCtx, pItem->bufGid_Entry, SG_NrElements(pItem->bufGid_Entry), pszGid_Entry)  );

	pItem->tneType = tneType;

	// assume we were given a UTF-8 entryname and try to create a UTF-32 copy.
	// this will fail with SG_ERR_UTF8INVALID otherwise.

	SG_ERR_CHECK(  SG_utf8__to_utf32__sz(pCtx, szEntryNameAsGiven,&pItem->pUtf32EntryName,&pItem->lenEntryNameChars)  );

	// we were given a valid UTF-8 entryname.  make a copy of it.

	SG_ERR_CHECK(  SG_STRDUP(pCtx, szEntryNameAsGiven,&pItem->szUtf8EntryName)  );
	pItem->lenEntryNameUtf8Bytes = (SG_uint32) strlen(pItem->szUtf8EntryName);

	// we have US-ASCII-only string if there were no multi-byte UTF-8 characters.

	pItem->b7BitClean = (pItem->lenEntryNameUtf8Bytes == pItem->lenEntryNameChars);

	if (!pItem->b7BitClean)
	{
		// we now have UTF-8 data with at least one character >0x7f.
		//
		// The MAC stores filenames in canonical NFD (and rearranges the
		// accent characters as necessary).
		//
		// compute apple-style NFD and NFC from the UTF-8 input.  we seem to get different
		// results if we do:
		//    nfd := NFD(orig)
		//    nfc := NFC(orig)
		// and
		//    nfd := NFD(orig)
		//    nfc := NFC(nfd)
		//
		// we choose the latter because that's probably how the MAC filesystem works.
		//
		// we ***DO NOT*** change case nor strip ignorables nor convert any SFM characters.

		SG_bool bIsCharSetUtf8;

		SG_ERR_CHECK(  SG_apple_unicode__compute_nfd__utf8(pCtx,
														   pItem->szUtf8EntryName,          pItem->lenEntryNameUtf8Bytes,
														   &pItem->szUtf8EntryNameAppleNFD, &pItem->lenEntryNameAppleNFDBytes)  );

		SG_ERR_CHECK(  SG_apple_unicode__compute_nfc__utf8(pCtx,
														   pItem->szUtf8EntryNameAppleNFD,  pItem->lenEntryNameAppleNFDBytes,
														   &pItem->szUtf8EntryNameAppleNFC, &pItem->lenEntryNameAppleNFCBytes)  );

		pItem->bHasDecomposition     = (strcmp(pItem->szUtf8EntryNameAppleNFC,pItem->szUtf8EntryNameAppleNFD) != 0);
		pItem->bIsProperlyComposed   = (strcmp(pItem->szUtf8EntryName,        pItem->szUtf8EntryNameAppleNFC) == 0);
		pItem->bIsProperlyDecomposed = (strcmp(pItem->szUtf8EntryName,        pItem->szUtf8EntryNameAppleNFD) == 0);

		// if the original input contained partially decomposed characters or
		// if the decomposed accent characters were in a random order, we flag it.

		if (!pItem->bIsProperlyComposed && !pItem->bIsProperlyDecomposed)
			pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__NON_CANONICAL_NFD;

		SG_ERR_CHECK(  SG_utf8_converter__is_charset_utf8(pCtx, pItem->pConverterRepoCharSet,&bIsCharSetUtf8)  );
		if (!bIsCharSetUtf8)
		{
			// The repo has defined a non-UTF-8 CharSet in the configuration.  This means
			// that users are allowed to use the repo when they have that their LOCALE CHARSET
			// set to that (and only that) value.   This means that we might have to convert
			// entrynames from UTF-8 (as we store them internally) to that charset when we
			// are working on a Linux system, for example.
			//
			// Note: They are always free to set their LOCALE to UTF-8.)
			//
			// So here, we check for entrynames that cannot be converted to that charset.
			// As they would cause problems for Linux users later.  Later we will run the
			// charset-based names thru the collider.
			//
			//
			// try to convert the original name to a CHARSET-based string without using
			// substitution/slug characters.  this should fail, rather than substitute,
			// if there are characters that are not in this character set.  we want to
			// know how this entryname will look in the filesystem on a Linux system.
			// if it cannot be cleanly converted, Linux users will have problems with
			// this entry.  We ***ASSUME*** that all Linux-based users of this repo are
			// using the same CHARSET.
			//
			// TODO I'm assuming here that when populating a working directory that we
			// TODO also fail rather than substitute.

			SG_utf8_converter__to_charset__sz__sz(pCtx,
												  pItem->pConverterRepoCharSet,
												  szEntryNameAsGiven,
												  &pItem->szCharSetEntryName);
			if (SG_CONTEXT__HAS_ERR(pCtx))
			{
				pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__CHARSET;
				SG_context__err_reset(pCtx);
			}
			else
			{
				pItem->lenEntryNameCharSetBytes = (SG_uint32)strlen(pItem->szCharSetEntryName);
			}
		}
	}

	SG_ASSERT(  (pItem->szUtf8EntryName && pItem->pUtf32EntryName)  &&  "Coding Error."  );
	SG_ASSERT(  (pItem->lenEntryNameUtf8Bytes >= pItem->lenEntryNameChars)  &&  "Coding Error."  );
	SG_ASSERT(  (pItem->lenEntryNameChars > 0)  &&  "Zero length entryname??"  );

	*ppItem = pItem;
	return;

fail:
	SG_ERR_IGNORE(  _sg_wc_port_item__free(pCtx, pItem)  );
}

//////////////////////////////////////////////////////////////////

void _sg_wc_port_collider_assoc_data__free(SG_context * pCtx, sg_wc_port_collider_assoc_data * pData)
{
	if (!pData)
		return;

	// we *DO NOT* own the pData->pItem.

	SG_free(pCtx, pData);
}

void _sg_wc_port_collider_assoc_data__alloc(SG_context * pCtx,
											SG_wc_port_flags why,
											sg_wc_port_item * pItem,
											sg_wc_port_collider_assoc_data ** ppData)
{
	sg_wc_port_collider_assoc_data * pData = NULL;

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pData)  );

	pData->portFlagsWhy = why;
	pData->pItem = pItem;

	*ppData = pData;
}

//////////////////////////////////////////////////////////////////

void SG_wc_port__free(SG_context * pCtx, SG_wc_port * pDir)
{
	if (!pDir)
		return;

	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pDir->prbCollider, (SG_free_callback *)_sg_wc_port_collider_assoc_data__free);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pDir->prbItems,    (SG_free_callback *)_sg_wc_port_item__free);
	SG_STRING_NULLFREE(pCtx, pDir->pStringLog);
    SG_UTF8_CONVERTER_NULLFREE(pCtx, pDir->pConverterRepoCharSet);

	SG_free(pCtx, pDir);
}

void SG_wc_port__alloc(SG_context * pCtx,
					   SG_repo * pRepo,
					   const SG_pathname * pPathWorkingDirectoryTop,
					   SG_wc_port_flags portMask,
					   SG_wc_port ** ppDir)
{
	SG_wc_port * pDir = NULL;
	char * pszCharSet = NULL;

	SG_ERR_CHECK(  SG_wc_port__get_charset_from_config(pCtx, pRepo, pPathWorkingDirectoryTop, &pszCharSet)  );

	SG_ERR_CHECK_RETURN(  SG_wc_port__alloc__with_charset(pCtx, portMask, pszCharSet, &pDir)  );

	SG_NULLFREE(pCtx, pszCharSet);
	*ppDir = pDir;
	return;

fail:
	SG_NULLFREE(pCtx, pszCharSet);
	SG_WC_PORT_NULLFREE(pCtx, pDir);
}

void SG_wc_port__alloc__with_charset(SG_context * pCtx,
									 SG_wc_port_flags mask,
									 const char * pszCharSetName,       // optional
									 SG_wc_port ** ppDir)
{
	SG_wc_port * pDir = NULL;

	SG_NULLARGCHECK(ppDir);
	// pszCharSetName is optional  -- if omitted we assume utf8.

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pDir)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pDir->pStringLog)  );

	pDir->mask = mask;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pDir->prbCollider)  );
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pDir->prbItems)  );

	SG_ERR_CHECK(  SG_utf8_converter__alloc(pCtx, pszCharSetName, &pDir->pConverterRepoCharSet)  );

	*ppDir = pDir;
	return;

fail:
	SG_WC_PORT_NULLFREE(pCtx, pDir);
}

//////////////////////////////////////////////////////////////////

static void _sg_wc_port__add_item_to_list(SG_context * pCtx,
										  SG_wc_port * pDir,
										  sg_wc_port_item * pItem)
{
	// add this item to assoc-array of items.
	// this assoc-array is keyed off of the original entryname that we were given.
	//
	// we return SG_ERR_RBTREE_DUPLICATEKEY if we already have data for this entryname.
	// (in the case of a trivial collision).

	SG_NULLARGCHECK_RETURN(pDir);
	SG_NULLARGCHECK_RETURN(pItem);

	SG_ERR_CHECK_RETURN(  SG_rbtree__add__with_assoc(pCtx, pDir->prbItems,pItem->szUtf8EntryName,(void *)pItem)  );
}

//////////////////////////////////////////////////////////////////

static void _sg_wc_port__indiv__test_invalid_or_restricted_00_7f(SG_context * pCtx,
																 sg_wc_port_item * pItem)
{
	// see if entryname has an invalid or restricted character in the
	// range [0x00 .. 0x7f].
	//
	// We know about the following INVALID characters:
	//
	// [] On Windows, the following are invalid:
	//    {} the control characters 0x00 .. 0x1f
	//    {} the characters  < > : " / \ | ? *
	//
	// [] Everywhere / is invalid.
	//
	// http://msdn.microsoft.com/en-us/library/aa365247.aspx
	//
	// We also define a set of RESTRICTED characters in the [0x00 .. 0x7f]
	// range that are known to cause problems in some combinations of OS
	// and filesystems.
	//
	// [] On Mac OS 10.5 to thumbdrive (with vfat), 0x7f DEL is invalid.
	// [] On some combinations of a Linux VM mounting an XP NTFS using
	//    VMware's vmhgfs, '%' get url-encoded giving "%25".  That is,
	//    "foo%bar.txt" gets converted to "foo%25bar.txt".
	//
	// We set INVALID if any of the CI characters are found.
	// We set RESTRICTED if any of the CR characters are found.

	const char * sz;
	SG_bool bHaveCI = SG_FALSE;
	SG_bool bHaveCR = SG_FALSE;
	SG_byte b;
	SG_byte key;

	// The following characters are either invalid or otherwise problematic
	// on at least one OS or filesystem.

#define CR			0x01	// in __INDIV__RESTRICTED_00_7F
#define CI			0x02	// in __INDIV__INVALID_00_7F

	static SG_byte sa_00_7F[0x80] = { CI, CI, CI, CI,   CI, CI, CI, CI, 	// 0x00 .. 0x07 control chars
									  CI, CI, CI, CI,   CI, CI, CI, CI, 	// 0x08 .. 0x0f control chars
									  CI, CI, CI, CI,   CI, CI, CI, CI, 	// 0x10 .. 0x17 control chars
									  CI, CI, CI, CI,   CI, CI, CI, CI, 	// 0x18 .. 0x1f control chars
									  0,  0,  CI, 0,    0,  CR, 0,  0,		// 0x20 .. 0x27 " %
									  0,  0,  CI, 0,    0,  0,  0,  CI,		// 0x28 .. 0x2f * /
									  0,  0,  0,  0,    0,  0,  0,  0,		// 0x30 .. 0x37
									  0,  0,  CI, 0,    CI, 0,  CI, CI,		// 0x38 .. 0x3f : < > ?
									  0,  0,  0,  0,    0,  0,  0,  0,		// 0x40 .. 0x47
									  0,  0,  0,  0,    0,  0,  0,  0,		// 0x48 .. 0x4f
									  0,  0,  0,  0,    0,  0,  0,  0,		// 0x50 .. 0x57
									  0,  0,  0,  0,    CI, 0,  0,  0,		// 0x58 .. 0x5f backslash
									  0,  0,  0,  0,    0,  0,  0,  0,		// 0x60 .. 0x67
									  0,  0,  0,  0,    0,  0,  0,  0,		// 0x68 .. 0x6f
									  0,  0,  0,  0,    0,  0,  0,  0,		// 0x70 .. 0x77
									  0,  0,  0,  0,    CI, 0,  0,  CR,		// 0x78 .. 0x7f | DEL
	};

	SG_UNUSED(pCtx);

	for (sz=pItem->szUtf8EntryName; (*sz); sz++)
	{
		b = (SG_byte)*sz;

		if (b >= 0x80)
			continue;

		key = sa_00_7F[b];
		if (key == 0)		// a normal, valid character
			continue;

		if (key & CI)
		{
			bHaveCI = SG_TRUE;
			if (bHaveCR)
				break;		// found both, so we can stop looking
		}

		if (key & CR)
		{
			bHaveCR = SG_TRUE;
			if (bHaveCI)
				break;		// found both, so we can stop looking
		}
	}

	if (bHaveCR)
		pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__RESTRICTED_00_7F;

	if (bHaveCI)
		pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__INVALID_00_7F;

#undef CR
#undef CI
}

static void _sg_wc_port__indiv__test_name_length(SG_context * pCtx,
												 sg_wc_port_item * pItem)
{
	// see if entryname exceeds length limits on any OS/FS.
	//
	// For entrynames, there are several OS- and filesystem-specific issues here:
	//
	// [] The total number of bytes that can be used for an entryname.
	// [] The on-disk encoding of the entryname (8-bit, 16-bit, and/or UTF-x); that is,
    //    is there a "character" based limit or a "code-point" based limit.
	// [] Whether entryname length limit is independent of pathname limit.
	//
	// Since we can't know all combinations of limits and since we need to make
	// this the minimum value, I'm going to arbitrarily make this a 255 byte limit
	// because Linux has 255 BYTE entryname limits.  Other platforms may be more
	// or in code-points/characters.
	//
	//
	// For pathnames, there are several issues here:
	//
	// [] On Windows (without the \\?\ prefix) pathnames are limited to 260 CHARACTERS.
	// [] On Windows (with the \\?\ prefix) that limit goes to either 32k or 64k (i've
	//    seen both in the docs (TODO discover if this is 32k characters/code-points
	//    which consume 64k bytes)).
	// [] Some platforms have no overall limit.
	//
	// Windows has the 260 for paths, but we always use the \\?\ prefix, so
	// we won't hit it, but the user might not be able to access a deeply nested
	// file with their editor.
	//
	// If there is an overall OS- or filesystem-specific pathname length limit, it
	// will be hit in terms of the working copy.  And we cannot tell where they
	// might put that.  So we're limited to complaining if the repo-relative path
	// hits the a limit.
	//
	// Since we're looking for a minimum, I'm going with the 256 because on Windows,
	// the 260 maps into: 256 for entryname + 3 for "C:\" + 1 for terminating NUL.
	// So with a relative path, 256 would be the longest allowable in 260.

	SG_UNUSED( pCtx );

	// we count BYTES not CODE-POINTS because Linux has byte-based limits.
	// Linux has the smallest limit for the entryname (255 bytes).
	// Mac and Winodws allow 255 or more characters because they use some
	// variation of UTF-16 internally.
	//
	// if a Linux user is allowed to use a non-UTF-8 LOCALE, we check the
	// length of the charset-based version.
	//
	// we always check the UTF-8-based length because Linux users are allowed
	// to have the LOCALE set to either.

	if (   ((pItem->szCharSetEntryName) && (pItem->lenEntryNameCharSetBytes > 255))
		|| (pItem->lenEntryNameUtf8Bytes > 255))
		pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__ENTRYNAME_LENGTH;

	// 2011/08/07 Since we cannot tell where another user will put the root of
	//            their working directory, we cannot do overall pathname length
	//            tests with any certanty, so we don't bother.

}

static void _sg_wc_port__indiv__test_final_dot_space(SG_context * pCtx,
													 sg_wc_port_item * pItem)
{
	// see if entryname has a final SPACE and/or PERIOD.
	//
	// The Windows (Win32 layer) subsystem does not allow filenames that end
	// with a final space or dot.  It silently strips them out.  This is a
	// backwards-compatibility issue with MSDOS.
	//
	// The manuals say *A* final space or dot is silently trimmed, but testing
	// shows that *ANY NUMBER* of final spaces, final dots, or any combination of
	// the two are trimmed.
	//
	// QUESTION: can I create a file named "......" on Windows?  or will this
	// always collide with ".." because of the trimming?
	//
	// This is not a restriction imposed by NTFS, but rather by the Windows subsystem
	// on top of NTFS.  You can create them from a DOS-box if you use a \\?\ style
	// pathname.  And you can remotely create such a file from Linux onto a SAMBA
	// mounted partition from Windows XP; Windows can see the files, but cannot
	// access/delete them (unless you use a \\?\ style path).
	//
	// Final spaces and dots are not a problem for other platforms.

	char ch;

	SG_UNUSED(pCtx);

	if (pItem->szUtf8EntryName[0] == '.')
	{
		SG_bool bDot = (pItem->lenEntryNameUtf8Bytes == 1);
		SG_bool bDotDot = ((pItem->lenEntryNameUtf8Bytes == 2) && (pItem->szUtf8EntryName[1] == '.'));

		SG_ASSERT( !bDot && !bDotDot && "Should not get Dot or DotDot??" );
		if (bDot || bDotDot)
			return;
	}

	ch = pItem->szUtf8EntryName[pItem->lenEntryNameUtf8Bytes-1];
	if ((ch==' ') || (ch=='.'))
		pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__FINAL_DOT_SPACE;
}

#if defined(SG_WC_PORT_DEBUG_BAD_NAME)
static void _sg_wc_port__indiv__test_debug_bad_name(SG_context * pCtx,
													sg_wc_port_item * pItem)
{
	size_t len = strlen(SG_WC_PORT_DEBUG_BAD_NAME);

	SG_UNUSED(pCtx);

	// allow any entryname that begins with the DEBUG_BAD_NAME.

    if (0 == strncmp(pItem->szUtf8EntryName, SG_WC_PORT_DEBUG_BAD_NAME, len))
    {
	    pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__DEBUG_BAD_NAME;
    }
}
#endif

static void _sg_wc_port__indiv__test_msdos_device(SG_context * pCtx,
												  sg_wc_port_item * pItem)
{
	// see if the entryname is a reserved MSDOS device name, such as "COM1".
	//
	// [] Windows doesn't like files with a reserved device name, like "COM1".
	// [] Some Windows apps also have problems with those names with a suffix,
	//    like "COM1.txt".
	//
	// http://msdn.microsoft.com/en-us/library/aa365247.aspx

	char * sz;

	SG_UNUSED(pCtx);

	//static const char * sa_devices[] =
	//	{
	//		"aux",
	//		"com1", "com2", "com3", "com4", "com5", "com6", "com7", "com8", "com9",
	//		"con",
	//		"lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9",
	//		"nul",
	//		"prn",
	//	};

#define CH(ch,l,u)	(((ch)==(l)) || ((ch)==(u)))
#define C19(ch)		(((ch)>='1') && ((ch)<='9'))
#define CDOTNUL(ch)	(((ch)=='.') || ((ch)==0))

	sz = pItem->szUtf8EntryName;
	switch (sz[0])
	{
	case 'a':
	case 'A':
		if (CH(sz[1],'u','U')  &&  CH(sz[2],'x','X')  &&  CDOTNUL(sz[3]))
			goto found_match;
		return;

	case 'c':
	case 'C':
		if (CH(sz[1],'o','O'))
		{
			if (CH(sz[2],'m','M')  &&  C19(sz[3])  &&  CDOTNUL(sz[4]))
				goto found_match;
			if (CH(sz[2],'n','N')  &&  CDOTNUL(sz[3]))
				goto found_match;
		}
		return;

	case 'l':
	case 'L':
		if (CH(sz[1],'p','P')  &&  CH(sz[2],'t','T')  &&  C19(sz[3])  &&  CDOTNUL(sz[4]))
			goto found_match;
		return;

	case 'n':
	case 'N':
		if (CH(sz[1],'u','U')  &&  CH(sz[2],'l','L')  &&  CDOTNUL(sz[3]))
			goto found_match;
		return;

	case 'p':
	case 'P':
		if (CH(sz[1],'r','R')  &&  CH(sz[2],'n','N')  &&  CDOTNUL(sz[3]))
			goto found_match;
		return;

	default:
		return;
	}

#undef CH
#undef C19
#undef CDOTNUL

found_match:
	pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__MSDOS_DEVICE;
}

static void _sg_wc_port__indiv__test_nonbmp(SG_context * pCtx,
											sg_wc_port_item * pItem)
{
	// see if the entryname contains a non-bmp character ( > U+FFFF ).
	//
	// This can be problematic because some platforms:
	//
	// [] support Non-BMP characters in entrynames (either because they
	//    (actively/blindly) allow arbitrary UTF-8 sequences or (actively/
	//     blindly) allow the multi-word UTF-16 sequences).
	//
	// [] don't support Non-BMP characters (because they use UCS-2 or
	//    Unicode 2.0 internally).
	//
	// [] actively (and silently) REPLACE Non-BMP characters with a Unicode
	//    replacement character.  We've observed U+FFFD REPLACEMENT CHARACTER
	//    being used for this.   So, for example, create() may succeed but
	//    readdir() won't find it because of a character substitution.

	SG_uint32 k;

	SG_UNUSED(pCtx);

	if (pItem->b7BitClean)
		return;

	for (k=0; k<pItem->lenEntryNameChars; k++)
	{
		if (pItem->pUtf32EntryName[k] > 0x0000ffff)
		{
			pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__NONBMP;
			return;
		}
	}
}

static void _sg_wc_port__indiv__test_ignorable(SG_context * pCtx,
											   sg_wc_port_item * pItem)
{
	// see if entryname contains Unicode "Ignorable" characters.
	//
	// Various systems differ on how "Ignorable" characters are handled:
	//
	// [] significant - preserved on create, significant during lookups.
	// [] ignored - preserved on create, ignoed on lookups.
	//
	// HFS+ (and HFSX in case-insensitive mode) allow files to be created
	// with "ignorable" characters.  When you create a filename containing
	// ignorable characters, you can see the ignorables with readdir().
	//
	// But you can stat() the file using a filename that contains:
	// {} the ignorables,
	// {} one that omits the ignorables,
	// {} one that has DIFFERENT ignorables in it.
	//
	// Basically, they are omitted from the lookup comparison.
	//
	// http://developer.apple.com/technotes/tn/tn1150.html#UnicodeSubtleties
	// See also Apple's FastUnicodeCompare.c

	SG_bool bContainsIgnorables;

	if (pItem->b7BitClean)
		return;

	SG_ERR_CHECK_RETURN(  SG_apple_unicode__contains_ignorables__utf32(pCtx,
																	   pItem->pUtf32EntryName,
																	   &bContainsIgnorables)  );
	if (bContainsIgnorables)
		pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__IGNORABLE;
}

static void _sg_wc_port__indiv__test_shortnames(SG_context * pCtx,
												sg_wc_port_item * pItem)
{
	// On Windows, files/directories can have 2 names:
	// [] the long name
	// [] the somewhat hidden 8.3 short name
	//
	// Normally, we only deal with the long names.  But if someone on
	// Linux creates both "Program Files" and "PROGRA~1" as 2 separate
	// directories and then tries to move the tree to Windows, they may
	// have problems.
	//
	// Since the short name is auto-generated by Windows (a variable
	// length prefix and a sequence number), we cannot tell what it
	// will be for any given long name.
	//
	// Therefore, we complain if the given name ***LOOKS*** like it
	// might be a short name.
	//
	// http://msdn.microsoft.com/en-us/library/aa365247.aspx
	// http://en.wikipedia.org/wiki/8.3_filename
	//
	// based upon the Wikipedia article and some experimentation,
	// shortnames look like:
	//
	//    01234567
	// [] XXXXXX~1.XXX
	// [] XXhhhh~1.XXX
	// [] Xhhhh~1.XXX

	SG_int32 * p32;

	SG_UNUSED(pCtx);

#define C09(ch)		(((ch)>='0') && ((ch)<='9'))
#define CAF(ch)		(((ch)>='A') && ((ch)<='F'))
#define Caf(ch)		(((ch)>='a') && ((ch)<='f'))
#define CHEX(ch)	(C09(ch) || CAF(ch) || Caf(ch))
#define CDOTNUL(ch)	(((ch)=='.') || ((ch)==0))
#define CTILDA(ch)	((ch)=='~')

	p32 = pItem->pUtf32EntryName;

	if (pItem->lenEntryNameChars >= 7)
	{
		if (CTILDA(p32[5]) && C09(p32[6]) && CDOTNUL(p32[7]))
			if (CHEX(p32[1]) && CHEX(p32[2]) && CHEX(p32[3]) && CHEX(p32[4]))
				goto found_match;
	}
	if (pItem->lenEntryNameChars >= 8)
	{
		if (CTILDA(p32[6]) && C09(p32[7]) && CDOTNUL(p32[8]))
			goto found_match;
	}

	return;

found_match:
	pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__SHORTNAMES;

#undef C09
#undef CAF
#undef Caf
#undef CHEX
#undef CDOTNUL
#undef CTILDA
}

static void _sg_wc_port__indiv__test_sfm_ntfs(SG_context * pCtx,
											  sg_wc_port_item * pItem)
{
	// The Mac has something called "Services for Macintosh" that
	// allows the mac to use NTFS filesystems.  It maps the various
	// invalid ntfs characters 0x00..0x1f, '"', '*', '/', ':', and
	// etc ***as well as*** final spaces and dots into Unicode
	// Private Use characters in the range [u+f001 .. u+f029].
	//
	// i've confirmed that this happens between Mac OS X 10.5.5
	// and a remote NTFS partition on XP.
	//
	// i have not tested whether it happens on directly mounted
	// NTFS partitions, but i'm not sure it matters.  the important
	// thing is that somebody does it, so we need to test for it.
	//
	// The exact remapping is described in:
	//
	// http://support.microsoft.com/kb/q117258/
	//
	// ***NOTE*** This is documented as if it were a Windows feature
	// and something that NT 3.5 and 4.0 exported to MACs.  ***BUT***
	// I found evidence in the mac source that they''ve got code for
	// doing it too.  ***And*** the mac version substitues ':' for '/'
	// in the table because they changed pathname separators between
	// OS 9 and OS X.
	//
	// http://www.opensource.apple.com/darwinsource/10.5.6/xnu-1228.9.59/bsd/vfs/vfs_utfconv.c

	SG_bool bContainsSFM;

	if (pItem->b7BitClean)
		return;

	SG_ERR_CHECK_RETURN(  SG_apple_unicode__contains_sfm_chars__utf32(pCtx,
																	  pItem->pUtf32EntryName,
																	  &bContainsSFM)  );
	if (bContainsSFM)
		pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__SFM_NTFS;
}

static void _sg_wc_port__indiv__test_leading_dash(SG_context * pCtx,
												  sg_wc_port_item * pItem)
{
	// check for a leading dash in the filename.  this can cause problems
	// if a user on Windows creates a file with a leading dash (where the
	// forward slash is used to denote command line arguments) and then is
	// used on Linux (where the dash is used to denote command line arguments).

	SG_UNUSED(pCtx);

	if (pItem->szUtf8EntryName[0] == '-')
		pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__LEADING_DASH;
}

#if 0
static void _sg_wc_port__indiv__test_symlink(SG_context * pCtx,
											 sg_wc_port_item * pItem)
{
	SG_UNUSED(pCtx);

	if (pItem->tneType == SG_TREENODEENTRY_TYPE_SYMLINK)
		pItem->flagsObserved |= SG_WC_PORT_FLAGS__INDIV__SYMLINK;
}
#endif

//////////////////////////////////////////////////////////////////

static void _sg_wc_port__alloc_indiv_msg(SG_context * pCtx,
										 SG_string ** ppStringMsg,
										 SG_wc_port * pDir,
										 SG_wc_port_flags wf)
{
	SG_string * pString = NULL;
	SG_uint32 nr_set = 0;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );

	// append a series of descriptions for what was done to the entryname
	// to make it collide with something else.

#define M(wf,bit,msg)													\
	SG_STATEMENT(														\
		if ((wf) & (pDir->mask) & (bit))								\
		{																\
			if (nr_set++)												\
				SG_ERR_CHECK_RETURN(  SG_string__append__sz(pCtx,		\
															pString,	\
															" ")  );	\
			SG_ERR_CHECK_RETURN(  SG_string__append__sz(pCtx,			\
														pString,		\
														(msg))  );		\
		}																\
		)

	M(wf,SG_WC_PORT_FLAGS__INDIV__INVALID_00_7F,			"INVALID_CHARS");
	M(wf,SG_WC_PORT_FLAGS__INDIV__RESTRICTED_00_7F,			"RESTRICTED_CHARS");
	M(wf,SG_WC_PORT_FLAGS__INDIV__ENTRYNAME_LENGTH,			"ENTRYNAME_LENGTH");
//	M(wf,SG_WC_PORT_FLAGS__INDIV__PATHNAME_LENGTH,			"PATHNAME_LENGTH");
	M(wf,SG_WC_PORT_FLAGS__INDIV__FINAL_DOT_SPACE,			"FINAL_DOT_SPACE");
	M(wf,SG_WC_PORT_FLAGS__INDIV__MSDOS_DEVICE,				"MSDOS_DEVICE");
	M(wf,SG_WC_PORT_FLAGS__INDIV__NONBMP,					"UNICODE_NON_BMP");
	M(wf,SG_WC_PORT_FLAGS__INDIV__IGNORABLE,				"UNICODE_IGNORABLE");
	M(wf,SG_WC_PORT_FLAGS__INDIV__CHARSET,					"CHARSET");
	M(wf,SG_WC_PORT_FLAGS__INDIV__SHORTNAMES,				"MSDOS_SHORTNAME");
	M(wf,SG_WC_PORT_FLAGS__INDIV__SFM_NTFS,					"APPLE_SFM_NTFS");
	M(wf,SG_WC_PORT_FLAGS__INDIV__NON_CANONICAL_NFD,		"UNICODE_NON_CANONICAL");
#if defined(SG_WC_PORT_DEBUG_BAD_NAME)
	M(wf,SG_WC_PORT_FLAGS__INDIV__DEBUG_BAD_NAME,			"DEBUG_BAD_NAME");
#endif
	M(wf,SG_WC_PORT_FLAGS__INDIV__LEADING_DASH,				"LEADING_DASH");
#if 0
	M(wf,SG_WC_PORT_FLAGS__INDIV__SYMLINK,					"SYMLINK");
#endif

#undef M

	*ppStringMsg = pString;
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

/**
 * Log a message for any individual potential problems.
 *
 */
static void _sg_wc_port__log_indiv(SG_context * pCtx,
								   SG_wc_port * pDir,
								   sg_wc_port_item * pItem,
								   SG_string * pStringToLogInto)
{
	SG_string * pStringWhy = NULL;

	SG_ERR_CHECK(  _sg_wc_port__alloc_indiv_msg(pCtx, &pStringWhy, pDir, pItem->flagsObserved)  );
	
	SG_ERR_CHECK(  SG_string__append__format(pCtx,
											 pStringToLogInto,
											 ("Problematic entryname: '%s' [%s]\n"),
											 pItem->szUtf8EntryName,
											 SG_string__sz(pStringWhy))  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringWhy);
}

//////////////////////////////////////////////////////////////////

/**
 * Inspect the given file/directory entryname (which is a part of the
 * container's relative pathname) and see if it has any problematic
 * chars and add it to our container so that we can check for collisions.
 *
 * If we find a problem, we update the pItem->flagsObserved.
 *
 * We try to be as verbose as possible and completely list warnings
 * rather than stopping after the first one.
 */
static void _sg_wc_port__inspect_item(SG_context * pCtx,
									  SG_wc_port * pDir,
									  sg_wc_port_item * pItem)
{
	// run all the individual inspections on the given entryname and
	// repo-relative pathname and set any __INDIV__ bits that apply.

	SG_NULLARGCHECK_RETURN(pItem);

#if defined(SG_WC_PORT_DEBUG_BAD_NAME)
	SG_ERR_CHECK(  _sg_wc_port__indiv__test_debug_bad_name(pCtx, pItem)  );
#endif
	SG_ERR_CHECK(  _sg_wc_port__indiv__test_invalid_or_restricted_00_7f(pCtx, pItem)  );
	SG_ERR_CHECK(  _sg_wc_port__indiv__test_name_length(pCtx, pItem)  );
	SG_ERR_CHECK(  _sg_wc_port__indiv__test_final_dot_space(pCtx, pItem)  );
	SG_ERR_CHECK(  _sg_wc_port__indiv__test_msdos_device(pCtx, pItem)  );
	SG_ERR_CHECK(  _sg_wc_port__indiv__test_nonbmp(pCtx, pItem)  );
	SG_ERR_CHECK(  _sg_wc_port__indiv__test_ignorable(pCtx, pItem)  );
	SG_ERR_CHECK(  _sg_wc_port__indiv__test_shortnames(pCtx, pItem)  );
	SG_ERR_CHECK(  _sg_wc_port__indiv__test_sfm_ntfs(pCtx, pItem)  );
	SG_ERR_CHECK(  _sg_wc_port__indiv__test_leading_dash(pCtx, pItem)  );

#if 0
	if (pItem->tneType == SG_TREENODEENTRY_TYPE__INVALID)
	{
		// Caller did not have or did not want us to look at the tneType
		// on this item.  (This is needed to allow to prevent symlinks
		// from being ADDED, and yet not choke on FOUND symlinks.)
	}
	else
	{
		SG_ERR_CHECK(  _sg_wc_port__indiv__test_symlink(pCtx, pItem)  );
	}
#endif

	// update the union of all warning flags.  this only includes the
	// __INDIV__ bits and thus is possibly incomplete because we haven't
	// computed the potential collisions yet.

	pDir->flagsUnion |= pItem->flagsObserved;

	// if the item is new and it has one or more potential problems that
	// we care about, then give a detailed description.

	if (pItem->flagsObserved & pDir->mask & SG_WC_PORT_FLAGS__MASK_INDIV)
	{
		SG_ERR_CHECK(  _sg_wc_port__log_indiv(pCtx, pDir, pItem, pDir->pStringLog)  );
		SG_ERR_CHECK(  _sg_wc_port__log_indiv(pCtx, pDir, pItem, pItem->pStringItemLog)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _sg_wc_port__alloc_collision_msg(SG_context * pCtx,
											 SG_string ** ppStringMsg,
											 SG_wc_port * pDir,
											 SG_wc_port_flags why)
{
	SG_string * pString = NULL;
	SG_uint32 nr_set = 0;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );

	// append a series of descriptions for what was done to the entryname
	// to make it collide with something else.

#define M(wf,bit,msg)													\
	SG_STATEMENT(														\
		if ((wf) & (pDir->mask) & (bit))								\
		{																\
			if (nr_set++)												\
				SG_ERR_CHECK_RETURN(  SG_string__append__sz(pCtx,		\
															pString,	\
															" ")  );	\
			SG_ERR_CHECK_RETURN(  SG_string__append__sz(pCtx,			\
														pString,		\
														(msg))  );		\
		}																\
		)

	M(why,SG_WC_PORT_FLAGS__COLLISION__ORIGINAL,			"ORIGINAL");
	M(why,SG_WC_PORT_FLAGS__COLLISION__CASE,				"CASE");
	M(why,SG_WC_PORT_FLAGS__COLLISION__FINAL_DOT_SPACE,		"FINAL_DOT_SPACE");
	M(why,SG_WC_PORT_FLAGS__COLLISION__PERCENT25,			"PERCENT25");
	M(why,SG_WC_PORT_FLAGS__COLLISION__CHARSET,				"CHARSET");
	M(why,SG_WC_PORT_FLAGS__COLLISION__SFM_NTFS,			"APPLE_SFM_NTFS");
	M(why,SG_WC_PORT_FLAGS__COLLISION__NONBMP,				"UNICODE_NON_BMP");
	M(why,SG_WC_PORT_FLAGS__COLLISION__IGNORABLE,			"UNICODE_IGNORABLE");
	M(why,SG_WC_PORT_FLAGS__COLLISION__NFC,					"UNICODE_NFC");
#if defined(SG_WC_PORT_DEBUG_BAD_NAME)
	M(why,SG_WC_PORT_FLAGS__COLLISION__DEBUG_BAD_NAME,		"DEBUG_BAD_NAME");
#endif

	// TODO more...

#undef M

	*ppStringMsg = pString;
	return;

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}
	
/**
 * Generate a log message for this actual/potential collision
 * into the given string.  Note that we use this routine to
 * build the overall summary log and the individual item logs
 * (which explains why we pass it in rather than just using
 * the log in either pDir or pItem[12]).
 */
static void _sg_wc_port__log_collision(SG_context * pCtx,
									   SG_wc_port * pDir, const char * szUtf8Key,
									   sg_wc_port_item * pItem1, SG_wc_port_flags why1,
									   sg_wc_port_item * pItem2, SG_wc_port_flags why2,
									   SG_string * pStringToLogInto)
{
	SG_string * pStringWhy1 = NULL;
	SG_string * pStringWhy2 = NULL;

	SG_ERR_CHECK(  _sg_wc_port__alloc_collision_msg(pCtx, &pStringWhy1, pDir, why1)  );
	SG_ERR_CHECK(  _sg_wc_port__alloc_collision_msg(pCtx, &pStringWhy2, pDir, why2)  );
	
	SG_ERR_CHECK(  SG_string__append__format(pCtx,
											 pStringToLogInto,
											 ("Potential entryname collision on: '%s':\n"
											  "              '%s' [%s]\n"
											  "              '%s' [%s]\n"),
											 szUtf8Key,
											 pItem1->szUtf8EntryName, SG_string__sz(pStringWhy1),
											 pItem2->szUtf8EntryName, SG_string__sz(pStringWhy2))  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringWhy1);
	SG_STRING_NULLFREE(pCtx, pStringWhy2);
}

//////////////////////////////////////////////////////////////////

static void _my_assoc_items(SG_context * pCtx,
							sg_wc_port_item * pItem_p,
							sg_wc_port_item * pItem_q)
{
#if 1 || TRACE_WC_PORT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "PortAssocItem: %s %s\n",
							   pItem_p->bufGid_Entry,
							   pItem_q->bufGid_Entry)  );
	SG_ASSERT(  (strcmp(pItem_p->bufGid_Entry, pItem_q->bufGid_Entry) != 0)  );
#endif

	SG_ERR_CHECK(  SG_rbtree__update__with_assoc(pCtx, pItem_p->prbCollidedWith, pItem_q->bufGid_Entry, pItem_q, NULL)  );
	SG_ERR_CHECK(  SG_rbtree__update__with_assoc(pCtx, pItem_q->prbCollidedWith, pItem_p->bufGid_Entry, pItem_p, NULL)  );

fail:
	return;
}
	
/**
 * We optionally associate each item with each other in the per-item
 * "collided-with" map and incrementally build a transitive closure
 * on this collision with any previous collisions involving either item.
 *
 * Assume we have items {A, B, C} in one partition (each with 2 edges
 * to the other 2).  And items {X, Y, Z} in another partition (with 2
 * edges each).  And have been asked to associate A and X with each
 * other and then do the closure.  We should finish with 1 partition
 * with 6 items each having 5 edges.
 * 
 * This is not the most efficient transitive closure calculation, but
 * I expect in the normal case the partitions for A and X to have 1
 * member each. So I'm not worried about it.
 *
 */
static void _sg_wc_port__associate_items_in_collision(SG_context * pCtx,
													  sg_wc_port_item * pItem1,
													  sg_wc_port_item * pItem2)
{
	SG_rbtree_iterator * pIter1 = NULL;
	SG_rbtree_iterator * pIter2 = NULL;
	const char * pszKey1_k;
	const char * pszKey2_j;
	sg_wc_port_item * pItem1_k;
	sg_wc_port_item * pItem2_j;
	SG_bool bHavePrior1 = (pItem1->prbCollidedWith != NULL);
	SG_bool bHavePrior2 = (pItem2->prbCollidedWith != NULL);
	SG_bool bFound1;
	SG_bool bFound2;

	if (!pItem1->bufGid_Entry[0] || !pItem2->bufGid_Entry[0])
		return;		// if caller didn't give us GIDs, don't bother.

	if (bHavePrior1 && bHavePrior2)
	{
		SG_bool bAlreadyKnown12 = SG_FALSE;

		// the "collided-with" set always has a complete closure
		// on all of the collisions we already know about, so if
		// we already know these 2 items are related, we're done.

		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pItem1->prbCollidedWith, pItem2->bufGid_Entry, &bAlreadyKnown12, NULL)  );
		if (bAlreadyKnown12)
			return;

		// step 1: associate {B, C} with {Y, Z}.

		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter1, pItem1->prbCollidedWith,
												  &bFound1, &pszKey1_k, (void **)&pItem1_k)  );
		while (bFound1)
		{
			SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter2, pItem2->prbCollidedWith,
													  &bFound2, &pszKey2_j, (void **)&pItem2_j)  );
			while (bFound2)
			{
				SG_ERR_CHECK(  _my_assoc_items(pCtx, pItem1_k, pItem2_j)  );

				SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter2,
														 &bFound2, &pszKey2_j, (void **)&pItem2_j)  );
			}
			SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter2);
			
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter1,
													 &bFound1, &pszKey1_k, (void **)&pItem1_k)  );
		}
		SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter1);
	}

	if (!bHavePrior1)
		SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pItem1->prbCollidedWith)  );
	if (!bHavePrior2)
		SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pItem2->prbCollidedWith)  );

	// step 2: if {B, C} present, associate {B, C} with X.
	if (bHavePrior1)
	{
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter1, pItem1->prbCollidedWith,
												  &bFound1, &pszKey1_k, (void **)&pItem1_k)  );
		while (bFound1)
		{
			SG_ERR_CHECK(  _my_assoc_items(pCtx, pItem1_k, pItem2)  );

			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter1,
													 &bFound1, &pszKey1_k, (void **)&pItem1_k)  );
		}
		SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter1);
	}

	// step 3: if {Y, Z} present, associate {Y, Z} with A.
	if (bHavePrior2)
	{
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter2, pItem2->prbCollidedWith,
												  &bFound2, &pszKey2_j, (void **)&pItem2_j)  );
		while (bFound2)
		{
			SG_ERR_CHECK(  _my_assoc_items(pCtx, pItem1, pItem2_j)  );

			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter2,
													 &bFound2, &pszKey2_j, (void **)&pItem2_j)  );
		}
		SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter2);
	}

	// step 4: associate A with X.
	SG_ERR_CHECK(  _my_assoc_items(pCtx, pItem1, pItem2)  );

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter1);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter2);
}


//////////////////////////////////////////////////////////////////

/**
 * add an individual name mangling to our container.
 * if we get a collision, log it and continue.
 */
static void _sg_wc_port__add_mangling(SG_context * pCtx,
									  SG_wc_port * pDir, const char * szUtf8Key,
									  sg_wc_port_item * pItem, SG_wc_port_flags portWhy)
{
	sg_wc_port_collider_assoc_data * pData = NULL;
	sg_wc_port_collider_assoc_data * pDataCollision;
	SG_bool bDuplicateKey;

#if TRACE_WC_PORT
	{
		SG_int_to_string_buffer bufI64;
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "PORT: for [%s] add mangling [%s] [reason %s]\n",
								   pItem->szUtf8EntryName,
								   szUtf8Key,
								   SG_uint64_to_sz__hex(portWhy,bufI64))  );
	}
#endif

	// portWhy gives us the SG_WC_PORT_FLAGS__PAIRWISE__ bit describing why we are
	// adding this entry to prbCollider.  For example, this can be because the key
	// is the original entry name or because we are adding the lowercase version
	// and etc.  This will be used when we have a collision and generate a better
	// warning.  For example on a Linux system, you might get something like:
	//      the lowercase("FOO") collided with the lowercase("Foo")

	SG_ERR_CHECK_RETURN(  _sg_wc_port_collider_assoc_data__alloc(pCtx, portWhy,pItem,&pData)  );

	// try to add (key,(why,item)) to rbtree.  if this succeeds, we did not
	// collide with anything and can just return.

	SG_rbtree__add__with_assoc(pCtx, pDir->prbCollider,szUtf8Key,pData);
	if (SG_CONTEXT__HAS_ERR(pCtx) == SG_FALSE)
		return;

	bDuplicateKey = SG_context__err_equals(pCtx,SG_ERR_RBTREE_DUPLICATEKEY);

	// we could not add (key,pData) to the rbtree, so free pData before we
	// go any further.

	SG_ERR_IGNORE(  _sg_wc_port_collider_assoc_data__free(pCtx, pData)  );
	pData = NULL;

	if (!bDuplicateKey)
		SG_ERR_RETHROW_RETURN;

	SG_context__err_reset(pCtx);

	// we have a collision.  make sure it isn't us.  (this could happen if
	// 2 different manglings just happen to produce the same result (such
	// as when we are intially given a completely lowercase filename and our
	// normalization computes a matching one).)

	SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx, pDir->prbCollider,szUtf8Key,NULL,(void **)&pDataCollision)  );
	if (pDataCollision->pItem == pItem)
		return;

	// we have a real collision.  add the WHY bits to the actual warnings
	// on these items.

	pItem->flagsObserved |= portWhy;
	pDataCollision->pItem->flagsObserved |= pDataCollision->portFlagsWhy;

	// update the union of all warning flags on the entire directory to include
	// the WHY bits here.  note that the directory warnings bits are possibly
	// still incomplete because we haven't completed computing the collisions yet.

	pDir->flagsUnion |= portWhy | pDataCollision->portFlagsWhy;

	// if one or both are new items and caller cares about this type of problem,
	// then we should give a detailed collision description.

	if ((portWhy | pDataCollision->portFlagsWhy) & pDir->mask & SG_WC_PORT_FLAGS__MASK_COLLISION)
	{
		SG_ERR_CHECK_RETURN(  _sg_wc_port__log_collision(pCtx,
														 pDir,szUtf8Key,
														 pItem,portWhy,
														 pDataCollision->pItem,pDataCollision->portFlagsWhy,
														 pDir->pStringLog)  );
		SG_ERR_CHECK_RETURN(  _sg_wc_port__log_collision(pCtx,
														 pDir,szUtf8Key,
														 pItem,portWhy,
														 pDataCollision->pItem,pDataCollision->portFlagsWhy,
														 pItem->pStringItemLog)  );
		SG_ERR_CHECK_RETURN(  _sg_wc_port__log_collision(pCtx,
														 pDir,szUtf8Key,
														 pDataCollision->pItem,pDataCollision->portFlagsWhy,
														 pItem,portWhy,
														 pDataCollision->pItem->pStringItemLog)  );
	}

	SG_ERR_CHECK_RETURN(  _sg_wc_port__associate_items_in_collision(pCtx, pDataCollision->pItem, pItem)  );

}

//////////////////////////////////////////////////////////////////

static void _sg_wc_port__collide_item_7bit_clean(SG_context * pCtx,
												 SG_wc_port * pDir,
												 sg_wc_port_item * pItem)
{
	// do 7-bit clean portion of collide_item using the given entryname
	// and flags as a starting point.
	//
	// that is, we compute an "effectively equivalent" entryname; one that
	// or more actual filesystems may interpret/confuse with the given entryname.
	// this is not an exact match -- this is an equivalence class.  that is, i'm
	// not going to individually compute all the possible permutations of confusion
	// (such as only folding case for the mac and then separately stripping final
	// whitespace for windows).  instead, we apply all the transformations and
	// compute the base that all members of this class will map to.

	char * szCopy = NULL;
	char * p;
	const char * szEntryNameStart = pItem->szUtf8EntryName;
	SG_wc_port_flags portWhyStart = SG_WC_PORT_FLAGS__NONE;
	SG_wc_port_flags portWhyComputed = portWhyStart;

	// create a copy of the original entryname so that we can trash/munge it.

	SG_ERR_CHECK(  SG_STRDUP(pCtx, szEntryNameStart,&szCopy)  );

	if (pItem->flagsObserved & SG_WC_PORT_FLAGS__INDIV__RESTRICTED_00_7F)
	{
		// Whenn using VMWare's host-guest filesystem (vmhgfs), we sometimes get
		// filenames with '%' mapped to "%25".  I've seen this running an Ubunutu 8.04
		// VM inside VMWare 6 on XP with an NTFS directory shared into the VM.
		//
		// TODO determine if this was a bug in VMWare or if they want this feature.
		//
		// TODO determine if this happens recursively -- that is, if "foo%.txt" get
		// TODO mapped to "foo%25.txt", can that get mapped to "foo%%2525.txt"?
		//
		// Since the point here is to determine *potential* collisions (and we can't
		// tell if the user actually created the file with this name or if VMHGFS
		// munged it), collapse all "%25" to "%" and don't worry about which one we
		// actually had.

		p = szCopy;
		while (*p)
		{
			if ((p[0]=='%')  &&  (p[1]=='2')  && (p[2]=='5'))
			{
				memmove(&p[1],&p[3],strlen(&p[3])+1);
				portWhyComputed |= SG_WC_PORT_FLAGS__COLLISION__PERCENT25;

				// restart with this '%' so that we collapse "%%2525" to "%%".

				continue;
			}

			p++;
		}
	}

	if (pItem->flagsObserved & SG_WC_PORT_FLAGS__INDIV__FINAL_DOT_SPACE)
	{
		// on a Windows/NTFS filesystem, final space and dot normally get stripped.
		// the docs only talk about the final character, but testing shows that all
		// trailing space/dot get stripped.

		p = szCopy + strlen(szCopy) - 1;
		while (p > szCopy)
		{
			if ((*p != ' ') && (*p != '.'))
				break;

			*p-- = 0;
			portWhyComputed |= SG_WC_PORT_FLAGS__COLLISION__FINAL_DOT_SPACE;
		}
	}

	// both Mac and Windows are case-preserving, but not case-sensitive.  so
	// we convert the entryname to lowercase to simulate this.  for US-ASCII,
	// both platforms use the same case-conversion rules.

	for (p=szCopy; (*p); p++)
	{
		if ((*p >= 'A') && (*p <= 'Z'))
		{
			*p = 'a' + (*p - 'A');
			portWhyComputed |= SG_WC_PORT_FLAGS__COLLISION__CASE;
		}
	}

	// if we added something to the flags, add our string to the collider.

	if (portWhyComputed != portWhyStart)
	{
		SG_ERR_CHECK(  _sg_wc_port__add_mangling(pCtx, pDir,szCopy,pItem,portWhyComputed)  );
	}
	else
	{
		// this is a coding error, but it is safe to let this continue.
		// our collision detector will just be missing an entry.
		SG_ASSERT( (strcmp(szEntryNameStart,szCopy) == 0)  &&  "Filename munged but no bits set." );
	}

	// fall-thru to common cleanup.

fail:
	SG_NULLFREE(pCtx, szCopy);
}

static void _sg_wc_port__collide_item_32bit(SG_context * pCtx,
											SG_wc_port * pDir,
											sg_wc_port_item * pItem)
{
	// do full collision detection assuming we have UTF-32 data that
	// contains some non-US-ASCII characters.
	//
	// again, we want to compute the base entryname for the equivalence class
	// rather than all possible permutations.

	SG_int32 * pUtf32Copy = NULL;
	SG_int32 * pUtf32CopyNFC = NULL;
	char * pUtf8Copy = NULL;
	char * pUtf8CopyNFC = NULL;
	char * pUtf8CopyNFD = NULL;
	SG_uint32 k;
	SG_uint32 lenCopyChars;
	SG_bool bHadFinalDotSpaceSFM = SG_FALSE;
	SG_wc_port_flags portWhyStart = SG_WC_PORT_FLAGS__NONE;
	SG_wc_port_flags portWhyComputed = portWhyStart;

	// create a copy of the UTF-32 entryname so that we can trash/munge it.

	SG_ERR_CHECK(  SG_allocN(pCtx, pItem->lenEntryNameChars+1,pUtf32Copy)  );

	memcpy(pUtf32Copy,pItem->pUtf32EntryName,pItem->lenEntryNameChars*sizeof(SG_int32));
	lenCopyChars = pItem->lenEntryNameChars;

	if (pItem->flagsObserved & SG_WC_PORT_FLAGS__INDIV__NONBMP)
	{
		// convert all non-bmp (those > u+ffff) characters into a
		// U+FFFD REPLACEMENT CHARACTER.  some systems support and
		// respect non-bmp characters and some platforms fold them
		// all to this replacement character.

		for (k=0; k<lenCopyChars; k++)
		{
			if (pUtf32Copy[k] > 0x00ffff)
			{
				pUtf32Copy[k] = 0x00fffd;
				portWhyComputed |= SG_WC_PORT_FLAGS__COLLISION__NONBMP;
			}
		}
	}

	if (pItem->flagsObserved & SG_WC_PORT_FLAGS__INDIV__RESTRICTED_00_7F)
	{
		// see note in 7bit version about "%25" on VMHGFS.

		k = 0;
		while (k < lenCopyChars-2)
		{
			if ((pUtf32Copy[k] == '%')  &&  (pUtf32Copy[k+1] == '2')  &&  (pUtf32Copy[k+2] == '5'))
			{
				// strip out "25" in "%25".  we add 1 to bring along the trailing null.
				// then restart loop with this '%' so that we collapse "%%2525" to "%%".

				memmove(&pUtf32Copy[k+1],&pUtf32Copy[k+3],
					   sizeof(SG_int32)*(lenCopyChars - (k+3) + 1));
				lenCopyChars -= 2;

				portWhyComputed |= SG_WC_PORT_FLAGS__COLLISION__PERCENT25;
			}
			else
			{
				k++;
			}
		}
	}

	if (pItem->flagsObserved & SG_WC_PORT_FLAGS__INDIV__SFM_NTFS)
	{
		// go thru the string and convert any SFM characters to the US-ASCII equivalents.
		// (we have to do this before we do the final_dot_space checks.)

		for (k=0; k<lenCopyChars; k++)
		{
			SG_int32 ch32;

			SG_ERR_CHECK(  SG_apple_unicode__map_sfm_char_to_ascii__utf32(pCtx, pUtf32Copy[k],&ch32)  );
			if (ch32 != pUtf32Copy[k])
			{
				pUtf32Copy[k] = ch32;
				portWhyComputed |= SG_WC_PORT_FLAGS__COLLISION__SFM_NTFS;

				if ((ch32 == ' ') || (ch32 == '.'))
					bHadFinalDotSpaceSFM = SG_TRUE;
			}
		}
	}

	if ((pItem->flagsObserved & SG_WC_PORT_FLAGS__INDIV__FINAL_DOT_SPACE)  ||  bHadFinalDotSpaceSFM)
	{
		// see note in 7bit version for final dot and space.

		while (lenCopyChars > 0)
		{
			SG_int32 ch32 = pUtf32Copy[lenCopyChars-1];

			if ((ch32 != '.') && (ch32 != ' '))
				break;

			pUtf32Copy[--lenCopyChars] = 0;
			portWhyComputed |= SG_WC_PORT_FLAGS__COLLISION__FINAL_DOT_SPACE;
		}
	}

	if (pItem->bHasDecomposition)
	{
		// the NFC and NFD forms of the entryname are different.
		// we (arbitrarily) pick NFC as the base case for the collider
		// and convert everything to that.
		//
		// we have to do this before we strip ignorables so that we don't
		// accidentally combine or permute 2 accent characters that were
		// separated by an ignorable.

		if (!pItem->bIsProperlyComposed)
		{
			// <rant>I'm sick of these Unicode routines (ICU's, Apple's,
			// and my wrappers for them).  Some want UTF-8, some want UTF-16,
			// and some want UTF-32.  Apple's routines take UTF-8 and secretely
			// convert to UTF-16 to do the work and then convert the result
			// back to UTF-8.  So I have to convert from UTF-32 to UTF-8 just
			// so that I can call it.</rant>

			SG_uint32 lenUtf8Copy, lenUtf8CopyNFC, lenUtf8CopyNFD, lenUtf32CopyNFC;

			// convert our working UTF-32 buffer to UTF-8

			SG_ERR_CHECK(  SG_utf8__from_utf32(pCtx, pUtf32Copy,&pUtf8Copy,&lenUtf8Copy)  );
			if (pItem->bIsProperlyDecomposed)
			{
				// if original is canonical NFD, we can directly compute NFC from it.
				SG_ERR_CHECK(  SG_apple_unicode__compute_nfc__utf8(pCtx, pUtf8Copy,lenUtf8Copy, &pUtf8CopyNFC, &lenUtf8CopyNFC)  );
			}
			else
			{
				// if original is non-canonical NFD, we first have to make it canonical NFD
				// and then convert that to NFC.  (yes, we get different answers.)
				SG_ERR_CHECK(  SG_apple_unicode__compute_nfd__utf8(pCtx, pUtf8Copy,   lenUtf8Copy,    &pUtf8CopyNFD, &lenUtf8CopyNFD)  );
				SG_ERR_CHECK(  SG_apple_unicode__compute_nfc__utf8(pCtx, pUtf8CopyNFD,lenUtf8CopyNFD, &pUtf8CopyNFC, &lenUtf8CopyNFC)  );
			}

			// convert our UTF-8 NFC result back to UTF-32 and replace our working UTF-32 buffer.

			SG_ERR_CHECK(  SG_utf8__to_utf32__buflen(pCtx, pUtf8CopyNFC,lenUtf8CopyNFC, &pUtf32CopyNFC,&lenUtf32CopyNFC)  );

			SG_NULLFREE(pCtx, pUtf8Copy);
			SG_NULLFREE(pCtx, pUtf8CopyNFC);

			SG_free(pCtx, pUtf32Copy);
			pUtf32Copy = pUtf32CopyNFC;
			pUtf32CopyNFC = NULL;

			lenCopyChars = lenUtf32CopyNFC;

			portWhyComputed |= SG_WC_PORT_FLAGS__COLLISION__NFC;
		}
	}

	if (pItem->flagsObserved & SG_WC_PORT_FLAGS__INDIV__IGNORABLE)
	{
		// apple ignores various characters in filenames (they are preserved
		// in the name but are ignored on lookups).  so we strip them out to
		// catch any ambituities/collisions.

		SG_bool bIgnorable;

		k = 0;
		while (k < lenCopyChars)
		{
			SG_ERR_CHECK(  SG_apple_unicode__is_ignorable_char__utf32(pCtx, pUtf32Copy[k],&bIgnorable)  );

			if (bIgnorable)
			{
				memmove(&pUtf32Copy[k],&pUtf32Copy[k+1],
					   sizeof(SG_int32)*(lenCopyChars-k));
				lenCopyChars -= 1;

				portWhyComputed |= SG_WC_PORT_FLAGS__COLLISION__IGNORABLE;
			}
			else
			{
				k++;
			}
		}
	}

	// convert entryname to lowercase for the collider.  since apple and windows
	// are not case sensitive, we make lowercase the base class for the collider.

	for (k=0; k<lenCopyChars; k++)
	{
		SG_int32 ch32Apple;
		SG_int32 ch32AppleICU;
		SG_int32 ch32ICU;

		// Apple's lowercase routines are based upon "their interpretation"
		// of Unicode 3.2.  This is what they have hard-coded in their filesystem.
		// So our collider needs to fold case the way that they do.
		//
		// BUT, Windows/NTFS has their own set of rules hard-coded in their filesystem.
		// So our collider needs to fold case the way that they do.
		// TODO I haven't found a reference for this yet.
		//
		// We have the Apple code and we have ICU (based upon Unicode 5.1).
		// So let's run it thru both and see what happens.  Again we're only
		// trying compute the base of an equivalence class -- not the various
		// permutation members.  This may cause some suble false-positives,
		// but I don't care.

		SG_ERR_CHECK(  SG_apple_unicode__to_lower_char__utf32(pCtx, pUtf32Copy[k],&ch32Apple)  );
		SG_ERR_CHECK(  SG_utf32__to_lower_char(pCtx, ch32Apple,&ch32AppleICU)  );

		SG_ERR_CHECK(  SG_utf32__to_lower_char(pCtx, pUtf32Copy[k],&ch32ICU)  );

		SG_ASSERT(  (ch32ICU == ch32AppleICU)  &&  "Inconsistent lowercase."  );

		if (ch32AppleICU != pUtf32Copy[k])
		{
			pUtf32Copy[k] = ch32AppleICU;

			portWhyComputed |= SG_WC_PORT_FLAGS__COLLISION__CASE;
		}
	}

	// if we added something to the flags, add our string to the collider.

	if (portWhyComputed != portWhyStart)
	{
		SG_uint32 lenUtf8Copy;

		SG_ERR_CHECK(  SG_utf8__from_utf32(pCtx, pUtf32Copy,&pUtf8Copy,&lenUtf8Copy)  );
		SG_ERR_CHECK(  _sg_wc_port__add_mangling(pCtx, pDir,pUtf8Copy,pItem,portWhyComputed)  );
		SG_NULLFREE(pCtx, pUtf8Copy);
	}
	else
	{
		// this is a coding error, but it is safe to let this continue.
		// our collision detector will just be missing an entry.
		SG_ASSERT( (lenCopyChars == pItem->lenEntryNameChars)
				&& (memcmp(pItem->pUtf32EntryName,pUtf32Copy,lenCopyChars) == 0)
				&& "UTF-32 Filename munged but no bits set." );
	}

fail:
	SG_NULLFREE(pCtx, pUtf32Copy);
	SG_NULLFREE(pCtx, pUtf32CopyNFC);
	SG_NULLFREE(pCtx, pUtf8Copy);
	SG_NULLFREE(pCtx, pUtf8CopyNFC);
	SG_NULLFREE(pCtx, pUtf8CopyNFD);
}

#if defined(SG_WC_PORT_DEBUG_BAD_NAME)
static void _sg_wc_port__collide_debug_bad_name(SG_context * pCtx,
														SG_wc_port * pDir,
														sg_wc_port_item * pItem)
{
	// we define that all entrynames that begin with <debug_bad_name> collide.
	// strip off anything after the fixed prefix and add that to the collider.
	// for these tests we ***DO NOT*** both folding case and all the other stuff.
	// we only ***EXACT MATCH*** on the prefix.

	char * szCopy = NULL;
	const char * szEntryNameStart = pItem->szUtf8EntryName;
	SG_wc_port_flags portWhyStart = SG_WC_PORT_FLAGS__NONE;
	SG_wc_port_flags portWhyComputed = portWhyStart;

	if (pItem->flagsObserved & SG_WC_PORT_FLAGS__INDIV__DEBUG_BAD_NAME)
	{
		size_t lenPrefix = strlen(SG_WC_PORT_DEBUG_BAD_NAME);

		if (pItem->lenEntryNameUtf8Bytes > lenPrefix)
		{
			SG_ERR_CHECK(  SG_STRDUP(pCtx, szEntryNameStart,&szCopy)  );

			szCopy[lenPrefix] = 0;
			portWhyComputed |= SG_WC_PORT_FLAGS__COLLISION__DEBUG_BAD_NAME;

			SG_ERR_CHECK(  _sg_wc_port__add_mangling(pCtx, pDir,szCopy,pItem,portWhyComputed)  );
		}
	}

fail:
	SG_NULLFREE(pCtx, szCopy);
}
#endif

/**
 * Compute the various possible name manglings and accumulate
 * them in our container and look for potential collisions.
 */
static void _sg_wc_port__collide_item(SG_context * pCtx,
									  SG_wc_port * pDir,
									  sg_wc_port_item * pItem)
{
	// add the original/unmodified UTF-8 entryname and if necessary
	// the CHARSET-based version to the collider.

	SG_ERR_CHECK(  _sg_wc_port__add_mangling(pCtx,
											 pDir,pItem->szUtf8EntryName,
											 pItem,SG_WC_PORT_FLAGS__COLLISION__ORIGINAL)  );
	if (pItem->szCharSetEntryName)
		SG_ERR_CHECK(  _sg_wc_port__add_mangling(pCtx,
												 pDir,pItem->szCharSetEntryName,
												 pItem,SG_WC_PORT_FLAGS__COLLISION__CHARSET)  );

#if defined(SG_WC_PORT_DEBUG_BAD_NAME)
	SG_ERR_CHECK(  _sg_wc_port__collide_debug_bad_name(pCtx,pDir,pItem)  );
#endif

	if (pItem->b7BitClean)
	{
		// we have US-ASCII only data.  we can take some shortcuts because none
		// of the Unicode stuff is necessary.
		//
		// Use the UTF-8 entryname and do all the case folding and other such
		// tricks to compute a "base" value for the equivalence class that this
		// entryname is a member of.

		SG_ERR_CHECK(  _sg_wc_port__collide_item_7bit_clean(pCtx,pDir,pItem)  );
	}
	else
	{
		// we have UTF-8 with some non-US-ASCII characters so we have to do it
		// the hard way.

		SG_ERR_CHECK(  _sg_wc_port__collide_item_32bit(pCtx,pDir,pItem)  );
	}

	// TODO detect potential collisions items that are "shortnames"
	// TODO (pItem->flagsObserved & SG_WC_PORT_FLAGS__INDIV__SHORTNAMES)
	// TODO with other items non-shortname items.  this might require
	// TODO computing a wildcard or psuedo-shortname for all non-shortname
	// TODO items and adding them to a short-name-collider and then seeing
	// TODO if the given shortname matches the pattern.  This will likely
	// TODO have lots of false positives.  Perhaps the __INDIV__SHORTNAMES
	// TODO warning is sufficient.

	// fall-thru to common cleanup.

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_wc_port__add_item(SG_context * pCtx,
						  SG_wc_port * pDir,
						  const char * pszGid_Entry,
						  const char * szEntryName,
						  SG_treenode_entry_type tneType,
						  SG_bool * pbIsDuplicate)
{
	SG_ERR_CHECK_RETURN(  SG_wc_port__add_item__with_assoc(pCtx,
														   pDir,
														   pszGid_Entry,
														   szEntryName,
														   tneType,
														   NULL,
														   pbIsDuplicate,
														   NULL)  );
}

void SG_wc_port__add_item__with_assoc(SG_context * pCtx,
									  SG_wc_port * pDir,
									  const char * pszGid_Entry,
									  const char * szEntryName,
									  SG_treenode_entry_type tneType,
									  void * pVoidAssocData,
									  SG_bool * pbIsDuplicate,
									  void ** ppVoidAssocData_Original)
{
	sg_wc_port_item * pItemAllocated = NULL;
	sg_wc_port_item * pItem;

	SG_NULLARGCHECK_RETURN(pDir);
	SG_NULLARGCHECK_RETURN(pbIsDuplicate);
	// ppVoidAssocData_Original is optional

#if TRACE_WC_PORT
	{
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "PORT: add_item__with_assoc: [gid %s][%s][%d]\n",
								   pszGid_Entry, szEntryName, tneType)  );
	}
#endif

	// allocate an Item for this entryname and add it to our assoc-array of items.
	// if the entryname exactly matches something that we have already processed,
	// we have a trivial collision.

	SG_ERR_CHECK(  _sg_wc_port_item__alloc(pCtx,
										   pszGid_Entry,
										   szEntryName,
										   tneType,
										   pDir->pConverterRepoCharSet,
										   pVoidAssocData,
										   &pItemAllocated)  );

	_sg_wc_port__add_item_to_list(pCtx,pDir,pItemAllocated);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (SG_context__err_equals(pCtx,SG_ERR_RBTREE_DUPLICATEKEY))
			goto report_duplicate;

		SG_ERR_RETHROW;
	}

	pItem = pItemAllocated;		// we can still reference the item,
	pItemAllocated = NULL;		// but we no longer own it.

	SG_ERR_CHECK(  _sg_wc_port__inspect_item(pCtx,pDir,pItem)  );
	SG_ERR_CHECK(  _sg_wc_port__collide_item(pCtx,pDir,pItem)  );
	*pbIsDuplicate = SG_FALSE;

	return;

report_duplicate:
	// We don't throw SG_ERR_WC_PORT_DUPLICATE because we also
	// want to optionally return the assoc-data for the other item.
	// See sg_wc_liveview_dir__can_add_new_entryname().
	SG_context__err_reset(pCtx);
	*pbIsDuplicate = SG_TRUE;
	if (ppVoidAssocData_Original)
		SG_ERR_CHECK(  SG_wc_port__get_item_result_flags__with_assoc(pCtx,pDir,szEntryName,
																	 NULL,NULL,
																	 ppVoidAssocData_Original)  );
fail:
	SG_ERR_IGNORE(  _sg_wc_port_item__free(pCtx, pItemAllocated)  );
}

//////////////////////////////////////////////////////////////////

void SG_wc_port__get_results(SG_context * pCtx,
							 SG_wc_port * pDir,
							 SG_wc_port_flags * pFlagsUnion,
							 const SG_string ** ppStringLog)
{
	SG_NULLARGCHECK_RETURN(pDir);

	if (pFlagsUnion)
		*pFlagsUnion = (pDir->flagsUnion & pDir->mask);

	if (ppStringLog)
		*ppStringLog = pDir->pStringLog;
}

//////////////////////////////////////////////////////////////////

void SG_wc_port__get_item_result_flags(SG_context * pCtx,
									   SG_wc_port * pDir,
									   const char * szEntryName,
									   SG_wc_port_flags * pFlagsObserved,
									   const SG_string ** ppStringItemLog)
{
	SG_ERR_CHECK_RETURN(  SG_wc_port__get_item_result_flags__with_assoc(pCtx,
																		pDir,
																		szEntryName,
																		pFlagsObserved,
																		ppStringItemLog,
																		NULL)  );
}

void SG_wc_port__get_item_result_flags__with_assoc(SG_context * pCtx,
												   SG_wc_port * pDir,
												   const char * szEntryName,
												   SG_wc_port_flags * pFlagsObserved,
												   const SG_string ** ppStringItemLog,
												   void ** ppVoidAssocData)
{
	sg_wc_port_item * pItem;

	SG_NULLARGCHECK_RETURN(pDir);

	SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx, pDir->prbItems,szEntryName,NULL,(void **)&pItem)  );

	if (pFlagsObserved)
		*pFlagsObserved = (pItem->flagsObserved & pDir->mask);

	if (ppStringItemLog)
		*ppStringItemLog = pItem->pStringItemLog;

	if (ppVoidAssocData)
		*ppVoidAssocData = pItem->pVoidAssocData;
}

//////////////////////////////////////////////////////////////////

void SG_wc_port__foreach_with_issues(SG_context * pCtx,
									 SG_wc_port * pDir,
									 SG_wc_port__foreach_callback * pfn,
									 void * pVoidCallerData)
{
	SG_rbtree_iterator * pIter = NULL;
	const sg_wc_port_item * pItem;
	const char * pszEntryName;
	SG_varray * pvaCollidedWith = NULL;
	SG_bool bFound;

	SG_NULLARGCHECK_RETURN(pDir);
	SG_NULLARGCHECK_RETURN(pfn);

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter, pDir->prbItems,
											  &bFound, &pszEntryName, (void **)&pItem)  );
	while (bFound)
	{
		if (pItem->flagsObserved & pDir->mask)
		{
			if (pItem->prbCollidedWith)
				SG_ERR_CHECK(  SG_rbtree__to_varray__keys_only(pCtx, pItem->prbCollidedWith, &pvaCollidedWith)  );

			SG_ERR_CHECK(  (*pfn)(pCtx,
								  pszEntryName,
								  (pItem->flagsObserved & pDir->mask),
								  pItem->pStringItemLog,
								  &pvaCollidedWith,		// let caller steal it if they want it.
								  pItem->pVoidAssocData,
								  pVoidCallerData)  );
			SG_VARRAY_NULLFREE(pCtx, pvaCollidedWith);
		}

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter,
												 &bFound, &pszEntryName, (void **)&pItem)  );
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
	SG_VARRAY_NULLFREE(pCtx, pvaCollidedWith);
}

//////////////////////////////////////////////////////////////////

/**
 * This is a TEMPORARY DEBUG facility to change the PORT MASK.
 * See W8714.  Later, we'll have a proper API to tune this.
 * This is mainly needed for Linux-only sites that don't want
 * to be limited to cross-platform-safe filenames (like having
 * a directory with "README" and "ReadMe").
 *
 */
static void _debug_get_port_mask(SG_context * pCtx,
								 SG_repo * pRepo,
								 SG_wc_port_flags * pMask)
{
	char * pszValue = NULL;

	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, "debug_port_mask", pRepo, &pszValue, NULL)  );
	if (pszValue)
	{
		SG_uint64 ui64 = 0;

		SG_ERR_CHECK(  SG_uint64__parse__strict(pCtx, &ui64, pszValue)  );
		*pMask = (SG_wc_port_flags)ui64;
	}
	else
	{
		// if not set, assume the most restrictive for maximum portability of WDs.
		*pMask = SG_WC_PORT_FLAGS__ALL;
	}

fail:
	SG_NULLFREE(pCtx, pszValue);
}

void SG_wc_port__get_mask_from_config(SG_context * pCtx,
									  SG_repo * pRepo,
									  const SG_pathname * pPathWorkingDirectoryTop,
									  SG_wc_port_flags * pMask)
{
	SG_UNUSED(pPathWorkingDirectoryTop);

	SG_ERR_CHECK_RETURN(  _debug_get_port_mask(pCtx, pRepo, pMask)  );
}

void SG_wc_port__get_charset_from_config(SG_context * pCtx,
										 SG_repo * pRepo,
										 const SG_pathname * pPathWorkingDirectoryTop,
										 char ** ppszCharSet)
{
	SG_UNUSED(pCtx);
	SG_UNUSED(pRepo);
	SG_UNUSED(pPathWorkingDirectoryTop);

	// for now assume no locale and always use UTF-8.

	*ppszCharSet = NULL;
}
