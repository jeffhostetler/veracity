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
 * @file sg_daglca.c
 *
 * NOTE: the generation is now a SG_int32 rather than a SG_int64.
 *
 *================================================================
 *
 * TODO see if we want to change the work-queue key to <-gen>.<static++>
 *
 *================================================================
 *
 * TODO think about whether it would be helpful for us to include
 * TODO info on criss-cross-peers when we iterate over SPCAs as we
 * TODO return the results.
 *
 *        NOTE: Here's how to do this.  The problem should be restated
 *        to say which nodes have multiple significant ancestors; that
 *        is significant ancestors that are effectively peers from the
 *        point of view of any given node.
 *
 *        Add a bitvector for immediate-ancestors and a counter to
 *        count the number of immediate-ancestors.
 *
 *        After the computation is completed and _fixup_result_map()
 *        has been called.  We then visit each significant node (either
 *        using the aData array or the sorted-result map) and OR our
 *        bit into each descendant's ancestor bitvector and increment
 *        the count.
 *
 *        When finished, nodes which have more than one ancestor bit
 *        (or ancestor-count > 1) are the result of some form of
 *        criss-cross.
 *
 *================================================================
 *
 * @details Routine/Class to determine the DAGNODE which is the
 * Least Common Ancestor (LCA) on a given set of DAGNODEs in a DAG.
 *
 * Our callers need the LCA to allow them to perform a MERGE operation.
 * For our purposes here, we describe this operation as:
 *
 * M := MERGE(dag, Am, {L0,L1,...,Ln-1})
 *
 * That is, a new dagnode M is the result of "merging" n leaf dagnodes
 * (named L0, L1, thru Ln-1) and some yet-to-be-determined LCA Am which
 * is defined as:
 *
 * Am := LCA(dag, {L0,L1,...,Ln-1})
 *
 * In the simple case, this LCA is unique and can then be used as the
 * ancestor in a traditional 3-way merge using a tool like DiffMerge.
 * However, this summary over-simplifies the problem.
 *
 * The literature talks about this problem as finding the LCA of 2
 * nodes.  We want to solve it for n nodes and we want to be aware of
 * the criss-cross merge problem.  (http://revctrl.org/CrissCrossMerge)
 *
 *================================================================
 *
 * Let me define a "Partial" Common Ancestor (PCA) as a common ancestor
 * of 2 or more leaves (a subset of the complete set of input leaves).
 *
 * Let me define a "Significant" Partial Common Ancestor (SPCA) as:
 * [SPCA1] one of (possibly many) PCAs where all of the leaves in the
 *         subset came together for the first time.  That is, the branch
 *         point that is "closest" to the leaves in question.
 * [SPCA2] or one where "peer" SPCAs and/or leaves came together for the
 *         first time.  (needed for criss-cross)
 *
 * For example, consider:
 *
 *                     A
 *                    / \.
 *                   /   \.
 *                  /     \.
 *                 /       \.
 *                /         \.
 *               /           \.
 *              B             C
 *             / \           / \.
 *            /   \         /   \.
 *           /     D       E     F
 *          /       \     /      /\.
 *         /         \   /      /  \.
 *        /           \ /      /    \.
 *       G             H      I      J
 *        \           / \     |\    /|
 *         \         /   \    | \  / |
 *          \       /     \   |  \/  |
 *           \     /       \  |  /\  |
 *            \   /         \ | /  \ |
 *             \ /           \|/    \|
 *              L0            L1    L2
 *
 *                 Figure 1.
 *
 * [I use "\." in the figure rather than a plain backslash to keep
 * the compiler from complaining.]
 *
 *    SPCA(dag, {L0,L1}) --> {H}
 *     LCA(dag, {L0,L1}) --> {H}
 * nonSPCA(dag, {L0,L1}) --> {A,B,C,D,E}.
 *
 *    SPCA(dag, {L1,L2}) --> {I,J, F*}
 *     LCA(dag, {L1,L2}) --> {F}
 * nonSPCA(dag, {L1,L2}) --> {A,C}.
 *
 *    SPCA(dag, {L0,L1,L2}) --> {C,F,H,I,J}
 *     LCA(dag, {L0,L1,L2}) --> {C}
 * nonSPCA(dag, {L0,L1,L2}) --> {A}.
 *
 * *F is special because of criss-cross merge.  This is somewhat like
 * a recursive SPCA(dag, {I,J}) --> {F}.
 *
 * When the SPCA of a set of leaves contains more than one item, it
 * implies that there is some type of criss-cross peering.
 *
 * The absolute LCA is defined as the single SPCA over the entire
 * set of input leaves.
 *
 * The set of input leaves and SPCAs define the "essense" and/or
 * topology of the dag (FROM THE POINT OF VIEW OF THOSE LEAVES).
 * Other dagnodes may be elided from the dag.  For example, in the
 * above graph we may have 1000 commits/branches/merges between G
 * and L0 and as long as there are no edges from them to anything
 * else in the dag, we can ignore them here.
 *
 *================================================================
 *
 * Let me define a "Significant" Node (SN) as a leaf or one of
 * these SPCAs.
 *
 * For the purposes of discussion, we assign each SN an ItemIndex
 * as we find it.  Leaves are given index values 0..n-1, SPCAs
 * are assigned values starting with n.  The root-most SPCA will
 * be the LCA.
 *
 * We use the ItemIndex as subscripts into 3 BitVectors:
 *
 * [a] bvLeaf - for Leaf node k (Lk), it contains {k}; for non-leaf
 *              nodes it is empty.
 *
 * [b] bvImmediateDescendants - for any given node this contains the
 *              indexes of the SNs that are immediately below it.
 *              When we distill the dag down to its essense (and
 *              get rid of irrelevant nodes), these nodes are
 *              immediate children of this node.
 *
 * [c] bvClosure - for any given node this contains the indexes of
 *              ALL SNs that are below this node.
 *
 * We can use this information if we want to build a new
 * DAG-LCA-GRAPH that contains only the significant nodes.
 * In this graph, links would go downward from the root/LCA
 * to the leaves.  This graph would capture the *ESSENTIAL* structure
 * of the full dag FROM THE POINT OF VIEW OF THE SET OF LEAVES.
 *
 *================================================================
 *
 * [1] In the 2-way case, we have 2 different graph topologies that we
 *     need to address:
 *
 *     [1a] 2-Way-Normal
 *
 *                  A
 *                 / \.
 *               [X] [Y]
 *               /     \.
 *             L0       L1
 *
 *          Let [X] and [Y] be an arbitrary graph fragments that do
 *          not intersect.  So, from the point of view of {L0,L1}, we
 *          can reduce this to:
 *
 *                  A
 *                 / \.
 *               L0   L1
 *
 *          So, in this case the value is:
 *
 *          SPCA(dag, {L0,L1}) --> {A}
 *           LCA(dag, {L0,L1}) --> {A}.
 *
 *     [1b] Criss-Cross
 *
 *          [I've eliminated the [Bx] and [Cy] that would really clutter
 *          up this graph, but they are there and must be addressed.]
 *
 *                  A
 *                 / \.
 *               B0   C0
 *                |\ /|
 *                | X |
 *                |/ \|
 *               B1   C1
 *                |\ /|
 *                | X |
 *                |/ \|
 *               B2   C2
 *                |\ /|
 *                | X |
 *                |/ \|
 *               L0   L1
 *
 *          In this case L0 and L1 have 2 common ancestors B2 and C2.
 *          They have 2 common ancestors B1 and C1 and so on.  B0 and C0
 *          have common ancestor A.  So the value of LCA(dag, {L0,L1})
 *          ***depends upon what you want it to be***.
 *
 *          The problem is that the typical auto-merge code ***may*** generate
 *          different results for
 *                 MERGE(dag, B2, {L0,L1})
 *             and MERGE(dag, C2, {L0,L1}).
 *
 *          And if we auto-merge with MERGE(dag, A, {L0,L1}), the user ***may***
 *          have to re-resolve any conflicts that they already addressed when
 *          they created B1, C1, B2, or C2.
 *
 *          So using the (somewhat recursive) definition:
 *
 *          SPCA(dag, {L0,L1}) --> {B2,C2, B1*,C1*, B0**,C0**, A***}.
 *           LCA(dag, {L0,L1}) --> {A}.
 *
 *     [1c] Extra branches.
 *
 *          Consider the following case:
 *
 *                   A0
 *                  / \.
 *                 /   \.
 *                /     \.
 *               /       \.
 *              /         \.
 *             B           C
 *            / \         / \.
 *           /   \       /   \.
 *          /     \     /     \.
 *         /       \   /       \.
 *        /         \ /         \.
 *      [X0]         A1         [Y0]
 *        \         / \         /
 *         \       /   \       /
 *          \   [X1]   [Y1]   /
 *           \   /       \   /
 *            \ /         \ /
 *             L0          L1
 *
 *          CLAIM: The LCA of {L0,L1} is {A1}.
 *
 *                 One could argue that we should consider B and C as peer
 *                 SPCAs of {L0,L1} because of the effects of [X0] and [Y0],
 *                 respectively.  And therefore, the LCA would be {A0}.
 *
 *                 But I claim that the changes in [X0] and [Y0] are just
 *                 noise in this graph and could be considered as if they were
 *                 part of [X1] and [Y1], respectively.
 *
 *                 That is, from an LCA-perspective we could think of this as:
 *
 *                   A0
 *                  / \.
 *                 /   \.
 *                /     \.
 *               B       C
 *                \     /
 *                 \   /
 *                  \ /
 *                   A1
 *                  / \.
 *                 /   \.
 *               [X]   [Y]
 *               /       \.
 *              /         \.
 *             L0          L1
 *
 *                 This makes B and C non-significant (nonSPCAs) rather than
 *                 SPCAs.
 *
 *          TODO: Think about this with some actual test data.
 *
 *     [1d] A leaf which is an ancestor of the other leaf.
 *
 *          Consider the following case:
 *
 *                  A
 *                  |
 *                  L0
 *                  |
 *                  L1
 *
 *          One could argue that we should just return {L0} as the LCA.
 *          But that might cause the merge code to force the user thru
 *          the trivial merge:
 *
 *               MERGE(dag, L0, {L0,L1}) --> L1
 *
 *          I think it would be better to stop the music as soon as we
 *          detect this condition.
 *
 *          NOTE 2012/09/04 When we have this case our caller might
 *          NOTE            want to look at doing a fast-forward merge.
 *
 * [2] In the n-way case, there are several things to consider:
 *
 *     [2a] Does the n-way merge produce one result dagnode M or a series
 *          of intermediate merges and one final merge?  For example:
 *
 *                  A
 *                 / \.
 *                B   C
 *               / \   \.
 *             L0   L1  L2
 *               \  |  /
 *                \ | /
 *                 \|/
 *                  M
 *
 *          versus:
 *
 *                  A
 *                 / \.
 *                B   C
 *               / \   \.
 *             L0   L1  L2
 *               \ /   /
 *                m0  /
 *                 \ /
 *                  M
 *
 *          I believe that GIT does the former with its OCTOPUS merge.
 *          I know that Monotone does the latter.
 *
 *          We don't care how the caller does the merge and whether or
 *          not it generates only an M dagnode or both M and m0 dagnodes
 *          or whether it does a combination of the 2 using tempfiles to
 *          simulate m0 and only generate M.
 *
 *          Our only concern here is that we provide enough information
 *          for the caller to decide the "best possible" merge.  It is
 *          free to use or ignore any SPCAs that we may provide.
 *
 *     [2b] Do we report only the "absolute" LCA or also SPCAs?
 *
 *          It is possible that the "correct" answer depends upon what the
 *          caller wants to try to do.  In the graph in [2a], if we have a
 *          bunch of version controlled files and some have been modified
 *          in L0 and L1 and some in L0 and L2, we may want to show the user
 *          several invocations of DiffMerge using B as the SPCA for some
 *          files and A as the LCA for the other files.  For files that were
 *          modified in L0, L1, and L2, we may want to show 2 invocations of
 *          DiffMerge in series first with B and then with A.  Even that may
 *          depend on whether the changes within the files are orthogonal.
 *
 *          So, we have:
 *
 *          SPCA(dag, {L0,L1,L2}) --> {A,B}
 *           LCA(dag, {L0,L1,L2}) --> {A}.
 *
 *     [2c] Sorting SPCAs.
 *
 *          When we do present SPCAs, we should give an ordering so
 *          that the caller can sanely present them to the user.  For
 *          example:
 *
 *                  A
 *                 / \.
 *                /   \.
 *               /     \.
 *              /       \.
 *             B         C
 *            / \       / \.
 *          L0   L1   L2   L3
 *
 *          The caller will probably want to do this as:
 *
 *               MERGE(dag, B, {L0,L1}) --> m0
 *               MERGE(dag, C, {L2,L3}) --> m1
 *               MERGE(dag, A, {m0,m1}) --> M
 *
 *          rather than:
 *
 *               MERGE(dag, A, {L0,L1}) --> m0
 *               MERGE(dag, A, {m0,L2}) --> m1
 *               MERGE(dag, A, {m1,L3}) --> M
 *
 *          or some other permutation.
 *
 *     [2d] n-way with Criss-Crosses
 *
 *          Things get really funky when we have a criss-cross and
 *          extra leaves threading into the mess.
 *
 *                  A
 *                 / \.
 *               B0   C0
 *                |\ /|\.
 *                | X | \.
 *                |/ \|  \.
 *               B1   C1  L3
 *                |\ /|\.
 *                | X | \.
 *                |/ \|  \.
 *               B2   C2  L2
 *                |\ /|
 *                | X |
 *                |/ \|
 *               L0   L1
 *
 *          TODO describe what we want to have happen here.
 *
 *     [2e] another form of criss-cross
 *
 *          things also get a little confusing when we have
 *          something like this where L1 has multiple significant
 *          peer ancestors and we want to merge {L0,L1,L2} in one
 *          step.
 *
 *                     A
 *                    / \.
 *                   /   \.
 *                  /     \.
 *                 /       \.
 *                /         \.
 *               /           \.
 *              B             C
 *             / \           / \.
 *            /   \         /   \.
 *           /     D       E     F
 *          /       \     /       \.
 *         /         \   /         \.
 *        /           \ /           \.
 *       G             H             J
 *        \           / \           / \.
 *         \         /   \         /   \.
 *          \       /     \       /     \.
 *           \     /       \     /       \.
 *            \   /         \   /         \.
 *             \ /           \ /           \.
 *              L0            L1            L2
 *
 *
 * [3] Multiple initial checkins.
 *
 *     As discussed in sg_dag_sqlite3.c, we have to allow for multiple
 *     initial checkins.  This causes us to have (potentially) a forest
 *     of non-connected dags -or- (potentially) a (somewhat) connected
 *     dag with an "implied" null root.
 *
 *     A merge across disconnected pieces would make the null root the
 *     LCA.
 *
 *                A0     A1
 *               / \     |
 *              B   C    X
 *             / \       |
 *            L0 L1      L2
 *
 *     LCA(dag, {L0,L1})    --> {B}
 *     LCA(dag, {L0,L1,L2}) --> {NULLROOT}
 *
 *
 *     Likewise, with a connected dag:
 *
 *                A0     A1
 *               / \     |
 *              B   C    X
 *             / \   \  / \.
 *            L0 L1   L2   L3
 *
 *     LCA(dag, {L0,L1})       --> {B}
 *     LCA(dag, {L0,L1,L2})    --> {A0}
 *     LCA(dag, {L2,L3})       --> {X}
 *     LCA(dag, {L0,L1,L2,L3}) --> {NULLROOT}
 *
 *     9/21/09 UPDATE: I don't think we have to worry about this case
 *                     on properly created/initialized REPOs because we
 *                     now create a trivial/initial checkin as soon as
 *                     the REPO is created.  In some of our unittests
 *                     before vv was available (where we created REPOs
 *                     and CSETs in separate steps), we were able to exploit
 *                     this.
 *
 *                     TODO go thru and remove some of the asserts and
 *                     TODO stuff that deals with nullroots (probably have
 *                     TODO to fix some unit tests first).
 *
 * TODO to be continued...
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
#define TRACE_DAGLCA			0
#define TRACE_DAGLCA_SUMMARY	0
#else
#define TRACE_DAGLCA			0
#define TRACE_DAGLCA_SUMMARY	0
#endif

//////////////////////////////////////////////////////////////////

/**
 * Instance data to associate with the dagnodes that we have
 * fetched from disk as we walk the dag.
 */
struct _my_data
{
	SG_dagnode *			pDagnode;
	SG_int32				genDagnode;

	// the ItemIndex of this dagnode if it is a Significant Node.
	// Otherwise, -1.

	SG_int32				itemIndex;
	SG_daglca_node_type		nodeType;

	// if this node is a leaf, bvLeaf contains a single 1 bit
	// that corresponds to the itemIndex.  that is, bvLeaf[itemIndex] is 1.
	// it also means that daglca.aData_SignificantNodes[itemIndex] contains
	// this node.

	SG_bitvector *			pbvLeaf;

	// when a node is marked significant (and itemIndex is set >= 0)
	// we get bit defined for it.  for leaf nodes, this is the same
	// as pbvLeaf; for SPCA/LCA nodes, this will be different.

	SG_bitvector *			pbvMySignificantBit;

	// a bitvector of the significant nodes that are the immediate descendants
	// of this node.  (this can be converted into child-links in the lca
	// essense graph.)

	SG_bitvector *			pbvImmediateDescendants;

	// a bitvector of all significant nodes that are descendants of this node
	// (and this node itself).

	SG_bitvector *			pbvClosure;
};

typedef struct _my_data my_data;

/**
 * SG_daglca is a structure that we use to collection information
 * on a portion of the dag and compute common ancestor(s).  This is
 * our workspace as we compute.
 */
struct _SG_daglca
{
	// a callback to do the work of actually fetching a dagnode from disk.

	FN__sg_fetch_dagnode *	pFnFetchDagnode;
	void *					pVoidFetchDagnodeData;

	SG_uint32				nrLeafNodes;
	SG_uint32				nrSignificantNodes;
	SG_bitvector *			pbvClaimedMask;		// tells which bits are in use

	SG_bool					bFrozen;

    SG_uint64               iDagNum;

	// pRB_FetchedCache is a cache of all of the fetched nodes.
	// It owns the associated my_data structures (which in turn,
	// own all the individual dagnodes).
	//
	// The key is the HID of the dagnode.
	//
	// Think of this as map< dn.hid, my_data * >

	SG_rbtree *				pRB_FetchedCache;

	// pRB_SortedWorkQueue is an ordered to-do list.  It is a
	// sorted list of nodes that we should visit.
	//
	// It ***DOES NOT*** own the associated data pointers;
	// it borrows them from pRB_Fetched.
	//
	// The key is a combination of the generation and HID
	// of the dagnode, such that the deepest nodes in the
	// DAG are listed first.
	//
	// Note: we don't need the second part of the key to be
	// the HID, it could be a static monotonically increasing
	// sequence number.  (anything to give us a unique value.)
	//
	// Think of this as map< (-dn.generation,dn.hid), my_data * >

	SG_rbtree *				pRB_SortedWorkQueue;

	// pRB_SortedResults is a sorted list of the resulting
	// Significant Nodes.  It sorted by generation so that the
	// absolute LCA should be first.   SPCAs and Leaves will
	// follow but may be interleaved depending on the individual
	// depths in the tree.  Nodes at the same depth will be in
	// random order.
	//
	// It ***DOES NOT*** own the associated data pointers;
	// it borrows them from pRB_Fetched.
	//
	// The key is a combination of the generation and HID
	// of the dagnode, such that the shallowest nodes in the
	// DAG are listed first.
	//
	// Think of this as map< (+dn.generation,dn.hid), my_data * >

	SG_rbtree *				pRB_SortedResultMap;

	// a fixed-length vector containing only Significant Nodes.
	//
	// we do not own the my_data structures; they are borrowed
	// from pRB_Fetched.
	//
	// We use this as a map from an itemIndex to a pointer.
	//
	// Nodes are added to this list as we identify that they
	// are significant.  This matches the bitvector ordering.
	// So aData[0] corresponds to bit[0].
	//
	// This ordering is of *NO* use to the caller.

	SG_vector *				pvecSignificantNodes;	// array[ my_data * ]	does not own them.
};

//////////////////////////////////////////////////////////////////

static void _my_data__free(SG_context * pCtx, my_data * pData)
{
	if (!pData)
		return;

	SG_DAGNODE_NULLFREE(pCtx, pData->pDagnode);

	SG_BITVECTOR_NULLFREE(pCtx, pData->pbvLeaf);
	SG_BITVECTOR_NULLFREE(pCtx, pData->pbvMySignificantBit);
	SG_BITVECTOR_NULLFREE(pCtx, pData->pbvImmediateDescendants);
	SG_BITVECTOR_NULLFREE(pCtx, pData->pbvClosure);

	SG_NULLFREE(pCtx, pData);
}
static void _my_data__free__cb(SG_context * pCtx, void * pVoid)
{
	_my_data__free(pCtx, (my_data *)pVoid);
}

#if defined(DEBUG) || defined(TRACE_DAGLCA)
void _my_data_debug__dump_to_console(SG_context * pCtx, const my_data * pData, const char * pszLabel)
{
	const char * pszHid;
	char * psz_leaf = NULL;
	char * psz_immediate_descendants = NULL;
	char * psz_closure = NULL;
	char * psz_my_significant_bit = NULL;
	SG_uint32 nrParents;

	SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pData->pDagnode, &pszHid)  );
	SG_ERR_CHECK(  SG_dagnode__count_parents(pCtx, pData->pDagnode, &nrParents)  );

	SG_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pData->pbvLeaf, &psz_leaf)  );
	SG_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pData->pbvImmediateDescendants, &psz_immediate_descendants)  );
	SG_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pData->pbvClosure, &psz_closure)  );

	if (pData->itemIndex < 0)
		psz_my_significant_bit = NULL;
	else
		SG_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pData->pbvMySignificantBit, &psz_my_significant_bit)  );
	
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "%30s: %s [gen %8d][#par %2d][ndx %3d][type %d] [bvL '%s...'][bvMe '%s...'][bvID '%s...'][bvC '%s...'] %s\n",
							   "TraceDagLca.my_data",
							   pszHid,
							   pData->genDagnode,
							   nrParents,
							   pData->itemIndex,
							   (SG_uint32)pData->nodeType,
							   psz_leaf,
							   ((psz_my_significant_bit) ? psz_my_significant_bit : ""),
							   psz_immediate_descendants,
							   psz_closure,
							   pszLabel)  );
