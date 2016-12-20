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
#include <sghash.h>

#include <sg_vv2__public_typedefs.h>
#include <sg_vv2__public_prototypes.h>

struct sg_fast_import_state
{
    SG_repo* pRepo;
    SG_repo_tx_handle* ptx;
    SG_file* pfi;
    SG_bool b_done;
    SG_byte* buf;
    SG_uint32 buflen;
    SG_vhash* pvh_branch_state;
    SG_vhash* pvh_blob_marks;
	SG_vhash* pvh_blob_sha1s; // our blob HIDs keyed by their Git-style SHA1 hash
    SG_vhash* pvh_commit_marks;
    SG_vhash* pvh_emails;
    SG_strpool* pool;
    SG_uint32 count_commits;
    SG_uint32 count_done;
    char* psz_initial_csid;
};

struct node
{
	struct node* pCurrentParent; /* null for the root treenode
									   * only.  This is the CURRENT
									   * parent. */
	
	const char* pszgidMovedFrom; /* non-null iff this item was moved here
								  * from somewhere else. used to check
								  * for split moves */

	SG_treenode_entry* pBaselineEntry; /* NULL if this item wasn't in
										* the baseline.  Also NULL for
										* the very top node in the
										* tree.  If non-NULL, this is
										* a copy which needs to be
										* freed. */

	SG_treenode_entry* pNewEntry; /* During commit, each item which
								   * appears in the committed tree
								   * gets a new entry, which is stored
								   * here.  Post-commit this entry is
								   * moved to pBaselineEntry. */

	const char* pszgid; /* This is here for convenience.  If
						 * pBaselineEntry is present, its gid will
						 * always be the same as this one.  If
						 * pCurrentParent is non-NULL, then this gid
						 * is the same gid which is used for the key
						 * in pCurrentParent->prbItems. Note that this
						 * member is not identified as current or
						 * baseline, because it cannot change. */
	
	const char* pszCurrentName; /* If NULL, use the name in the entry. */
	
	const char* pszCurrentHid;

    SG_int64 iCurrentAttributeBits;

	SG_treenode_entry_type type;
	SG_uint16 saved_flags;
	
	/* Stuff that's valid only for type == DIRECTORY */
	SG_rbtree* prbItems;  /* NULL until it's needed. */
};

#define sg_PTNODE_SAVED_FLAG_DELETED    8

#define sg_PTNODE_SAVED_FLAG_MOVED_AWAY    16

struct sg_pending_commit
{
    SG_changeset* pcs;
    struct node* pSuperRoot;
    char* psz_baseline;
    SG_varray* pva_parents;
    char* psz_comment;
    char buf_email[256];
    SG_int64 when;
    char branch[256];
    char tag[256];
    SG_int64 time_begin;
    SG_int64 time_end;
};

static void store_blob(
	SG_context* pCtx,
    struct sg_fast_import_state* pst,
    SG_uint64 len,
    char** ppsz_hid
	)
{
	SG_uint32 sofar = 0;
	SG_uint32 got = 0;
	SG_repo_store_blob_handle* pbh = NULL;
	SGHASH_handle* phh = NULL;
	SG_error err = SG_ERR_OK;
	SG_int_to_string_buffer intbuf;
	char psz_git_header[27]; // length of int_to_string_buffer (21) + remaining header chars
	char psz_git_hash[41];
	const char* psz_existing_hid = NULL;

	SG_NULLARGCHECK_RETURN(pst);

	// initialize a handle for generating the blob's Git-style SHA1 hash
	// we might need to refer to this blob later by that hash
	err = SGHASH_init("SHA1/160", &phh);
	if (SG_IS_ERROR(err))
	{
		SG_ERR_THROW(err);
	}

	// hash the Git-style blob header: "blob LENGTH\0"
	SG_ERR_CHECK(  SG_sprintf(pCtx, psz_git_header, 27u, "blob %s", SG_uint64_to_sz(len, intbuf))  );
	err = SGHASH_update(phh, (SG_byte*)psz_git_header, SG_STRLEN(psz_git_header) + 1u); // +1 because we WANT the NULL terminator
	if (SG_IS_ERROR(err))
	{
		SG_ERR_THROW(err);
	}

    if (!pst->ptx)
    {
        SG_ERR_CHECK(  SG_repo__begin_tx(pCtx, pst->pRepo, &pst->ptx)  );
    }

	SG_ERR_CHECK(  SG_repo__store_blob__begin(
                pCtx, 
                pst->pRepo, 
                pst->ptx, 
                SG_BLOBENCODING__FULL, 
                NULL, 
                len, 
                0, 
                NULL, 
                &pbh)  );

	while (sofar < (SG_uint32) len)
	{
		SG_uint32 want = 0;
		if ((len - sofar) > pst->buflen)
		{
			want = pst->buflen;
		}
		else
		{
			want = (SG_uint32) (len - sofar);
		}
		SG_ERR_CHECK(  SG_file__read(pCtx, pst->pfi, want, pst->buf, &got)  );
		SG_ERR_CHECK(  SG_repo__store_blob__chunk(pCtx, pst->pRepo, pbh, got, pst->buf, NULL)  );

		// add this chunk to the hash calculation
		err = SGHASH_update(phh, pst->buf, got);
		if (SG_IS_ERROR(err))
		{
			SG_ERR_THROW(err);
		}

		sofar += got;
	}

	SG_ERR_CHECK(  SG_repo__store_blob__end(pCtx, pst->pRepo, pst->ptx, &pbh, ppsz_hid)  );

	// finish the hash calculation
	err = SGHASH_final(&phh, psz_git_hash, sizeof(psz_git_hash));
	if (SG_IS_ERROR(err))
	{
		SG_ERR_THROW(err);
	}

	// map the Git-style hash to our HID
	// so we can use it to find the blob later
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pst->pvh_blob_sha1s, psz_git_hash, &psz_existing_hid)  );
	if (psz_existing_hid == NULL)
	{
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pst->pvh_blob_sha1s, psz_git_hash, *ppsz_hid)  );
	}
	else if (strcmp(*ppsz_hid, psz_existing_hid) != 0)
	{
		// We've calculated this Git-style hash before.
		// It SHOULD be indicating the same HID as last time, but it's not.
		// The only way I can think of that this is possible is that there are
		// two different blobs that actually have a hash collision on their
		// Git-style SHA1 hash, but because the destination Veracity repo
		// is using a different hash algorithm, those HIDs do NOT collide.
		// Regardless of how it happened, though, we can't continue.
		// Firstly, how did we get a repo in a Git-compatible format that included
		// two different blobs with the same Git-style SHA1 hash?  Git shouldn't
		// allow that, and other systems exporting to a Git format should probably
		// also have noticed the problem and blown up along the way.
		// Secondly, we can't very well choose which HID to map to here, and we
		// can't map to both.  If we look up this Git-style hash later, it needs to
		// unambiguously indicate a single HID.  In theory we could perhaps keep
		// importing and hope that this Git-style hash is never referenced later in
		// the file.  That even seems reasonably likely, since blobs aren't
		// referenced by hash very frequently.  However, this problem still indicates
		// a bad import file (containing two blobs with a hash collision) and should
		// also be so rare as to basically never happen, so I'm not going to bother
		// trying to handle it any further right now.
		SG_ERR_THROW2(  SG_ERR_VHASH_DUPLICATEKEY, (pCtx, "Same Git SHA1 (%s) maps to different Veracity HIDs (%s and %s)", psz_git_hash, *ppsz_hid, psz_existing_hid));
	}

	return;

fail:
	if (pbh)
    {
		SG_ERR_IGNORE(  SG_repo__store_blob__abort(pCtx, pst->pRepo, pst->ptx, &pbh)  );
    }
	if (phh)
	{
		SGHASH_abort(&phh);
	}
}

static void x_skip_optional_lf(
    SG_context * pCtx,
    struct sg_fast_import_state* pst
	)
{
    SG_uint64 initial_pos = 0;
    SG_byte c = 0;
    SG_uint32 got = 0;

    SG_ERR_CHECK(  SG_file__tell(pCtx, pst->pfi, &initial_pos)  );

    SG_file__read(pCtx, pst->pfi, 1, &c, &got);
    if (SG_context__has_err(pCtx))
    {
	if (SG_context__err_equals(pCtx, SG_ERR_EOF))
	{
	    SG_context__err_reset(pCtx);
	    return;
	}
	else
	{
	    SG_ERR_CHECK_CURRENT;
	}
    }

    if (10 != c)
    {
        SG_ERR_CHECK(  SG_file__seek(pCtx, pst->pfi, initial_pos)  );
    }

fail:
    ;
}

struct sg_line
{
    SG_uint64 tell;
    SG_string* pstr_line;
	char** words;
    SG_uint32 count_words;
    SG_bool b_eof;
};

static void line_dispose(
	SG_context* pCtx,
    struct sg_line* pline
    )
{
    SG_STRINGLIST_NULLFREE(pCtx, pline->words, pline->count_words);
    SG_STRING_NULLFREE(pCtx, pline->pstr_line);
    memset(pline, 0, sizeof(struct sg_line));
}

static void line_read(
	SG_context* pCtx,
    SG_file* pfi,
    struct sg_line* pline
    )
{
    SG_ERR_CHECK(  line_dispose(pCtx, pline)  );
    SG_ERR_CHECK(  SG_file__tell(pCtx, pfi, &pline->tell)  );
    while (1)
    {
        SG_ERR_CHECK(  SG_file__read__line__LF(pCtx, pfi, &pline->pstr_line)  );
        if (!pline->pstr_line)
        {
            pline->b_eof = SG_TRUE;
            break;
        }

        //fprintf(stderr, "line (at %10d): %s\n", (SG_uint32) pline->tell, SG_string__sz(pline->pstr_line));
        if (!SG_string__sz(pline->pstr_line)[0])
        {
            // TODO empty string
            break;
        }

        if ('#' != SG_string__sz(pline->pstr_line)[0])
        {
            SG_ERR_CHECK(  SG_string__split__asciichar(pCtx, pline->pstr_line, ' ', 32, &pline->words, &pline->count_words) );
            break;
        }
        else
        {
            SG_STRING_NULLFREE(pCtx, pline->pstr_line);
        }
    }

fail:
    ;
}

static void line_unread(
	SG_context* pCtx,
    SG_file* pfi,
    struct sg_line* pline
    )
{
    SG_ERR_CHECK(  SG_file__seek(pCtx, pfi, pline->tell)  );
    SG_ERR_CHECK(  line_dispose(pCtx, pline)  );

fail:
    ;
}

static void read_optional_mark(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* pline,
    SG_uint32* p_idnum
    )
{
    SG_uint32 idnum = 0;

    if (0 == strcmp(pline->words[0], "mark"))
    {
        if (2 != pline->count_words)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }

        if (pline->words[1][0] != ':')
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }

        SG_ERR_CHECK(SG_uint32__parse__strict(pCtx, &idnum, pline->words[1]+1));

        if (0 == idnum)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }

        SG_ERR_CHECK(  line_dispose(pCtx, pline)  );
        SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

        *p_idnum = idnum;
    }

fail:
    ;
}

static void is_branch_closed(
	SG_context* pCtx,
	SG_repo*    pRepo,
	const char* szBranch,
	SG_bool*    pClosed
	)
{
	SG_vhash* pBranches       = NULL;
	SG_vhash* pClosedBranches = NULL;

	SG_ERR_CHECK(  SG_vc_branches__get_whole_pile(pCtx, pRepo, &pBranches)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pBranches, "closed", &pClosedBranches)  );
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pClosedBranches, szBranch, pClosed)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pBranches);
	return;
}

static void x_do__blob__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    struct sg_line line;
    SG_uint32 idnum = 0;
    char* psz_hid = NULL;

    SG_UNUSED(p_line_cmd);

    memset(&line, 0, sizeof(struct sg_line));
    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );

    SG_ERR_CHECK(  read_optional_mark(pCtx, pst, &line, &idnum)  );

    if (0 != strcmp(line.words[0], "data"))
    {
        SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
    }

    if (2 != line.count_words)
    {
        SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
    }

    if (
            (SG_STRLEN(line.words[1]) >= 2)
            && ('<' == line.words[1][0])
            && ('<' == line.words[1][1])
       )
    {
        // TODO delimited
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }
    else
    {
        SG_uint64 len = 0;
        SG_uint64 pos = 0;

        SG_ERR_CHECK(SG_uint64__parse__strict(pCtx, &len, line.words[1]));

        // in pass1, just skip the blob
        SG_ERR_CHECK(  SG_file__tell(pCtx, pst->pfi, &pos)  );
        SG_ERR_CHECK(  SG_file__seek(pCtx, pst->pfi, pos + len)  );
    }

    SG_ERR_CHECK(  x_skip_optional_lf(pCtx, pst)  );

fail:
    SG_NULLFREE(pCtx, psz_hid);
    SG_ERR_IGNORE(  line_dispose(pCtx, &line)  );
}

static void x_do__blob(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    struct sg_line line;
    SG_uint32 idnum = 0;
    char* psz_hid = NULL;
    char buf_idnum[32];

    SG_UNUSED(p_line_cmd);

    memset(&line, 0, sizeof(struct sg_line));
    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );

    SG_ERR_CHECK(  read_optional_mark(pCtx, pst, &line, &idnum)  );

    if (idnum > 0)
    {
        SG_ERR_CHECK(  SG_sprintf(pCtx, buf_idnum, sizeof(buf_idnum), ":%d", idnum)  );
    }

    if (0 != strcmp(line.words[0], "data"))
    {
        SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
    }

    if (2 != line.count_words)
    {
        SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
    }

    if (
            (SG_STRLEN(line.words[1]) >= 2)
            && ('<' == line.words[1][0])
            && ('<' == line.words[1][1])
       )
    {
        // TODO delimited
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }
    else
    {
        SG_uint64 len = 0;

        SG_ERR_CHECK(SG_uint64__parse__strict(pCtx, &len, line.words[1]));

        // TODO check len > 0

        SG_ERR_CHECK(  store_blob(pCtx,
                    pst,
                    len,
                    &psz_hid)  );
        if (idnum)
        {
            // store the mark
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pst->pvh_blob_marks, buf_idnum, psz_hid)  );
        }
        SG_NULLFREE(pCtx, psz_hid);
    }

    SG_ERR_CHECK(  x_skip_optional_lf(pCtx, pst)  );

