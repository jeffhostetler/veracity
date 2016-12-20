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
 * @file sg__do_cmd_dump_lca.c
 *
 * @details Handle 'vv dump_lca'.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

#define sg_NUM_CHARS_FOR_DOT 8

//////////////////////////////////////////////////////////////////

/**
 * The DAGFRAG code now takes a generic FETCH-DAGNODE callback function
 * rather than assuming that it should call SG_repo__fetch_dagnode().
 * This allows the DAGFRAG code to not care whether it is above or below
 * the REPO API layer.
 *
 * my_fetch_dagnodes_from_repo() is a generic wrapper for
 * SG_repo__fetch_dagnode() and is used by the code in
 * sg_dagfrag to request dagnodes from disk.  we don't really
 * need this function since FN__sg_fetch_dagnode and
 * SG_repo__fetch_dagnode() have an equivalent prototype,
 * but with callbacks in loops calling callbacks calling callbacks,
 * it's easy to get lost....
 *
 */
static FN__sg_fetch_dagnode my_fetch_dagnodes_from_repo;

void my_fetch_dagnodes_from_repo(SG_context * pCtx, void * pCtxFetchDagnode, SG_uint64 uiDagNum, const char * szHidDagnode, SG_dagnode ** ppDagnode)
{
	SG_ERR_CHECK_RETURN(  SG_repo__fetch_dagnode(pCtx, (SG_repo *)pCtxFetchDagnode, uiDagNum, szHidDagnode, ppDagnode)  );
}

//////////////////////////////////////////////////////////////////

static void _get_tag_if_present(SG_context * pCtx, SG_repo * pRepo, const char * pszHid, char ** ppszTag)
{
	// if the given cset has any tags associated with it, get the first one.

	SG_varray * pvaCSets = NULL;
	SG_varray * pvaTags = NULL;
	char * pszTag = NULL;
	SG_uint32 count;

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaCSets)  );
	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pvaCSets, pszHid)  );
	SG_ERR_CHECK(  SG_vc_tags__list_for_given_changesets(pCtx, pRepo, &pvaCSets, &pvaTags)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaTags, &count)  );
	if (count > 0)
	{
		const char * pszTag_ref;
		SG_vhash * pvh_0;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaTags, 0, &pvh_0)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_0, "tag", &pszTag_ref)  );
		SG_ERR_CHECK(  SG_strdup(pCtx, pszTag_ref, &pszTag)  );
	}

	*ppszTag = pszTag;
	pszTag = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaCSets);
	SG_VARRAY_NULLFREE(pCtx, pvaTags);
	SG_NULLFREE(pCtx, pszTag);
}

//////////////////////////////////////////////////////////////////
// Routines to draw edges when not verbose.

struct _add_significant_edge_data
{
	const char * pszHid_ancestor;
};

static SG_rbtree_foreach_callback _add_significant_edge_cb;

static void _add_significant_edge_cb(SG_context * pCtx,
									 const char * pszHidDescendant,
									 SG_UNUSED_PARAM(void * assocData),
									 void * pVoidEdgeData)
{
	struct _add_significant_edge_data * pEdgeData = (struct _add_significant_edge_data *)pVoidEdgeData;

	SG_UNUSED(assocData);

	SG_ERR_CHECK_RETURN(  SG_console(pCtx, SG_CS_STDOUT, "_%s -> _%s;\n", pszHidDescendant, pEdgeData->pszHid_ancestor)  );
}

static SG_daglca__foreach_callback _add_significant_edges_cb;

static void _add_significant_edges_cb(SG_context * pCtx,
									  const char * pszHid,
									  SG_daglca_node_type nodeType,
									  SG_UNUSED_PARAM(SG_int32 generation),
									  SG_rbtree * prbImmediateDescendants,	// we must free this
									  SG_UNUSED_PARAM(void * pVoidNodeData))
{
	struct _add_significant_edge_data EdgeData;

	SG_UNUSED(nodeType);
	SG_UNUSED(generation);
	SG_UNUSED(pVoidNodeData);

	if (!prbImmediateDescendants)
		return;
		
	EdgeData.pszHid_ancestor = pszHid;

	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx,
									  prbImmediateDescendants,
									  _add_significant_edge_cb,
									  &EdgeData)  );

fail:
	SG_RBTREE_NULLFREE(pCtx, prbImmediateDescendants);
}