fail:
	SG_NULLFREE(pCtx, psz_leaf);
	SG_NULLFREE(pCtx, psz_immediate_descendants);
	SG_NULLFREE(pCtx, psz_closure);
	SG_NULLFREE(pCtx, psz_my_significant_bit);
}
#endif

//////////////////////////////////////////////////////////////////

/**
 * Allocate a my_data structure for this dagnode.  Add it to the
 * fetched-cache.  if dagnode is null we assume the implied null
 * root node.
 *
 * The fetched-cache takes ownership of both the dagnode and the
 * my_data structure.
 *
 * We only populate the basic fields in my_data; WE DO NOT SET
 * ANY OF THE BIT-VECTOR OR ITEM-INDEX FIELDS.
 */
static void _cache__add(SG_context * pCtx,
						SG_daglca * pDagLca,
						SG_dagnode * pDagnode,	// if we are successful, you no longer own this.
						my_data ** ppData)		// you do not own this.
{
	const char * szHid;
	my_data * pDataAllocated = NULL;
	my_data * pDataInstalled = NULL;
	my_data * pOldData = NULL;

	SG_NULLARGCHECK_RETURN(pDagLca);
	SG_NULLARGCHECK_RETURN(ppData);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pDataAllocated)  );

	pDataAllocated->itemIndex = -1;
	pDataAllocated->nodeType = SG_DAGLCA_NODE_TYPE__NOBODY;

	SG_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pDataAllocated->pbvLeaf, 64)  );
	SG_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pDataAllocated->pbvImmediateDescendants, 64)  );
	SG_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pDataAllocated->pbvClosure, 64)  );
	pDataAllocated->pbvMySignificantBit = NULL;		// defer this
	
	SG_ASSERT(  (pDagnode)  );

	SG_ERR_CHECK(  SG_dagnode__get_generation(pCtx,pDagnode,&pDataAllocated->genDagnode)  );
	SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx,pDagnode,&szHid)  );

	SG_ERR_CHECK(  SG_rbtree__update__with_assoc(pCtx,
												 pDagLca->pRB_FetchedCache,szHid,
												 pDataAllocated,(void **)&pOldData)  );
	pDataInstalled = pDataAllocated;
	pDataAllocated = NULL;

	pDataInstalled->pDagnode = pDagnode;

	// fetched-cache now owns pData and pDagnode.

	SG_ASSERT_RELEASE_RETURN2(  (!pOldData),
						(pCtx,"Possible memory leak adding [%s] to daglca cache.",szHid)  );

	*ppData = pDataInstalled;
	return;