fail:
    SG_NULLFREE(pCtx, psz_hid);
    SG_ERR_IGNORE(  line_dispose(pCtx, &line)  );
}

static void x_do__checkpoint__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);
}

static void x_do__progress__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);
}

static void x_do__cat_blob__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);
}

static void x_do__feature__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);
}

static void x_do__option__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);
}

static void x_do__ls__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);
}

static void x_do__checkpoint(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);

    // TODO could commit and restart the tx?
    SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );

fail:
    ;
}

static void x_do__progress(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);
    SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );

fail:
    ;
}

static void x_do__cat_blob(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);
    SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );

fail:
    ;
}

static void x_do__feature(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);
    SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );

fail:
    ;
}

static void x_do__option(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);
    SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );

fail:
    ;
}

static void x_do__ls(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);
    SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );

fail:
    ;
}

static void x_node__free(SG_context* pCtx, struct node* ptn);

static void x_node__alloc__top(
	SG_context* pCtx, 
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
	const char* pszHid
	)
{
	struct node* ptn = NULL;
	
    SG_ASSERT(!pc->pSuperRoot);

	SG_ERR_CHECK(  SG_alloc1(pCtx, ptn)  );

	ptn->type = SG_TREENODEENTRY_TYPE_DIRECTORY;
	ptn->prbItems = NULL;
	ptn->pszgid = NULL;
	SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, pszHid, &ptn->pszCurrentHid)  );

	pc->pSuperRoot = ptn;

	return;

fail:
	x_node__free(pCtx, ptn);
}

#if 0
static void x_node__alloc__load(
	SG_context* pCtx, 
	struct node** ppNew,
    struct sg_fast_import_state* pst,
	const SG_treenode_entry* pEntry,
	const char* pszCurrentName,
	SG_treenode_entry_type type,
	const char* pszGid,
	const char* pszHid,
    SG_int64 iAttributeBits,
	const char* pszgidMovedFrom,
	SG_uint16 saved_flags
	)
{
	struct node* ptn = NULL;
	
	SG_ERR_CHECK(  SG_alloc1(pCtx, ptn)  );

	ptn->type = type;
	ptn->saved_flags = saved_flags;

	if (pEntry)
	{
		SG_ERR_CHECK(  SG_TREENODE_ENTRY__ALLOC__COPY(pCtx, &ptn->pBaselineEntry, pEntry)  );
	}
	if (pszCurrentName)
	{
		SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, pszCurrentName, &ptn->pszCurrentName)  );
	}
	if (pszGid)
	{
		SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, pszGid, &ptn->pszgid)  );
	}
	if (pszHid)
	{
		SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, pszHid, &ptn->pszCurrentHid)  );
	}
    ptn->iCurrentAttributeBits = iAttributeBits;
	if (pszgidMovedFrom)
	{
		SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, pszgidMovedFrom, &ptn->pszgidMovedFrom)  );
	}

	*ppNew = ptn;
	
	return;

fail:
	x_node__free(pCtx, ptn);
}
#endif

void x_node__add_entry(
        SG_context* pCtx, 
        struct node* ptnParent, 
        struct node* ptnEntry
        );

static void x_node__get_hid(
	SG_context* pCtx, 
	const struct node* ptn,
	const char** ppszhid
	);

static void x_node__delete_if_empty(
	SG_context* pCtx,
	struct node* ptn
	);

static void x_node__alloc__move(
	SG_context* pCtx, 
    struct node* ptn_dir_to,
    const char* psz_name,
    struct node* ptn_old,
    struct sg_fast_import_state* pst
	)
{
	struct node* ptn = NULL;
    SG_bool b_already_there = SG_FALSE;
	
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, ptn_dir_to->prbItems, ptn_old->pszgid, &b_already_there, (void**) &ptn)  );
    if (b_already_there)
    {
        // assert ptn
        // assert ptn->flags moved away
        // assert ptn_old->pszgidMovedFrom == ptn_dir_to->pszgid
        // can just remove ptn_old from ptn_old->pCurrentParent

        ptn->saved_flags &= (~sg_PTNODE_SAVED_FLAG_MOVED_AWAY);
        SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, psz_name, &ptn->pszCurrentName)  );
        SG_ERR_CHECK(  x_node__get_hid(pCtx, ptn_old, &ptn->pszCurrentHid)  );
        ptn->iCurrentAttributeBits = ptn_old->iCurrentAttributeBits;
        ptn->prbItems = ptn_old->prbItems;
        ptn_old->prbItems = NULL;
        SG_ERR_CHECK(  SG_rbtree__remove(pCtx, ptn_old->pCurrentParent->prbItems, ptn_old->pszgid)  );
        SG_ERR_CHECK(  x_node__delete_if_empty(pCtx, ptn_old->pCurrentParent)  );
        x_node__free(pCtx, ptn_old);
        ptn_old = NULL;
    }
    else
    {
        SG_ERR_CHECK(  SG_alloc1(pCtx, ptn)  );

        SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, psz_name, &ptn->pszCurrentName)  );

        ptn->type = ptn_old->type;
        ptn->pszgid = ptn_old->pszgid;
        ptn->pszgidMovedFrom = ptn_old->pCurrentParent->pszgid;
        SG_ERR_CHECK(  x_node__get_hid(pCtx, ptn_old, &ptn->pszCurrentHid)  );
        ptn->iCurrentAttributeBits = ptn_old->iCurrentAttributeBits;
        ptn->prbItems = ptn_old->prbItems;

        ptn_old->prbItems = NULL;
        ptn_old->saved_flags |= sg_PTNODE_SAVED_FLAG_MOVED_AWAY;
        SG_ERR_CHECK(  x_node__delete_if_empty(pCtx, ptn_old->pCurrentParent)  );

        SG_ERR_CHECK(  x_node__add_entry(pCtx, ptn_dir_to, ptn)  );
    }

fail:
    ;
}

static void x_node__alloc__new__with_gid(
	SG_context* pCtx, 
	struct node** ppNew,
    struct sg_fast_import_state* pst,
	const char* pszName,
	SG_treenode_entry_type type,
	const char * pszGid
	)
{
	struct node* ptn = NULL;
	
	SG_NULLARGCHECK_RETURN(ppNew);
	SG_NULLARGCHECK_RETURN(pst);
	SG_NULLARGCHECK_RETURN(pszName);

	SG_ARGCHECK_RETURN(  ((type == SG_TREENODEENTRY_TYPE_DIRECTORY)
						  || (type == SG_TREENODEENTRY_TYPE_REGULAR_FILE)
						  || (type == SG_TREENODEENTRY_TYPE_SYMLINK)
						  || (type == SG_TREENODEENTRY_TYPE_SUBMODULE)),
						 type);
	
	SG_ERR_CHECK(  SG_alloc1(pCtx, ptn)  );

	ptn->type = type;
	ptn->prbItems = NULL;
	SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, pszName, &ptn->pszCurrentName)  );
	SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, pszGid, &ptn->pszgid)  );
	
	*ppNew = ptn;

	return;

fail:
	x_node__free(pCtx, ptn);
}

static void x_node__alloc__new(
	SG_context* pCtx, 
	struct node** ppNew,
    struct sg_fast_import_state* pst,
	const char* pszName,
	SG_treenode_entry_type type
	)
{
	char bufGid[SG_GID_BUFFER_LENGTH];

	SG_ERR_CHECK_RETURN(  SG_gid__generate(pCtx, bufGid, sizeof(bufGid))  );
	SG_ERR_CHECK_RETURN(  x_node__alloc__new__with_gid(pCtx, ppNew, pst, pszName, type, bufGid)  );
}

//static SG_free_callback x_node__free;

static void x_node__free(SG_context* pCtx, struct node* ptn)
{
	if (!ptn)
	{
		return;
	}

	if (ptn->prbItems)
	{
		SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, ptn->prbItems, (SG_free_callback *)x_node__free);
	}

	SG_TREENODE_ENTRY_NULLFREE(pCtx, ptn->pBaselineEntry);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, ptn->pNewEntry);

	SG_NULLFREE(pCtx, ptn);
}

#if 0
/*
 * If a dir node is not dirty, then we don't need its info loaded,
 * so unload it.
 */
static void x_node__unload_dir_entries(SG_context* pCtx, struct node* ptn)
{
	SG_NULLARGCHECK_RETURN(ptn);

	SG_ARGCHECK_RETURN(ptn->type == SG_TREENODEENTRY_TYPE_DIRECTORY, ptn->type);

	if (ptn->prbItems)
	{
		SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, ptn->prbItems, (SG_free_callback *)x_node__free);
	}

}
#endif

static void x_node__alloc__entry(
	SG_context* pCtx, 
	struct node** ppNew,
    struct sg_fast_import_state* pst,
	const SG_treenode_entry* pEntry,
	const char* pszgid
	)
{
	struct node* ptn = NULL;

	SG_NULLARGCHECK_RETURN(ppNew);
	SG_NULLARGCHECK_RETURN(pst);
	SG_NULLARGCHECK_RETURN(pEntry);

	SG_ERR_CHECK(  SG_alloc1(pCtx, ptn)  );

	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pEntry, &ptn->type)  );
	SG_ERR_CHECK(  SG_TREENODE_ENTRY__ALLOC__COPY(pCtx, &ptn->pBaselineEntry, pEntry)  );
	SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, pszgid, &ptn->pszgid)  );
    SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, pEntry, &ptn->iCurrentAttributeBits)  );
	ptn->prbItems = NULL;

	*ppNew = ptn;

	return;
	
fail:
	x_node__free(pCtx, ptn);
}

void x_node__add_entry(
        SG_context* pCtx, 
        struct node* ptnParent, 
        struct node* ptnEntry
        )
{
	SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, ptnParent->prbItems, ptnEntry->pszgid, ptnEntry)  );

	ptnEntry->pCurrentParent = ptnParent;

fail:
	return;
}

static void x_commit__setup_parents(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc
	)
{
    char* pszidUserSuperRoot = NULL;
    SG_rbtree* prbLeaves = NULL;

    if (!pc->psz_baseline)
    {
        // TODO get the last checkin on the "branch"

        SG_ERR_CHECK(  SG_STRDUP(pCtx, pst->psz_initial_csid, &pc->psz_baseline)  );
        SG_ERR_CHECK(  SG_changeset__add_parent(pCtx, pc->pcs, pc->psz_baseline)  );
    }

    SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pst->pRepo, pc->psz_baseline, &pszidUserSuperRoot)  );
    SG_ERR_CHECK(  x_node__alloc__top(pCtx, pst, pc, pszidUserSuperRoot)  );

fail:
    SG_NULLFREE(pCtx, pszidUserSuperRoot);
    SG_RBTREE_NULLFREE(pCtx, prbLeaves);
}

static void x_commit__begin(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);

    memset(pc, 0, sizeof(struct sg_pending_commit));

    SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &pc->time_begin)  );
    if (!pst->ptx)
    {
        SG_ERR_CHECK(  SG_repo__begin_tx(pCtx, pst->pRepo, &pst->ptx)  );
    }

	SG_ERR_CHECK(  SG_changeset__alloc__committing(pCtx, SG_DAGNUM__VERSION_CONTROL, &pc->pcs)  );
	SG_ERR_CHECK(  SG_changeset__set_version(pCtx, pc->pcs, SG_CSET_VERSION__CURRENT)  );
    SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pc->pva_parents)  );

    return;

fail:
	SG_CHANGESET_NULLFREE(pCtx, pc->pcs);
}

static void x_commit__author(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    struct sg_line* pline
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);
    SG_NULLARGCHECK_RETURN(pline);

    // TODO we could use this information to generate a second audit

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    ;
}

static void x_commit__when(
    SG_context * pCtx,
    const char* psz_when,
    SG_int64* pi
    )
{
    SG_int64 v = 0;
    const char* p = psz_when;

    SG_NULLARGCHECK_RETURN(psz_when);
    SG_NULLARGCHECK_RETURN(pi);

    while (' ' != *p)
    {
        v = v * 10 + (*p - '0');
        p++;
    }

    v *= 1000;

    *pi = v;
}

static void x_commit__committer__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* pline,
    char* buf_email,
    SG_int64* p_when
	)
{
    const char* psz_when = NULL;
    SG_int64 when = -1;
    char* q = NULL;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(buf_email);
    SG_NULLARGCHECK_RETURN(p_when);
    SG_NULLARGCHECK_RETURN(pline);

    psz_when = SG_string__sz(pline->pstr_line);
    while ('<' != *psz_when)
    {
        psz_when++;
    }
    psz_when++;

    q = buf_email;
    while ('>' != *psz_when)
    {
        *q++ = *psz_when++;
    }
    *q = 0;
    psz_when++;
    psz_when++;

    SG_ERR_CHECK(  x_commit__when(pCtx, psz_when, &when)  );

#if 0
    {
        char buf[256];

		SG_ERR_CHECK(  SG_time__formatRFC850(pCtx,when,buf,sizeof(buf))  );
        printf("%s\n", buf);
    }
#endif

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

    *p_when = when;

fail:
    ;
}

