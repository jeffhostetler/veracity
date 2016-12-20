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
 *
 * @file u1100_large_blob.c
 *
 * @details A test to exercise the storage and retrieval of
 * a very large (4gig+several bytes) blob.  The test will be run
 * both with compression and without.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"

//////////////////////////////////////////////////////////////////
// we define a little trick here to prefix all global symbols (type,
// structures, functions) with our test name.  this allows all of
// the tests in the suite to be #include'd into one meta-test (without
// name collisions) when we do a GCOV run.

#define MyMain()				TEST_MAIN_WITH_ARGS(u1100_large_blob)
#define MyDcl(name)				u1100_large_blob__##name
#define MyFn(name)				u1100_large_blob__##name

#define MyFourGig				(1024LL*1024*1024*4)	// win build fails with constant overflow without LL

//////////////////////////////////////////////////////////////////

void MyFn(create_repo)(SG_context * pCtx, SG_repo ** ppRepo)
{
	// caller must free returned value.

	char buf_admin_id[SG_GID_BUFFER_LENGTH];
    char buf_repo_name[SG_GID_BUFFER_LENGTH];

	VERIFY_ERR_CHECK(  SG_gid__generate(pCtx, buf_admin_id, sizeof(buf_admin_id))  );
	VERIFY_ERR_CHECK(  SG_gid__generate(pCtx, buf_repo_name, sizeof(buf_repo_name))  );

	VERIFY_ERR_CHECK(  SG_repo__create__completely_new__empty__closet(pCtx, buf_admin_id, NULL, NULL, buf_repo_name)  );
	VERIFY_ERR_CHECK(  SG_repo__open_repo_instance(pCtx, buf_repo_name, ppRepo)  );

fail:
    ;
}

void MyFn(create_tmp_src_dir)(SG_context * pCtx, SG_pathname * ppPathnameTempDir)
{
	// create a temp directory in the current directory to be the
	// home of some userfiles, if the directory does not already exist.
	// caller must free returned value.

	SG_bool bExists;
	SG_fsobj_type FsObjType;
	SG_fsobj_perms FsObjPerms;

	VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, ppPathnameTempDir, &bExists, &FsObjType, &FsObjPerms)  );
	if (!bExists)
		VERIFY_ERR_CHECK(  SG_fsobj__mkdir_recursive__pathname(pCtx, ppPathnameTempDir)  );

	INFOP("mktmpdir",("Temp Src Dir is [%s]",SG_pathname__sz(ppPathnameTempDir)));

	return;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void MyFn(create_blob_from_file)(SG_context * pCtx,
								 SG_repo * pRepo,
								 const SG_pathname * pPathnameTempDir,
								 SG_uint64 lenFile)
{
	// create a file of length "lenFile" in the temp directory.
	// use it to create a blob.
	// try to create it a second time and verify that we get an duplicate-hid error.

	// Originally, this test created the 4gig test file each time on the fly.
	// Since creating a 4gig file takes some time, we've decided that if a file "largeblob"
	// exists at the given location, we will not re-create the file, allowing the user
	// (i.e. the build system) to just pass in the same location each time and re-use the file.
	// If the file is not found, it will be created.

	const char* pszLargeBlobFilename = "largeblob";
	char buf_tid_random2[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname * pPathnameTempFile1 = NULL;
	SG_pathname * pPathnameTempFile2 = NULL;
	SG_file * pFileTempFile1 = NULL;
	SG_file * pFileTempFile2 = NULL;
	char* pszidHidBlob1 = NULL;
	char* pszidHidBlob1Dup = NULL;
	char* pszidHidBlob2 = NULL;
	char* pszidHidVerify1 = NULL;
	char* pszidHidVerify2 = NULL;
	SG_bool bEqual;
	SG_repo_tx_handle* pTx = NULL;
	SG_uint64 iBlobFullLength = 0;

	//////////////////////////////////////////////////////////////////
	// create temp-file-1 of length "lenFile" in the temp directory.

	/* Quickly create large file whose contents are irrelevant. */
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathnameTempFile1,
		pPathnameTempDir, pszLargeBlobFilename)  );
	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathnameTempFile1, 
		SG_FILE_OPEN_OR_CREATE | SG_FILE_TRUNC | SG_FILE_RDWR, 
		0644, &pFileTempFile1)  );

#if defined(WINDOWS)
	{
		HANDLE hf = NULL;
		LONG low = lenFile & 0xFFFFFFFF;
		LONG high = lenFile >> 32;

		VERIFY_ERR_CHECK(  SG_file__get_handle(pCtx, pFileTempFile1, &hf)  );

		SetFilePointer(hf, low, &high, FILE_BEGIN);
		SetEndOfFile(hf);
	}
#else
	{
		int fd = -1;
		int rc = -1;
		
		VERIFY_ERR_CHECK(  SG_file__get_fd(pCtx, pFileTempFile1, &fd)  );
		rc = ftruncate(fd, lenFile);
		VERIFY_COND("ftruncate64 indicates success", rc == 0);
	}