fail:
	SG_ERR_IGNORE(  _my_data__free(pCtx, pDataAllocated)  );
}

static void _cache__lookup(SG_context * pCtx,
						   const SG_daglca * pDagLca, const char * szHid,
						   SG_bool * pbAlreadyPresent,
						   my_data ** ppData)	// you do not own this; don't free it.
{
	// see if szHid is present in cache.  return the associated my_data structure.

	SG_bool bFound;
	my_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pDagLca);
	SG_NULLARGCHECK_RETURN(ppData);
	SG_NULLARGCHECK_RETURN(pbAlreadyPresent);
	SG_NULLARGCHECK_RETURN(szHid);

	SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx, pDagLca->pRB_FetchedCache,szHid,&bFound,(void **)&pData)  );

	*pbAlreadyPresent = bFound;
	*ppData = ((bFound) ? pData : NULL);
}

//////////////////////////////////////////////////////////////////

static void _work_queue__insert(SG_context * pCtx,
								SG_daglca * pDagLca, my_data * pData)
{
	// insert an entry into sorted work-queue for the my_data structure
	// associated with a dagnode.
	//
	// the work-queue contains a list of the dagnodes that we need
	// to examine.  we seed it with the set of n leaves.  it then
	// evolves as we walk the parent links in each node.  it maintains
	// the "active frontier" of the dag that we are exploring.
	//
	// to avoid lots of O(n^2) and worse problems, we maintain the
	// queue sorted by dagnode depth/generation in the dag.  this
	// allows us to visit nodes in an efficient order.
	//
	// the sort key looks like <-generation>.<hid> "%08x.%s"
	//
	// Note: we don't need the second part of the key to be
	// the HID, it could be a static monotonically increasing
	// sequence number.  (anything to give us a unique value.)
	// I'm going to use the HID for debugging.

	char bufSortKey[SG_HID_MAX_BUFFER_LENGTH + 20];
	const char * szHid;

	SG_ASSERT(  (pData->pDagnode)  );

	SG_ERR_CHECK_RETURN(  SG_dagnode__get_id_ref(pCtx,pData->pDagnode,&szHid)  );
	SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx,
									 bufSortKey,sizeof(bufSortKey),
									 "%08x.%s", (-pData->genDagnode), szHid)  );

	// the work-queue ***DOES NOT*** take ownership of pData; it
	// borrows pData from the fetched-cache which owns all pData's.

	SG_ERR_CHECK_RETURN(  SG_rbtree__update__with_assoc(pCtx,
														pDagLca->pRB_SortedWorkQueue,
														bufSortKey,
														pData,NULL)  );
}

static void _work_queue__remove_first(SG_context * pCtx,
									  SG_daglca * pDagLca,
									  SG_bool * pbFound,
									  my_data ** ppData)	// you do not own this
{
	// remove the first element in the work-queue.
	// this should be one of the deepest nodes in the frontier
	// of the dag that we have yet to visit.
	//
	// you don't own the returned pData (because the work-queue
	// borrowed it from the fetched-queue).

	// TODO we should add a SG_rbtree__remove__first__with_assoc().
	// TODO that would save a full lookup.

	const char * szKey;
	my_data * pData;
	SG_bool bFound;

	SG_ERR_CHECK_RETURN(  SG_rbtree__iterator__first(pCtx,
													 NULL,pDagLca->pRB_SortedWorkQueue,
													 &bFound,
													 &szKey,
													 (void**)&pData)  );
	if (bFound)
		SG_ERR_CHECK_RETURN(  SG_rbtree__remove(pCtx,
												pDagLca->pRB_SortedWorkQueue,szKey)  );

	*pbFound = bFound;
	*ppData = ((bFound) ? pData : NULL);
}

//////////////////////////////////////////////////////////////////

static void _result_map__insert(SG_context * pCtx,
								SG_daglca * pDagLca, my_data * pData)
{
	// insert an entry into the sorted-result map for the my_data structure
	// associated with a dagnode.
	//
	// the sorted-result map contains a list of the Significant Nodes
	// that we have identified.  it is ordered by generation so that
	// the absolute LCA should be first when we are finished walking
	// the dag.
	//
	// the sort key looks like <+generation>.<hid> "%08x.%s"
	//
	// Note: we don't need the second part of the key to be
	// the HID, it could be a static monotonically increasing
	// sequence number.  (anything to give us a unique value.)
	// I'm going to use the HID for debugging.

	char bufSortKey[SG_HID_MAX_BUFFER_LENGTH + 20];
	const char * szHid;

	SG_NULLARGCHECK_RETURN(pData->pDagnode);

	SG_ERR_CHECK_RETURN(  SG_dagnode__get_id_ref(pCtx,pData->pDagnode,&szHid)  );

	SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx,
									 bufSortKey,sizeof(bufSortKey),
									 "%08x.%s", (pData->genDagnode), szHid)  );

	// the sorted-result map ***DOES NOT*** take ownership of pData; it
	// borrows pData from the fetched-cache which owns all pData's.

	SG_ERR_CHECK_RETURN(  SG_rbtree__update__with_assoc(pCtx,
														pDagLca->pRB_SortedResultMap,
														bufSortKey,
														pData,NULL)  );
}

//////////////////////////////////////////////////////////////////

static void _significant_node_vec__insert_leaf(SG_context * pCtx,
											   SG_daglca * pDagLca,
											   my_data * pData,
											   SG_int32 * pItemIndex)
{
	// claim a spot in the significant-node vector for this leaf.

	SG_int32 kItem;
	SG_uint32 kCheck;

	SG_NULLARGCHECK_RETURN(pDagLca);
	SG_NULLARGCHECK_RETURN(pData);
	SG_NULLARGCHECK_RETURN(pItemIndex);

	SG_ASSERT_RELEASE_RETURN2(  (pData->nodeType==SG_DAGLCA_NODE_TYPE__NOBODY),
						(pCtx,"Cannot insert item twice in daglca.")  );

	kItem = pDagLca->nrSignificantNodes++;
	SG_ERR_CHECK_RETURN(  SG_vector__append(pCtx, pDagLca->pvecSignificantNodes, pData, &kCheck)  );	// borrow pointer
	SG_ASSERT(  ((SG_int32)kCheck == kItem)  );		// i wanted to do a SG_vector__set() to make sure we get the expected cell.
	
	SG_ERR_CHECK_RETURN(  _result_map__insert(pCtx,pDagLca,pData)  );

	pData->itemIndex = kItem;
	pData->nodeType = SG_DAGLCA_NODE_TYPE__LEAF;

	// bvLeaf = (1ULL << kItem)
	SG_ERR_CHECK_RETURN(  SG_bitvector__set_bit(pCtx, pData->pbvLeaf, kItem, SG_TRUE)  );

	// bvMySignificantBit = bvLeaf
	SG_ERR_CHECK_RETURN(  SG_BITVECTOR__ALLOC__COPY(pCtx, &pData->pbvMySignificantBit, pData->pbvLeaf)  );

	// bvImmediateDescendants = 0
	SG_ERR_CHECK_RETURN(  SG_bitvector__zero(pCtx, pData->pbvImmediateDescendants)  );

	// bvClosure = bvLeaf
	SG_ERR_CHECK_RETURN(  SG_bitvector__assign__bv__eq__bv(pCtx, pData->pbvClosure, pData->pbvLeaf)  );

	// claimed-mask tell which bits are currently in use.
	// bvClaimedMask |= bvLeaf;
	SG_ERR_CHECK_RETURN(  SG_bitvector__assign__bv__or_eq__bv(pCtx, pDagLca->pbvClaimedMask, pData->pbvLeaf)  );

	pDagLca->nrLeafNodes++;

#if TRACE_DAGLCA
	SG_ERR_IGNORE(  _my_data_debug__dump_to_console(pCtx, pData, "bottom of __insert_leaf")  );
#endif

	// since we always put the leaves first in the significant-node vector,
	// these counts should match.

	SG_ASSERT_RELEASE_RETURN(  (pDagLca->nrLeafNodes==pDagLca->nrSignificantNodes)  );

	*pItemIndex = kItem;
}

static void _significant_node_vec__insert_spca(SG_context * pCtx,
											   SG_daglca * pDagLca,
											   my_data * pData)
{
	// claim a spot in the significant-node vector for this SPCA.

	SG_int32 kItem;
	SG_uint32 kCheck;
	SG_bool bIsZero;

	SG_NULLARGCHECK_RETURN(pDagLca);
	SG_NULLARGCHECK_RETURN(pData);

	SG_ASSERT_RELEASE_RETURN(  ((pData->nodeType==SG_DAGLCA_NODE_TYPE__NOBODY) && "Cannot insert twice.")  );

	// bIsZero = (bvLeaf == 0)
	SG_ERR_CHECK(  SG_bitvector__operator__bv__eq_eq__zero(pCtx, pData->pbvLeaf, &bIsZero)  );
	SG_ASSERT_RELEASE_RETURN(  (bIsZero && "Leaf node cannot be a parent of another leaf.")  );

	kItem = pDagLca->nrSignificantNodes++;
	SG_ERR_CHECK(  SG_vector__append(pCtx, pDagLca->pvecSignificantNodes, pData, &kCheck)  );	// borrow pointer
	SG_ASSERT(  ((SG_int32)kCheck == kItem)  );		// i wanted to do a SG_vector__set() to make sure we get the expected cell.

	SG_ERR_CHECK(  _result_map__insert(pCtx,pDagLca,pData)  );

	pData->itemIndex = kItem;
	pData->nodeType = SG_DAGLCA_NODE_TYPE__SPCA;

	// we don't set "pData->bvImmediateDescendants" because caller must compute it.

	// by definition, we are defining a new significant-node.
	// bvMySignificantBit = (1ULL << kItem)
	SG_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pData->pbvMySignificantBit, 64)  );
	SG_ERR_CHECK(  SG_bitvector__set_bit(pCtx, pData->pbvMySignificantBit, kItem, SG_TRUE)  );
	
	// we OR this into our closure so that it
	// will trickle up.  BTW, this allows us to handle the criss-cross (as
	// shown in Figure 1 and section [1b]) without needing to go recursive.

	// bvClosure |= bvMySignificantBit
	SG_ERR_CHECK(  SG_bitvector__assign__bv__or_eq__bv(pCtx, pData->pbvClosure, pData->pbvMySignificantBit)  );

	// claimed-mask tell which bits are currently in use.
	// bvClaimedMask |= bvMySignificantBit
	SG_ERR_CHECK(  SG_bitvector__assign__bv__or_eq__bv(pCtx, pDagLca->pbvClaimedMask, pData->pbvMySignificantBit)  );

#if TRACE_DAGLCA
	SG_ERR_IGNORE(  _my_data_debug__dump_to_console(pCtx, pData, "bottom of __insert_spca")  );
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _fetch_dagnode_into_cache_and_queue(SG_context * pCtx,
												SG_daglca * pDagLca,
												const char * szHid,
												my_data ** ppData)
{
	// fetch the dagnode from disk, allocating a dagnode.
	// install it into the fetched-cache, allocating a my_data structure.
	// both of these will be owned by the fetched-cache.
	// add it to the frontier of the work-queue.
	//
	// if szHid is null, we assume the implied null root node.

	SG_dagnode * pDagnodeAllocated = NULL;
	my_data * pDataCached = NULL;

	if (szHid)
		SG_ERR_CHECK(  (*pDagLca->pFnFetchDagnode)(pCtx,
												   pDagLca->pVoidFetchDagnodeData,
												   pDagLca->iDagNum,
												   szHid,
												   &pDagnodeAllocated)  );

	SG_ERR_CHECK(  _cache__add(pCtx,pDagLca,pDagnodeAllocated,&pDataCached)  );
	pDagnodeAllocated = NULL;		// fetched-cache takes ownership of dagnode

	SG_ERR_CHECK(  _work_queue__insert(pCtx,pDagLca,pDataCached)  );
	// we never owned pDataCached.

	*ppData = pDataCached;
	return;

fail:
	SG_DAGNODE_NULLFREE(pCtx, pDagnodeAllocated);
}

//////////////////////////////////////////////////////////////////