static void x_commit__committer(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    struct sg_line* pline
	)
{
    const char* psz_when = NULL;
    char* q = NULL;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);
    SG_NULLARGCHECK_RETURN(pline);

    psz_when = SG_string__sz(pline->pstr_line);
    while ('<' != *psz_when)
    {
        psz_when++;
    }
    psz_when++;

    q = pc->buf_email;
    while ('>' != *psz_when)
    {
        *q++ = *psz_when++;
    }
    *q = 0;
    psz_when++;
    psz_when++;

    SG_ERR_CHECK(  x_commit__when(pCtx, psz_when, &pc->when)  );

#if 0
    {
        char buf[256];

		SG_ERR_CHECK(  SG_time__formatRFC850(pCtx,pc->when,buf,sizeof(buf))  );
        printf("%s\n", buf);
    }
#endif

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    ;
}

static void x_commit__from(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    struct sg_line* pline
	)
{
    const char* psz_csid = NULL;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);
    SG_NULLARGCHECK_RETURN(pline);

    if (':' == pline->words[1][0])
    {
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pst->pvh_commit_marks, pline->words[1], &psz_csid)  );
    }
    else
    {
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }

    SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_csid, &pc->psz_baseline)  );

	SG_ERR_CHECK(  SG_changeset__add_parent(pCtx, pc->pcs, psz_csid)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pc->pva_parents, pc->psz_baseline)  );

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    ;
}

static void x_commit__merge(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    struct sg_line* pline
	)
{
    const char* psz_csid = NULL;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);
    SG_NULLARGCHECK_RETURN(pline);

    if (':' == pline->words[1][0])
    {
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pst->pvh_commit_marks, pline->words[1], &psz_csid)  );
    }
    else
    {
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }

	SG_ERR_CHECK(  SG_changeset__add_parent(pCtx, pc->pcs, psz_csid)  );
    SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pc->pva_parents, psz_csid)  );
    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    ;
}

static void x_commit__comment__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* pline
	)
{
    SG_uint64 len = 0;
    SG_uint64 pos = 0;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pline);

    SG_ERR_CHECK(SG_uint64__parse__strict(pCtx, &len, pline->words[1]));

    // in pass1, just skip the comment blob
    SG_ERR_CHECK(  SG_file__tell(pCtx, pst->pfi, &pos)  );
    SG_ERR_CHECK(  SG_file__seek(pCtx, pst->pfi, pos + len)  );

    SG_ERR_CHECK(  x_skip_optional_lf(pCtx, pst)  );
    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    ;
}

static void x_commit__comment(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    struct sg_line* pline
	)
{
    SG_uint64 len = 0;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);
    SG_NULLARGCHECK_RETURN(pline);

    SG_ERR_CHECK(SG_uint64__parse__strict(pCtx, &len, pline->words[1]));

    if (len)
    {
        SG_ERR_CHECK(  SG_alloc(pCtx, (SG_uint32) (len+1), 1, &pc->psz_comment)  );
        SG_ERR_CHECK(  SG_file__read(pCtx, pst->pfi, (SG_uint32) len, (SG_byte*) pc->psz_comment, NULL)  );
        pc->psz_comment[len] = 0;

        if (len > 32000)
        {
            //printf("\n\nLONG COMMENT (%d chars):\n%s\n\n", (int) len, pc->psz_comment);
            pc->psz_comment[32000] = 0;
        }
    }

    SG_ERR_CHECK(  x_skip_optional_lf(pCtx, pst)  );
    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    ;
}

static void x_node__create_item__with_gid(
	SG_context* pCtx, 
    struct sg_fast_import_state* pst,
	struct node* ptn, 
	const char* pszName,
	SG_treenode_entry_type tneType,
	const char * pszGid,
	struct node** ppNewNode
	)
{
	struct node* pNewNode_Allocated = NULL;
	struct node* pNewNode;						// we do not own this

    SG_ERR_CHECK(  x_node__alloc__new__with_gid(pCtx, &pNewNode_Allocated, pst, pszName, tneType, pszGid)  );

    SG_ERR_CHECK(  x_node__add_entry(pCtx, ptn, pNewNode_Allocated)  );
    pNewNode = pNewNode_Allocated;
    pNewNode_Allocated = NULL;			// we don't own it any more.

	*ppNewNode = pNewNode;

	return;

fail:
    x_node__free(pCtx, pNewNode_Allocated);
}

static void x_node__get_hid(
	SG_context* pCtx, 
	const struct node* ptn,
	const char** ppszhid
	)
{
	
	if (ptn->pszCurrentHid)
	{
		*ppszhid = ptn->pszCurrentHid;
	}
	else if (ptn->pBaselineEntry)
	{
		SG_ERR_CHECK_RETURN(  SG_treenode_entry__get_hid_blob(pCtx, ptn->pBaselineEntry, ppszhid)  );
	}
	else
	{
		*ppszhid = NULL;
	}
}

static void x_node__get_name(
	SG_context* pCtx, 
	const struct node* ptn,
	const char** ppszName
	)
{
	if (ptn->pszCurrentName)
	{
		*ppszName = ptn->pszCurrentName;
	}
	else if (ptn->pBaselineEntry)
	{
		SG_ERR_CHECK_RETURN(  SG_treenode_entry__get_entry_name(pCtx, ptn->pBaselineEntry, ppszName)  );
	}
	else
	{
		*ppszName = NULL;
	}
}

static void x_node__create_entry_for_commit(
	SG_context* pCtx,
	struct node* ptnSub
	)
{
	SG_treenode_entry* pEntry = NULL;

    if (
        (ptnSub->saved_flags & sg_PTNODE_SAVED_FLAG_DELETED)
        || (ptnSub->saved_flags & sg_PTNODE_SAVED_FLAG_MOVED_AWAY)
        )
    {
        /* ignore this then */
    }
    else
    {
        const char* pszName = NULL;
        const char* pszHid = NULL;

        SG_ERR_CHECK(  x_node__get_name(pCtx, ptnSub, &pszName)  );
        SG_ERR_CHECK(  x_node__get_hid(pCtx, ptnSub, &pszHid)  );

        if (!pszHid || !*pszHid)
            SG_ERR_THROW2(  SG_ERR_TREENODE_ENTRY_VALIDATION_FAILED,
                            (pCtx, "HID not set when creating TNE for %s '%s'",
                             ptnSub->pszgid, ((pszName) ? pszName : "(null)"))  );

        SG_ERR_CHECK(  SG_TREENODE_ENTRY__ALLOC(pCtx, &pEntry)  );
        SG_ERR_CHECK(  SG_treenode_entry__set_entry_name(pCtx, pEntry, pszName)  );
        SG_ERR_CHECK(  SG_treenode_entry__set_entry_type(pCtx, pEntry, ptnSub->type)  );
        SG_ERR_CHECK(  SG_treenode_entry__set_hid_blob(pCtx, pEntry, pszHid)  );
        SG_ERR_CHECK(  SG_treenode_entry__set_attribute_bits(pCtx, pEntry, ptnSub->iCurrentAttributeBits)  );
    }

	ptnSub->pNewEntry = pEntry;

	return;

fail:
	SG_TREENODE_ENTRY_NULLFREE(pCtx, pEntry);
}

static void x_add_treenode(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    SG_treenode** ppTreenode,
    char** ppszidHidReturned
    )
{
	char* pszidHid = NULL;
	SG_uint64 iBlobFullLength = 0;

	SG_NULLARGCHECK_RETURN(pc);
	SG_NULLARGCHECK_RETURN(pst->pRepo);
	SG_NULLARGCHECK_RETURN(ppTreenode);
	SG_NULLARGCHECK_RETURN(*ppTreenode);

#if 0
    {
        SG_vhash* pvh = NULL;
        SG_uint32 count = 0;
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );
		SG_ERR_CHECK(  SG_treenode__count(pCtx, *ppTreenode, &count)  );
		for (i=0; i<count; i++)
		{
			const SG_treenode_entry* pEntry = NULL;
            const char* psz_gid = NULL;
            const char* psz_name = NULL;
			
			SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, *ppTreenode, i, &psz_gid, &pEntry)  );

            SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pEntry, &psz_name)  );

            SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh, psz_name)  );
		}

        SG_VHASH_NULLFREE(pCtx, pvh);
    }
#endif
	SG_ERR_CHECK(  SG_treenode__save_to_repo(pCtx, *ppTreenode,pst->pRepo,pst->ptx, &iBlobFullLength)  );

	// Try to add the HID of the Blob to the Changeset Blob-List.
	// We ***ASSUME*** that if the HID is already present in the Blob-List, it just returns OK.

	SG_ERR_CHECK(  SG_treenode__get_id(pCtx, *ppTreenode,&pszidHid)  );

	SG_ERR_CHECK(  SG_changeset__tree__add_treenode(pCtx, pc->pcs, pszidHid, ppTreenode)  );

	if (ppszidHidReturned)
		*ppszidHidReturned = pszidHid;
	else
		SG_NULLFREE(pCtx, pszidHid);

	return;

fail:
	SG_NULLFREE(pCtx, pszidHid);
}

static void x_node__load_dir_entries(
        SG_context* pCtx, 
        struct sg_fast_import_state* pst,
        struct node* ptn
        )
{
	SG_uint32 count = 0;
	SG_uint32 i;
	struct node* pNewNode = NULL;
	const char* pszidGid = NULL;
	SG_treenode* pTreenode = NULL;
	const char* pszhid = NULL;
	
	SG_NULLARGCHECK_RETURN(ptn);
	SG_ARGCHECK_RETURN(ptn->type == SG_TREENODEENTRY_TYPE_DIRECTORY, ptn->type);

	if (ptn->prbItems)
		return;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &ptn->prbItems)  );

	SG_ERR_CHECK(  x_node__get_hid(pCtx, ptn, &pszhid)  );
	
	if (pszhid)
	{
		SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pst->pRepo, pszhid, &pTreenode)  );
		
		SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenode, &count)  );
		for (i=0; i<count; i++)
		{
			const SG_treenode_entry* pEntry = NULL;
			
			SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pTreenode, i, &pszidGid, &pEntry)  );
			SG_ERR_CHECK(  x_node__alloc__entry(pCtx, &pNewNode, pst, pEntry, pszidGid)  );
			SG_ERR_CHECK(  x_node__add_entry(pCtx, ptn, pNewNode)  );
			pNewNode = NULL;
		}
		
		SG_TREENODE_NULLFREE(pCtx, pTreenode);
	}

	return;

fail:
	SG_TREENODE_NULLFREE(pCtx, pTreenode);
	x_node__free(pCtx, pNewNode);
}

static void x_node__find_entry_by_name_not_recursive(
        SG_context* pCtx, 
        const struct node* ptnDir, 
        const char* pszName, 
        struct node** ppResult
        )
{
	SG_bool b = SG_FALSE;
	const char* pszKey = NULL;
	struct node* ptnSub = NULL;
	SG_rbtree_iterator* pIterator = NULL;
	const char* pszEntryName = NULL;
	struct node* ptnResult = NULL;

	SG_NULLARGCHECK_RETURN(ptnDir);
	SG_NULLARGCHECK_RETURN(ptnDir->prbItems);

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, ptnDir->prbItems, &b, &pszKey, (void**) &ptnSub)  );
	while (b)
	{
        SG_ERR_CHECK(  x_node__get_name(pCtx, ptnSub, &pszEntryName)  );
        
        if (0 == strcmp(pszName, pszEntryName))
        {
            ptnResult = ptnSub;
            break;
        }
		
		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &b, &pszKey, (void**) &ptnSub)  );
	}
	SG_rbtree__iterator__free(pCtx, pIterator);
	pIterator = NULL;

	*ppResult = ptnResult;

	return;

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
}

static void x_node__find_entry_by_name_not_recursive__non_directory(
        SG_context* pCtx, 
        const struct node* ptnDir, 
        const char* pszName, 
        struct node** ppResult
        )
{
	SG_bool b = SG_FALSE;
	const char* pszKey = NULL;
	struct node* ptnSub = NULL;
	SG_rbtree_iterator* pIterator = NULL;
	const char* pszEntryName = NULL;
	struct node* ptnResult = NULL;

	SG_NULLARGCHECK_RETURN(ptnDir);
	SG_NULLARGCHECK_RETURN(ptnDir->prbItems);

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, ptnDir->prbItems, &b, &pszKey, (void**) &ptnSub)  );
	while (b)
	{
		if (ptnSub->type != SG_TREENODEENTRY_TYPE_DIRECTORY)
		{
			SG_ERR_CHECK(  x_node__get_name(pCtx, ptnSub, &pszEntryName)  );

			if (0 == strcmp(pszName, pszEntryName))
			{
				ptnResult = ptnSub;
				break;
			}
		}
		
		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &b, &pszKey, (void**) &ptnSub)  );
	}
	SG_rbtree__iterator__free(pCtx, pIterator);
	pIterator = NULL;

	*ppResult = ptnResult;

	return;

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
}

static void x_node__find_entry_by_name_not_recursive__active(
        SG_context* pCtx, 
        const struct node* ptnDir, 
        const char* pszName, 
        struct node** ppResult
        )
{
	SG_bool b = SG_FALSE;
	const char* pszKey = NULL;
	struct node* ptnSub = NULL;
	SG_rbtree_iterator* pIterator = NULL;
	const char* pszEntryName = NULL;
	struct node* ptnResult = NULL;

	SG_NULLARGCHECK_RETURN(ptnDir);
	SG_NULLARGCHECK_RETURN(ptnDir->prbItems);

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, ptnDir->prbItems, &b, &pszKey, (void**) &ptnSub)  );
	while (b)
	{
		if (
			(ptnSub->saved_flags & sg_PTNODE_SAVED_FLAG_DELETED)
			|| (ptnSub->saved_flags & sg_PTNODE_SAVED_FLAG_MOVED_AWAY)
			)
		{
		}
		else
		{
			SG_ERR_CHECK(  x_node__get_name(pCtx, ptnSub, &pszEntryName)  );
			
			if (0 == strcmp(pszName, pszEntryName))
			{
				ptnResult = ptnSub;
				break;
			}
		}
		
		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &b, &pszKey, (void**) &ptnSub)  );
	}
	SG_rbtree__iterator__free(pCtx, pIterator);
	pIterator = NULL;

	*ppResult = ptnResult;

	return;

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
}

