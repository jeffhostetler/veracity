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

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_STATUS__PUBLIC_TYPEDEFS_H
#define H_SG_WC_STATUS__PUBLIC_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * This is the PUBLIC STATUS for an item.  We aggregate
 * all of the various flag bits from the different tables
 * and the live state of the working directory.
 *
 * MANY COMBINATIONS OF THESE BITS ARE POSSIBLE.  The
 * original pendingtree code had some assumptions about
 * some of them being mutually-exclusive, but those have
 * been relaxed.
 *
 * For example, something can be:
 * [] MERGE_CREATED *and* DELETED -- MERGE created something
 *    (that was added in the other branch) that the user
 *    subsequently has RESOLVE delete)
 * [] ADDED *and* LOST -- say they 'vv ADD' it and then
 *    /bin/rm it or /bin/mv it).
 * [] Whereas LOST+DELETED is not really possible.
 * 
 */
typedef SG_uint64 SG_wc_status_flags;

#define SG_WC_STATUS_FLAGS__ZERO							((SG_wc_status_flags)0x0ULL)

//////////////////////////////////////////////////////////////////
// TYPE bits -- like tneType
//////////////////////////////////////////////////////////////////

#define SGWC__T_OFFSET	(0)
#define SGWC__T_BITS	(6)
#define SGWC__T(b)		((b) << SGWC__T_OFFSET)

#define SG_WC_STATUS_FLAGS__T__FILE								SGWC__T(0x01ULL)
#define SG_WC_STATUS_FLAGS__T__DIRECTORY						SGWC__T(0x02ULL)
#define SG_WC_STATUS_FLAGS__T__SYMLINK							SGWC__T(0x04ULL)	// a Linux or MAC symlink
#define SG_WC_STATUS_FLAGS__T__SUBREPO							SGWC__T(0x08ULL)
#define SG_WC_STATUS_FLAGS__T__DEVICE							SGWC__T(0x10ULL)	// uncontrollable item like a FIFO or BlkDev
#define SG_WC_STATUS_FLAGS__T__BOGUS							SGWC__T(0x20ULL)	// path was bogus and we can't determine a tneType
#define SG_WC_STATUS_FLAGS__T__MASK								SGWC__T((1ULL<<SGWC__T_BITS)-1)

//////////////////////////////////////////////////////////////////
// various auxillary bits
//////////////////////////////////////////////////////////////////

#define SGWC__A_OFFSET	(SGWC__T_OFFSET + SGWC__T_BITS)
#define SGWC__A_BITS	(2)
#define SGWC__A(b)		((b) << SGWC__A_OFFSET)

#define SG_WC_STATUS_FLAGS__A__MULTIPLE_CHANGE					SGWC__A(0x01ULL)	// more than one change bit is set.

// A 'sparse' item is one that wasn't populated during the
// checkout/update/merge/whatever.  I'm not putting it with
// the structural-change bits because it doesn't reflect an
// actual change in the item (just our view of it).  There
// are a few assumptions on a 'sparse' item:
// [] it is easily confused with LOST status
// [] a SPARSE item could be MOVED/RENAMED (without actually
//    realizing the item (we could play TNE games and not
//    actually have to populate it just to move/rename it))
// [] a SPARSE item could be DELETED without actually populating
//    the item
// [] a SPARSE item is, by definition, un-modified/un-changed;
//    the content/attrbits cannot be altered until it
//    is populated
// [] the ROOT directory ("@/") cannot be SPARSE

#define SG_WC_STATUS_FLAGS__A__SPARSE							SGWC__A(0x02ULL)

#define SG_WC_STATUS_FLAGS__A__MASK								SGWC__A((1ULL<<SGWC__A_BITS)-1)

//////////////////////////////////////////////////////////////////

#define SGWC__R_OFFSET	(SGWC__A_OFFSET + SGWC__A_BITS)
#define SGWC__R_BITS	(1)
#define SGWC__R(b)		((b) << SGWC__R_OFFSET)

#define SG_WC_STATUS_FLAGS__R__RESERVED							SGWC__R(0x01ULL)	// .sgdrawer/.sgcloset/...
#define SG_WC_STATUS_FLAGS__R__MASK								SGWC__R((1ULL<<SGWC__R_BITS)-1)

//////////////////////////////////////////////////////////////////

#define SGWC__U_OFFSET	(SGWC__R_OFFSET + SGWC__R_BITS)
#define SGWC__U_BITS	(3)
#define SGWC__U(b)		((b) << SGWC__U_OFFSET)