#endif

	//////////////////////////////////////////////////////////////////
	// use temp file to create a blob.
	// we get the HID back.  (we need to free it later.)

	VERIFY_ERR_CHECK(  SG_repo__begin_tx(pCtx, pRepo, &pTx)  );
	VERIFY_ERR_CHECK(  SG_repo__store_blob_from_file(pCtx, pRepo, pTx,
		SG_TRUE, pFileTempFile1, &pszidHidBlob1, &iBlobFullLength)  );
	VERIFY_ERR_CHECK(  SG_repo__commit_tx(pCtx, pRepo, &pTx)  );

	{
		SG_blob_encoding encoding;
		SG_uint64 lenRaw, lenFull;
		SG_repo_fetch_blob_handle* pFetchHandle = NULL;
		VERIFY_ERR_CHECK(  SG_repo__fetch_blob__begin(pCtx, pRepo, pszidHidBlob1, SG_FALSE, &encoding,
			NULL, &lenRaw, &lenFull, &pFetchHandle)  );
		VERIFY_ERR_CHECK(  SG_repo__fetch_blob__abort(pCtx, pRepo, &pFetchHandle)  );
		VERIFY_COND("lengths correct for uncompressed blob", lenRaw==lenFull);
		VERIFY_COND("encoding correct for uncompressed blob", encoding == SG_BLOBENCODING__ALWAYSFULL);
	}

	INFOP("create_blob_from_file",("Created blob [%s]",(pszidHidBlob1)));

	//////////////////////////////////////////////////////////////////
	// create empty temp-file-2 and try to read the blob from the repo.

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_tid_random2, sizeof(buf_tid_random2), 32)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathnameTempFile2,pPathnameTempDir,buf_tid_random2)  );

	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathnameTempFile2,SG_FILE_RDWR|SG_FILE_CREATE_NEW,0644,&pFileTempFile2)  );

	VERIFY_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx, pRepo,pszidHidBlob1,pFileTempFile2,NULL)  );

	//////////////////////////////////////////////////////////////////
	// verify that the contents of temp-file-2 is identical to the
	// contents of temp-file-1.  (we already know that the HIDs match
	// and was verified during the fetch, but there are times when the
	// HID is just being used as a key -- it doesn't mean that what we
	// actually restored is correct.

	VERIFY_ERR_CHECK(  SG_repo__alloc_compute_hash__from_file(pCtx, pRepo, pFileTempFile1, &pszidHidVerify1)  );

	VERIFY_ERR_CHECK(  SG_repo__alloc_compute_hash__from_file(pCtx, pRepo, pFileTempFile2, &pszidHidVerify2)  );

	bEqual = (0 == (strcmp(pszidHidVerify1,pszidHidVerify2)));
	VERIFY_COND("create_blob_from_file(verify v1==v2)",bEqual);

	bEqual = (0 == (strcmp(pszidHidVerify1,pszidHidBlob1)));
	VERIFY_COND("create_blob_from_file(verify v1==id)",bEqual);

fail:
	SG_ERR_IGNORE(  SG_file__close(pCtx, &pFileTempFile1)  );
	SG_ERR_IGNORE(  SG_file__close(pCtx, &pFileTempFile2)  );

	SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPathnameTempFile1)  );
	SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPathnameTempFile2)  );

	SG_NULLFREE(pCtx, pszidHidBlob1);
	SG_NULLFREE(pCtx, pszidHidBlob1Dup);
	SG_NULLFREE(pCtx, pszidHidBlob2);
	SG_NULLFREE(pCtx, pszidHidVerify1);
	SG_NULLFREE(pCtx, pszidHidVerify2);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameTempFile1);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameTempFile2);
}

void MyFn(create_some_large_blobs_from_files)(SG_context * pCtx,
											  SG_repo * pRepo,
											  const char* pszPath)
{
	char * szRepoId;
	SG_pathname * pPathnameTempDir = NULL;

	if (pszPath)
		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathnameTempDir, pszPath)  );
	else
		VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &pPathnameTempDir)  );

	VERIFY_ERR_CHECK(  MyFn(create_tmp_src_dir)(pCtx,pPathnameTempDir)  );

    VERIFY_ERR_CHECK_DISCARD(  SG_repo__get_repo_id(pCtx, pRepo, &szRepoId)  );

	INFOP("create_some_large_blobs_from_files",("RepoID[%s] TempDir[%s] ",
										  szRepoId,
										  SG_pathname__sz(pPathnameTempDir)
										  ));
    SG_NULLFREE(pCtx, szRepoId);

	VERIFY_ERR_CHECK(  MyFn(create_blob_from_file)(pCtx, pRepo,pPathnameTempDir, (SG_uint64)MyFourGig+5)  );

	// we don't delete pPathnameTempDir here so that it may be reused, to bypass creating the large file every time

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathnameTempDir);
}