//////////////////////////////////////////////////////////////////

struct _graph_data
{
	SG_rbtree *		prbLabeledNodes;
	SG_rbtree *		prbGenerationList;
	SG_repo *		pRepo;
	SG_daglca *		pDagLca;
	const char *	pszHidLCA;
	SG_uint64       uiDagNum;
};

static SG_daglca__foreach_callback _add_significant_node_no_edges_cb;

static void _add_significant_node_no_edges_cb(SG_context * pCtx,
											  const char * pszHid,
											  SG_daglca_node_type nodeType,
											  SG_int32 generation,
											  SG_rbtree * prbImmediateDescendants,	// we must free this
											  void * pVoidGraphData)
{
	struct _graph_data * pGraphData = (struct _graph_data *)pVoidGraphData;
	char bufHid_ancestor[1 + sg_NUM_CHARS_FOR_DOT];
	char bufGen[20];
    const char* pszLabel = NULL;
	char * pszTag = NULL;

	// for display purposes, we truncate each ID to fewer characters.

	memcpy(bufHid_ancestor, pszHid, sg_NUM_CHARS_FOR_DOT);
	bufHid_ancestor[sg_NUM_CHARS_FOR_DOT] = 0;
	pszLabel = bufHid_ancestor;

	// see if there is a TAG for it.

	SG_ERR_IGNORE(  _get_tag_if_present(pCtx, pGraphData->pRepo, pszHid, &pszTag)  );

    // dot can't handle ids which start with a digit, so we prefix everything with an underscore.
	// give each type of node a unique style.

	switch (nodeType)
	{
	default:
		SG_ASSERT(  (0)  );
		break;

	case SG_DAGLCA_NODE_TYPE__LEAF:
		if (pszTag)
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
									  "    _%s [shape=box, fontsize=16, label=\"LEAF %s %s\"];\n",
									  pszHid, pszLabel, pszTag)  );
		else
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
									  "    _%s [shape=box, fontsize=16, label=\"LEAF %s\"];\n",
									  pszHid, pszLabel)  );
		break;

	case SG_DAGLCA_NODE_TYPE__SPCA:
		if (pszTag)
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
									  "    _%s [shape=box, fontsize=12, label=\"SPCA %s %s\"];\n",
									  pszHid, pszLabel, pszTag)  );
		else
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
									  "    _%s [shape=box, fontsize=12, label=\"SPCA %s\"];\n",
									  pszHid, pszLabel)  );
		SG_ERR_CHECK(  SG_sprintf(pCtx, bufGen, sizeof(bufGen), "gen_%08d", generation)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
								  "    { rank = same; %s; _%s; }\n",
								  bufGen, pszHid)  );
		SG_ERR_CHECK(  SG_rbtree__update(pCtx, pGraphData->prbGenerationList, bufGen)  );
		break;

	case SG_DAGLCA_NODE_TYPE__LCA:
		SG_ERR_CHECK(  SG_sprintf(pCtx, bufGen, sizeof(bufGen), "gen_%08d", generation)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
								  "    { rank = same; %s; LCA; _%s; };\n",
								  bufGen, pszHid)  );
		SG_ERR_CHECK(  SG_rbtree__update(pCtx, pGraphData->prbGenerationList, bufGen)  );
		if (pszTag)
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
									  "    _%s [shape=ellipse, fontsize=16, style=bold, label=\"LCA %s %s\"];\n",
									  pszHid, pszLabel, pszTag)  );
		else
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
									  "    _%s [shape=ellipse, fontsize=16, style=bold, label=\"LCA %s\"];\n",
									  pszHid, pszLabel)  );
		break;
	}

	// remember this significant node so that we don't double-declare/stylize it when
	// we start filling in the regular/non-special nodes and edges.

	SG_ERR_CHECK(  SG_rbtree__add(pCtx, pGraphData->prbLabeledNodes, pszHid)  );
	
fail:
	SG_RBTREE_NULLFREE(pCtx, prbImmediateDescendants);
	SG_NULLFREE(pCtx, pszTag);
}