void x_find_existing_directory(
	SG_context* pCtx, 
    struct sg_fast_import_state* pst,
	struct node* pNode,
	const char** ppszPathParts,
	SG_uint32 count_parts,
	SG_uint32 cur_part,
	struct node** ppResult
	)
{
	struct node* ptn = NULL;
	struct node* pNewNode = NULL;
	
	SG_NULLARGCHECK_RETURN(pNode);
	SG_NULLARGCHECK_RETURN(ppszPathParts);
	SG_NULLARGCHECK_RETURN(ppResult);
	SG_ARGCHECK_RETURN((count_parts > 0), count_parts);
	SG_ARGCHECK_RETURN((SG_TREENODEENTRY_TYPE_DIRECTORY == pNode->type), nNode->type);

	SG_ERR_CHECK(  x_node__load_dir_entries(pCtx, pst, pNode)  );

	SG_ERR_CHECK(  x_node__find_entry_by_name_not_recursive__active(pCtx, pNode, ppszPathParts[cur_part], &ptn)  );
	
	if (!ptn)
	{
        SG_ERR_THROW2(  SG_ERR_PENDINGTREE_PARENT_DIR_MISSING_OR_INACTIVE, (pCtx, "%s", ppszPathParts[cur_part])  );
	}
	
	if ((cur_part +1) < count_parts)
	{
		SG_ERR_CHECK_RETURN(  x_find_existing_directory(pCtx, pst, ptn, ppszPathParts, count_parts, cur_part+1, ppResult)  );
		return;
	}
	else
	{
		SG_ARGCHECK_RETURN((SG_TREENODEENTRY_TYPE_DIRECTORY == ptn->type), ptn->type);

		SG_ERR_CHECK(  x_node__load_dir_entries(pCtx, pst, ptn)  );
		
		*ppResult = ptn;
		return;
	}
	
fail:
	x_node__free(pCtx, pNewNode);
}

void x_find_directory(
	SG_context* pCtx, 
    struct sg_fast_import_state* pst,
	struct node* pNode,
	SG_bool bSynthesizeMissingParents,
	const char** ppszPathParts,
	SG_uint32 count_parts,
	SG_uint32 cur_part,
	struct node** ppResult
	)
{
	struct node* ptn = NULL;
	struct node* pNewNode = NULL;
	
	SG_NULLARGCHECK_RETURN(pNode);
	SG_NULLARGCHECK_RETURN(ppszPathParts);
	SG_NULLARGCHECK_RETURN(ppResult);
	SG_ARGCHECK_RETURN((count_parts > 0), count_parts);
	SG_ARGCHECK_RETURN((SG_TREENODEENTRY_TYPE_DIRECTORY == pNode->type), nNode->type);

	SG_ERR_CHECK(  x_node__load_dir_entries(pCtx, pst, pNode)  );

	SG_ERR_CHECK(  x_node__find_entry_by_name_not_recursive__active(pCtx, pNode, ppszPathParts[cur_part], &ptn)  );
	
	if (!ptn)
    {
        SG_ERR_CHECK(  x_node__find_entry_by_name_not_recursive(pCtx, pNode, ppszPathParts[cur_part], &ptn)  );
        if (
                ptn
                && (ptn->saved_flags & sg_PTNODE_SAVED_FLAG_DELETED)
                )
        {
            ptn->saved_flags &= (~sg_PTNODE_SAVED_FLAG_DELETED);
        }
        else
        {
            ptn = NULL;
        }
    }

	if (!ptn)
	{
		if (!bSynthesizeMissingParents)
        {
			SG_ERR_THROW(  SG_ERR_PENDINGTREE_PARENT_DIR_MISSING_OR_INACTIVE  );
        }

#if TRACE_PENDINGTREE
		SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
								   "x_node__find_directory: synthesizing directory node for [%s]\n",
								   ppszPathParts[cur_part])  );
#endif
		SG_ERR_CHECK(  x_node__alloc__new(pCtx, &pNewNode, pst, ppszPathParts[cur_part], SG_TREENODEENTRY_TYPE_DIRECTORY)  );
		SG_ERR_CHECK(  x_node__add_entry(pCtx, pNode, pNewNode)  );
		ptn = pNewNode;
		pNewNode = NULL;
	}
	
	if ((cur_part +1) < count_parts)
	{
		SG_ERR_CHECK_RETURN(  x_find_directory(pCtx, pst, ptn, bSynthesizeMissingParents, ppszPathParts, count_parts, cur_part+1, ppResult)  );
		return;
	}
	else
	{
		if (ptn->type != SG_TREENODEENTRY_TYPE_DIRECTORY)
		{
			// This should only happen in a circumstance where a single commit includes the deletion of
			// a file AND the creation of a directory with the same name, and the fast-import file happens
			// to list the creation before the deletion.  In that case, we're going to find the file whose
			// delete we haven't processed yet, rather than a directory node.
			// To handle this, we'll simply make a sibling node that actually is a directory and use it.
			// This will temporarily place us in the invalid state of having a directory and file with the
			// same path, but when the delete is processed later it will remove the file node and return
			// us to a valid state.
			SG_ERR_CHECK(  x_node__alloc__new(pCtx, &pNewNode, pst, ppszPathParts[cur_part], SG_TREENODEENTRY_TYPE_DIRECTORY)  );
			SG_ERR_CHECK(  x_node__add_entry(pCtx, pNode, pNewNode)  );
			ptn = pNewNode;
			pNewNode = NULL;
		}

		SG_ERR_CHECK(  x_node__load_dir_entries(pCtx, pst, ptn)  );
		
		*ppResult = ptn;
		return;
	}
	
fail:
	x_node__free(pCtx, pNewNode);
}

static void x_path_to_existing_node(
	SG_context* pCtx, 
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    const char* psz_path,
    struct node** ppParent,
	struct node** ppNode
	)
{
	SG_string* pStrRepoPath = NULL;
	const char** apszParts = NULL;
	SG_uint32 count_parts = 0;
	SG_strpool* pPool = NULL;
	struct node* ptn_dir = NULL;
	struct node* ptn_file = NULL;

	SG_ERR_CHECK(  SG_STRPOOL__ALLOC(pCtx, &pPool, 10)  );
	
    SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStrRepoPath, "@/")  );
    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pStrRepoPath, psz_path)  );

	SG_ERR_CHECK(  SG_repopath__split(pCtx, SG_string__sz(pStrRepoPath), pPool, &apszParts, &count_parts)  );

	SG_ERR_CHECK(  x_find_existing_directory(pCtx, pst, pc->pSuperRoot, apszParts, count_parts - 1, 0, &ptn_dir)  );

    // ptn_dir now contains the parent dir

	SG_ERR_CHECK(  x_node__find_entry_by_name_not_recursive(pCtx, ptn_dir, apszParts[count_parts-1], &ptn_file)  );
    if (ptn_file)
    {
        if (SG_TREENODEENTRY_TYPE_DIRECTORY == ptn_file->type)
        {
            SG_ERR_CHECK(  x_node__load_dir_entries(pCtx, pst, ptn_file)  );
        }
    }

    *ppParent = ptn_dir;
	*ppNode = ptn_file;

	SG_NULLFREE(pCtx, apszParts);
	SG_STRING_NULLFREE(pCtx, pStrRepoPath);

	SG_STRPOOL_NULLFREE(pCtx, pPool);

	return;

fail:
	SG_STRING_NULLFREE(pCtx, pStrRepoPath);
	SG_NULLFREE(pCtx, apszParts);
	SG_STRPOOL_NULLFREE(pCtx, pPool);
}

static void x_path_to_existing_node__non_directory(
	SG_context* pCtx, 
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    const char* psz_path,
    struct node** ppParent,
	struct node** ppNode
	)
{
	SG_string* pStrRepoPath = NULL;
	const char** apszParts = NULL;
	SG_uint32 count_parts = 0;
	SG_strpool* pPool = NULL;
	struct node* ptn_dir = NULL;
	struct node* ptn_file = NULL;

	SG_ERR_CHECK(  SG_STRPOOL__ALLOC(pCtx, &pPool, 10)  );
	
    SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStrRepoPath, "@/")  );
    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pStrRepoPath, psz_path)  );

	SG_ERR_CHECK(  SG_repopath__split(pCtx, SG_string__sz(pStrRepoPath), pPool, &apszParts, &count_parts)  );

	SG_ERR_CHECK(  x_find_existing_directory(pCtx, pst, pc->pSuperRoot, apszParts, count_parts - 1, 0, &ptn_dir)  );

    // ptn_dir now contains the parent dir

	SG_ERR_CHECK(  x_node__find_entry_by_name_not_recursive__non_directory(pCtx, ptn_dir, apszParts[count_parts-1], &ptn_file)  );
    if (ptn_file)
    {
        if (SG_TREENODEENTRY_TYPE_DIRECTORY == ptn_file->type)
        {
            SG_ERR_CHECK(  x_node__load_dir_entries(pCtx, pst, ptn_file)  );
        }
    }

    *ppParent = ptn_dir;
	*ppNode = ptn_file;

	SG_NULLFREE(pCtx, apszParts);
	SG_STRING_NULLFREE(pCtx, pStrRepoPath);

	SG_STRPOOL_NULLFREE(pCtx, pPool);

	return;

fail:
	SG_STRING_NULLFREE(pCtx, pStrRepoPath);
	SG_NULLFREE(pCtx, apszParts);
	SG_STRPOOL_NULLFREE(pCtx, pPool);
}

static void x_path_to_node__or_create_file(
	SG_context* pCtx, 
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    const char* psz_path,
	struct node** ppNode
	)
{
	SG_string* pStrRepoPath = NULL;
	const char** apszParts = NULL;
	SG_uint32 count_parts = 0;
	SG_strpool* pPool = NULL;
	struct node* ptn_dir = NULL;
	struct node* ptn_file = NULL;

	SG_ERR_CHECK(  SG_STRPOOL__ALLOC(pCtx, &pPool, 10)  );
	
    SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStrRepoPath, "@/")  );
    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pStrRepoPath, psz_path)  );

	SG_ERR_CHECK(  SG_repopath__split(pCtx, SG_string__sz(pStrRepoPath), pPool, &apszParts, &count_parts)  );

	SG_ERR_CHECK(  x_find_directory(pCtx, pst, pc->pSuperRoot, SG_TRUE, apszParts, count_parts - 1, 0, &ptn_dir)  );

    // ptn_dir now contains the parent dir

	SG_ERR_CHECK(  x_node__find_entry_by_name_not_recursive__active(pCtx, ptn_dir, apszParts[count_parts-1], &ptn_file)  );
    if (!ptn_file)
    {
        char bufGid[SG_GID_BUFFER_LENGTH];

        SG_ERR_CHECK_RETURN(  SG_gid__generate(pCtx, bufGid, sizeof(bufGid))  );

        SG_ERR_CHECK(  x_node__create_item__with_gid(
                    pCtx,
                    pst,
                    ptn_dir,
                    apszParts[count_parts-1],
                    SG_TREENODEENTRY_TYPE_REGULAR_FILE,
                    bufGid,
                    &ptn_file
                    )  );
    }

	*ppNode = ptn_file;

	SG_NULLFREE(pCtx, apszParts);
	SG_STRING_NULLFREE(pCtx, pStrRepoPath);

	SG_STRPOOL_NULLFREE(pCtx, pPool);

	return;

fail:
	SG_STRING_NULLFREE(pCtx, pStrRepoPath);
	SG_NULLFREE(pCtx, apszParts);
	SG_STRPOOL_NULLFREE(pCtx, pPool);
}

static void x_strip_quotes(
	SG_context* pCtx,
	const char* szValue,
	char** ppStripped
	)
{
	SG_uint32 uLength    = 0u;
	char*     szStripped = NULL;

	SG_NULLARGCHECK(szValue);
	SG_ARGCHECK(szValue[0u] == '"', szValue);
	SG_NULLARGCHECK(ppStripped);

	uLength = SG_STRLEN(szValue);
	if (szValue[uLength - 1u] != '"')
	{
		SG_ERR_THROW2(SG_ERR_MALFORMED_GIT_FAST_IMPORT, (pCtx, "Path starts with double-quote but doesn't end with one: %s", szValue));
	}

	SG_ERR_CHECK(  SG_allocN(pCtx, uLength - 1u, szStripped)  );
	SG_ERR_CHECK(  SG_strncpy(pCtx, szStripped, uLength - 1u, szValue + 1u, uLength - 2u)  );

	*ppStripped = szStripped;
	szStripped = NULL;

fail:
	SG_NULLFREE(pCtx, szStripped);
	return;
}

static void x_commit__filemodify__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* pline
	)
{
    const char* psz_path = NULL;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pline);

    psz_path = SG_string__sz(pline->pstr_line);
    SG_ASSERT('M' == *psz_path);
    psz_path++;
    SG_ASSERT(' ' == *psz_path);
    psz_path++;

    while (' ' != *psz_path)
    {
        psz_path++;
    }
    psz_path++;

    while (' ' != *psz_path)
    {
        psz_path++;
    }
    psz_path++;

    if (0 == strcmp("inline", pline->words[2]))
    {
        SG_uint64 len = 0;
        SG_uint64 pos = 0;

        SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );
        if (0 != strcmp(pline->words[0], "data"))
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(SG_uint64__parse__strict(pCtx, &len, pline->words[1]));

        // in pass1, just skip the comment blob
        SG_ERR_CHECK(  SG_file__tell(pCtx, pst->pfi, &pos)  );
        SG_ERR_CHECK(  SG_file__seek(pCtx, pst->pfi, pos + len)  );

        SG_ERR_CHECK(  x_skip_optional_lf(pCtx, pst)  );
    }

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    ;
}

