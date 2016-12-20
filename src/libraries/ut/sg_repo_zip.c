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
 * @file sg_repo_zip.c
 */

#include <sg.h>

static void _fetch_blob_to_zip_file(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* pszHid, 
        SG_string* pstrPath, 
        const char* pszName, 
        SG_zip** pzip
        )
{
	SG_repo_fetch_blob_handle* pbh = NULL;
    SG_uint64 len = 0;
    SG_uint32 got = 0;
    SG_byte* p_buf = NULL;
    SG_uint64 left = 0;
	SG_string* pPathFile = NULL;
    SG_bool b_done = SG_FALSE;

	SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pPathFile, SG_string__sz(pstrPath))  );

	if (strlen(SG_string__sz(pPathFile)) > 0)
	{
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pPathFile, "/") );
	}
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pPathFile, pszName) );

	SG_ERR_CHECK(  SG_zip__begin_file(pCtx, *pzip, SG_string__sz(pPathFile))  );
	SG_ERR_CHECK(  SG_repo__fetch_blob__begin(pCtx, pRepo,  pszHid, SG_TRUE, NULL, NULL, NULL, &len, &pbh)  );
	left = len;
	SG_ERR_CHECK(  SG_alloc(pCtx, SG_STREAMING_BUFFER_SIZE, 1, &p_buf)  );
	while (!b_done)
	{
		SG_uint32 want = SG_STREAMING_BUFFER_SIZE;
		if (want > left)
		{
			want = (SG_uint32) left;
		}
		SG_ERR_CHECK(  SG_repo__fetch_blob__chunk(pCtx, pRepo, pbh, want, p_buf, &got, &b_done)  );
		SG_ERR_CHECK(  SG_zip__write(pCtx, *pzip, p_buf, got)  );
		left -= got;
	}
	SG_ERR_CHECK(  SG_repo__fetch_blob__end(pCtx, pRepo, &pbh)  );
    SG_ASSERT(0 == left);
	SG_ERR_CHECK(  SG_zip__end_file(pCtx, *pzip)  );
	SG_NULLFREE(pCtx, p_buf);
	SG_STRING_NULLFREE(pCtx, pPathFile);
	return;

fail:
	SG_ERR_IGNORE(  SG_repo__fetch_blob__abort(pCtx, pRepo, &pbh)  );
	SG_STRING_NULLFREE(pCtx, pPathFile);
    SG_NULLFREE(pCtx, p_buf);
}

static void _zip_repo_treenode(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* pszHid, 
        SG_string** ppstrPath, 
        SG_zip** pzip
        )
{
	SG_treenode* pTreenode = NULL;
	SG_uint32 count = 0;
	SG_uint32 i;

	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszHid, &pTreenode)  );
	SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenode, &count)  );
	if (count == 0)
	{
		SG_string* pPathFolderEmpty = NULL;
		SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pPathFolderEmpty, SG_string__sz(*ppstrPath))  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pPathFolderEmpty, "/") );
		SG_ERR_CHECK(  SG_zip__add_folder(pCtx, *pzip, SG_string__sz(pPathFolderEmpty))  );
		SG_STRING_NULLFREE(pCtx, pPathFolderEmpty);
	}
	for (i=0; i<count; i++)
	{
		const SG_treenode_entry* pEntry = NULL;
		SG_treenode_entry_type type;
		const char* pszName = NULL;
		const char* pszidHid = NULL;
		const char* pszgid = NULL;

		SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pTreenode, i, &pszgid, &pEntry)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pEntry, &type)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pEntry, &pszName)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pEntry, &pszidHid)  );

		if (SG_TREENODEENTRY_TYPE_DIRECTORY == type)
		{
			SG_string* pPathFolder = NULL;
			SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pPathFolder, SG_string__sz(*ppstrPath))  );

			if (strlen(SG_string__sz(pPathFolder)) > 0)
			{
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, pPathFolder, "/") );
			}
			if (strcmp(pszName, "@") != 0)
			{
				SG_ERR_CHECK(  SG_string__append__sz(pCtx, pPathFolder, pszName) );
			}
			SG_ERR_CHECK(  _zip_repo_treenode(pCtx, pRepo, pszidHid, &pPathFolder, &(*pzip))  );
			SG_STRING_NULLFREE(pCtx, pPathFolder);
		}
		else if (SG_TREENODEENTRY_TYPE_REGULAR_FILE == type)
		{
			SG_ERR_CHECK(  _fetch_blob_to_zip_file(pCtx, pRepo, pszidHid, *ppstrPath, pszName, &(*pzip) ) );
		}
        else
        {
            // TODO we should probably zip other things here too, right?
            // can zip do a symlink?
            // what do we do about a submodule?
        }
	}
fail:
	SG_TREENODE_NULLFREE(pCtx, pTreenode);
}

void SG_repo__zip(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_hid_cs, 
        const char* psz_path
        )
{
	char* pszHidTreeNode = NULL;
	SG_string* pstrPathInit = NULL;
	SG_zip* pzip = NULL;
	SG_rbtree* prbLeaves = NULL;
	SG_pathname* pPathZip = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(psz_hid_cs);
	SG_NULLARGCHECK_RETURN(psz_path);

	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, psz_hid_cs, &pszHidTreeNode)  );

	SG_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pPathZip, psz_path)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pstrPathInit, "")  );
	SG_ERR_CHECK(  SG_zip__open(pCtx, pPathZip, &pzip)  );
	SG_ERR_CHECK(  _zip_repo_treenode(pCtx, pRepo, pszHidTreeNode, &pstrPathInit, &pzip) );
	SG_ERR_CHECK(  SG_zip__nullclose(pCtx, &pzip) );

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPathZip);
	SG_NULLFREE(pCtx, pszHidTreeNode);
	SG_STRING_NULLFREE(pCtx, pstrPathInit);
	if (pzip)
    {
		 SG_zip__nullclose(pCtx, &pzip);
    }
    SG_RBTREE_NULLFREE(pCtx, prbLeaves);
}