static const char * _get_edge_color(SG_context * pCtx,
									struct _graph_data * pGraphData,
									const char * psz_hid_child_endpoint,
									const char * psz_hid_parent_endpoint)
{
	// Advisory routine to color an edge in the graph.
	// If this node is a descendant of the LCA, color it red.
	// Note that the arrows in the graph go from "child--->parent"
	// (and I think the graph is upside-down).

	const char * pszColor = "gray";

	if (strcmp(psz_hid_parent_endpoint, pGraphData->pszHidLCA) == 0)
	{
		pszColor = "red";
	}
	else if (strcmp(psz_hid_child_endpoint, pGraphData->pszHidLCA) == 0)
	{
		pszColor = "blue";
	}
	else
	{
		SG_dagquery_relationship dqRel_child  = SG_DAGQUERY_RELATIONSHIP__UNKNOWN;
		SG_dagquery_relationship dqRel_parent = SG_DAGQUERY_RELATIONSHIP__UNKNOWN;
		
		// Making 2 full dagquery calls is dreadfully expensive, but we only
		// do it for debug output and it helps make the graph more readable.
		SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pGraphData->pRepo,
															 pGraphData->uiDagNum,
															 psz_hid_child_endpoint,
															 pGraphData->pszHidLCA,
															 SG_FALSE,
															 SG_FALSE,
															 &dqRel_child)  );
		SG_ERR_CHECK(  SG_dagquery__how_are_dagnodes_related(pCtx, pGraphData->pRepo,
															 pGraphData->uiDagNum,
															 psz_hid_parent_endpoint,
															 pGraphData->pszHidLCA,
															 SG_FALSE,
															 SG_FALSE,
															 &dqRel_parent)  );
		if (dqRel_child != dqRel_parent)
		{
			pszColor = "green";
		}
		else
		{
			switch (dqRel_parent)
			{
			default:
			case SG_DAGQUERY_RELATIONSHIP__UNKNOWN:
			case SG_DAGQUERY_RELATIONSHIP__SAME:
				pszColor = "gray";
				break;
			
			case SG_DAGQUERY_RELATIONSHIP__PEER:
				pszColor = "black";
				break;

			case SG_DAGQUERY_RELATIONSHIP__ANCESTOR:
				pszColor = "blue";
				break;

			case SG_DAGQUERY_RELATIONSHIP__DESCENDANT:
				pszColor = "red";
				break;
			}
		}
	}

fail:
	return pszColor;
}
									
static SG_rbtree_foreach_callback _fill_verbose_graph_cb;

static void _fill_verbose_graph_cb(SG_context* pCtx, 
								   const char * pszKey,
								   void * pVoidAssoc,
								   void * pVoidData)
{
	struct _graph_data * pGraphData = (struct _graph_data *)pVoidData;
	SG_dagnode * pDagnodeCurrent = (SG_dagnode *)pVoidAssoc;
	const char * pszHidCurrent;
    SG_uint32 count_parents = 0;
    const char** parents = NULL;
	char * pszTag = NULL;
	SG_int32 genCurrent;
	char bufGen[20];
	SG_bool bAlreadyLabeled;

	SG_UNUSED( pszKey );

	SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx, pDagnodeCurrent, &genCurrent)  );

	SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pDagnodeCurrent, &pszHidCurrent)  );
	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pGraphData->prbLabeledNodes, pszHidCurrent, &bAlreadyLabeled, NULL)  );

	if (!bAlreadyLabeled)
	{
		char bufLabel[1 + sg_NUM_CHARS_FOR_DOT];

        // for display purposes, we truncate each ID to fewer characters.

        memcpy(bufLabel, pszHidCurrent, sg_NUM_CHARS_FOR_DOT);
        bufLabel[sg_NUM_CHARS_FOR_DOT] = 0;

		// see if there is a TAG for it.

		SG_ERR_IGNORE(  _get_tag_if_present(pCtx, pGraphData->pRepo, pszHidCurrent, &pszTag)  );

		if (pszTag)
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
									  "    _%s [shape=circle, fontsize=10, label=\"%s %s\"];\n",
									  pszHidCurrent, bufLabel, pszTag)  );
		else
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
									  "    _%s [shape=circle, fontsize=10, label=\"%s\"];\n",
									  pszHidCurrent, bufLabel)  );
		SG_ERR_CHECK(  SG_sprintf(pCtx, bufGen, sizeof(bufGen), "gen_%08d", genCurrent)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
								  "    { rank = same; %s; _%s; }\n",
								  bufGen, pszHidCurrent)  );
		SG_ERR_CHECK(  SG_rbtree__update(pCtx, pGraphData->prbGenerationList, bufGen)  );

		SG_ERR_CHECK(  SG_rbtree__add(pCtx, pGraphData->prbLabeledNodes, pszHidCurrent)  );
    }

	SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pDagnodeCurrent, &count_parents, &parents)  );
	if (parents)
	{
        SG_uint32 i = 0;
        for (i=0; i<count_parents; i++)
		{
			// parents[] contains the edges from this node to the parent node(s).
			// make the edges that eventually lead to the LCA as RED.
			const char * pszColor = _get_edge_color(pCtx, pGraphData, pszHidCurrent, parents[i]);

			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "_%s -> _%s [color=\"%s\"];\n",
									  pszHidCurrent, parents[i], pszColor)  );
		}
	}