static void x_commit__filemodify(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    struct sg_line* pline
	)
{
    const char* psz_path = NULL;
	char* psz_path_stripped = NULL;
    const char* psz_hid = NULL;
    char* psz_hid_freeme = NULL;
    struct node* ptn = NULL;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);
    SG_NULLARGCHECK_RETURN(pline);

    psz_path = SG_string__sz(pline->pstr_line);
    SG_ASSERT('M' == *psz_path);
    psz_path++;
    SG_ASSERT(' ' == *psz_path);
    psz_path++;

    while (' ' != *psz_path)
    {
        psz_path++;
    }
    psz_path++;

    while (' ' != *psz_path)
    {
        psz_path++;
    }
    psz_path++;

	if (psz_path[0u] == '"')
	{
		SG_ERR_CHECK(  x_strip_quotes(pCtx, psz_path, &psz_path_stripped)  );
		psz_path = psz_path_stripped;
	}

    SG_ERR_CHECK(  x_path_to_node__or_create_file(pCtx, pst, pc, psz_path, &ptn)  );

    if (
            (0 == strcmp("100644", pline->words[1]))
            || (0 == strcmp("644", pline->words[1]))
       )
    {
        // normal, non-executable file
        ptn->iCurrentAttributeBits = 0;
    }
    else if (
            (0 == strcmp("100755", pline->words[1]))
            || (0 == strcmp("755", pline->words[1]))
       )
    {
        // normal, executable
        ptn->iCurrentAttributeBits = 1;
    }
    else if (0 == strcmp("120000", pline->words[1]))
    {
        // symlink
        ptn->type = SG_TREENODEENTRY_TYPE_SYMLINK;
    }
    else if (0 == strcmp("160000", pline->words[1]))
    {
        // git link
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }
    else if (0 == strcmp("040000", pline->words[1]))
    {
        // subdir
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }
    else
    {
        SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
    }

    if (':' == pline->words[2][0])
    {
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pst->pvh_blob_marks, pline->words[2], &psz_hid)  );
    }
    else if (0 == strcmp("inline", pline->words[2]))
    {
        struct sg_line line;
        SG_uint64 len = 0;

        memset(&line, 0, sizeof(struct sg_line));
        SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );

        if (0 != strcmp(line.words[0], "data"))
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(SG_uint64__parse__strict(pCtx, &len, line.words[1]));

        SG_ERR_CHECK(  store_blob(pCtx,
                    pst,
                    len,
                    &psz_hid_freeme)  );
        psz_hid = psz_hid_freeme;

        SG_ERR_CHECK(  x_skip_optional_lf(pCtx, pst)  );

        SG_ERR_IGNORE(  line_dispose(pCtx, &line)  );
    }
	else if (SG_STRLEN(pline->words[2]) == 40u)
	{
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pst->pvh_blob_sha1s, pline->words[2], &psz_hid)  );
	}
    else
    {
        SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED, (pCtx, "%s", SG_string__sz(pline->pstr_line))  );
    }

    // now set the new contents of the file

    SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, psz_hid, &ptn->pszCurrentHid)  );

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
	SG_NULLFREE(pCtx, psz_path_stripped);
    SG_NULLFREE(pCtx, psz_hid_freeme);
}

static void x_commit__filedelete__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* pline
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pline);

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    ;
}

static void x_commit__deleteall__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* pline
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pline);

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    ;
}

static void x_commit__filerename__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* pline
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pline);

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    ;
}

static void x_commit__filecopy__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* pline
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pline);

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    ;
}

static void x_node__delete_if_empty(
	SG_context* pCtx,
	struct node* ptn
	)
{
	SG_rbtree_iterator* pIterator = NULL;
	SG_bool b = SG_FALSE;
	SG_bool b_empty = SG_TRUE;
    struct node* ptnSub = NULL;

    if (!ptn->pCurrentParent)
    {
        return;
    }

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, ptn->prbItems, &b, NULL, (void**) &ptnSub)  );

    b_empty = SG_TRUE;
    while (b)
    {
        if (
                (!(ptnSub->saved_flags & sg_PTNODE_SAVED_FLAG_DELETED))
                && (!(ptnSub->saved_flags & sg_PTNODE_SAVED_FLAG_MOVED_AWAY))
           )
        {
            b_empty = SG_FALSE;
            break;
        }

        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &b, NULL, (void**) &ptnSub)  );
    }

    if (b_empty)
    {
        ptn->saved_flags |= sg_PTNODE_SAVED_FLAG_DELETED;
        SG_ERR_CHECK(  x_node__delete_if_empty(pCtx, ptn->pCurrentParent)  );
    }

fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
}

static void x_node__delete_recursive(
	SG_context* pCtx,
    struct sg_fast_import_state* pst,
	struct node* ptn
	)
{
	SG_rbtree_iterator* pIterator = NULL;
	SG_bool b = SG_FALSE;
	struct node* ptnSub = NULL;

	if (ptn->type == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		SG_ERR_CHECK(  x_node__load_dir_entries(pCtx, pst, ptn)  );
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, ptn->prbItems, &b, NULL, (void**) &ptnSub)  );
		while (b)
		{
			SG_ERR_CHECK(  x_node__delete_recursive(pCtx, pst, ptnSub)  );
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &b, NULL, (void**) &ptnSub)  );
		}
	}

	ptn->saved_flags |= sg_PTNODE_SAVED_FLAG_DELETED;

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
}

static void x_commit__deleteall(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    struct sg_line* pline
	)
{
	static const char* pszRoot[] = { "@", };
	struct node* ptnRoot = NULL;
    struct node* ptnSub = NULL;
	SG_bool b = SG_FALSE;
	SG_rbtree_iterator* pit = NULL;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);
    SG_NULLARGCHECK_RETURN(pline);

	SG_ERR_CHECK(  x_find_existing_directory(pCtx, pst, pc->pSuperRoot, pszRoot, SG_NrElements(pszRoot), 0u, &ptnRoot)  );
	SG_ERR_CHECK(  x_node__load_dir_entries(pCtx, pst, ptnRoot)  );
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, ptnRoot->prbItems, &b, NULL, (void**) &ptnSub)  );
	while (b)
	{
		SG_ERR_CHECK(  x_node__delete_recursive(pCtx, pst, ptnSub)  );
		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, NULL, (void**) &ptnSub)  );
	}
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    ;
}

static void x_commit__filedelete(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    struct sg_line* pline
	)
{
    const char* psz_path = NULL;
	char* psz_path_stripped = NULL;
    struct node* ptn_dir = NULL;
    struct node* ptn = NULL;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);
    SG_NULLARGCHECK_RETURN(pline);

    psz_path = SG_string__sz(pline->pstr_line);
    SG_ASSERT('D' == *psz_path);
    psz_path++;
    SG_ASSERT(' ' == *psz_path);
    psz_path++;

	if (psz_path[0u] == '"')
	{
		SG_ERR_CHECK(  x_strip_quotes(pCtx, psz_path, &psz_path_stripped)  );
		psz_path = psz_path_stripped;
	}

    //fprintf(stderr, "%s\n", SG_string__sz(pline->pstr_line));
    SG_ERR_CHECK(  x_path_to_existing_node__non_directory(pCtx, pst, pc, psz_path, &ptn_dir, &ptn)  );

    ptn->saved_flags |= sg_PTNODE_SAVED_FLAG_DELETED;

    SG_ERR_CHECK(  x_node__delete_if_empty(pCtx, ptn_dir)  );

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
    SG_NULLFREE(pCtx, psz_path_stripped);
}

static void x_commit__filerename(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    struct sg_line* pline
	)
{
    struct node* ptn_dir_from = NULL;
    struct node* ptn_from = NULL;
    struct node* ptn_dir_to = NULL;
    //struct node* ptn_to = NULL;
    const char* p = NULL;
	char* p_stripped = NULL;
    char buf[1024];
	char* buf_stripped = NULL;
	const char* from = NULL;
    char* q = NULL;
	char divider = ' ';
	SG_string* pStrRepoPath = NULL;
	const char** apszParts = NULL;
	SG_uint32 count_parts = 0;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);
    SG_NULLARGCHECK_RETURN(pline);

    //fprintf(stderr, "\nRENAME:%s\n", SG_string__sz(pline->pstr_line));

    p = SG_string__sz(pline->pstr_line);
    SG_ASSERT('R' == *p);
    p++;
    SG_ASSERT(' ' == *p);
    p++;

    q = buf;
    while (
            *p
            && (*p != divider)
          )
    {
		if (q == buf && *p == '"')
		{
			divider = '"';
		}
        *q++ = *p++;
    }
	if (*p == '"')
	{
		*q++ = *p++;
	}
    *q = 0;
    SG_ASSERT(' ' == *p);
    p++;

	if (buf[0u] == '"')
	{
		SG_ERR_CHECK(  x_strip_quotes(pCtx, buf, &buf_stripped)  );
		from = buf_stripped;
	}
	else
	{
		from = buf;
	}

    SG_ERR_CHECK(  x_path_to_existing_node(pCtx, pst, pc, from, &ptn_dir_from, &ptn_from)  );

	if (p[0u] == '"')
	{
		SG_ERR_CHECK(  x_strip_quotes(pCtx, p, &p_stripped)  );
		p = p_stripped;
	}

    SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStrRepoPath, "@/")  );
    SG_ERR_CHECK(  SG_string__append__sz(pCtx, pStrRepoPath, p)  );

	SG_ERR_CHECK(  SG_repopath__split(pCtx, SG_string__sz(pStrRepoPath), pst->pool, &apszParts, &count_parts)  );

	SG_ERR_CHECK(  x_find_directory(pCtx, pst, pc->pSuperRoot, SG_TRUE, apszParts, count_parts - 1, 0, &ptn_dir_to)  );

    if (ptn_dir_from == ptn_dir_to)
    {
		SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, apszParts[count_parts-1], &ptn_from->pszCurrentName)  );
    }
    else
    {
        SG_ERR_CHECK(  x_node__alloc__move(pCtx, ptn_dir_to, apszParts[count_parts-1], ptn_from, pst)  );
    }

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

fail:
	SG_NULLFREE(pCtx, buf_stripped);
	SG_NULLFREE(pCtx, p_stripped);
    SG_STRING_NULLFREE(pCtx, pStrRepoPath);
	SG_NULLFREE(pCtx, apszParts);
}

static void x_commit__filecopy(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    struct sg_line* pline
	)
{
    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);
    SG_NULLARGCHECK_RETURN(pline);

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, pline)  );

    SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );

fail:
    ;
}

static void x_node__is_file_dirty(
	SG_context* pCtx, 
	const struct node* ptn,
	SG_bool* pbDirty
	)
{
	const char* pszhid = NULL;
	
	if (!ptn->pBaselineEntry)
	{
		*pbDirty = SG_TRUE;
		return;
	}

	if (!ptn->pszCurrentHid)
	{
		*pbDirty = SG_FALSE;
		return;
	}
	
	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptn->pBaselineEntry, &pszhid)  );

	if (0 == strcmp(pszhid, ptn->pszCurrentHid))
	{
		*pbDirty = SG_FALSE;
		return;
	}

	*pbDirty = SG_TRUE;
	return;

fail:
	return;
}

static void x_node__check_parent_dirty_non_recursive(
	SG_context* pCtx,
	struct node* ptn,
	SG_bool* pbParentDirty
	)
{
	/* This function never clears the flag, it merely sets it when
	 * needed.  So if it is already set, we're done. */
	if (*pbParentDirty)
	{
		return;
	}

	if (
		(ptn->saved_flags & sg_PTNODE_SAVED_FLAG_DELETED)
		|| (ptn->saved_flags & sg_PTNODE_SAVED_FLAG_MOVED_AWAY)
		|| (ptn->pszgidMovedFrom)
		|| (
			(!ptn->pBaselineEntry)
			&& ptn->pCurrentParent
			)
		)
	{
		*pbParentDirty = SG_TRUE;
		return;
	}

	if (ptn->pszCurrentName)
	{
		const char* pszBaselineName = NULL;

		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, ptn->pBaselineEntry, &pszBaselineName)  );

		if (0 != strcmp(ptn->pszCurrentName, pszBaselineName))
		{
			*pbParentDirty = SG_TRUE;
			return;
		}
	}

    if (ptn->pBaselineEntry)
    {
        SG_int64 iBaselineAttributeBits = 0;

        SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, ptn->pBaselineEntry, &iBaselineAttributeBits)  );
        if (iBaselineAttributeBits != ptn->iCurrentAttributeBits)
        {
            *pbParentDirty = SG_TRUE;
            return;
        }
    }

fail:
	return;
}

