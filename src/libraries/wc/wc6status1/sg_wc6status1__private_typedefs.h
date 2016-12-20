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

#ifndef H_SG_WC6STATUS1__PRIVATE_TYPEDEFS_H
#define H_SG_WC6STATUS1__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

struct _sg_wc_tx__status1__item
{
	char * pszGid;
	SG_treenode_entry_type tneType;

	struct
	{
		SG_int64 attrbits;
		char * pszGidParent;
		char * pszEntryname;
		char * pszHid;
		SG_string * pStringRepoPath;
	} h;

	struct
	{
		SG_int64 attrbits;
		char * pszGidParent;
		char * pszEntryname;
		char * pszHid;
		SG_string * pStringRepoPath;

		sg_wc_liveview_item * pLVI;

	} wc;

	SG_bool bPresentIn_H;
	SG_bool bPresentIn_WC;

	SG_bool bReported;
};

typedef struct _sg_wc_tx__status1__item sg_wc_tx__status1__item;

//////////////////////////////////////////////////////////////////

struct _sg_wc_tx__status1__data
{
	SG_wc_tx * pWcTx;				// we do not own this
	const SG_rev_spec * pRevSpec;	// we do not own this
	const char * pszWasLabel_H;		// we do not own this
	const char * pszWasLabel_WC;	// we do not own this

	SG_rbtree * prbItems;			// map["gid" --> _s1_item] -- we own the items
	SG_rbtree * prbIndex_H;			// map["@0/path" --> _s1_item] -- we borrow the items
	SG_rbtree * prbIndex_WC;		// map["@/path" --> _s1_item] -- we borrow the items
	char * pszHidCSet;

	SG_bool bListUnchanged;			// user arguments
	SG_bool bNoIgnores;
	SG_bool bNoTSC;
	SG_bool bListSparse;
	SG_bool bListReserved;

	SG_bool bIsParent;				// true if pszHidCSet is one of the WD parents
};

typedef struct _sg_wc_tx__status1__data sg_wc_tx__status1__data;



//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC6STATUS1__PRIVATE_TYPEDEFS_H