void MyFn(clone_with_large_blob)(SG_context* pCtx, SG_repo* pRepoSrc)
{
	const char* szSrcName;
	char bufDestName[SG_TID_MAX_BUFFER_LENGTH];
	
	SG_repo* pRepoDest = NULL;
	SG_blobset* pbs = NULL;
	SG_vhash* pvhBlobs = NULL;
	SG_uint32 count_blobs = 0;
	SG_uint32 i;

	struct SG_clone__params__all_to_something ats;
	struct SG_clone__demands demands;

	VERIFY_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepoSrc, &szSrcName)  );

	// Do exact clone and verify blobs
	VERIFY_ERR_CHECK(  SG_tid__generate(pCtx, bufDestName, sizeof(bufDestName))  );
	VERIFY_ERR_CHECK(  SG_clone__to_local(pCtx, szSrcName, NULL, NULL, bufDestName, NULL, NULL, NULL)  );
	VERIFY_ERR_CHECK(  SG_repo__open_repo_instance(pCtx, bufDestName, &pRepoDest)  );
	SG_ERR_CHECK(  SG_repo__list_blobs(pCtx, pRepoDest, 0, 0, 0, &pbs)  );
	SG_ERR_CHECK(  SG_blobset__get(pCtx, pbs, 0, 0, &pvhBlobs)  );
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhBlobs, &count_blobs)  );
	for (i=0; i<count_blobs; i++)
	{
		const char* psz_hid = NULL;
		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhBlobs, i, &psz_hid, NULL)  );
		VERIFY_ERR_CHECK(  SG_repo__verify_blob(pCtx, pRepoDest, psz_hid, NULL)  );
	}
	SG_REPO_NULLFREE(pCtx, pRepoDest);
	SG_BLOBSET_NULLFREE(pCtx, pbs);
	SG_VHASH_NULLFREE(pCtx, pvhBlobs);

	// Do clone with zlib pack and verify blobs
	memset(&ats, 0, sizeof(ats));
	ats.new_encoding = SG_BLOBENCODING__ZLIB;
	VERIFY_ERR_CHECK(  SG_clone__init_demands(pCtx, &demands)  );
	demands.zlib_savings_over_full = -1;
	demands.vcdiff_savings_over_full = -1;
	demands.vcdiff_savings_over_zlib = -1;
	VERIFY_ERR_CHECK(  SG_tid__generate(pCtx, bufDestName, sizeof(bufDestName))  );
	VERIFY_ERR_CHECK(  SG_clone__to_local(pCtx, szSrcName, NULL, NULL, bufDestName, &ats, NULL, &demands)  );
	VERIFY_ERR_CHECK(  SG_repo__open_repo_instance(pCtx, bufDestName, &pRepoDest)  );
	SG_ERR_CHECK(  SG_repo__list_blobs(pCtx, pRepoDest, 0, 0, 0, &pbs)  );
	SG_ERR_CHECK(  SG_blobset__get(pCtx, pbs, 0, 0, &pvhBlobs)  );
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvhBlobs, &count_blobs)  );
	for (i=0; i<count_blobs; i++)
	{
		const char* psz_hid = NULL;
		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvhBlobs, i, &psz_hid, NULL)  );
		VERIFY_ERR_CHECK(  SG_repo__verify_blob(pCtx, pRepoDest, psz_hid, NULL)  );
	}


/* fall through */
fail:
	SG_REPO_NULLFREE(pCtx, pRepoDest);
	SG_BLOBSET_NULLFREE(pCtx, pbs);
	SG_VHASH_NULLFREE(pCtx, pvhBlobs);
}

//////////////////////////////////////////////////////////////////

MyMain()
{
    SG_getopt * pGetopt = NULL;

	SG_repo * pRepo = NULL;

	TEMPLATE_MAIN_START;

	VERIFY_ERR_CHECK(  MyFn(create_repo)(pCtx,&pRepo)  );
    
	SG_ERR_CHECK(  SG_getopt__alloc(pCtx, argc, argv, &pGetopt)  );
	if (pGetopt->count_args >= 2 && pGetopt->paszArgs[1] && *(pGetopt->paszArgs[1]))
	{
    	// usage: u1100_large_blob <large_blob_directory>
		BEGIN_TEST(  MyFn(create_some_large_blobs_from_files)(pCtx, pRepo, pGetopt->paszArgs[1])  );
	}
	else
		BEGIN_TEST(  MyFn(create_some_large_blobs_from_files)(pCtx, pRepo, NULL)  );

	BEGIN_TEST(  MyFn(clone_with_large_blob)(pCtx, pRepo)  );


	// fall-thru to common cleanup
fail:

    SG_GETOPT_NULLFREE(pCtx, pGetopt);

	SG_REPO_NULLFREE(pCtx, pRepo);

	TEMPLATE_MAIN_END;
}

#undef MyMain
#undef MyDcl
#undef MyFn