#define SG_WC_STATUS_FLAGS__U__LOST								SGWC__U(0x01ULL)	// controlled but lost
#define SG_WC_STATUS_FLAGS__U__FOUND							SGWC__U(0x02ULL)	// uncontrolled+found
#define SG_WC_STATUS_FLAGS__U__IGNORED							SGWC__U(0x04ULL)	// uncontrolled+ignored
#define SG_WC_STATUS_FLAGS__U__MASK								SGWC__U((1ULL<<SGWC__U_BITS)-1)

//////////////////////////////////////////////////////////////////

#define SGWC__S_OFFSET	(SGWC__U_OFFSET + SGWC__U_BITS)
#define SGWC__S_BITS	(6)
#define SGWC__S(b)		((b) << SGWC__S_OFFSET)

#define SG_WC_STATUS_FLAGS__S__ADDED							SGWC__S(0x01ULL)
#define SG_WC_STATUS_FLAGS__S__DELETED							SGWC__S(0x02ULL)
#define SG_WC_STATUS_FLAGS__S__RENAMED							SGWC__S(0x04ULL)
#define SG_WC_STATUS_FLAGS__S__MOVED							SGWC__S(0x08ULL)
#define SG_WC_STATUS_FLAGS__S__MERGE_CREATED					SGWC__S(0x10ULL)
#define SG_WC_STATUS_FLAGS__S__UPDATE_CREATED					SGWC__S(0x20ULL)
#define SG_WC_STATUS_FLAGS__S__MASK								SGWC__S((1ULL<<SGWC__S_BITS)-1)

//////////////////////////////////////////////////////////////////
// The __C__ bits define observed dynamic (non-structural) changes
// These refer to diff(baseline,live) and therefore can only be set
// when NOT ADDED and NOT DELETED.

#define SGWC__C_OFFSET	(SGWC__S_OFFSET + SGWC__S_BITS)
#define SGWC__C_BITS	(2)
#define SGWC__C(b)		((b) << SGWC__C_OFFSET)

#define SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED					SGWC__C(0x01ULL)

// The "modified" bit for files/symlinks/subrepos.
// This can be set/computed at any time.
#define SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED					SGWC__C(0x02ULL)

#define SG_WC_STATUS_FLAGS__C__MASK								SGWC__C((1ULL<<SGWC__C_BITS)-1)

//////////////////////////////////////////////////////////////////
// The __M__ bits help indicate status after a MERGE.

#define SGWC__M_OFFSET	(SGWC__C_OFFSET + SGWC__C_BITS)
#define SGWC__M_BITS	(4)
#define SGWC__M(b)		((b) << SGWC__M_OFFSET)

#define SG_WC_STATUS_FLAGS__M__AbCM								SGWC__M(0x01ULL)
#define SG_WC_STATUS_FLAGS__M__ABcM								SGWC__M(0x02ULL)

#define SG_WC_STATUS_FLAGS__M__AUTO_MERGED						SGWC__M(0x04ULL)
#define SG_WC_STATUS_FLAGS__M__AUTO_MERGED_EDITED				SGWC__M(0x08ULL)

#define SG_WC_STATUS_FLAGS__M__MASK								SGWC__M((1ULL<<SGWC__M_BITS)-1)

//////////////////////////////////////////////////////////////////
// The __X__ bits help indicate conflict status after a MERGE.

#define SGWC__X_OFFSET	(SGWC__M_OFFSET + SGWC__M_BITS)
#define SGWC__X_BITS	(2)
#define SGWC__X(b)		((b) << SGWC__X_OFFSET)

#define SG_WC_STATUS_FLAGS__X__UNRESOLVED						SGWC__X(0x1ULL)		// only 1 of __X__UNRESOLVED and __X__RESOLVED
#define SG_WC_STATUS_FLAGS__X__RESOLVED							SGWC__X(0x2ULL)		// can be set at any time.

#define SG_WC_STATUS_FLAGS__X__MASK								SGWC__X((1ULL<<SGWC__X_BITS)-1)

//////////////////////////////////////////////////////////////////
// The __XR__ bits help indicate RESOLVED INDIVIDUAL CHOICES within a conflict.

#define SGWC__XR_OFFSET	(SGWC__X_OFFSET + SGWC__X_BITS)
#define SGWC__XR_BITS	(5)
#define SGWC__XR(b)		((b) << SGWC__XR_OFFSET)