static void x_node__commit(
	SG_context* pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
	struct node* ptn,
	const SG_string* pRepoPath,
	SG_bool* pbParentDirty
	)
{
	SG_rbtree_iterator* pIterator = NULL;
	SG_string* pMyRepoPath = NULL;
	const char* pszgid = NULL;
	const char* pszName = NULL;
	SG_bool b;
	SG_treenode* pTreenode = NULL;
	SG_file* pFile = NULL;
	char* pszidHid = NULL;
	SG_treenode_entry* pEntry = NULL;
	SG_string* pstrLink = NULL;
    SG_string* pstr_repo_path = NULL;
    SG_vhash* pvh_user = NULL;

	SG_NULLARGCHECK_RETURN(ptn);
	SG_NULLARGCHECK_RETURN(pst);

	if (
		(ptn->saved_flags & sg_PTNODE_SAVED_FLAG_DELETED)
		|| (ptn->saved_flags & sg_PTNODE_SAVED_FLAG_MOVED_AWAY)
		)
	{
		/* ignore this then */
		*pbParentDirty = SG_TRUE;
		goto done;
	}

	if (SG_TREENODEENTRY_TYPE_DIRECTORY == ptn->type)
	{
		SG_bool bNeedToWriteMyTreenode = SG_FALSE;

		SG_ERR_CHECK(  SG_treenode__alloc(pCtx, &pTreenode)  );
		SG_ERR_CHECK(  SG_treenode__set_version(pCtx, pTreenode, SG_TN_VERSION__CURRENT)  );

		if (ptn->prbItems)
		{
			struct node* ptnSub = NULL;

			SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, ptn->prbItems, &b, &pszgid, (void**) &ptnSub)  );

			while (b)
			{
				SG_ERR_CHECK(  x_node__get_name(pCtx, ptnSub, &pszName)  );

				if (pRepoPath)
				{
					SG_ERR_CHECK(  SG_repopath__combine__sz(pCtx, SG_string__sz(pRepoPath), pszName, (SG_TREENODEENTRY_TYPE_DIRECTORY == ptnSub->type), &pMyRepoPath)  );
				}
				else
				{
					SG_ASSERT('@' == pszName[0]);
					SG_ASSERT(0 == pszName[1]);

					SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pMyRepoPath, "@/")  );
				}

				SG_ERR_CHECK(  x_node__commit(pCtx, pst, pc, ptnSub, pMyRepoPath, &bNeedToWriteMyTreenode)  );

				SG_ERR_CHECK(  x_node__create_entry_for_commit(pCtx, ptnSub)  );

				if (ptnSub->pNewEntry)
				{
					SG_ERR_CHECK(  SG_TREENODE_ENTRY__ALLOC__COPY(pCtx, &pEntry, ptnSub->pNewEntry)  );
					SG_ERR_CHECK(  SG_treenode__add_entry(pCtx, pTreenode, ptnSub->pszgid, &pEntry)  );
				}

				SG_STRING_NULLFREE(pCtx, pMyRepoPath);

				SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIterator, &b, &pszgid, (void**) &ptnSub)  );
			}
			SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
		}

		if (
			(!ptn->pBaselineEntry)
			&& (ptn->pCurrentParent)
			)
		{
			bNeedToWriteMyTreenode = SG_TRUE;
		}

		if (bNeedToWriteMyTreenode)
		{
			SG_ERR_CHECK(  x_add_treenode(pCtx, pst, pc, &pTreenode, &pszidHid)  );
            SG_ERR_CHECK(  SG_strpool__add__sz(pCtx, pst->pool, pszidHid, &ptn->pszCurrentHid)  );
            SG_NULLFREE(pCtx, pszidHid);
            *pbParentDirty = SG_TRUE;
		}

		SG_TREENODE_NULLFREE(pCtx, pTreenode);
	}
	else if ((SG_TREENODEENTRY_TYPE_REGULAR_FILE == ptn->type)
			 || (SG_TREENODEENTRY_TYPE_SYMLINK == ptn->type))
	{
		SG_bool bFileIsDirty = SG_FALSE;

		SG_ERR_CHECK(  x_node__is_file_dirty(pCtx, ptn, &bFileIsDirty)  );

		if (bFileIsDirty)
		{
			*pbParentDirty = SG_TRUE;
		}
    }

	if (!*pbParentDirty)
	{
		SG_ERR_CHECK(  x_node__check_parent_dirty_non_recursive(pCtx, ptn, pbParentDirty)  );
	}

done:
	return;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_user);
	SG_STRING_NULLFREE(pCtx, pstr_repo_path);
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_STRING_NULLFREE(pCtx, pstrLink);
	SG_NULLFREE(pCtx, pszidHid);
	SG_STRING_NULLFREE(pCtx, pMyRepoPath);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIterator);
    SG_TREENODE_ENTRY_NULLFREE(pCtx, pEntry);
	SG_TREENODE_NULLFREE(pCtx, pTreenode);
}

static void x_commit__save_all_treenodes(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    char** ppsz_hid_tree_root
	)
{
    SG_bool b_changed = SG_FALSE;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);
    SG_NULLARGCHECK_RETURN(ppsz_hid_tree_root);

	SG_ERR_CHECK(  x_node__commit(pCtx, pst, pc, pc->pSuperRoot, NULL, &b_changed)  );

    SG_ERR_CHECK(  SG_STRDUP(pCtx, pc->pSuperRoot->pszCurrentHid, ppsz_hid_tree_root)  );

fail:
    x_node__free(pCtx, pc->pSuperRoot);
    pc->pSuperRoot = NULL;
}

static void x_commit__end(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_pending_commit* pc,
    char** ppsz_csid
	)
{
    char* psz_hid_tree_root = NULL;
    SG_dagnode* pdn = NULL;
    SG_string* pstr_branch_name = NULL;
    SG_string* pstr_tag_name = NULL;
    char* psz_trim_comment = NULL;
    SG_audit q;
    const char* psz_userid = NULL;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(pc);

    SG_ERR_CHECK(  x_commit__save_all_treenodes(pCtx, pst, pc, &psz_hid_tree_root)  );

	SG_ERR_CHECK(  SG_changeset__tree__set_root(pCtx, pc->pcs, psz_hid_tree_root)  );

	SG_ERR_CHECK(  SG_changeset__save_to_repo(pCtx,
													 pc->pcs,
													 pst->pRepo,pst->ptx)  );
	SG_ERR_CHECK(  SG_changeset__create_dagnode(pCtx,
												pc->pcs,
												&pdn)  );
    SG_ERR_CHECK(  SG_dagnode__get_id(pCtx, pdn, ppsz_csid)  );

	// step 2 is to use the DAGNODE to update the on-disk DAG using a
	// DAG TRANSACTION.

	SG_repo__store_dagnode(pCtx,
						   pst->pRepo,
						   pst->ptx,
						   SG_DAGNUM__VERSION_CONTROL,
						   pdn);
    pdn = NULL;

	if (SG_context__err_equals(pCtx,SG_ERR_DAGNODE_ALREADY_EXISTS))
    {
		SG_context__err_reset(pCtx);
    }
	else
    {
		SG_ERR_CHECK_CURRENT;
    }

    SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pst->pvh_emails, pc->buf_email, &psz_userid)  );
    SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pst->pRepo, pc->when, psz_userid)  );
    if (!pc->when)
    {
        pc->when = 1; // TODO hack
    }
    SG_ERR_CHECK(  SG_repo__store_audit(pCtx, pst->pRepo, pst->ptx, *ppsz_csid, SG_DAGNUM__VERSION_CONTROL, q.who_szUserId, q.when_int64)  );

	SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pst->pRepo, &pst->ptx)  );
    pst->ptx = NULL;

    pst->count_done++;

    if (pc->psz_comment && *(pc->psz_comment))
    {
        SG_uint32 len = 0;
        SG_ERR_CHECK(  SG_sz__trim(pCtx, pc->psz_comment, &len, &psz_trim_comment)  );
        if (len)
        {
            SG_ERR_CHECK(  SG_utf8__fix__sz(pCtx, (unsigned char*) psz_trim_comment)  );
            SG_ERR_CHECK(  SG_vc_comments__add(pCtx, pst->pRepo, *ppsz_csid, psz_trim_comment, &q)  );
        }
        SG_NULLFREE(pCtx, psz_trim_comment);
    }
    if (pc->branch[0])
    {
		SG_bool b_closed = SG_FALSE;

        SG_ERR_CHECK(  SG_validate__sanitize__trim(
                    pCtx, 
                    pc->branch, 
                    1,
                    256, 
                    SG_VALIDATE__BANNED_CHARS,
                    SG_VALIDATE__RESULT__ALL, 
                    "_", 
                    "_", 
                    &pstr_branch_name
                    )  );
        SG_ERR_CHECK(  SG_vc_branches__add_head(pCtx, pst->pRepo, SG_string__sz(pstr_branch_name), *ppsz_csid, NULL, SG_FALSE, &q)  );

		// if the branch is closed, re-open it
		SG_ERR_CHECK(  is_branch_closed(pCtx, pst->pRepo, SG_string__sz(pstr_branch_name), &b_closed)  );
		if (b_closed)
		{
			SG_ERR_CHECK(  SG_vc_branches__reopen(pCtx, pst->pRepo, SG_string__sz(pstr_branch_name), &q)  );
		}

        SG_STRING_NULLFREE(pCtx, pstr_branch_name);
    }
    if (pc->tag[0])
    {
        SG_ERR_CHECK(  SG_validate__sanitize__trim(
                    pCtx, 
                    pc->tag, 
                    1,
                    256, 
                    SG_VALIDATE__BANNED_CHARS,
                    SG_VALIDATE__RESULT__ALL, 
                    "_", 
                    "_", 
                    &pstr_tag_name
                    )  );
        SG_ERR_CHECK(  SG_vc_tags__add(pCtx, pst->pRepo, *ppsz_csid, SG_string__sz(pstr_tag_name), &q, SG_TRUE)  );
        SG_STRING_NULLFREE(pCtx, pstr_tag_name);
    }

    SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &pc->time_end)  );

    SG_NULLFREE(pCtx, pc->psz_baseline);
    SG_NULLFREE(pCtx, pc->psz_comment);

    SG_ERR_CHECK(  SG_log__set_finished(pCtx, pst->count_done)  );

fail:
    SG_VARRAY_NULLFREE(pCtx, pc->pva_parents);
    SG_NULLFREE(pCtx, psz_hid_tree_root);
	SG_CHANGESET_NULLFREE(pCtx, pc->pcs);
    SG_STRING_NULLFREE(pCtx, pstr_branch_name);
    SG_NULLFREE(pCtx, psz_trim_comment);
}

static void x_do__commit__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_uint32 idnum = 0;
    struct sg_line line;
    char buf_email[256];
    SG_int64 when = -1;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);

    memset(&line, 0, sizeof(struct sg_line));
    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );
    SG_ERR_CHECK(  read_optional_mark(pCtx, pst, &line, &idnum)  );

    if (0 == strcmp(line.words[0], "author"))
    {
        SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );
    }

    if (0 != strcmp(line.words[0], "committer"))
    {
        SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
    }

    SG_ERR_CHECK(  x_commit__committer__pass1(pCtx, pst, &line, buf_email, &when)  );

    SG_ERR_TRY(  SG_vhash__add__string__sz(pCtx, pst->pvh_emails, buf_email, "")  );
	SG_ERR_CATCH_IGNORE(SG_ERR_VHASH_DUPLICATEKEY);
	SG_ERR_CATCH_END;

    if (0 != strcmp(line.words[0], "data"))
    {
        SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
    }

    SG_ERR_CHECK(  x_commit__comment__pass1(pCtx, pst, &line)  );
    
    if (
            (line.count_words > 1)
            && (0 == strcmp(line.words[0], "from"))
            )
    {
        SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );
    }

    while (
            (line.count_words > 1)
            && (0 == strcmp(line.words[0], "merge"))
       )
    {
        SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );
    }

    while (1)
    {
        if (0 == line.count_words)
        {
            SG_ERR_CHECK(  line_unread(pCtx, pst->pfi, &line)  );
            break;
        }

        if (0 == strcmp(line.words[0], "M"))
        {
            SG_ERR_CHECK(  x_commit__filemodify__pass1(pCtx, pst, &line)  );
        }
        else if (0 == strcmp(line.words[0], "D"))
        {
            SG_ERR_CHECK(  x_commit__filedelete__pass1(pCtx, pst, &line)  );
        }
        else if (0 == strcmp(line.words[0], "C"))
        {
            SG_ERR_CHECK(  x_commit__filecopy__pass1(pCtx, pst, &line)  );
        }
        else if (0 == strcmp(line.words[0], "R"))
        {
            SG_ERR_CHECK(  x_commit__filerename__pass1(pCtx, pst, &line)  );
        }
        else if (0 == strcmp(line.words[0], "deleteall"))
        {
            SG_ERR_CHECK(  x_commit__deleteall__pass1(pCtx, pst, &line)  );
        }
        else
        {
            SG_ERR_CHECK(  line_unread(pCtx, pst->pfi, &line)  );
            break;
        }
    }

    /*
    
        'committer' (SP <name>)? SP LT <email> GT SP <when> LF
        data
        ('from' SP <committish> LF)?
        ('merge' SP <committish> LF)?
        (filemodify | filedelete | filecopy | filerename | filedeleteall | notemodify)*
        LF?

    */

    SG_ERR_CHECK(  x_skip_optional_lf(pCtx, pst)  );

    pst->count_commits++;

fail:
    ;
}

#define REFS_HEADS "refs/heads/"
#define REFS_TAGS "refs/tags/"