void SG_daglca__alloc(SG_context * pCtx,
					  SG_daglca ** ppNew,
					  SG_uint64 iDagNum,
					  FN__sg_fetch_dagnode * pFnFetchDagnode,
					  void * pVoidFetchDagnodeData)
{
	SG_daglca * pDagLca = NULL;

	SG_NULLARGCHECK_RETURN(ppNew);
	SG_NULLARGCHECK_RETURN(pFnFetchDagnode);

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pDagLca)  );

    pDagLca->iDagNum = iDagNum;
	pDagLca->pFnFetchDagnode = pFnFetchDagnode;
	pDagLca->pVoidFetchDagnodeData = pVoidFetchDagnodeData;

	pDagLca->nrLeafNodes = 0;
	pDagLca->nrSignificantNodes = 0;
	pDagLca->bFrozen = SG_FALSE;

	SG_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pDagLca->pbvClaimedMask, 64)  );

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx,&pDagLca->pRB_FetchedCache)  );
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx,&pDagLca->pRB_SortedWorkQueue)  );
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx,&pDagLca->pRB_SortedResultMap)  );

	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx,&pDagLca->pvecSignificantNodes,64)  );

	*ppNew = pDagLca;
	return;

fail:
	SG_DAGLCA_NULLFREE(pCtx, pDagLca);
}

void SG_daglca__free(SG_context * pCtx, SG_daglca * pDagLca)
{
	if (!pDagLca)
		return;

	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pDagLca->pRB_FetchedCache, _my_data__free__cb);

	SG_RBTREE_NULLFREE(pCtx, pDagLca->pRB_SortedWorkQueue);
	SG_RBTREE_NULLFREE(pCtx, pDagLca->pRB_SortedResultMap);

	SG_VECTOR_NULLFREE(pCtx, pDagLca->pvecSignificantNodes);
	SG_BITVECTOR_NULLFREE(pCtx, pDagLca->pbvClaimedMask);

	SG_NULLFREE(pCtx, pDagLca);
}

//////////////////////////////////////////////////////////////////

void SG_daglca__add_leaf(SG_context * pCtx, SG_daglca * pDagLca, const char * szHid)
{
	// add a leaf to the set of leaves.

	SG_bool bAlreadyExists = SG_FALSE;
	my_data * pDataFound;
	my_data * pDataCached;
	SG_int32 kIndex;

	SG_NULLARGCHECK_RETURN(pDagLca);
	SG_NONEMPTYCHECK_RETURN(szHid);

	// you cannot add leaves after we start grinding because
	// of the way we compute the SPCAs (and we want the leaves
	// to be in spots 0 .. n-1 in the significant-node vector).

	if (pDagLca->bFrozen)
		SG_ERR_THROW_RETURN(  SG_ERR_INVALID_WHILE_FROZEN  );

	// if they give us the same dagnode twice, we abort.
	// this problem is hard enough without having to worry
	// about duplicate leaves gumming up the works.

	SG_ERR_CHECK_RETURN(  _cache__lookup(pCtx,pDagLca,szHid,&bAlreadyExists,&pDataFound)  );
	if (bAlreadyExists)
		SG_ERR_THROW_RETURN(  SG_ERR_DAGNODE_ALREADY_EXISTS  );

	// fetch the dagnode from disk into the fetched-cache
	// and add it to the work-queue.  we don't own pDataCached
	// (the fetched-cache does).

	SG_ERR_CHECK_RETURN(  _fetch_dagnode_into_cache_and_queue(pCtx,pDagLca,szHid,&pDataCached)  );

	// since this is a leaf, we always insert it into the  significant-node vector
	// and assign it an index.

	SG_ERR_CHECK_RETURN(  _significant_node_vec__insert_leaf(pCtx,pDagLca,pDataCached,&kIndex)  );
}

//////////////////////////////////////////////////////////////////

struct _add_leaves_data
{
	SG_daglca * pDagLca;
};

static SG_rbtree_foreach_callback _sg_daglca__add_leaves_callback;

static void _sg_daglca__add_leaves_callback(SG_context * pCtx,
											const char * szKey,
											SG_UNUSED_PARAM(void * pAssocData),
											void * pVoidAddLeavesData)
{
	struct _add_leaves_data * pAddLeavesData = (struct _add_leaves_data *)pVoidAddLeavesData;

	SG_UNUSED(pAssocData);

	SG_ERR_CHECK_RETURN(  SG_daglca__add_leaf(pCtx,pAddLeavesData->pDagLca,szKey)  );
}

void SG_daglca__add_leaves(SG_context * pCtx,
						   SG_daglca * pDagLca,
						   const SG_rbtree * prbLeaves)
{
	struct _add_leaves_data add_leaves_data;

	SG_NULLARGCHECK_RETURN(pDagLca);
	SG_NULLARGCHECK_RETURN(prbLeaves);

	add_leaves_data.pDagLca = pDagLca;

	SG_ERR_CHECK_RETURN(  SG_rbtree__foreach(pCtx,
											 prbLeaves,
											 _sg_daglca__add_leaves_callback,
											 &add_leaves_data)  );
}

//////////////////////////////////////////////////////////////////

static void _process_parent_closure(SG_context * pCtx,
									SG_daglca * pDagLca,
									const my_data * pDataChild,
									my_data * pDataParent)
{
	// decide how to merge the closure bitvectors in the child and parent dagnodes
	// and if necessary add the parent to the significant-node vector.

	SG_bitvector * pbvUnion = NULL;
	SG_bool bAreEqual;

	SG_UNUSED( pDagLca );

	// bvUnion = (pDataChild->bvClosure | pDataParent->bvClosure);
	SG_ERR_CHECK(  SG_BITVECTOR__ALLOC__COPY(pCtx, &pbvUnion, pDataChild->pbvClosure)  );
	SG_ERR_CHECK(  SG_bitvector__assign__bv__or_eq__bv(pCtx, pbvUnion, pDataParent->pbvClosure)  );

	// bAreEqual = (bvUnion == pDataParent->bvClosure)
	SG_ERR_CHECK(  SG_bitvector__operator__bv__eq_eq__bv(pCtx, pbvUnion, pDataParent->pbvClosure, &bAreEqual)  );
	if (bAreEqual)
	{
		// our set of significant descendants is a equal to or a subset of the parent's.
		// this edge adds no new "significant" paths to the parent.   and so, we have
		// no reason to mark the parent as a SPCA.
		//
		//         A
		//          \.
		//           C
		//          / \.
		//         D   E
		//        / \ /
		//       L0  L1
		//
		//       Figure 2.
		//
		// If D-->C is processed first, it sets bits {L0,L1,D} on C.
		// Then when E-->C is processed, it only has bits {L1}, so as
		// described in [1c], we don't mark C.

		goto done;
	}

	// pDataParent->bvClosure = bvUnion;
	SG_ERR_CHECK(  SG_bitvector__assign__bv__eq__bv(pCtx, pDataParent->pbvClosure, pbvUnion)  );
	
	// bAreEqual = (bvUnion == pDataChild->bvClosure)
	SG_ERR_CHECK(  SG_bitvector__operator__bv__eq_eq__bv(pCtx, pbvUnion, pDataChild->pbvClosure, &bAreEqual)  );
	if (bAreEqual)
	{
		// the parent's set of significant descendants is a proper subset of ours.
		// we have no reason to mark the parent as a SPCA because the child has
		// all the same descendands and is closer to the leaves.
		//
		// in Figure 2, suppose E-->C went first setting bits {L1} on C.
		// then when D-->C is processed, D has more bits set, but this should not
		// cause C to be marked, because D has all the same bits and is closer to
		// L0 and L1.
		//
		// we do update the significant descendants because that needs to trickle up.
		// TODO 2010/01/13 by "significant descendants" did i mean closure?

		goto done;
	}

	// the union is larger than either, so we should
	// consider marking this node as a SPCA.  but we
	// defer actually doing it until we've see all of
	// the sibling children of this parent because we
	// want the full union before we make a firm decision.

	// consider L0-->D and then L1-->D in Figure 2.
	//
	// if we already suspect the parent is special, we may
	// be adding more information.  for example, this can
	// happen when we have more than 2 way branch.
	//
	//         A
	//          \.
	//           C
	//          / \.
	//      ___D   E
	//     /  / \ /
	//    L2 L0  L1
	//
	//       Figure 3.
	//
	// if L0-->D sets bits {L0}, then L1-->D adds {L1} creating an SPCA
	// with {L0,L1}, then when L2-->D is processed, we add {L2} to D.

	if (pDataParent->itemIndex == -1)
		pDataParent->itemIndex = -2;

done:
	;
fail:
	SG_BITVECTOR_NULLFREE(pCtx, pbvUnion);
}

static void _process_parent(SG_context * pCtx,
							SG_daglca * pDagLca,
							const my_data * pDataChild,
							const char * szHidParent)
{
	// analyze the child-->parent relationship and update attributes
	// on the parent.
	//
	// if szHidParent is NULL, we assume the implied null root node.

	SG_bool bFound = SG_FALSE;
	my_data * pDataParent;		// we never own this

#if TRACE_DAGLCA
	SG_ERR_IGNORE(  _my_data_debug__dump_to_console(pCtx, pDataChild, "child at top of __process_parent")  );
#endif

	SG_ERR_CHECK(  _cache__lookup(pCtx,pDagLca,szHidParent,&bFound,&pDataParent)  );
	if (!bFound)
	{
		// we have never visited this parent dagnode, so we don't know anything about it.
		// add it to the fetched-cache and the work-queue.
		//
		// let the parent start accumulating the closure on significant descendants by
		// starting with the child's closure.
		//
		// if the child is a significant node, then it is the only immediate descendant for
		// the parent (so far).  if not, we bubble up the child's immediate descendants.
		//
		// we won't know if the parent is a SPCA until it has been referenced by multiple
		// "significant" paths thru the dag.

		SG_ERR_CHECK(  _fetch_dagnode_into_cache_and_queue(pCtx,pDagLca,szHidParent,&pDataParent)  );

		// pDataParent->bvClosure = pDataChild->bvClosure;
		SG_ERR_CHECK(  SG_bitvector__assign__bv__eq__bv(pCtx, pDataParent->pbvClosure, pDataChild->pbvClosure)  );

		if (pDataChild->itemIndex >= 0)
		{
			SG_ASSERT(  (pDataChild->pbvMySignificantBit)  );

			// pDataParent->bvImmediateDescendants = pDataChild->bvMySignificantBit
			SG_ERR_CHECK(  SG_bitvector__assign__bv__eq__bv(pCtx,
															pDataParent->pbvImmediateDescendants,
															pDataChild->pbvMySignificantBit)  );
		}
		else
		{
			SG_ASSERT(  (pDataChild->pbvMySignificantBit == NULL)  );

			// pDataParent->bvImmediateDescendants = pDataChild->bvImmediateDescendants;
			SG_ERR_CHECK(  SG_bitvector__assign__bv__eq__bv(pCtx,
															pDataParent->pbvImmediateDescendants,
															pDataChild->pbvImmediateDescendants)  );
		}

#if TRACE_DAGLCA
		SG_ERR_IGNORE(  _my_data_debug__dump_to_console(pCtx, pDataParent, "new parent in __process_parent")  );
#endif
	}
	else
	{
		// the parent is in the fetched-cache, so we have seen it before.  and since it
		// will have a smaller generation than us (by construction), it must also be in
		// the work-queue.

		// if the parent is a leaf, then it is a leaf ***and*** an ancestor of one
		// of the other leaves.  We agreed in [1d] to shut this down.
		//
		// with the original bitvectors, this was just (pDataParent->bvLeaf != 0),
		// but that's too expensive with the new bitvectors, so this is the same:

		if ((pDataParent->itemIndex >= 0) && (pDataParent->itemIndex < (SG_int32)pDagLca->nrLeafNodes))
		{
			// we want to throw a meaningful error message that one leaf is an ancestor of another.
			// in the 2-way case, this is easy to compute.  in the n-way case, we just leave it as
			// a general message.  we CANNOT just use pDataChild because that is just the immediate
			// descendant of the parent (the edge which detected the relationship); it is not
			// necessarily the leaf.

			if (pDagLca->nrLeafNodes <= 2)
			{
				SG_int32 itemIndex_Other = (!pDataParent->itemIndex);	// the 2 leaves are in [0] and [1].
				my_data * pData_Other;
				const char * pszHid_Other;

				SG_ERR_CHECK(  SG_vector__get(pCtx, pDagLca->pvecSignificantNodes,
											  itemIndex_Other, (void **)&pData_Other)  );
				SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pData_Other->pDagnode, &pszHid_Other)  );
				SG_ERR_THROW2(  SG_ERR_DAGLCA_LEAF_IS_ANCESTOR,
								(pCtx, "Changeset %s is an ancestor of changeset %s.",
								 szHidParent, pszHid_Other)  );
			}
			else
			{
				// TODO 2011/06/16 With the data in the various bitvectors, we could
				// TODO            get the specific child or a list of the descendant
				// TODO            children and list them in the error message, but I
				// TODO            don't think it's worth the bother.
				SG_ERR_THROW2(  SG_ERR_DAGLCA_LEAF_IS_ANCESTOR,
								(pCtx, "Changeset %s is an ancestor of one of the other changesets.",
								 szHidParent)  );
			}
		}

		// compare the child and parent's closure bitvectors and decide the significance
		// of the parent.

		SG_ERR_CHECK(  _process_parent_closure(pCtx,pDagLca,pDataChild,pDataParent)  );

		// if the child is a significant node, then we want to make it an
		// immediate descendant of the parent -- BUT -- we need to remove any
		// immediate descendants from the parent that are also ours -- because
		// the child is between them.  (If the child is not a significant node,
		// we just bubble up the child's immediate descendants.)
		//
		//         A
		//          \.
		//           C
		//          /|
		//         D |
		//        / \|
		//       L0  L1
		//
		// Figure 2a.  (For our purposes here, this graph is equivalent to Figure 2.)
		//
		// In this example, after L1 is processed, ImmDesc(D)-->{L0,L1} and
		// ImmDesc(C)-->{L1}.  After D is processed, we want ImmDesc(C)-->{D}
		// because the edge L1-->C or L1-->E-->C are discardable.

		if (pDataChild->itemIndex >= 0)
		{
			SG_ASSERT(  (pDataChild->pbvMySignificantBit)  );

			// pDataParent->bvImmediateDescendants &= ~pDataChild->bvImmediateDescendants;
			SG_ERR_CHECK(  SG_bitvector__assign__bv__and_eq__not__bv(pCtx,
																	 pDataParent->pbvImmediateDescendants,
																	 pDataChild->pbvImmediateDescendants)  );

			// pDataParent->bvImmediateDescendants |= (1ULL << pDataChild->itemIndex);
			SG_ERR_CHECK(  SG_bitvector__assign__bv__or_eq__bv(pCtx,
															   pDataParent->pbvImmediateDescendants,
															   pDataChild->pbvMySignificantBit)  );
		}
		else
		{
			SG_ASSERT(  (pDataChild->pbvMySignificantBit == NULL)  );
			
			// pDataParent->bvImmediateDescendants |= pDataChild->bvImmediateDescendants;
			SG_ERR_CHECK(  SG_bitvector__assign__bv__or_eq__bv(pCtx,
															   pDataParent->pbvImmediateDescendants,
															   pDataChild->pbvImmediateDescendants)  );
		}