fail:
	SG_NULLFREE(pCtx, pszTag);
}

static void _make_generation_column(SG_context * pCtx,
									struct _graph_data * pGraphData)
{
	const char * pszPrev = NULL;
	SG_rbtree_iterator * pIter = NULL;
	const char * psz_k;
	SG_bool b;

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter, pGraphData->prbGenerationList, &b, &psz_k, NULL)  );
	while (b)
	{
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "    %s [shape=plaintext, fontsize=8];\n", psz_k)  );

		if (pszPrev)
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "    %s -> %s;\n", psz_k, pszPrev)  );
		pszPrev = psz_k;

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter, &b, &psz_k, NULL)  );
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
}

static void _create_graph(SG_context * pCtx,
						  SG_repo * pRepo,
						  SG_daglca * pDagLca,
						  SG_uint64 uiDagNum,
						  SG_bool bVerbose)
{
	struct _graph_data graph_data;

	memset(&graph_data, 0, sizeof(graph_data));
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &graph_data.prbLabeledNodes)  );
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &graph_data.prbGenerationList)  );
	graph_data.pRepo = pRepo;
	graph_data.pDagLca = pDagLca;
	graph_data.uiDagNum = uiDagNum;
	SG_ERR_CHECK(  SG_daglca__iterator__first(pCtx, NULL, pDagLca, SG_FALSE,
											  &graph_data.pszHidLCA,
											  NULL, NULL, NULL)  );

	// iterate thru the SIGNIFICANT nodes and create stylized the boxes for them by type.

	SG_ERR_CHECK(  SG_daglca__foreach(pCtx, pDagLca, SG_TRUE, _add_significant_node_no_edges_cb, &graph_data)  );

	if (bVerbose)
	{
		// plot all nodes that the daglca code needed to fetch to compute the LCA.
		// this may show a few more nodes that the ones strictly between the leaves
		// and lca, but it's interesting to see the fringe and see how much extra
		// work we did.

		SG_ERR_CHECK(  SG_daglca_debug__foreach_visited_dagnode(pCtx, pDagLca, _fill_verbose_graph_cb, &graph_data)  );
	}
	else
	{
		// draw the edges for only the reduced graph.

		SG_ERR_CHECK(  SG_daglca__foreach(pCtx, pDagLca, SG_TRUE, _add_significant_edges_cb, NULL)  );
	}
		
	// create generation column so that nodes are vertially lined up.

	SG_ERR_CHECK(  _make_generation_column(pCtx, &graph_data)  );

fail:
	SG_RBTREE_NULLFREE(pCtx, graph_data.prbLabeledNodes);
	SG_RBTREE_NULLFREE(pCtx, graph_data.prbGenerationList);
}

//////////////////////////////////////////////////////////////////