static void x_do__commit(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_uint32 idnum = 0;
    struct sg_line line;
    struct sg_pending_commit commit;
    char* psz_csid = NULL;
    const char* psz_ref = NULL;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);

    memset(&line, 0, sizeof(struct sg_line));
    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );
    SG_ERR_CHECK(  read_optional_mark(pCtx, pst, &line, &idnum)  );

    x_commit__begin(pCtx, pst, &commit);

    psz_ref = SG_string__sz(p_line_cmd->pstr_line);
    while (' ' != *psz_ref)
    {
        psz_ref++;
    }
    psz_ref++;

    if (0 == strncmp(psz_ref, REFS_HEADS, strlen(REFS_HEADS)))
    {
        SG_ERR_CHECK(  SG_strcpy(pCtx, commit.branch, sizeof(commit.branch), psz_ref+strlen(REFS_HEADS))  );
        commit.tag[0] = 0;
    }
    else if (0 == strncmp(psz_ref, REFS_TAGS, strlen(REFS_TAGS)))
    {
#if 0
        SG_ERR_CHECK(  SG_strcpy(pCtx, commit.branch, sizeof(commit.branch), "master")  );
#endif

        SG_ERR_CHECK(  SG_strcpy(pCtx, commit.tag, sizeof(commit.branch), psz_ref+strlen(REFS_TAGS))  );
    }
    else
    {
        commit.branch[0] = 0;
        commit.tag[0] = 0;
    }

    if (0 == strcmp(line.words[0], "author"))
    {
        SG_ERR_CHECK(  x_commit__author(pCtx, pst, &commit, &line)  );
    }

    if (0 != strcmp(line.words[0], "committer"))
    {
        SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
    }

    SG_ERR_CHECK(  x_commit__committer(pCtx, pst, &commit, &line)  );

    if (0 != strcmp(line.words[0], "data"))
    {
        SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
    }

    SG_ERR_CHECK(  x_commit__comment(pCtx, pst, &commit, &line)  );
    
    if (
            (line.count_words > 1)
            && (0 == strcmp(line.words[0], "from"))
            )
    {
        SG_ERR_CHECK(  x_commit__from(pCtx, pst, &commit, &line)  );
    }

    while (
            (line.count_words > 1)
            && (0 == strcmp(line.words[0], "merge"))
       )
    {
        SG_ERR_CHECK(  x_commit__merge(pCtx, pst, &commit, &line)  );
    }

    x_commit__setup_parents(pCtx, pst, &commit);

    while (1)
    {
        if (0 == line.count_words)
        {
            SG_ERR_CHECK(  line_unread(pCtx, pst->pfi, &line)  );
            break;
        }

        if (0 == strcmp(line.words[0], "M"))
        {
            SG_ERR_CHECK(  x_commit__filemodify(pCtx, pst, &commit, &line)  );
        }
        else if (0 == strcmp(line.words[0], "D"))
        {
            SG_ERR_CHECK(  x_commit__filedelete(pCtx, pst, &commit, &line)  );
        }
        else if (0 == strcmp(line.words[0], "C"))
        {
            SG_ERR_CHECK(  x_commit__filecopy(pCtx, pst, &commit, &line)  );
        }
        else if (0 == strcmp(line.words[0], "R"))
        {
            SG_ERR_CHECK(  x_commit__filerename(pCtx, pst, &commit, &line)  );
        }
        else if (0 == strcmp(line.words[0], "deleteall"))
        {
            SG_ERR_CHECK(  x_commit__deleteall(pCtx, pst, &commit, &line)  );
        }
        else
        {
            SG_ERR_CHECK(  line_unread(pCtx, pst->pfi, &line)  );
            break;
        }
    }

    /*
    
        'committer' (SP <name>)? SP LT <email> GT SP <when> LF
        data
        ('from' SP <committish> LF)?
        ('merge' SP <committish> LF)?
        (filemodify | filedelete | filecopy | filerename | filedeleteall | notemodify)*
        LF?

    */

    SG_ERR_CHECK(  x_skip_optional_lf(pCtx, pst)  );

    x_commit__end(pCtx, pst, &commit, &psz_csid);

    if (idnum)
    {
        char buf[32];

        SG_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), ":%d", idnum)  );

        // store the mark
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pst->pvh_commit_marks, buf, psz_csid)  );
    }


fail:
    SG_NULLFREE(pCtx, psz_csid);
}

static void x_do__tag__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    struct sg_line line;
    char buf_email[256];
    SG_int64 when;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);

    memset(&line, 0, sizeof(struct sg_line));
    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );
    // now line is 'from'

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );
    // now line is 'tagger'
    {
        const char* psz_when = NULL;
        char* q = NULL;

        psz_when = SG_string__sz(line.pstr_line);
        while ('<' != *psz_when)
        {
            psz_when++;
        }
        psz_when++;

        q = buf_email;
        while ('>' != *psz_when)
        {
            *q++ = *psz_when++;
        }
        *q = 0;
        psz_when++;
        psz_when++;

        SG_ERR_CHECK(  x_commit__when(pCtx, psz_when, &when)  );
    }

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );
    // now line is 'data'

    {
        SG_uint64 len = 0;
        SG_uint64 pos = 0;

        SG_ERR_CHECK(SG_uint64__parse__strict(pCtx, &len, line.words[1]));

        // TODO what should we do with comments on tags?
        SG_ERR_CHECK(  SG_file__tell(pCtx, pst->pfi, &pos)  );
        SG_ERR_CHECK(  SG_file__seek(pCtx, pst->pfi, pos + len)  );
    }

    SG_ERR_CHECK(  x_skip_optional_lf(pCtx, pst)  );

    SG_ERR_TRY(  SG_vhash__add__string__sz(pCtx, pst->pvh_emails, buf_email, "")  );
	SG_ERR_CATCH_IGNORE(SG_ERR_VHASH_DUPLICATEKEY);
	SG_ERR_CATCH_END;

fail:
    SG_ERR_IGNORE(  line_dispose(pCtx, &line)  );
}

static void x_do__reset__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    SG_uint64 initial_pos = 0;
    SG_string* pstr_line = NULL;
	char** words = NULL;
    SG_uint32 count_words = 0;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);

    SG_ERR_CHECK(  SG_file__tell(pCtx, pst->pfi, &initial_pos)  );
    SG_ERR_CHECK(  SG_file__read__line__LF(pCtx, pst->pfi, &pstr_line)  );
    if (!pstr_line)
    {
        goto done;
    }

    if (0 == SG_STRLEN(SG_string__sz(pstr_line)))
    {
        // just a LF
        goto done;
    }

	SG_ERR_CHECK(  SG_string__split__asciichar(pCtx, pstr_line, ' ', 32, &words, &count_words) );
    if (0 == strcmp(words[0], "from"))
    {
    }
    else
    {
        SG_ERR_CHECK(  SG_file__seek(pCtx, pst->pfi, initial_pos)  );
    }

    SG_ERR_CHECK(  x_skip_optional_lf(pCtx, pst)  );

done:
fail:
    SG_STRING_NULLFREE(pCtx, pstr_line);
	SG_STRINGLIST_NULLFREE(pCtx, words, count_words);
}

static void x_do__tag(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    struct sg_line line;
    const char* psz_tag = NULL;
    const char* psz_csid = NULL;
    char buf_email[256];
    SG_int64 when = 0;
	SG_string* pstr_tag_name = NULL;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);

    psz_tag = SG_string__sz(p_line_cmd->pstr_line);
    while (*psz_tag && (' ' != *psz_tag))
    {
        psz_tag++;
    }
    psz_tag++;
    // psz_tag now points to the name of the tag

    memset(&line, 0, sizeof(struct sg_line));
    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );
    // now line is 'from'
    if (':' == line.words[1][0])
    {
		SG_bool b_found = SG_FALSE;

		// check for a commit with this mark
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pst->pvh_commit_marks, line.words[1], &b_found)  );
		if (b_found)
		{
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pst->pvh_commit_marks, line.words[1], &psz_csid)  );
		}
		else
		{
			// Git tags can also reference arbitrary blobs, so check our blob marks
			// this merely determines whether or not the import file is malformed
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pst->pvh_blob_marks, line.words[1], &b_found)  );
			if (b_found == SG_FALSE)
			{
				SG_ERR_THROW2(  SG_ERR_MALFORMED_GIT_FAST_IMPORT, (pCtx, "mark not found: %s", line.words[1])  );
			}
			// if we found the blob mark, then the file is fine
			// we'll skip this tag though, because our tags only reference changesets
			// we leave psz_csid as NULL to indicate this
		}
    }
    else
    {
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );
    // now line is 'tagger'
    {
        const char* psz_when = NULL;
        char* q = NULL;

        psz_when = SG_string__sz(line.pstr_line);
        while ('<' != *psz_when)
        {
            psz_when++;
        }
        psz_when++;

        q = buf_email;
        while ('>' != *psz_when)
        {
            *q++ = *psz_when++;
        }
        *q = 0;
        psz_when++;
        psz_when++;

        SG_ERR_CHECK(  x_commit__when(pCtx, psz_when, &when)  );
    }

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );
    // now line is 'data'

    {
        SG_uint64 len = 0;
        SG_uint64 pos = 0;

        SG_ERR_CHECK(SG_uint64__parse__strict(pCtx, &len, line.words[1]));

        // TODO what should we do with comments on tags?
        SG_ERR_CHECK(  SG_file__tell(pCtx, pst->pfi, &pos)  );
        SG_ERR_CHECK(  SG_file__seek(pCtx, pst->pfi, pos + len)  );
    }

    SG_ERR_CHECK(  x_skip_optional_lf(pCtx, pst)  );

    // add the tag
    if (psz_csid != NULL)
    {
        SG_audit q;
        const char* psz_userid = NULL;

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pst->pvh_emails, buf_email, &psz_userid)  );
        if (!when)
        {
            when = 1; // TODO hack
        }

		// if we're in the middle of a transaction, commit it
		// because adding the tag will want to start and commit its own transaction
		if (pst->ptx != NULL)
		{
			SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pst->pRepo, &pst->ptx)  );
			pst->ptx = NULL;
		}

        SG_ERR_CHECK(  SG_validate__sanitize__trim(
                    pCtx, 
                    psz_tag, 
                    1,
                    256, 
                    SG_VALIDATE__BANNED_CHARS,
                    SG_VALIDATE__RESULT__ALL, 
                    "_", 
                    "_", 
                    &pstr_tag_name
                    )  );

        SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pst->pRepo, when, psz_userid)  );
        SG_ERR_CHECK(  SG_vc_tags__add(pCtx, pst->pRepo, psz_csid, SG_string__sz(pstr_tag_name), &q, SG_TRUE)  );
    }

fail:
    SG_ERR_IGNORE(  line_dispose(pCtx, &line)  );
	SG_STRING_NULLFREE(pCtx, pstr_tag_name);
}

static void x_do__reset(
    SG_context * pCtx,
    struct sg_fast_import_state* pst,
    struct sg_line* p_line_cmd
	)
{
    struct sg_line line;
    const char* psz_ref = NULL;
    char buf[256];
    SG_audit q;
    SG_string* pstr_branch = NULL;
    SG_string* pstr_tag = NULL;

    SG_NULLARGCHECK_RETURN(pst);
    SG_NULLARGCHECK_RETURN(p_line_cmd);

    memset(&line, 0, sizeof(struct sg_line));

    buf[0] = 0;

    psz_ref = SG_string__sz(p_line_cmd->pstr_line);
    while (' ' != *psz_ref)
    {
        psz_ref++;
    }
    psz_ref++;

    if (0 == strncmp(psz_ref, REFS_HEADS, strlen(REFS_HEADS)))
    {
        SG_ERR_CHECK(  SG_strcpy(pCtx, buf, sizeof(buf), psz_ref+strlen(REFS_HEADS))  );
        SG_ERR_CHECK(  SG_validate__sanitize__trim(
                    pCtx, 
                    buf, 
                    1,
                    256, 
                    SG_VALIDATE__BANNED_CHARS,
                    SG_VALIDATE__RESULT__ALL, 
                    "_", 
                    "_", 
                    &pstr_branch
                    )  );
    }
    else if (0 == strncmp(psz_ref, REFS_TAGS, strlen(REFS_TAGS)))
    {
        SG_ERR_CHECK(  SG_strcpy(pCtx, buf, sizeof(buf), psz_ref+strlen(REFS_TAGS))  );
        SG_ERR_CHECK(  SG_validate__sanitize__trim(
                    pCtx, 
                    buf, 
                    1,
                    256, 
                    SG_VALIDATE__BANNED_CHARS,
                    SG_VALIDATE__RESULT__ALL, 
                    "_", 
                    "_", 
                    &pstr_tag
                    )  );
    }
    else
    {
        SG_ERR_CHECK(  SG_strcpy(pCtx, buf, sizeof(buf), psz_ref)  );
        SG_ERR_CHECK(  SG_validate__sanitize__trim(
                    pCtx, 
                    buf, 
                    1,
                    256, 
                    SG_VALIDATE__BANNED_CHARS,
                    SG_VALIDATE__RESULT__ALL, 
                    "_", 
                    "_", 
                    &pstr_tag
                    )  );
    }

    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );

    SG_ERR_CHECK(  SG_audit__init(pCtx, &q, pst->pRepo, SG_AUDIT__WHEN__NOW, SG_NOBODY__USERID)  );

	// if we're in the middle of a transaction, commit it
	// because acting on tags/branches will want to start and commit its own transaction
	if (pst->ptx != NULL)
	{
		SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, pst->pRepo, &pst->ptx)  );
		pst->ptx = NULL;
	}

    if (
            (line.count_words > 1)
            && (0 == strcmp(line.words[0], "from"))
            )
    {
        if (':' == line.words[1][0])
        {
            const char* psz_csid = NULL;

            SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pst->pvh_commit_marks, line.words[1], &psz_csid)  );

            if (pstr_branch)
            {
				SG_bool b_closed = SG_FALSE;

                SG_ERR_CHECK(  SG_vc_branches__add_head(pCtx, pst->pRepo, SG_string__sz(pstr_branch), psz_csid, NULL, SG_FALSE, &q)  );

				// if the branch is closed, re-open it
				SG_ERR_CHECK(  is_branch_closed(pCtx, pst->pRepo, SG_string__sz(pstr_branch), &b_closed)  );
				if (b_closed)
				{
					SG_ERR_CHECK(  SG_vc_branches__reopen(pCtx, pst->pRepo, SG_string__sz(pstr_branch), &q)  );
				}
            }
            else
            {
                SG_ERR_CHECK(  SG_vc_tags__add(pCtx, pst->pRepo, psz_csid, SG_string__sz(pstr_tag), &q, SG_TRUE)  );
            }
        }
        else
        {
            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
        }
    }
    else
    {
        SG_ERR_CHECK(  line_unread(pCtx, pst->pfi, &line)  );

        if (pstr_branch)
        {
			SG_bool b_exists = SG_FALSE;

			// if the branch exists and is open, close it
			SG_ERR_CHECK(  SG_vc_branches__exists(pCtx, pst->pRepo, SG_string__sz(pstr_branch), &b_exists)  );
			if (b_exists == SG_TRUE)
			{
				SG_bool b_closed = SG_FALSE;

				SG_ERR_CHECK(  is_branch_closed(pCtx, pst->pRepo, SG_string__sz(pstr_branch), &b_closed)  );
				if (b_closed == SG_FALSE)
				{
					SG_ERR_CHECK(  SG_vc_branches__close(pCtx, pst->pRepo, SG_string__sz(pstr_branch), &q)  );
				}
			}
        }
        else
        {
            // TODO remove a tag?
        }
    }

    SG_ERR_CHECK(  x_skip_optional_lf(pCtx, pst)  );