#if TRACE_DAGLCA
		SG_ERR_IGNORE(  _my_data_debug__dump_to_console(pCtx, pDataParent, "old parent in __process_parent")  );
#endif
	}

	return;

fail:
	return;
}

static void _is_closure_complete(SG_context * pCtx,
								 SG_daglca * pDagLca,
								 SG_bitvector * pbvClosure,
								 SG_bool * pbComplete)
{
	// we have processed all the various significant paths in the
	// dag and have reached a single node.  this closure computed
	// for this node should be complete -- contain all leaves and
	// all SPCAs (optionally including itself).
	//
	// verify that.

	// *pbComplete = (bvClosure == pDagLca->bvClaimedMask);

	SG_ERR_CHECK_RETURN(  SG_bitvector__operator__bv__eq_eq__bv(pCtx,
																pbvClosure,
																pDagLca->pbvClaimedMask,
																pbComplete)  );
}

static void _fixup_result_map(SG_context * pCtx,
							  SG_daglca * pDagLca)
{
	// change the node-type on the first SPCA to LCA to aid our callers.

	const char * szKey;
	my_data * pDataFirst;
	SG_bool bFound;
	SG_bool bComplete;
	SG_rbtree_iterator * pIter = NULL;

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx,
											  &pIter,
											  pDagLca->pRB_SortedResultMap,
											  &bFound,
											  &szKey,
											  (void**)&pDataFirst)  );
	// At least 2 nodes should exist because we added the leaves before we started.
	if (!bFound)
	{
		char buf[SG_DAGNUM__BUF_MAX__HEX];
		buf[0] = 0;
		SG_ERR_IGNORE(  SG_dagnum__to_sz__hex(pCtx, pDagLca->iDagNum, buf, sizeof(buf))  );
		SG_ERR_THROW2(  SG_ERR_DAGLCA_NO_ANCESTOR,
						(pCtx, "Empty sorted-result map in dagnum '%s'.", buf)  );
	}
	// The first node should be special (a non-leaf)
	// unless the graph is disconnected.  See W9553.
	if (pDataFirst->nodeType != SG_DAGLCA_NODE_TYPE__SPCA)
	{
		const char * pszHidFirst = NULL;
		char buf[SG_DAGNUM__BUF_MAX__HEX];
		buf[0] = 0;
		SG_ERR_IGNORE(  SG_dagnum__to_sz__hex(pCtx, pDagLca->iDagNum, buf, sizeof(buf))  );
		SG_ERR_IGNORE(  SG_dagnode__get_id_ref(pCtx, pDataFirst->pDagnode, &pszHidFirst)  );
		SG_ERR_THROW2(  SG_ERR_DAGLCA_NO_ANCESTOR,
						(pCtx, "First node '%s' has type '%d' in dagnum '%s'; expected '%d'.",
						 ((pszHidFirst) ? pszHidFirst : "(null)"),
						 (SG_uint32)pDataFirst->nodeType,
						 buf,
						 (SG_uint32)SG_DAGLCA_NODE_TYPE__SPCA)  );
	}
	// The above test can be tricked if the graph is disconnected,
	// you have n>2 leaves, *AND* you have an SPCA that covers a
	// subset of the leaves.  To be a true LCA it must cover all of
	// the leaves.
	SG_ERR_CHECK(  _is_closure_complete(pCtx, pDagLca,pDataFirst->pbvClosure,&bComplete)  );
	if (!bComplete)
	{
		const char * pszHidFirst = NULL;
		char buf[SG_DAGNUM__BUF_MAX__HEX];
		buf[0] = 0;
		SG_ERR_IGNORE(  SG_dagnum__to_sz__hex(pCtx, pDagLca->iDagNum, buf, sizeof(buf))  );
		SG_ERR_IGNORE(  SG_dagnode__get_id_ref(pCtx, pDataFirst->pDagnode, &pszHidFirst)  );
		SG_ERR_THROW2(  SG_ERR_DAGLCA_NO_ANCESTOR,
						(pCtx, "First node '%s' does not cover the given set of leaves in dagnum '%s'.",
						 ((pszHidFirst) ? pszHidFirst : "(null)"),
						 buf)  );
	}

	pDataFirst->nodeType = SG_DAGLCA_NODE_TYPE__LCA;

#if defined(DEBUG)
	{
		// verify that the second significant node is deeper than the LCA.
		// it must be because the LCA is a parent of it.

		my_data * pDataSecond;

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx,pIter,&bFound,&szKey,(void**)&pDataSecond)  );
		SG_ASSERT_RELEASE_FAIL(  ((bFound) && "DAGLCA should have second item in sorted-result map")  );
		SG_ASSERT_RELEASE_FAIL(  ((pDataSecond->genDagnode > pDataFirst->genDagnode)
						  && "DAGLCA second item in sorted-result set should be deeper than first")  );

		SG_ERR_CHECK(  _is_closure_complete(pCtx,pDagLca,pDataSecond->pbvClosure,&bComplete)  );
		SG_ASSERT_RELEASE_FAIL(  ((!bComplete) && "DAGLCA second item should not have complete closure")  );

		// TODO we *could* scan all the remaining items in our sorted-result map
		// TODO and verify that they don't have complete closures too.
	}
#endif

	// fall thru to common cleanup

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
}

//////////////////////////////////////////////////////////////////

/**
 * There are some graph shapes where we correctly compute the
 * immediate descendants of a node and beacuse of the order
 * that we see the children, we mark it "significant" when it
 * really isn't.
 * 
 *            A0
 *        _____|_________
 *        |      |      |
 *       x0     y0     z0
 *        |      |      |
 *       x1     y1      |
 *        |      |      |
 *        |     A1      |
 *         \   /  \     |
 *          \ /    y2   |
 *          L0       \ /
 *                    L1
 *
 * For example, when we are processing x0, y0, and z0 in the
 * above graph and updating the immediate-decendants
 * vector in A0, we compute either {L0, L1, A1} or {A1}
 * depending on which order we see x0, y0, and z0 (and this
 * order is dependent on the generation numbers and HIDs of
 * those nodes.
 *
 * If we see x0 and z0 before y0, then we mark A0 as
 * significant (because it is bringing together 2 paths)
 * with immediate-descendants {L0, L1}. Then when we see y0,
 * we fold in {A1}.  If we saw y0 first, we would have
 * carried forward {A1} and then when we say x0 or y0, we
 * see that the L0/L1 nodes were already included in the
 * closure and not new information.  So we'd just carry
 * forward {A1} in the immediate-descendants vector.
 *
 * The problem is that we can't tell when looking one of
 * x/y/z nodes whether A0 should be significant until we've
 * looked at them all.
 *
 * The correct answer is {A1}.
 *
 */                    
static void _check_for_false_positive_before_inserting_spca(SG_context * pCtx,
															SG_daglca * pDagLca,
															my_data * pDataSelf)
{
	SG_bitvector *  pbv_id_copy = NULL;
	SG_uint32 k, kLimit;
	SG_uint32 nr_bits_set_afterwards;

#if TRACE_DAGLCA
	SG_ERR_IGNORE(  _my_data_debug__dump_to_console(pCtx, pDataSelf, "check_for_false_positive: self")  );
#endif

	// copy pDataSelf->pbvImmediateDescendants so that we can
	// munge it and control the loop.  we only want to inspect
	// the bits set upon entry.
	// 
	// bv_id_copy = pDataSelf->bvImmediateDescendants;
	SG_ERR_CHECK(  SG_BITVECTOR__ALLOC__COPY(pCtx, &pbv_id_copy, pDataSelf->pbvImmediateDescendants)  );

	// kLimit = length(bv_id_copy)
	SG_ERR_CHECK(  SG_bitvector__length(pCtx, pbv_id_copy, &kLimit)  );

	for (k=0; k<kLimit; k++)
	{
		SG_bool b_self_k;

		SG_ERR_CHECK(  SG_bitvector__get_bit(pCtx, pDataSelf->pbvImmediateDescendants, k, SG_FALSE, &b_self_k)  );
		if (b_self_k)
		{
			// bit[k] is set in this nodes immediate descendants, so we
			// currently think that significant node k is an immediate
			// descendant.

			my_data * pData_k;
			SG_bool bAndIsZero;

			SG_ERR_CHECK(  SG_vector__get(pCtx, pDagLca->pvecSignificantNodes, k, (void **)&pData_k)  );

			// bAndIsZero = ((pDataSelf->bvImmediateDescendants & pData_k->bvImmediateDescendants) == 0)
			SG_ERR_CHECK(  SG_bitvector__operator__bv__and__bv__eq_eq__zero(pCtx,
																			pDataSelf->pbvImmediateDescendants,
																			pData_k->pbvImmediateDescendants,
																			&bAndIsZero)  );
			if (bAndIsZero == SG_FALSE)
			{
				// we picked up some edges that we shouldn't have
				// because of the order that the childen were
				// inspected.  we should reference those edges via
				// the child.

				// pDataSelf->bvImmediateDescendants &= ~pData_k->bvImmediateDescendants;
				SG_ERR_CHECK(  SG_bitvector__assign__bv__and_eq__not__bv(pCtx,
																		 pDataSelf->pbvImmediateDescendants,
																		 pData_k->pbvImmediateDescendants)  );
				
				// SG_ASSERT(  (pDataSelf->bvImmediateDescendants & bv_k)  );
				SG_ERR_CHECK(  SG_bitvector__get_bit(pCtx, pDataSelf->pbvImmediateDescendants, k, SG_FALSE, &b_self_k)  );
				SG_ASSERT(  (b_self_k)  );

#if TRACE_DAGLCA
				SG_ERR_IGNORE(  _my_data_debug__dump_to_console(pCtx, pData_k,   "check_for_false_positive: child_k")  );
				SG_ERR_IGNORE(  _my_data_debug__dump_to_console(pCtx, pDataSelf, "check_for_false_positive: self after k")  );
#endif
			}
		}
	}
	
	SG_ERR_CHECK(  SG_bitvector__count_set_bits(pCtx, pDataSelf->pbvImmediateDescendants, &nr_bits_set_afterwards)  );
	if (nr_bits_set_afterwards > 1)
	{
		SG_ERR_CHECK_RETURN(  _significant_node_vec__insert_spca(pCtx,pDagLca,pDataSelf)  );
	}

fail:
	SG_BITVECTOR_NULLFREE(pCtx, pbv_id_copy);
}

