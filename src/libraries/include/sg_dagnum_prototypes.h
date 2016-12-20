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
 * @file sg_dagnum_prototypes.h
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_DAGNUM_PROTOTYPES_H
#define H_SG_DAGNUM_PROTOTYPES_H

BEGIN_EXTERN_C;

#if DEBUG
#define SG_DAGNUM_STDOUT(d) { char buf[SG_DAGNUM__BUF_MAX__HEX]; SG_dagnum__to_sz__hex(pCtx, d, buf, sizeof(buf)); printf("%s\n", buf); }
#endif

void SG_dagnum__from_sz__hex(

	SG_context* pCtx,
	const char* buf,
	SG_uint64* piResult
	);

void SG_dagnum__to_sz__hex(
        SG_context* pCtx,
		SG_uint64 iDagNum,
        char* buf,
        SG_uint32 iBufSize
        );

END_EXTERN_C;

#endif //H_SG_DAGNUM_PROTOTYPES_H