fail:
    SG_STRING_NULLFREE(pCtx, pstr_branch);
    SG_STRING_NULLFREE(pCtx, pstr_tag);
    SG_ERR_IGNORE(  line_dispose(pCtx, &line)  );
}

static void x_add_existing_users(
	SG_context* pCtx,
    SG_repo* pRepo,
	SG_bool bNobody,
	SG_vhash* pResult
	)
{
	SG_varray* pUsers = NULL;
	SG_uint32  uUsers = 0u;
	SG_uint32  uUser  = 0u;

	SG_NULLARGCHECK(pRepo);
	SG_NULLARGCHECK(pResult);

	SG_ERR_CHECK(  SG_user__list_all(pCtx, pRepo, &pUsers)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pUsers, &uUsers)  );
	for (uUser = 0u; uUser < uUsers; ++uUser)
	{
		SG_vhash*   pUser      = NULL;
		const char* szUsername = NULL;
		const char* szRecId    = NULL;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pUsers, uUser, &pUser)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pUser, "name", &szUsername)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pUser, "recid", &szRecId)  );

		if (bNobody != SG_FALSE || strcmp(SG_NOBODY__USERID, szRecId) != 0)
		{
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pResult, szUsername, szRecId)  );
		}
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pUsers);
	return;
}

static void x_create_user_accounts(
    SG_context * pCtx,
    struct sg_fast_import_state* pst
	)
{
    char* psz_userid = NULL;
    char* psz_username_sanitized = NULL;
	char* psz_username_unique = NULL;
	SG_vhash* pvh_sanitized_usernames = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_sanitized_usernames)  );
	SG_ERR_CHECK(  x_add_existing_users(pCtx, pst->pRepo, SG_TRUE, pvh_sanitized_usernames)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pst->pvh_emails, &count)  );
    for (i=0; i<count; i++)
    {
        const char* psz_username = NULL;
		const char* psz_existing_userid = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__sz(pCtx, pst->pvh_emails, i, &psz_username, &psz_existing_userid)  );
		if (!*psz_existing_userid)
		{
			SG_ERR_CHECK(  SG_user__sanitize_username(pCtx, psz_username, &psz_username_sanitized)  );
			if (strcmp(psz_username, psz_username_sanitized) != 0 || strcmp(psz_username_sanitized, SG_NOBODY__USERNAME) == 0)
			{
				SG_ERR_CHECK(  SG_user__uniqify_username(pCtx, psz_username_sanitized, pvh_sanitized_usernames, &psz_username_unique)  );
				SG_NULLFREE(pCtx, psz_username_sanitized);
				psz_username_sanitized = psz_username_unique;
				psz_username_unique = NULL;
			}
			SG_ERR_CHECK(  SG_user__create(pCtx, pst->pRepo, psz_username_sanitized, &psz_userid)  );
			SG_ERR_CHECK(  SG_vhash__add__null(pCtx, pvh_sanitized_usernames, psz_username_sanitized)  );
			SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pst->pvh_emails, psz_username, psz_userid)  );
			SG_NULLFREE(pCtx, psz_userid);
			SG_NULLFREE(pCtx, psz_username_sanitized);
		}
    }

fail:
	SG_NULLFREE(pCtx, psz_username_unique);
    SG_NULLFREE(pCtx, psz_username_sanitized);
    SG_NULLFREE(pCtx, psz_userid);
	SG_VHASH_NULLFREE(pCtx, pvh_sanitized_usernames);
}

static void x_do_one_command(
    SG_context * pCtx,
    struct sg_fast_import_state* pst
	)
{
    struct sg_line line;

    memset(&line, 0, sizeof(struct sg_line));
    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );

    if (line.b_eof)
    {
        pst->b_done = SG_TRUE;
        goto done;
    }

    if (0 == strcmp(line.words[0], "commit"))
    {
        SG_ERR_CHECK(  x_do__commit(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "tag"))
    {
        SG_ERR_CHECK(  x_do__tag(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "reset"))
    {
        SG_ERR_CHECK(  x_do__reset(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "blob"))
    {
        if (line.count_words != 1)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(  x_do__blob(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "checkpoint"))
    {
        if (line.count_words != 1)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(  x_do__checkpoint(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "progress"))
    {
        SG_ERR_CHECK(  x_do__progress(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "cat-blob"))
    {
        if (line.count_words != 2)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(  x_do__cat_blob(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "ls"))
    {
        if (line.count_words != 2)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(  x_do__ls(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "feature"))
    {
        if (line.count_words != 2)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(  x_do__feature(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "option"))
    {
        if (line.count_words != 2)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(  x_do__option(pCtx, pst, &line)  );
    }
    else
    {
        SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
                (pCtx, "%s", SG_string__sz(line.pstr_line))
                );
    }

done:
fail:
    SG_ERR_IGNORE(  line_dispose(pCtx, &line)  );
}

void x_do_one_command__pass1(
    SG_context * pCtx,
    struct sg_fast_import_state* pst
	)
{
    struct sg_line line;

    memset(&line, 0, sizeof(struct sg_line));
    SG_ERR_CHECK(  line_read(pCtx, pst->pfi, &line)  );

    if (line.b_eof)
    {
        pst->b_done = SG_TRUE;
        goto done;
    }

    if (0 == strcmp(line.words[0], "commit"))
    {
        SG_ERR_CHECK(  x_do__commit__pass1(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "tag"))
    {
        SG_ERR_CHECK(  x_do__tag__pass1(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "reset"))
    {
        SG_ERR_CHECK(  x_do__reset__pass1(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "blob"))
    {
        if (line.count_words != 1)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(  x_do__blob__pass1(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "checkpoint"))
    {
        if (line.count_words != 1)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(  x_do__checkpoint__pass1(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "progress"))
    {
        SG_ERR_CHECK(  x_do__progress__pass1(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "cat-blob"))
    {
        if (line.count_words != 2)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(  x_do__cat_blob__pass1(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "ls"))
    {
        if (line.count_words != 2)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(  x_do__ls__pass1(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "feature"))
    {
        if (line.count_words != 2)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(  x_do__feature__pass1(pCtx, pst, &line)  );
    }
    else if (0 == strcmp(line.words[0], "option"))
    {
        if (line.count_words != 2)
        {
            SG_ERR_THROW(  SG_ERR_MALFORMED_GIT_FAST_IMPORT  );
        }
        SG_ERR_CHECK(  x_do__option__pass1(pCtx, pst, &line)  );
    }
    else
    {
        SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
                (pCtx, "%s", SG_string__sz(line.pstr_line))
                );
    }

done:
fail:
    SG_ERR_IGNORE(  line_dispose(pCtx, &line)  );
}

void SG_fast_import__import(
    SG_context * pCtx,
	const char * psz_fi,
	const char * psz_repo,
	const char * psz_storage,
	const char * psz_hash,
	const char * psz_shared_users
	)
{
    SG_pathname* pPath_fi = NULL;
    struct sg_fast_import_state st;
    SG_bool b_success = SG_FALSE;
    SG_bool b_created = SG_FALSE;
    SG_uint32 count_pop = 0;

    memset(&st, 0, sizeof(st));

    SG_NULLARGCHECK_RETURN(psz_repo);
    SG_NULLARGCHECK_RETURN(psz_fi);

	SG_ERR_CHECK(  SG_vv2__init_new_repo(pCtx,
											  psz_repo,
											  NULL,
											  psz_storage,
											  psz_hash,
											  SG_TRUE,
                                              psz_shared_users,
											  SG_FALSE,
											  NULL,
											  &st.psz_initial_csid)  );
    b_created = SG_TRUE;
    SG_ERR_CHECK(  SG_closet__descriptors__set_status(pCtx, psz_repo, SG_REPO_STATUS__CLONING)  );

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &st.pvh_branch_state)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &st.pvh_blob_marks)  );
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &st.pvh_blob_sha1s)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &st.pvh_commit_marks)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &st.pvh_emails)  );
	SG_ERR_CHECK(  SG_STRPOOL__ALLOC(pCtx, &st.pool, 32768)  );

    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath_fi, psz_fi)  );
    SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_fi, SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &st.pfi)  );

    SG_ERR_CHECK(  SG_alloc(pCtx, SG_STREAMING_BUFFER_SIZE, 1, &st.buf)  );
    st.buflen = SG_STREAMING_BUFFER_SIZE;

	SG_ERR_CHECK(  SG_repo__open_unavailable_repo_instance(pCtx, psz_repo, NULL, NULL, NULL, &st.pRepo)  );

	if (psz_shared_users != NULL)
	{
		SG_ERR_CHECK(  x_add_existing_users(pCtx, st.pRepo, SG_FALSE, st.pvh_emails)  );
	}

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Importing", SG_LOG__FLAG__NONE)  );
    count_pop++;
    SG_ERR_CHECK(  SG_log__set_steps(pCtx, 5, NULL)  );

    SG_ERR_CHECK(  SG_log__set_step(pCtx, "Initial scan")  );
    // pass 1
    while (!st.b_done)
    {
        SG_ERR_CHECK(  x_do_one_command__pass1(pCtx, &st)  );
    }
    SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

    SG_ERR_CHECK(  SG_log__set_step(pCtx, "Creating user accounts")  );
    SG_ERR_CHECK(  x_create_user_accounts(pCtx, &st)  );
    SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

    SG_ERR_CHECK(  SG_log__set_step(pCtx, "Storing")  );
    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Storing", SG_LOG__FLAG__NONE)  );
    count_pop++;
    SG_ERR_CHECK(  SG_log__set_steps(pCtx, st.count_commits, NULL)  );

    // pass 2
    SG_ERR_CHECK(  SG_file__seek(pCtx, st.pfi, 0)  );
    st.b_done = SG_FALSE;
    while (!st.b_done)
    {
        SG_ERR_CHECK(  x_do_one_command(pCtx, &st)  );
    }
    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    count_pop--;

    SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

	SG_ERR_CHECK(  SG_log__set_step(pCtx, "Cleaning branches")  );
	SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, st.pRepo, NULL)  );
	SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

    SG_ERR_CHECK(  SG_log__set_step(pCtx, "Committing")  );
    if (st.ptx)
    {
        SG_ERR_CHECK(  SG_repo__commit_tx(pCtx, st.pRepo, &st.ptx)  );
    }
    SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    count_pop--;

    SG_ERR_CHECK(  SG_closet__descriptors__set_status(pCtx, psz_repo, SG_REPO_STATUS__NORMAL)  );
    b_success = SG_TRUE;

fail:
    while (count_pop)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
        count_pop--;
    }
    SG_REPO_NULLFREE(pCtx, st.pRepo);
    if (!b_success && b_created)
    {
		SG_ERR_IGNORE(  SG_repo__delete_repo_instance(pCtx, psz_repo)  );
		SG_ERR_IGNORE(  SG_closet__descriptors__remove(pCtx, psz_repo)  );
    }
	SG_STRPOOL_NULLFREE(pCtx, st.pool);
    SG_NULLFREE(pCtx, st.psz_initial_csid);
    SG_VHASH_NULLFREE(pCtx, st.pvh_branch_state);
    SG_VHASH_NULLFREE(pCtx, st.pvh_blob_marks);
	SG_VHASH_NULLFREE(pCtx, st.pvh_blob_sha1s);
    SG_VHASH_NULLFREE(pCtx, st.pvh_commit_marks);
    SG_VHASH_NULLFREE(pCtx, st.pvh_emails);
    SG_NULLFREE(pCtx, st.buf);
	SG_FILE_NULLCLOSE(pCtx, st.pfi);
    SG_PATHNAME_NULLFREE(pCtx, pPath_fi);
}

/*

   vc_branches__cleanup after it's all done

   separate audit for the author line?

   tags

   reset

   do the tree, the audit, the comment, and the branch all in the same tx?

   why are some treendx updates REALLY slow?  treendx is keeping too
   much stuff in it.

   we know all the objectids up front.  we could vcdiff/pack
   as we store the blobs.

   repo could cache the last changeset added (for each dagnum?)
   so it doesn't need to re-load it for indexing, in the common
   cases.

   keep track of all the refs.  update them all at the end, or on
   a checkpoint.

   need to allow the import to be stopped and restarted.  incremental.
   save the marks?

   on clone/pack, delete the fragball as early as possible.

   tag/branch/user names should be uniqified as well as sanitized
   need to be careful they don't get uniqified into a name that is added later

*/