//////////////////////////////////////////////////////////////////

static void _can_short_circuit(SG_context * pCtx,
							   SG_daglca * pDagLca,
							   my_data * pData,
							   SG_bool * pbCanShortCircuit)
{
	const char * pszKey_k;
	SG_bitvector * pbvUnion = NULL;
	SG_bitvector * pbvPartialClosure = NULL;
	my_data * pData_k;
	SG_rbtree_iterator * pIter = NULL;
	SG_uint32 sumLeavesReferenced;
	SG_uint32 k;
	SG_bool bClosureComplete;
	SG_bool bFound;
	SG_bool bCanShortCircuit = SG_FALSE;
	SG_bool bIsEqual;

	if (pData->itemIndex < (SG_int32)pDagLca->nrLeafNodes)
	{
		// itemIndex is -2, -1, or a leaf [0 .. j-1].
		// this node is not a SPCA/LCA so we don't bother.

		goto done;
	}
	
	SG_ERR_CHECK(  _is_closure_complete(pCtx, pDagLca, pData->pbvClosure, &bClosureComplete)  );
	if (!bClosureComplete)
	{
		// this node is not a complete ancestor of all LEAVES and any
		// already identified SPCAs.

		goto done;
	}

	// it is tempting to grab the first node with a complete
	// closure (the first ancestor of all of the leaves that
	// we found) and stop searching.  but this would not allow
	// us to resolve the criss-crosses because they will appear
	// as peers with complete (so far) closures.  note that we
	// extend the set of bits with each criss-cross peer that
	// we find, so the closures of the peers (which may have
	// been complete when we first touched them) won't be
	// fully complete until we find *their* common ancestor.
	//
	// *BUT* if we can PROVE that there are no criss-crosses
	// possible past this point, we can let our caller stop
	// searching.

#if TRACE_DAGLCA
	{
		const char * pszHid;
		SG_uint32 nrQueued;

		SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pData->pDagnode, &pszHid)  );
		SG_ERR_CHECK(  SG_rbtree__count(pCtx,pDagLca->pRB_SortedWorkQueue,&nrQueued)  );
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "In _can_short_circuit: [looking at %s] [queue size %d]\n",
								   pszHid,
								   nrQueued)  );
	}
#endif

	//////////////////////////////////////////////////////////////////
	// by construction, the work queue contains unvisited
	// parent/ancestor nodes.  one or more leaf/child nodes
	// had parent links/edges to them.  we started accumulating
	// closure bits in the parent as we visited each child.
	// *BUT* since we haven't actually visited the parent
	// (work queue) nodes yet, their closure may not be
	// fully computed (i'm trying to avoid the term complete).
	//
	// our caller just removed the first node in the queue
	// and is processing it.  it's closure is fully computed.
	//
	// nodes in the work queue at the same generation also
	// have a fully computed closure.  nodes at earlier
	// generations (towards 0 (the root)) may not yet be
	// full computed (because there may be edges from this
	// (or peer nodes at this generation) to those earlier
	// nodes.
	//
	// So if we can prove that the remaining nodes in the work
	// queue (with the partial closures that they have now)
	// could not create any SPCA-peers of the current
	// node, then we can stop searching for the LCA and
	// declare that this node is it.
	//
	//
	// Clear as mud, right?
	//////////////////////////////////////////////////////////////////

	// bvThisNode = (1ULL << pData->itemIndex);
	SG_ASSERT(  ((pData->itemIndex >= 0) && (pData->pbvMySignificantBit))  );

	// bvPartialClosure = pData->bvClosure;
	// bvPartialClosure &= ~ bvThisNode;
	SG_ERR_CHECK(  SG_BITVECTOR__ALLOC__COPY(pCtx, &pbvPartialClosure, pData->pbvClosure)  );
	SG_ERR_CHECK(  SG_bitvector__assign__bv__and_eq__not__bv(pCtx, pbvPartialClosure, pData->pbvMySignificantBit)  );

	// compute the union of the (partial)closures over the other
	// nodes in the work queue.  this is the maxium/potential
	// closure that any graph starting at those nodes could
	// achieve (not counting the contribution from edges from
	// the current node).

	// bvUnion = 0;
	SG_ERR_CHECK(  SG_BITVECTOR__ALLOC(pCtx, &pbvUnion, 64)  );
	
	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx,
											  &pIter,
											  pDagLca->pRB_SortedWorkQueue,
											  &bFound,
											  &pszKey_k,
											  (void **)&pData_k)  );
	while (bFound)
	{
#if TRACE_DAGLCA
		SG_ERR_IGNORE(  _my_data_debug__dump_to_console(pCtx, pData_k, "short-circuit looking at:")  );
#endif
		// bvUnion |= pData_k->bvClosure;
		SG_ERR_CHECK(  SG_bitvector__assign__bv__or_eq__bv(pCtx, pbvUnion, pData_k->pbvClosure)  );
		
		// bIsEqual = (bvUnion == bvPartialClosure)
		SG_ERR_CHECK(  SG_bitvector__operator__bv__eq_eq__bv(pCtx, pbvUnion, pbvPartialClosure, &bIsEqual)  );
		if (bIsEqual)
			break;

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx,
												 pIter,
												 &bFound,
												 &pszKey_k,
												 (void **)&pData_k)  );
	}
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);

#if TRACE_DAGLCA
	{
		char * pszUnion = NULL;
		char * pszPartialClosure = NULL;

		SG_ERR_IGNORE(  SG_bitvector_debug__to_sz(pCtx, pbvUnion, &pszUnion)  );
		SG_ERR_IGNORE(  SG_bitvector_debug__to_sz(pCtx, pbvPartialClosure, &pszPartialClosure)  );
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "In _can_short_circuit: [union %s] [partial %s]\n",
								   pszUnion, pszPartialClosure)  );
		SG_NULLFREE(pCtx, pszUnion);
		SG_NULLFREE(pCtx, pszPartialClosure);
	}
#endif

	// bIsEqual = (bvUnion == bvPartialClosure)
	SG_ERR_CHECK(  SG_bitvector__operator__bv__eq_eq__bv(pCtx, pbvUnion, pbvPartialClosure, &bIsEqual)  );
	if (bIsEqual)
	{
		// the other nodes are *capable* of creating a peer
		// SPCA of this node (without using any of the edge
		// info from this node).  (they have already references
		// to all of the leaves and all of the existing SPCAs
		// between here and the leaves.)
		// 
		// so we can't ignore the work queue at this time.
		//
		// i've observed this when looking at at something
		// like this.  when the current node is 'x0' and 'y0'
		// is in the work queue, then we don't want to stop
		// because we don't want 'x0' to be the LCA.
		// see st_daglca_tests_010.js
		// 
		//        A0
		//       /  \.
		//     x0    y0
		//      |\  /|
		//      | \/ |
		//    x0a /\ y0a
		//      |/  \|
		//     x1    y1
		//      |\  /|
		//      | \/ |
		//    x1a /\ y1a
		//      |/  \|
		//     x2    y2
		//      |    |
		//     L0    L1

#if TRACE_DAGLCA
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "In _can_short_circuit: union equals partial-closure, preventing short circuit.\n")  );
#endif
		goto done;
	}

	// is more than one leaf referenced?  if so, they might
	// get together and create a SPCA before joining up
	// with an ancestor of this node, so again we need
	// to let this continue.

	sumLeavesReferenced = 0;
	for (k=0; k<pDagLca->nrLeafNodes; k++)
	{
		SG_bool b_k;

		SG_ERR_CHECK(  SG_bitvector__get_bit(pCtx, pbvUnion, k, SG_FALSE, &b_k)  );
		if (b_k)
			sumLeavesReferenced++;
	}
	if (sumLeavesReferenced > 1)
	{
#if TRACE_DAGLCA
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "In _can_short_circuit: %d leaves referenced, preventing short circuit.\n",
								   sumLeavesReferenced)  );
#endif
		goto done;
	}

	// We get here when looking at something like:
	// 
	//        A0
	//       /  \.
	//      |    |
	//    x0e   y0e
	//      |    |
	//    x0d   y0d
	//      |    |
	//    x0c    |
	//      |   A1
	//    x0b   /|
	//      |  / |
	//    x0a /  |
	//      |/   |
	//     x1    |
	//      |    |
	//      |    |
	//     L0    L1
	//
	// When we are looking at A1, we want to be able to stop
	// searching and not have to scan x0* and y0*.  Without
	// this short-cut, the caller's loop would continue until
	// the queue contained only A0 (and then disregard it
	// because A1 is a better choice).
	// See st_daglca_tests_015.js
	//
	//////////////////////////////////////////////////////////////////
	// NOTE: Just because we said we can short circuit things and ignore
	// the rest of the work queue doesn't mean that the current node is
	// the LCA.  It could be that we already found it (or rather a SPCA
	// node with a complete closure), but at the time there was stuff in
	// the queue that caused one of the above rejections but now as we
	// process the queue and stuff drops out, we get are successful.
	// so don't be surprised if a 'vv dump_lca --verbose' graph shows a
	// few more nodes/generations below the LCA.
	//////////////////////////////////////////////////////////////////

	bCanShortCircuit = SG_TRUE;

#if TRACE_DAGLCA
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "In _can_short_circuit: allowing short circuit.\n")  );
#endif
	
done:

	*pbCanShortCircuit = bCanShortCircuit;

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
	SG_BITVECTOR_NULLFREE(pCtx, pbvUnion);
	SG_BITVECTOR_NULLFREE(pCtx, pbvPartialClosure);
}

//////////////////////////////////////////////////////////////////

void SG_daglca__compute_lca(SG_context * pCtx, SG_daglca * pDagLca)
{
	// using the set of leaves already given, freeze pDagLca
	// and compute the SPCAs and LCA.

	SG_bool bFound = SG_FALSE;
	my_data * pDataQueued = NULL;
	SG_uint32 nrParents, kParent;
	SG_uint32 nrQueued;
	const char ** aszHidParents = NULL;
	SG_bool bCanShortCircuit = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pDagLca);

	SG_ARGCHECK_RETURN(  (pDagLca->nrLeafNodes >= 2),  "nrLeafNodes"  );

	// we can only do this computation once.

	if (pDagLca->bFrozen)
		SG_ERR_THROW(  SG_ERR_INVALID_WHILE_FROZEN  );

	pDagLca->bFrozen = SG_TRUE;

	while (1)
	{
		// process the work-queue until we find the LCA.
		//
		// remove the first dagnode in the work-queue.  because of
		// our sorting, this is the deepest dagnode in the dag (or
		// one of several at that depth).

		SG_ERR_CHECK(  _work_queue__remove_first(pCtx,pDagLca,&bFound,&pDataQueued)  );
#if TRACE_DAGLCA
		SG_ERR_IGNORE(  _my_data_debug__dump_to_console(pCtx, pDataQueued, "top of loop in __compute_lca")  );
#endif

		// this cannot happen because we should have stopped on the
		// previous iteration when it removed the last node in the queue.
		SG_ASSERT_RELEASE_FAIL(  ((bFound) && "DAGLCA Emptied work-queue??")  );

		if (pDataQueued->itemIndex == -2)
		{
			SG_ERR_CHECK(  _check_for_false_positive_before_inserting_spca(pCtx, pDagLca, pDataQueued)  );
		}

		SG_ERR_CHECK(  SG_rbtree__count(pCtx,pDagLca->pRB_SortedWorkQueue,&nrQueued)  );
		if (nrQueued == 0)
		{
			// we took the last entry in the work-queue.
			//
			// we should be able to stop without looking at the parents of this dagnode.
			//
			// by construction, all the parents of this node (that we will be processing
			// in the loop below) can only have the closure bits that we already have
			// and therefore none of the parents can be a SPCA.  so we don't need to go
			// any higher in the dag.
			//
			// NOTE THAT THIS NODE MAY OR MAY NOT BE THE LCA.  IN FACT, IT MAY NOT EVEN
			// BE AN SPCA.
			//
			// For example, in Figure 2, when we take C from the work-queue, it is not
			// an SPCA/LCA because D is and SPCA and is closer.  However, C should have
			// a complete set of bits set in its closure bitvector.

			goto FinishedScan;
		}

		_can_short_circuit(pCtx, pDagLca, pDataQueued, &bCanShortCircuit);
		if (bCanShortCircuit)
			goto FinishedScan;

		SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx,pDataQueued->pDagnode,&nrParents,&aszHidParents)  );

		for (kParent=0; kParent<nrParents; kParent++)
		{
			SG_ERR_CHECK(  _process_parent(pCtx,pDagLca,pDataQueued,aszHidParents[kParent])  );
		}
	}

