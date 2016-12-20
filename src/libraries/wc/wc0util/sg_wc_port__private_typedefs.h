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

#ifndef H_SG_WC_PORT__PRIVATE_TYPEDEFS_H
#define H_SG_WC_PORT__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Working space and individual results for evaluating the portability
 * of an individual file/directory.  This can be used as assoc-data during
 * the filename/directory collision tests.
 */

struct _sg_wc_port_item
{
	SG_wc_port_flags		flagsObserved;				// tests that hit - may include masked bits

	SG_utf8_converter *		pConverterRepoCharSet;		// a CharSet<-->UTF-8 converter for the charset configured with the repo. (we do not own this).

	char *					szUtf8EntryName;			// UTF-8 version of as-given entryname
	SG_int32 *				pUtf32EntryName;			// UTF-32 version of entryname

	char *					szCharSetEntryName;			// charset-based entry (when needed)
	char *					szUtf8EntryNameAppleNFD;	// UTF-8 in apple-nfd (when needed)
	char *					szUtf8EntryNameAppleNFC;	// UTF-8 in apple-nfc (when needed)

	SG_uint32				lenEntryNameChars;			// length of entryname in code-points/characters (independent of encoding)
	SG_uint32				lenEntryNameUtf8Bytes;		// strlen(szUtf8EntryName) in bytes
	SG_uint32				lenEntryNameCharSetBytes;	// strlen(szCharSetEntryName) in bytes when needed)
	SG_uint32				lenEntryNameAppleNFDBytes;	// strlen(szUtf8EntryNameAppleNFD) in bytes when needed
	SG_uint32				lenEntryNameAppleNFCBytes;	// strlen(szUtf8EntryNameAppleNFC) in bytes when needed

	SG_bool					b7BitClean;					// true if US-ASCII [0x00..0x7f] only
	SG_bool					bHasDecomposition;			// true if NFC != NFD (when needed)
	SG_bool					bIsProperlyComposed;		// true if given == NFC (when needed)
	SG_bool					bIsProperlyDecomposed;		// true if given == NFD (when needed)

	void *					pVoidAssocData;				// optional user-defined data to be associated with this item.  (we do not own this.)

	SG_string *				pStringItemLog;				// detailed log message for the problems with this item.
	SG_rbtree *				prbCollidedWith;			// map[gid --> pItem] (only if GIDs were given)  (we do not own the values)

	SG_treenode_entry_type	tneType;

	char					bufGid_Entry[SG_GID_BUFFER_LENGTH];		// optional, GID of entry for logging
};

typedef struct _sg_wc_port_item sg_wc_port_item;

//////////////////////////////////////////////////////////////////

struct _sg_wc_port
{
	SG_wc_port_flags		mask;			// bits we are interested in
	SG_wc_port_flags		flagsUnion;		// tests that hit - union over all items - may included masked bits

	SG_utf8_converter *		pConverterRepoCharSet;	// a CharSet<-->UTF-8 converter for the charset configured with the repo. (we do not own this).

	SG_rbtree *				prbCollider;			// rbtree<sz,collider_data> assoc-array of normalized entry names
	SG_rbtree *				prbItems;				// rbtree<sz,item> assoc-array of items

	SG_string *				pStringLog;				// cummulative log of problems for everything in directory.
};

//////////////////////////////////////////////////////////////////

struct _sg_wc_port_collider_assoc_data
{
	SG_wc_port_flags	portFlagsWhy;			// __PAIRWISE__ bit for why we added this entry to pDir->prbCollider
	sg_wc_port_item *	pItem;					// link to item data; we do not own this.
};

typedef struct _sg_wc_port_collider_assoc_data sg_wc_port_collider_assoc_data;

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_PORT__PRIVATE_TYPEDEFS_H
