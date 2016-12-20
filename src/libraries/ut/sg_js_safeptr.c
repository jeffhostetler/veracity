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

#include "sg_js_safeptr__private.h"

struct _SG_safeptr
{
    const char* psz_type;
    void* ptr;
};


//////////////////////////////////////////////////////////////////

#define MY_WRAP_RETURN(pCtx, pPtr, safe_type, ppNewSafePtr)       \
	SG_STATEMENT(                                                 \
		SG_safeptr * psp = NULL;                                  \
		SG_NULLARGCHECK_RETURN(  (pPtr)  );                       \
		SG_NULLARGCHECK_RETURN(  (ppNewSafePtr)  );               \
		SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, psp)  );            \
		psp->psz_type = (safe_type);                              \
		psp->ptr = (pPtr);                                        \
		*(ppNewSafePtr) = psp;                                    )

// Review: Jeff says: do we want to keep SG_ERR_SAFEPTR_NULL as the
//                    return value for a null argument or do we want
//                    use the standard SG_ERR_INVALIDARG ?

#define MY_UNWRAP_RETURN(pCtx, psp, safe_type, ppResult, c_type) \
	SG_STATEMENT(                                                \
		if (!(psp))                                              \
			SG_ERR_THROW_RETURN( SG_ERR_SAFEPTR_NULL );          \
		if (0 != strcmp((psp)->psz_type,(safe_type)))            \
			SG_ERR_THROW_RETURN( SG_ERR_SAFEPTR_WRONG_TYPE );    \
		SG_NULLARGCHECK_RETURN(  (ppResult)  );                  \
		*(ppResult) = (c_type)((psp)->ptr);                      )

//////////////////////////////////////////////////////////////////

/**
 * NOTE 2011/10/03 I added these __generic versions so that
 * NOTE            I could move the public declarations and
 * NOTE            implementations of the wrapper functions
 * NOTE            for specific types into the associated
 * NOTE            library -- in anticipation of us splitting
 * NOTE            up sglib.  For example, see wc/wc9js.
 *
 * TODO 2011/10/03 We have both 'SG_safeptr *' and 'SG_js_safeptr *'
 * TODO            defined on 'struct _SG_safeptr'.  I'd like to
 * TODO            deprecate the publid 'SG_safeptr *' type as we
 * TODO            get ready to support other bindings (such as
 * TODO            C# and Java).
 *
 */
void SG_js_safeptr__wrap__generic(SG_context * pCtx,
								  void * pVoidGeneric,
								  const char * pszType,
								  SG_js_safeptr ** ppSafePtr)
{
	MY_WRAP_RETURN(pCtx, pVoidGeneric, pszType, (SG_safeptr **)ppSafePtr);
}

void SG_js_safeptr__unwrap__generic(SG_context * pCtx,
									SG_js_safeptr * pSafePtr,
									const char * pszType,
									void ** ppVoidGeneric)
{
	MY_UNWRAP_RETURN(pCtx, pSafePtr, pszType, ppVoidGeneric, void *);
}

void SG_js_safeptr__free(SG_context * pCtx, SG_js_safeptr* p)
{
	SG_ERR_CHECK_RETURN(  SG_safeptr__free(pCtx, (SG_safeptr *)p)  );
}

//////////////////////////////////////////////////////////////////

void SG_safeptr__wrap__audit(SG_context* pCtx, SG_audit* p, SG_safeptr** ppsp)
{
	MY_WRAP_RETURN(pCtx, p, SG_SAFEPTR_TYPE__AUDIT, ppsp);
}
void SG_safeptr__unwrap__audit(SG_context* pCtx, SG_safeptr* psp, SG_audit** pp)
{
	MY_UNWRAP_RETURN(pCtx, psp, SG_SAFEPTR_TYPE__AUDIT, pp, SG_audit *);
}

void SG_safeptr__wrap__cbuffer(SG_context* pCtx, SG_cbuffer* p, SG_safeptr** ppsp)
{
	MY_WRAP_RETURN(pCtx, p, SG_SAFEPTR_TYPE__CBUFFER, ppsp);
}
void SG_safeptr__unwrap__cbuffer(SG_context* pCtx, SG_safeptr* psp, SG_cbuffer** pp)
{
	MY_UNWRAP_RETURN(pCtx, psp, SG_SAFEPTR_TYPE__CBUFFER, pp, SG_cbuffer *);
}

void SG_safeptr__wrap__blobset(SG_context* pCtx, SG_blobset* p, SG_safeptr** ppsp)
{
	MY_WRAP_RETURN(pCtx, p, SG_SAFEPTR_TYPE__BLOBSET, ppsp);
}
void SG_safeptr__unwrap__blobset(SG_context* pCtx, SG_safeptr* psp, SG_blobset** pp)
{
	MY_UNWRAP_RETURN(pCtx, psp, SG_SAFEPTR_TYPE__BLOBSET, pp, SG_blobset *);
}

void SG_safeptr__wrap__repo(SG_context* pCtx, SG_repo* p, SG_safeptr** ppsp)
{
	MY_WRAP_RETURN(pCtx, p, SG_SAFEPTR_TYPE__REPO, ppsp);
}
void SG_safeptr__unwrap__repo(SG_context* pCtx, SG_safeptr* psp, SG_repo** pp)
{
	MY_UNWRAP_RETURN(pCtx, psp, SG_SAFEPTR_TYPE__REPO, pp, SG_repo *);
}