FinishedScan:
	SG_ERR_CHECK(  _fixup_result_map(pCtx,pDagLca)  );
#if TRACE_DAGLCA_SUMMARY
	SG_ERR_IGNORE(  SG_daglca_debug__dump_to_console(pCtx, pDagLca)  );
#endif
	return;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_daglca__get_stats(SG_context * pCtx,
						  const SG_daglca * pDagLca,
						  SG_uint32 * pNrLCA,
						  SG_uint32 * pNrSPCA,
						  SG_uint32 * pNrLeaves)
{
	// return some stats.
	//
	// TODO is there any other summary data that would be useful?

	SG_NULLARGCHECK_RETURN(pDagLca);

	if (pNrLCA)				// this arg is trivial, i added it so that the
		*pNrLCA = 1;		// caller would not forget to include it their counts.

	if (pNrSPCA)
		*pNrSPCA = pDagLca->nrSignificantNodes - pDagLca->nrLeafNodes - 1;

	if (pNrLeaves)
		*pNrLeaves = pDagLca->nrLeafNodes;

	// TODO it might be interesting to compute these values by
	// TODO scanning the pDagLca->aData_SignfificantNodes or
	// TODO pDagLca->pRB_SortedResultMap and make sure that they
	// TODO match.
}

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)

#if TRACE_DAGLCA
static const char * aszNodeTypeNames[] = { "Nobody",
										   "Leaf  ",
										   "SPCA  ",
										   "LCA   " };

static void _sg_daglca_debug__dump_line_items(SG_context * pCtx,
											  const SG_daglca * pDagLca,
											  SG_uint32 indent,
											  SG_string * pStringOutput)
{
	SG_rbtree_iterator * pIter = NULL;
	SG_bool bFound;
	const char * szKey;
	my_data * pDataFound;
	char * psz_my_significant_bit = NULL;
	char * psz_immediate_descendants = NULL;
	char * psz_closure = NULL;
	char buf[4000];

	// dump the list of significant nodes in sorted order.
	// the LCA should be first.  this is a raw dump with
	// very little decoding.

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx,
											  &pIter,pDagLca->pRB_SortedResultMap,
											  &bFound,
											  &szKey,(void**)&pDataFound)  );
	while (bFound)
	{
		SG_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pDataFound->pbvMySignificantBit, &psz_my_significant_bit)  );
		SG_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pDataFound->pbvImmediateDescendants, &psz_immediate_descendants)  );
		SG_ERR_CHECK(  SG_bitvector_debug__to_sz(pCtx, pDataFound->pbvClosure, &psz_closure)  );

		SG_ERR_CHECK(  SG_sprintf(pCtx,buf,SG_NrElements(buf),
								  "%*c    %s %s [my '%s...'][id '%s...'][cl '%s...'] [itemIndex %d]\n",
								  indent,' ',
								  szKey,
								  aszNodeTypeNames[pDataFound->nodeType],
								  psz_my_significant_bit,
								  psz_immediate_descendants,
								  psz_closure,
								  pDataFound->itemIndex)  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx,pStringOutput,buf)  );

		SG_NULLFREE(pCtx, psz_my_significant_bit);
		SG_NULLFREE(pCtx, psz_immediate_descendants);
		SG_NULLFREE(pCtx, psz_closure);

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx,pIter,&bFound,&szKey,(void**)&pDataFound)  );
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
	SG_NULLFREE(pCtx, psz_my_significant_bit);
	SG_NULLFREE(pCtx, psz_immediate_descendants);
	SG_NULLFREE(pCtx, psz_closure);
}
#endif

void SG_daglca_debug__dump(SG_context * pCtx,
						   const SG_daglca * pDagLca,
						   const char * szLabel,
						   SG_uint32 indent,
						   SG_string * pStringOutput)
{
	SG_uint32 nrLeaves, nrSPCAs, nrLCAs;	// size of our results
	SG_uint32 nrFetched;					// of interest for big-O complexity
	SG_uint32 nrStillQueued;				// see if short-circuit taken
	char buf[4000];
	char bufDagNum[SG_DAGNUM__BUF_MAX__HEX+10];

	SG_NULLARGCHECK_RETURN(pDagLca);
	SG_NULLARGCHECK_RETURN(pStringOutput);

	if (!pDagLca->bFrozen)
		SG_ERR_THROW_RETURN(  SG_ERR_INVALID_UNLESS_FROZEN  );

	SG_ERR_CHECK(  SG_sprintf(pCtx,
							  buf,SG_NrElements(buf),
							  "%*cDagLca[%s]\n",
							  indent,' ',
							  ((szLabel) ? szLabel : ""))  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx,pStringOutput,buf)  );

	SG_ERR_CHECK(  SG_daglca__get_stats(pCtx,pDagLca,&nrLCAs,&nrSPCAs,&nrLeaves)  );
	SG_ERR_CHECK(  SG_rbtree__count(pCtx,pDagLca->pRB_FetchedCache,&nrFetched)  );
	SG_ERR_CHECK(  SG_rbtree__count(pCtx,pDagLca->pRB_SortedWorkQueue,&nrStillQueued)  );
	SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, pDagLca->iDagNum, bufDagNum, sizeof(bufDagNum))  );
	SG_ERR_CHECK(  SG_sprintf(pCtx,
							  buf,SG_NrElements(buf),
							  "%*c    [LCAs %d][SPCAs %d][Leaves %d] [Touched %d] [ShortCut %d] [DagNum %s]\n",
							  indent,' ',
							  nrLCAs,nrSPCAs,nrLeaves,nrFetched,nrStillQueued,bufDagNum)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx,pStringOutput,buf)  );

#if TRACE_DAGLCA
	SG_ERR_CHECK(  _sg_daglca_debug__dump_line_items(pCtx, pDagLca, indent, pStringOutput)  );
#endif

fail:
	return;
}

void SG_daglca_debug__dump_to_console(SG_context * pCtx,
									  const SG_daglca * pDagLca)
{
	SG_string * pString = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	SG_ERR_CHECK(  SG_daglca_debug__dump(pCtx, pDagLca, "", 4, pString)  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "\nDAGLCA Dump:\n%s\n", SG_string__sz(pString))  );

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}

#endif//DEBUG

//////////////////////////////////////////////////////////////////

void SG_daglca__format_summary(SG_context * pCtx,
							   const SG_daglca * pDagLca,
							   SG_string * pStringOutput)
{
	SG_daglca_iterator * pIterDagLca = NULL;
	SG_bool bFound = SG_TRUE;
	SG_int32 generation;
	const char * pszHid;
	SG_daglca_node_type nodeType;
	char buf[4000];

	SG_NULLARGCHECK_RETURN(pDagLca);
	SG_NULLARGCHECK_RETURN(pStringOutput);

	if (!pDagLca->bFrozen)
		SG_ERR_THROW_RETURN(  SG_ERR_INVALID_UNLESS_FROZEN  );

	SG_ERR_CHECK(  SG_daglca__iterator__first(pCtx, &pIterDagLca,
											  pDagLca, SG_TRUE,
											  &pszHid, &nodeType, &generation, NULL)  );
	while (bFound)
	{
		const char * pszNodeType = NULL;
		switch (nodeType)
		{
		case SG_DAGLCA_NODE_TYPE__LEAF: pszNodeType = "LEAF";   break;
		case SG_DAGLCA_NODE_TYPE__SPCA: pszNodeType = "SPCA";   break;
		case SG_DAGLCA_NODE_TYPE__LCA:  pszNodeType = "LCA";    break;
		default:                        pszNodeType = "NOBODY"; break;
		}
		SG_ERR_CHECK(  SG_sprintf(pCtx, buf, SG_NrElements(buf), "%s %s\n", pszHid, pszNodeType)  );
		SG_ERR_CHECK(  SG_string__append__sz(pCtx, pStringOutput, buf)  );

		SG_ERR_CHECK(  SG_daglca__iterator__next(pCtx, pIterDagLca,
												 &bFound,
												 &pszHid, &nodeType, &generation, NULL)  );
	}

fail:
	SG_DAGLCA_ITERATOR_NULLFREE(pCtx, pIterDagLca);
}

#if defined(DEBUG)
void SG_daglca_debug__dump_summary_to_console(SG_context * pCtx,
											  const SG_daglca * pDagLca)
{
	SG_string * pString = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );
	SG_ERR_CHECK(  SG_daglca__format_summary(pCtx, pDagLca, pString)  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "%s", SG_string__sz(pString))  );

fail:
	SG_STRING_NULLFREE(pCtx, pString);
}
#endif

//////////////////////////////////////////////////////////////////

/**
 * convert the 1 bits in the given bitvector to an rbtree
 * containing the HIDs of the associated significant nodes.
 *
 * This is more expensive than I would like, but it lets the
 * caller have, for example, the set of immediate descendants
 * of a particular node in a sorted thing that they can do
 * something with.
 *
 * Since we expect most significant nodes to have 2 immediate
 * descendants, this may be overkill.  But it does allow for
 * n-way merges.
 *
 * TODO we may also want to have a version of this that builds
 * TODO an array of HIDs (like SG_dagnode__get_parents()).
 *
 */
static void _bitvector_to_rbtree(SG_context * pCtx,
								 const SG_daglca * pDagLca,
								 const SG_bitvector * pbvInput,
								 SG_rbtree ** pprb)
{
	SG_rbtree * prb = NULL;
	SG_uint32 k, kLimit;
	SG_bool bIsZero;

	SG_ERR_CHECK(  SG_bitvector__operator__bv__eq_eq__zero(pCtx, pbvInput, &bIsZero)  );
	if (bIsZero)
	{
		*pprb = NULL;
		return;
	}

	SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS(pCtx,&prb,5,NULL)  );

	SG_ERR_CHECK(  SG_bitvector__length(pCtx, pbvInput, &kLimit)  );
	for (k=0; k<kLimit; k++)
	{
		SG_bool b_k;

		SG_ERR_CHECK(  SG_bitvector__get_bit(pCtx, pbvInput, k, SG_FALSE, &b_k)  );
		if (b_k)
		{
			my_data * pDataK;
			const char * szHidK;

			SG_ERR_CHECK(  SG_vector__get(pCtx, pDagLca->pvecSignificantNodes, k, (void **)&pDataK)  );
			SG_ASSERT(  (pDataK)  &&  "Invalid bit in bitvector"  );
			SG_ASSERT(  (pDataK->pDagnode && pDataK->genDagnode > 0)  );
			
			SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx,pDataK->pDagnode,&szHidK)  );

			SG_ERR_CHECK(  SG_rbtree__add(pCtx,prb,szHidK)  );
		}
	}

	*pprb = prb;
	return;

fail:
	SG_RBTREE_NULLFREE(pCtx, prb);
}

//////////////////////////////////////////////////////////////////

struct _SG_daglca_iterator
{
	const SG_daglca *		pDagLca;
	SG_rbtree_iterator *	prbIter;
	SG_bool					bWantLeaves;
};

void SG_daglca__iterator__free(SG_context * pCtx, SG_daglca_iterator * pDagLcaIter)
{
	if (!pDagLcaIter)
		return;

	// we do not own pDagLcaIter->pDagLca.

	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pDagLcaIter->prbIter);

	SG_NULLFREE(pCtx, pDagLcaIter);
}