#define SG_WC_STATUS_FLAGS__XR__EXISTENCE						SGWC__XR(0x01ULL)	// only 1 of __XR__k and __XU__k
#define SG_WC_STATUS_FLAGS__XR__NAME							SGWC__XR(0x02ULL)	// can be set at any time.
#define SG_WC_STATUS_FLAGS__XR__LOCATION						SGWC__XR(0x04ULL)	// if neither in the pair is set
#define SG_WC_STATUS_FLAGS__XR__ATTRIBUTES						SGWC__XR(0x08ULL)	// then this item did not have a
#define SG_WC_STATUS_FLAGS__XR__CONTENTS						SGWC__XR(0x10ULL)	// conflict of that choice.

#define SG_WC_STATUS_FLAGS__XR__MASK							SGWC__XR((1ULL<<SGWC__XR_BITS)-1)

//////////////////////////////////////////////////////////////////
// The __XU__ bits help indicate RESOLVED INDIVIDUAL CHOICES within a conflict.

#define SGWC__XU_OFFSET	(SGWC__XR_OFFSET + SGWC__XR_BITS)
#define SGWC__XU_BITS	(5)
#define SGWC__XU(b)		((b) << SGWC__XU_OFFSET)

#define SG_WC_STATUS_FLAGS__XU__EXISTENCE						SGWC__XU(0x01ULL)	// only 1 of __XR__k and __XU__k
#define SG_WC_STATUS_FLAGS__XU__NAME							SGWC__XU(0x02ULL)	// can be set at any time.
#define SG_WC_STATUS_FLAGS__XU__LOCATION						SGWC__XU(0x04ULL)	// if neither in the pair is set
#define SG_WC_STATUS_FLAGS__XU__ATTRIBUTES						SGWC__XU(0x08ULL)	// then this item did not have a
#define SG_WC_STATUS_FLAGS__XU__CONTENTS						SGWC__XU(0x10ULL)	// conflict of that choice.

#define SG_WC_STATUS_FLAGS__XU__MASK							SGWC__XU((1ULL<<SGWC__XU_BITS)-1)

//////////////////////////////////////////////////////////////////
// The __L__ bits help indicate active LOCKS.

#define SGWC__L_OFFSET	(SGWC__XU_OFFSET + SGWC__XU_BITS)
#define SGWC__L_BITS	(4)
#define SGWC__L(b)		((b) << SGWC__L_OFFSET)

#define SG_WC_STATUS_FLAGS__L__LOCKED_BY_USER					SGWC__L(0x01ULL)	// Current user owns a lock on it.
#define SG_WC_STATUS_FLAGS__L__LOCKED_BY_OTHER					SGWC__L(0x02ULL)	// One or more others own lock(s) on it.
#define SG_WC_STATUS_FLAGS__L__WAITING							SGWC__L(0x04ULL)	// Lock closed but we don't have the end-cset yet.
#define SG_WC_STATUS_FLAGS__L__PENDING_VIOLATION				SGWC__L(0x08ULL)	// File edited while locked by someone else.

#define SG_WC_STATUS_FLAGS__L__MASK								SGWC__L((1ULL<<SGWC__L_BITS)-1)

//////////////////////////////////////////////////////////////////
// TODO 2011/09/01 Does it make sense to optionally look for LOCKS and
// TODO            report them here?

//////////////////////////////////////////////////////////////////

#define SG_WC_STATUS_FLAGS__IS_UNCONTROLLED(f)		(((f) & (SG_WC_STATUS_FLAGS__U__FOUND \
															 |SG_WC_STATUS_FLAGS__U__IGNORED)) \
													 != SG_WC_STATUS_FLAGS__ZERO)

//////////////////////////////////////////////////////////////////

#if 0
// TODO 2011/10/13 think about something like this for the
// TODO            various status calls because we now have
// TODO            something like 3 bools on most of the calls
// TODO            and it's getting a little confusing.
// TODO
// TODO            Or maybe have a structure so we can also
// TODO            include the 'depth' argument.
/**
 * Flags controlling the various forms of STATUS.
 *
 */
typedef enum _sg_wc_status_params		SG_wc_status_params;

enum _sg_wc_status_params
{
	SG_WC_STATUS_PARAMS__DEFAULT				= 0x0000,
	SG_WC_STATUS_PARAMS__NO_IGNORES				= 0x0001,	// --no-ignores   : do not use .vvignores
	SG_WC_STATUS_PARAMS__NO_TSC					= 0x0002,	// --no-tsc       : do not use the timestamp cache
	SG_WC_STATUS_PARAMS__LIST_UNCHANGED			= 0x0004,	//                : request report of unchanged items
	SG_WC_STATUS_PARAMS__LIST_SPARSE			= 0x0008,	//                : request report of sparse items
	// list-extended-status
};
#endif

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_STATUS__PUBLIC_TYPEDEFS_H
