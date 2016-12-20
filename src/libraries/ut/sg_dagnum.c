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

void SG_dagnum__from_sz__hex(
        SG_context* pCtx,
		const char* pszDagnum,
        SG_uint64* piResult)
{
    SG_ERR_CHECK_RETURN(  SG_hex__parse_hex_uint64(pCtx, pszDagnum, SG_STRLEN(pszDagnum), piResult)  );
}

void SG_dagnum__to_sz__hex(
        SG_context* pCtx,
		SG_uint64 iDagNum,
        char* buf,
        SG_uint32 iBufSize
        )
{
    SG_NULLARGCHECK_RETURN(buf);
    SG_ARGCHECK_RETURN(iBufSize >= SG_DAGNUM__BUF_MAX__HEX, iBufSize);
    SG_hex__format_uint64(buf, iDagNum);
}

