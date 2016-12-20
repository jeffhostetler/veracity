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

#ifndef H_SG_WC_PCTNE__PRIVATE_TYPEDEFS_H
#define H_SG_WC_PCTNE__PRIVATE_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

struct _sg_wc_pctne__row
{
	sg_wc_db__tne_row * pTneRow;
	sg_wc_db__pc_row * pPcRow;
};

typedef struct _sg_wc_pctne__row sg_wc_pctne__row;

//////////////////////////////////////////////////////////////////

/**
 * Prototype for a callback function to receive a
 * pctne__row from a combined SELECT on the tne_L0
 * and tbl_pc tables.
 *
 * The callback may steal the pctne__row if it wants to.
 */
typedef void (sg_wc_pctne__foreach_cb)(SG_context * pCtx, void * pVoidData, sg_wc_pctne__row ** ppPT);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_PCTNE__PRIVATE_TYPEDEFS_H