void SG_safeptr__wrap__fetchblobhandle(SG_context* pCtx, SG_repo_fetch_blob_handle* p, SG_safeptr** ppsp)
{
	MY_WRAP_RETURN(pCtx, p, SG_SAFEPTR_TYPE__FETCHBLOBHANDLE, ppsp);
}
void SG_safeptr__unwrap__fetchblobhandle(SG_context* pCtx, SG_safeptr* psp, SG_repo_fetch_blob_handle** pp)
{
	MY_UNWRAP_RETURN(pCtx, psp, SG_SAFEPTR_TYPE__FETCHBLOBHANDLE, pp, SG_repo_fetch_blob_handle *);
}

void SG_safeptr__wrap__fetchfilehandle(SG_context* pCtx, SG_generic_file_response_context* p, SG_safeptr** ppsp)
{
	MY_WRAP_RETURN(pCtx, p, SG_SAFEPTR_TYPE__FETCHFILEHANDLE, ppsp);
}
void SG_safeptr__unwrap__fetchfilehandle(SG_context* pCtx, SG_safeptr* psp, SG_generic_file_response_context** pp)
{
	MY_UNWRAP_RETURN(pCtx, psp, SG_SAFEPTR_TYPE__FETCHFILEHANDLE, pp, SG_generic_file_response_context *);
}

void SG_safeptr__wrap__zingdb(SG_context* pCtx, sg_zingdb* p, SG_safeptr** ppsp)
{
	MY_WRAP_RETURN(pCtx, p, SG_SAFEPTR_TYPE__ZINGSTATE, ppsp);
}
void SG_safeptr__unwrap__zingdb(SG_context* pCtx, SG_safeptr* psp, sg_zingdb** pp)
{
	MY_UNWRAP_RETURN(pCtx, psp, SG_SAFEPTR_TYPE__ZINGSTATE, pp, sg_zingdb *);
}

void SG_safeptr__wrap__zingrecord(SG_context* pCtx, SG_zingrecord* p, SG_safeptr** ppsp)
{
	MY_WRAP_RETURN(pCtx, p, SG_SAFEPTR_TYPE__ZINGRECORD, ppsp);
}
void SG_safeptr__unwrap__zingrecord(SG_context* pCtx, SG_safeptr* psp, SG_zingrecord** pp)
{
	MY_UNWRAP_RETURN(pCtx, psp, SG_SAFEPTR_TYPE__ZINGRECORD, pp, SG_zingrecord *);
}

void SG_safeptr__wrap__zinglinkcollection(SG_context* pCtx, sg_zinglinkcollection* p, SG_safeptr** ppsp)
{
	MY_WRAP_RETURN(pCtx, p, SG_SAFEPTR_TYPE__ZINGLINKCOLLECTION, ppsp);
}
void SG_safeptr__unwrap__zinglinkcollection(SG_context* pCtx, SG_safeptr* psp, sg_zinglinkcollection** pp)
{
	MY_UNWRAP_RETURN(pCtx, psp, SG_SAFEPTR_TYPE__ZINGLINKCOLLECTION, pp, sg_zinglinkcollection *);
}

void SG_safeptr__wrap__statetxcombo(SG_context* pCtx, sg_statetxcombo* p, SG_safeptr** ppsp)
{
	MY_WRAP_RETURN(pCtx, p, SG_SAFEPTR_TYPE__STATETXCOMBO, ppsp);
}
void SG_safeptr__unwrap__statetxcombo(SG_context* pCtx, SG_safeptr* psp, sg_statetxcombo** pp)
{
	MY_UNWRAP_RETURN(pCtx, psp, SG_SAFEPTR_TYPE__STATETXCOMBO, pp, sg_statetxcombo *);
}

void SG_safeptr__wrap__committing(SG_context* pCtx, SG_committing* p, SG_safeptr** ppsp)
{
	MY_WRAP_RETURN(pCtx, p, SG_SAFEPTR_TYPE__COMMITTING, ppsp);
}
void SG_safeptr__unwrap__committing(SG_context* pCtx, SG_safeptr* psp, SG_committing** pp)
{
	MY_UNWRAP_RETURN(pCtx, psp, SG_SAFEPTR_TYPE__COMMITTING, pp, SG_committing *);
}

void SG_safeptr__wrap__xmlwriter(SG_context* pCtx, sg_xmlwriter* p, SG_safeptr** ppsp)
{
	MY_WRAP_RETURN(pCtx, p, SG_SAFEPTR_TYPE__XMLWRITER, ppsp);
}
void SG_safeptr__unwrap__xmlwriter(SG_context* pCtx, SG_safeptr* psp, sg_xmlwriter** pp)
{
	MY_UNWRAP_RETURN(pCtx, psp, SG_SAFEPTR_TYPE__XMLWRITER, pp, sg_xmlwriter *);
}

void SG_safeptr__free(SG_context * pCtx, SG_safeptr* p)
{
    if (!p)
        return;

    SG_NULLFREE(pCtx, p);
}

void SG_safeptr__null(SG_context* pCtx, SG_safeptr* psp)
{
    SG_NULLARGCHECK_RETURN(psp);

    psp->ptr = NULL;
}