static void do_dump_lca(SG_context * pCtx, SG_option_state * pOptSt, SG_repo * pRepo, SG_uint64 uiDagNum)
{
	SG_stringarray* psaFullHids = NULL;
	SG_stringarray* psaMissing = NULL;
    SG_rbtree* prbLeaves = NULL;
	SG_daglca * pDagLca = NULL;
	char * pszRevFullHid = NULL;
	SG_rbtree_iterator * pIter = NULL;
	SG_daglca_iterator * pIterDagLca = NULL;
	const char * pszKey;
	SG_bool bFound;
	SG_uint32 countRevSpecs = 0;
	SG_uint32 countActualRevs = 0;
	SG_string * pString = NULL;

	SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &countRevSpecs)  );
	SG_ERR_CHECK(  SG_rev_spec__set_dagnum(pCtx, pOptSt->pRevSpec, uiDagNum)  );

	if (countRevSpecs)
	{

		if (uiDagNum != SG_DAGNUM__VERSION_CONTROL)
		{
			SG_bool bVcOnly = SG_FALSE;
			SG_ERR_CHECK(  SG_rev_spec__has_non_vc_specs(pCtx, pOptSt->pRevSpec, &bVcOnly)  );
			if (bVcOnly)
			{
				SG_ERR_THROW2(  SG_ERR_USAGE,
					(pCtx, "The --tag and --branch are only valid for the Version Control DAG.")  );
			}
		}

		// filter-out/disregard any csets not present in the local repo.
		SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pRepo, pOptSt->pRevSpec, SG_TRUE, &psaFullHids, &psaMissing)  );
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaFullHids, &countActualRevs)  );
		if (countActualRevs < 2)
			SG_ERR_THROW2(  SG_ERR_USAGE, (pCtx, "At least 2 revisions must be specified.")  );

		SG_ERR_CHECK(  SG_stringarray__to_rbtree_keys(pCtx, psaFullHids, &prbLeaves)  );
	}
	else
	{
		// use the full set of all current leaves in the dag.
		SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, uiDagNum, &prbLeaves)  );
		SG_ERR_CHECK(  SG_rbtree__count(pCtx, prbLeaves, &countActualRevs)  );
		if (countActualRevs < 2)
			SG_ERR_THROW2(  SG_ERR_USAGE, (pCtx, "The DAG has only one leaf. At least two are necessary to calculate an LCA.")  );
	}

    SG_ERR_CHECK(  SG_daglca__alloc(pCtx, &pDagLca, uiDagNum, my_fetch_dagnodes_from_repo, pRepo)  );
    SG_ERR_CHECK(  SG_daglca__add_leaves(pCtx, pDagLca, prbLeaves)  );
	SG_ERR_CHECK(  SG_daglca__compute_lca(pCtx, pDagLca)  );

	if (pOptSt->bList)
	{
		// write a simple list of the significant nodes

		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
		SG_ERR_CHECK(  SG_daglca__format_summary(pCtx, pDagLca, pString)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s", SG_string__sz(pString))  );
	}
	else
	{
		// --verbose or no parameter (meaning distilled graph)
		//
		// write a version of the DAG graph in graphiz DOT format.

		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "digraph %s\n", "dump_lca")  );	// TODO 2011/03/14 convert dagnum into string
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "{\n")  );

		// write legend vertically down the edge

		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "    { node [shape=plaintext, fontsize=16]; Leaves -> LCA; }\n")  );

		// write the leaves/heads across the top

		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "    { rank = same;\n")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "      Leaves;\n")  );
		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter, prbLeaves, &bFound, &pszKey, NULL)  );
		while (bFound)
		{
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "       _%s;\n", pszKey)  );
			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter, &bFound, &pszKey, NULL)  );
		}
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "    };\n")  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "\n")  );

		// write all of the nodes and edges to the graph.

		SG_ERR_CHECK(  _create_graph(pCtx, pRepo, pDagLca, uiDagNum, pOptSt->bVerbose)  );

		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "}\n")  );
	}

fail:
    SG_DAGLCA_NULLFREE(pCtx, pDagLca);
	SG_DAGLCA_ITERATOR_NULLFREE(pCtx, pIterDagLca);
    SG_RBTREE_NULLFREE(pCtx, prbLeaves);
	SG_NULLFREE(pCtx, pszRevFullHid);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
	SG_STRINGARRAY_NULLFREE(pCtx, psaFullHids);
	SG_STRINGARRAY_NULLFREE(pCtx, psaMissing);
	SG_STRING_NULLFREE(pCtx, pString);
}

void do_cmd_dump_lca(SG_context * pCtx, SG_option_state * pOptSt)
{
	SG_repo * pRepo = NULL;

	SG_ERR_CHECK(  SG_cmd_util__get_repo_from_options_or_cwd(pCtx, pOptSt, &pRepo, NULL)  );

#if defined(DEBUG)
	SG_ERR_CHECK(  do_dump_lca(pCtx, pOptSt, pRepo,
							   ((pOptSt->bGaveDagNum) ? pOptSt->ui64DagNum : SG_DAGNUM__VERSION_CONTROL))  );
#else
	SG_ERR_CHECK(  do_dump_lca(pCtx, pOptSt, pRepo, SG_DAGNUM__VERSION_CONTROL)  );
#endif

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}

