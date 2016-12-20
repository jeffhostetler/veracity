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
 * @file sg_wc__status__flags_to_properties.c
 *
 * @details Routine to convert a set of status flags into
 * a vhash of properties suitable for use by JS.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

#define SET_BOOL_PROP(name)												\
	SG_STATEMENT(														\
		SG_ERR_CHECK(  SG_vhash__add__bool(pCtx,						\
										   pvhProperties,				\
										   (name),						\
										   SG_TRUE)  );					\
		)

#define SET_INT64_PROP(name, value)										\
	SG_STATEMENT(														\
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx,						\
											pvhProperties,				\
											(name),						\
											(value))  );				\
		)

//////////////////////////////////////////////////////////////////

/**
 * Convert the bits set in statusFlags into pvhProperties
 * (primarily so we can convert the status to JSON for JS).
 *
 */
void SG_wc__status__flags_to_properties(SG_context * pCtx,
										SG_wc_status_flags statusFlags,
										SG_vhash ** ppvhProperties)
{
	SG_vhash * pvhProperties = NULL;

	SG_NULLARGCHECK_RETURN( ppvhProperties );
	
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhProperties)  );

	if      (statusFlags & SG_WC_STATUS_FLAGS__T__FILE)          SET_BOOL_PROP("isFile");
	else if (statusFlags & SG_WC_STATUS_FLAGS__T__DIRECTORY)     SET_BOOL_PROP("isDirectory");
	else if (statusFlags & SG_WC_STATUS_FLAGS__T__SYMLINK)       SET_BOOL_PROP("isSymlink");
	else if (statusFlags & SG_WC_STATUS_FLAGS__T__SUBREPO)       SET_BOOL_PROP("isSubrepo");
	else if (statusFlags & SG_WC_STATUS_FLAGS__T__BOGUS)         SET_BOOL_PROP("isBogus");
	else if (statusFlags & SG_WC_STATUS_FLAGS__T__DEVICE)        SET_BOOL_PROP("isDevice");
	else SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );

	if (statusFlags & SG_WC_STATUS_FLAGS__A__MULTIPLE_CHANGE)    SET_BOOL_PROP("isMultiple");
	if (statusFlags & SG_WC_STATUS_FLAGS__A__SPARSE)             SET_BOOL_PROP("isSparse");

	if (statusFlags & SG_WC_STATUS_FLAGS__U__LOST)               SET_BOOL_PROP("isLost");
	if (statusFlags & SG_WC_STATUS_FLAGS__U__FOUND)              SET_BOOL_PROP("isFound");
	if (statusFlags & SG_WC_STATUS_FLAGS__U__IGNORED)            SET_BOOL_PROP("isIgnored");

	if (statusFlags & SG_WC_STATUS_FLAGS__R__RESERVED)           SET_BOOL_PROP("isReserved");

	if (statusFlags & SG_WC_STATUS_FLAGS__S__ADDED)              SET_BOOL_PROP("isAdded");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__DELETED)            SET_BOOL_PROP("isRemoved");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__RENAMED)            SET_BOOL_PROP("isRenamed");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__MOVED)              SET_BOOL_PROP("isMoved");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)      SET_BOOL_PROP("isMergeCreated");
	if (statusFlags & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)     SET_BOOL_PROP("isUpdateCreated");

	if (statusFlags & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED)   SET_BOOL_PROP("isModifiedAttributes");
	if (statusFlags & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED)   SET_BOOL_PROP("isNonDirModified");

	// These __M__EXISTENCE_ bits are only set in MSTATUS, but
	// it doesn't hurt to check.

	// only 1 of these __M__EXISTENCE should ever be set.
	if (statusFlags & SG_WC_STATUS_FLAGS__M__AbCM)    SET_BOOL_PROP("isExistence_ACM");	// existed in A,C,M but not B
	if (statusFlags & SG_WC_STATUS_FLAGS__M__ABcM)    SET_BOOL_PROP("isExistence_ABM"); // existed in A,B,M but not C

	// only 1 of these __M__AUTO_MERGED should ever be set.
	if (statusFlags & SG_WC_STATUS_FLAGS__M__AUTO_MERGED)    		SET_BOOL_PROP("isAutoMerged");
	if (statusFlags & SG_WC_STATUS_FLAGS__M__AUTO_MERGED_EDITED)    SET_BOOL_PROP("isAutoMergedEdited");

#if 0 && defined(DEBUG)
	SG_ERR_IGNORE(  SG_wc_debug__status__dump_flags_to_console(pCtx, statusFlags, "Flags2Properties")  );