void SG_daglca__iterator__first(SG_context * pCtx,
								SG_daglca_iterator ** ppDagLcaIter,		// returned (you must free this)
								const SG_daglca * pDagLca,				// input
								SG_bool bWantLeaves,					// input
								const char ** pszHid,					// output
								SG_daglca_node_type * pNodeType,		// output
								SG_int32 * pGeneration,					// output
								SG_rbtree ** pprbImmediateDescendants)	// output (you must free this)
{
	const char * szHid = NULL;
	SG_daglca_iterator * pDagLcaIter = NULL;
	SG_rbtree * prbImmediateDescendants = NULL;
	SG_bool bFound;
	my_data * pDataFirst;

	SG_NULLARGCHECK_RETURN(pDagLca);

	// alloc ppIter to be null.  it means that they only want the
	// first ancestor and don't want to iterate.

	if (!pDagLca->bFrozen)
		SG_ERR_THROW_RETURN(  SG_ERR_INVALID_UNLESS_FROZEN  );

	SG_ERR_CHECK(  SG_alloc1(pCtx,pDagLcaIter)  );

	pDagLcaIter->pDagLca = pDagLca;
	pDagLcaIter->bWantLeaves = bWantLeaves;

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx,
											  &pDagLcaIter->prbIter,
											  pDagLca->pRB_SortedResultMap,
											  &bFound,
											  NULL,(void **)&pDataFirst)  );
	SG_ASSERT_RELEASE_FAIL(  ((bFound) && "DAGLCA Empty sorted-result map")  );
	SG_ASSERT_RELEASE_FAIL(  ((pDataFirst->nodeType == SG_DAGLCA_NODE_TYPE__LCA)  &&  "DAGLCA first node not LCA?")  );

	if (pszHid)
	{
		SG_ASSERT(  (pDataFirst->pDagnode)  );

		SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx,pDataFirst->pDagnode,&szHid)  );
	}

	if (pprbImmediateDescendants)
	{
		SG_ERR_CHECK(  _bitvector_to_rbtree(pCtx,
											pDagLca,
											pDataFirst->pbvImmediateDescendants,
											&prbImmediateDescendants)  );
	}

	// now that we have all of the results, populate the
	// callers variables.  we don't want to do this until
	// we are sure that we won't fail.

	if (pszHid)
		*pszHid = szHid;
	if (pNodeType)
		*pNodeType = pDataFirst->nodeType;
	if (pGeneration)
		*pGeneration = pDataFirst->genDagnode;
	if (pprbImmediateDescendants)
		*pprbImmediateDescendants = prbImmediateDescendants;

	if (ppDagLcaIter)
		*ppDagLcaIter = pDagLcaIter;
	else
		SG_DAGLCA_ITERATOR_NULLFREE(pCtx, pDagLcaIter);

	return;

fail:
	SG_DAGLCA_ITERATOR_NULLFREE(pCtx, pDagLcaIter);
	SG_RBTREE_NULLFREE(pCtx, prbImmediateDescendants);
}

void SG_daglca__iterator__next(SG_context * pCtx,
							   SG_daglca_iterator * pDagLcaIter,
							   SG_bool * pbFound,
							   const char ** pszHid,
							   SG_daglca_node_type * pNodeType,
							   SG_int32 * pGeneration,
							   SG_rbtree ** pprbImmediateDescendants)
{
	const char * szHid = NULL;
	SG_rbtree * prbImmediateDescendants = NULL;
	SG_bool bFound;
	my_data * pData;

	SG_NULLARGCHECK_RETURN(pDagLcaIter);

	if (!pDagLcaIter->pDagLca->bFrozen)
		SG_ERR_THROW_RETURN(  SG_ERR_INVALID_UNLESS_FROZEN  );

	while (1)
	{
		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx,
												 pDagLcaIter->prbIter,
												 &bFound,
												 NULL,(void **)&pData)  );
		if (!bFound)
		{
			*pbFound = SG_FALSE;
			if (pszHid)
				*pszHid = NULL;
			if (pNodeType)
				*pNodeType = SG_DAGLCA_NODE_TYPE__NOBODY;
			if (pGeneration)
				*pGeneration = -1;
			if (pprbImmediateDescendants)
				*pprbImmediateDescendants = NULL;

			return;
		}

		if ((pData->nodeType == SG_DAGLCA_NODE_TYPE__SPCA)
			|| (pData->nodeType == SG_DAGLCA_NODE_TYPE__LCA)
			|| (pDagLcaIter->bWantLeaves && (pData->nodeType == SG_DAGLCA_NODE_TYPE__LEAF)))
		{
			if (pszHid)
			{
				SG_ASSERT(  (pData->pDagnode)  );

				SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx,pData->pDagnode,&szHid)  );
			}

			if (pprbImmediateDescendants)
			{
				SG_ERR_CHECK(  _bitvector_to_rbtree(pCtx,
													pDagLcaIter->pDagLca,
													pData->pbvImmediateDescendants,
													&prbImmediateDescendants)  );
			}

			// now that we have all of the results, populate the
			// callers variables.  we don't want to do this until
			// we are sure that we won't fail.

			if (pbFound)
				*pbFound = bFound;
			if (pszHid)
				*pszHid = szHid;
			if (pNodeType)
				*pNodeType = pData->nodeType;
			if (pGeneration)
				*pGeneration = pData->genDagnode;
			if (pprbImmediateDescendants)
				*pprbImmediateDescendants = prbImmediateDescendants;	// this may be null if we don't have any

			return;
		}

		SG_ASSERT_RELEASE_RETURN(  ((pData->nodeType == SG_DAGLCA_NODE_TYPE__LEAF)  &&  "DAGLCA unknown significant node type")  );
	}

	/*NOTREACHED*/

fail:
	SG_RBTREE_NULLFREE(pCtx, prbImmediateDescendants);
}

//////////////////////////////////////////////////////////////////

void SG_daglca__foreach(SG_context * pCtx,
						const SG_daglca * pDagLca, SG_bool bWantLeaves,
						SG_daglca__foreach_callback * pfn, void * pVoidCallerData)
{
	SG_daglca_iterator * pDagLcaIter = NULL;
	SG_bool bFound;
	const char * szHid;
	SG_daglca_node_type nodeType;
	SG_int32 gen;
	SG_rbtree * prb;

	// the callback must free prb.

	SG_ERR_CHECK(  SG_daglca__iterator__first(pCtx,
											  &pDagLcaIter,
											  pDagLca,bWantLeaves,
											  &szHid,&nodeType,&gen,&prb)  );
	do
	{
		SG_ERR_CHECK(  (*pfn)(pCtx,szHid,nodeType,gen,prb, pVoidCallerData)  );

		SG_ERR_CHECK(  SG_daglca__iterator__next(pCtx,
												 pDagLcaIter,
												 &bFound,&szHid,&nodeType,&gen,&prb)  );
	} while (bFound);

	// fall thru to common cleanup

fail:
	SG_DAGLCA_ITERATOR_NULLFREE(pCtx, pDagLcaIter);
}

//////////////////////////////////////////////////////////////////

void SG_daglca__get_all_descendant_leaves(SG_context * pCtx,
										  const SG_daglca * pDagLca,
										  const char * pszHid,
										  SG_rbtree ** pprbDescendantLeaves)
{
	SG_rbtree * prb = NULL;
	my_data * pData;
	SG_uint32 k;
	SG_bool bFound;

	SG_NULLARGCHECK_RETURN(pDagLca);
	SG_NONEMPTYCHECK_RETURN(pszHid);
	SG_NULLARGCHECK_RETURN(pprbDescendantLeaves);

	SG_ERR_CHECK(  _cache__lookup(pCtx, pDagLca, pszHid, &bFound, &pData)  );
	if (!bFound)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Node [%s] not found in DAGLCA graph.", pszHid)  );

	SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS(pCtx,&prb,5,NULL)  );

	// since all the leaf bits are [0 ... j-1] and the SPCAs are [j ... n-1]
	// and the LCA is [n], we don't need to walk the entire bitvector.

	for (k=0; k<pDagLca->nrLeafNodes; k++)
	{
		SG_bool b_k;

		SG_ERR_CHECK(  SG_bitvector__get_bit(pCtx, pData->pbvClosure, k, SG_FALSE, &b_k)  );
		if (b_k)
		{
			my_data * pDataK;
			const char * szHidK;

			SG_ERR_CHECK(  SG_vector__get(pCtx, pDagLca->pvecSignificantNodes, k, (void **)&pDataK)  );
			SG_ASSERT(  (pDataK)  &&  "Invalid bit in bitvector"  );

			SG_ASSERT(  (pDataK->nodeType == SG_DAGLCA_NODE_TYPE__LEAF)  );

			SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx,pDataK->pDagnode,&szHidK)  );
			SG_ERR_CHECK(  SG_rbtree__add(pCtx,prb,szHidK)  );
		}
	}

	*pprbDescendantLeaves = prb;
	return;

fail:
	SG_RBTREE_NULLFREE(pCtx, prb);
}

//////////////////////////////////////////////////////////////////

/**
 * This is a DEBUG HACK to let 'vv dump_lca --verbose' plot all
 * of the dagnodes that we touched when computing the LCA.
 * Don't use this for anything real.
 */
void SG_daglca_debug__foreach_visited_dagnode(SG_context * pCtx,
											  const SG_daglca * pDagLca,
											  SG_rbtree_foreach_callback * pfnCB,
											  void * pVoidData)
{
	SG_rbtree_iterator * pIter = NULL;
	const char * pszKey_k;
	my_data * pData_k;
	SG_bool bFound;

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx,
											  &pIter,
											  pDagLca->pRB_FetchedCache,
											  &bFound,
											  &pszKey_k, (void **)&pData_k)  );
	while (bFound)
	{
		SG_ERR_CHECK(  (*pfnCB)(pCtx, pszKey_k, pData_k->pDagnode, pVoidData)  );

		SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx,
												 pIter,
												 &bFound,
												 &pszKey_k, (void **)&pData_k)  );
	}

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
}

//////////////////////////////////////////////////////////////////

/**
 * Return our cached dagnode for the given HID
 * ***IFF*** we have one for it.
 *
 * If this node isn't cached, we return NULL.
 * That does not mean that the dagnode doesn't
 * exist, just that it isn't in our cache.
 *
 */
void SG_daglca__get_cached_dagnode(SG_context * pCtx,
								   const SG_daglca * pDagLca,
								   const char * pszHid,
								   const SG_dagnode ** ppDagnode)
{
	my_data * pData;
	SG_bool bFound = SG_FALSE;

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pDagLca->pRB_FetchedCache, pszHid,
								   &bFound, (void **)&pData)  );
	if (bFound)
		*ppDagnode = pData->pDagnode;
	else
		*ppDagnode = NULL;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Return TRUE if the query node is a descendant (any depth)
 * of the given ancestor node.  That is, is the query's bit
 * set in the ancestor's closure vector.
 *
 * WARNING: This function is only defined on the set of nodes
 * WARNING: that we visited and cached during the DAGLCA computation.
 * WARNING: Normally, this is sufficient for most needs, but I'm
 * WARNING: saying that it won't try to do a __dagquery or __dagwalk if
 * WARNING: it doesn't know the answer.
 *
 */
void SG_daglca__is_descendant(SG_context * pCtx,
							  const SG_daglca * pDagLca,
							  const char * pszHid_ancestor,
							  const char * pszHid_query,
							  SG_bool * pbResult)
{
	const my_data * pDataAncestor;
	const my_data * pDataQuery;
	SG_bool bResult = SG_FALSE;
	SG_bool bFound;

	if (strcmp(pszHid_query, pszHid_ancestor) == 0)
		goto done;	// same HID, so not descendant.

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pDagLca->pRB_FetchedCache, pszHid_ancestor, &bFound, (void **)&pDataAncestor)  );
	if (!bFound)
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "Ancestor '%s' not found in DAGLCA cache.", pszHid_ancestor)  );

	SG_ERR_CHECK(  SG_rbtree__find(pCtx, pDagLca->pRB_FetchedCache, pszHid_query, &bFound, (void **)&pDataQuery)  );
	if (!bFound)
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "Child '%s' not found in DAGLCA cache.", pszHid_query)  );

	if (pDataQuery->genDagnode <= pDataAncestor->genDagnode)
		goto done;	// generation too small, so can't be descendant.

	if (pDataQuery->itemIndex < 0)
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "Child '%s' has itemIndex==(%d) is not significant and doesn't have spot in closure vector.",
						 pszHid_query, pDataQuery->itemIndex)  );

	SG_ERR_CHECK(  SG_bitvector__get_bit(pCtx, pDataAncestor->pbvClosure, pDataQuery->itemIndex, SG_FALSE, &bResult)  );

done:
	*pbResult = bResult;

fail:
	;
}

//////////////////////////////////////////////////////////////////

void SG_daglca__get_dagnum(SG_context * pCtx,
						   const SG_daglca * pDagLca,
						   SG_uint64 * puiDagNum)
{
	SG_UNUSED( pCtx );

	*puiDagNum = pDagLca->iDagNum;
}
