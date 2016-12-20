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

#define SG_ATTRIBUTE_BIT__X  (1 <<  0)
/* TODO we need other attr bits */

void SG_attributes__bits__read(
    SG_context* pCtx,
	const SG_pathname* pPath,
    SG_int64 iBaselineAttributeBits,
    SG_int64* piBits
    )
{
	SG_fsobj_stat st;

	/* Get stat information first */
	SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath, &st)  );

	SG_ERR_CHECK(  SG_attributes__bits__from_perms(pCtx, st.type, st.perms, iBaselineAttributeBits, piBits)  );

fail:
	return;
}

void SG_attributes__bits__from_perms(
    SG_context* pCtx,
	SG_fsobj_type type,
	SG_fsobj_perms perms,
    SG_int64 iBaselineAttributeBits,
    SG_int64* piBits
    )
{
	// TODO 2011/10/12 delete this after the conversion to WC.

    SG_int64 iBits = iBaselineAttributeBits;
	SG_UNUSED(pCtx);
	SG_UNUSED(type);
	SG_UNUSED(perms);

    /* TODO do the mask trick to ignore stuff that is not appropriate for this
     * platform */

#if defined(LINUX) || defined(MAC)
    if (SG_FSOBJ_TYPE__REGULAR == type)
    {
        if (perms & S_IXUSR)
        {
            iBits |= SG_ATTRIBUTE_BIT__X;
        }
        else
        {
            iBits &= ~SG_ATTRIBUTE_BIT__X;
        }
    }
#endif

    *piBits = iBits;

	return;
}
void SG_attributes__bits__apply(
		SG_context* pCtx,
        const SG_pathname* pPath,
        SG_int64 iBits
        )
{
	SG_fsobj_stat st;

	/* Get stat information first */
	SG_ERR_CHECK(  SG_fsobj__stat__pathname(pCtx, pPath, &st)  );

#if defined(LINUX) || defined(MAC)
    if (SG_FSOBJ_TYPE__REGULAR == st.type)
    {
        if (iBits & SG_ATTRIBUTE_BIT__X)
        {
            st.perms |= S_IXUSR;
        }
        else
        {
            st.perms &= ~S_IXUSR;
        }
        SG_ERR_CHECK(  SG_fsobj__chmod__pathname(pCtx, pPath, st.perms)  );
    }
#endif

#if defined(WINDOWS)
    if (iBits & 2) // TODO
    {
    }
#endif

fail:
	return;
}