#endif
	if (statusFlags & SG_WC_STATUS_FLAGS__X__UNRESOLVED)
	{
		SET_BOOL_PROP("isUnresolved");

		// if we had an issue, the overall state is either resolved or
		// unresolved, so only one of __X__RESOLVED and __X__UNRESOLVED can be set.
		SG_ASSERT_RELEASE_FAIL( ((statusFlags & SG_WC_STATUS_FLAGS__X__RESOLVED) == 0) );

		// overall-unresolved implies that at least one choice hasn't been made.
		SG_ASSERT_RELEASE_FAIL(  ((statusFlags & SG_WC_STATUS_FLAGS__XU__MASK) != 0)  );

		// but overall-unresolved says nothing about resolution of individual choices.
		// so we may have __XR__ and __XU__ bits set.  But not both for any individual choice.
		SG_ASSERT_RELEASE_FAIL(  ( (((statusFlags & SG_WC_STATUS_FLAGS__XR__MASK)>>SGWC__XR_OFFSET)
									& ((statusFlags & SG_WC_STATUS_FLAGS__XU__MASK)>>SGWC__XU_OFFSET))
								   == 0)  );

		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__EXISTENCE)	SET_BOOL_PROP("isResolvedExistence");
		else if (statusFlags & SG_WC_STATUS_FLAGS__XU__EXISTENCE)	SET_BOOL_PROP("isUnresolvedExistence");
		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__NAME)		SET_BOOL_PROP("isResolvedName");
		else if (statusFlags & SG_WC_STATUS_FLAGS__XU__NAME)		SET_BOOL_PROP("isUnresolvedName");
		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__LOCATION)	SET_BOOL_PROP("isResolvedLocation");
		else if (statusFlags & SG_WC_STATUS_FLAGS__XU__LOCATION)	SET_BOOL_PROP("isUnresolvedLocation");
		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__ATTRIBUTES)	SET_BOOL_PROP("isResolvedAttributes");
		else if (statusFlags & SG_WC_STATUS_FLAGS__XU__ATTRIBUTES)	SET_BOOL_PROP("isUnresolvedAttributes");
		if      (statusFlags & SG_WC_STATUS_FLAGS__XR__CONTENTS)	SET_BOOL_PROP("isResolvedContents");
		else if (statusFlags & SG_WC_STATUS_FLAGS__XU__CONTENTS)	SET_BOOL_PROP("isUnresolvedContents");
	}
	else if (statusFlags & SG_WC_STATUS_FLAGS__X__RESOLVED)
	{
		SET_BOOL_PROP("isResolved");

		// overall resolved implies each choice has been resolved.
		SG_ASSERT_RELEASE_FAIL(  ((statusFlags & SG_WC_STATUS_FLAGS__XU__MASK) == 0)  );
		SG_ASSERT_RELEASE_FAIL(  ((statusFlags & SG_WC_STATUS_FLAGS__XR__MASK) != 0)  );

		if (statusFlags & SG_WC_STATUS_FLAGS__XR__EXISTENCE)	SET_BOOL_PROP("isResolvedExistence");
		if (statusFlags & SG_WC_STATUS_FLAGS__XR__NAME)			SET_BOOL_PROP("isResolvedName");
		if (statusFlags & SG_WC_STATUS_FLAGS__XR__LOCATION)		SET_BOOL_PROP("isResolvedLocation");
		if (statusFlags & SG_WC_STATUS_FLAGS__XR__ATTRIBUTES)	SET_BOOL_PROP("isResolvedAttributes");
		if (statusFlags & SG_WC_STATUS_FLAGS__XR__CONTENTS)		SET_BOOL_PROP("isResolvedContents");
	}

	if (statusFlags & SG_WC_STATUS_FLAGS__L__LOCKED_BY_USER)		SET_BOOL_PROP("isLockedByUser");
	if (statusFlags & SG_WC_STATUS_FLAGS__L__LOCKED_BY_OTHER)		SET_BOOL_PROP("isLockedByOther");
	if (statusFlags & SG_WC_STATUS_FLAGS__L__WAITING)				SET_BOOL_PROP("isLockedWaiting");
	if (statusFlags & SG_WC_STATUS_FLAGS__L__PENDING_VIOLATION)		SET_BOOL_PROP("isPendingLockViolation");

	SET_INT64_PROP("flags", statusFlags);

	*ppvhProperties = pvhProperties;
	return;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhProperties);
}

