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
 * @file sg_wc_attrbits__private_typedefs.h
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_ATTRBITS__PRIVATE_TYPEDEFS_H
#define H_SG_WC_ATTRBITS__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

#define SG_WC_ATTRBITS__ZERO		((SG_uint64)0x0000)

// chmod +x on a file.
#define SG_WC_ATTRBITS__FILE_X		((SG_uint64)0x0001)

// All defined bits.
#define SG_WC_ATTRBITS__MASK		((SG_uint64)0x0001)

//////////////////////////////////////////////////////////////////

/**
 * The set of ATTRBITS that we can set on an item is a SG_uint64
 * value of bits.  (Think a big set of Unix Mode Bits, but with
 * different meanings for the various bits.)
 *
 * These bits are defined/shared across all platforms.  So one
 * bit vector value representing a union of the specific bits
 * set on this platform with the inherited bits from other platforms.
 *
 * The purpose of this structure is to defined the various bit
 * masks so that we can tell which bits can be modified on this
 * platform and which should be considered read-only.
 *
 * For example, we have the 'x' bit indicating that we should
 * 'chmod +x' the item on Linux and Mac.  This doesn't do anything
 * on Windows -- nor should a stat() on Windows trick us into
 * stripping off the Linux/Mac 'x' bit -or- report a mode bit
 * change.
 *
 */
struct _sg_wc_attrbits_data
{
	// the complete set of ATTRBITS (may be UI64_MAX_INT or may be tighter).
	SG_uint64		mask_defined;

	// the set of ATTRBITS that are respected on this platform.
	SG_uint64		mask_defined_platform;
};

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_ATTRBITS__PRIVATE_TYPEDEFS_H
