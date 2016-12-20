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
 * @file sg_repo_prototypes.h
 *
 * @details
 * The SG_repo layer is responsible for maintaining the contents
 * of an on-disk repository (or REPO).  The REPO is very stupid.
 * It contains opaque blobs and the essense of the DAG structure.
 *
 * <ol>
 * <li>
 * The SG_repo class is one level of abstraction higher than the
 * blob layer; SG_repo is a wrapper around those calls.
 * Blobs are stored/fetched using the SG_repo__{fetch,store}_* routines.
 * These will (using vtable magic) read and write the contents
 * of opaque blobs to the underlying storage (be it the filesystem
 * directly or a hidden SQL database).  These routines should ONLY be called
 * by the various SG_{type}__load_from_repo() and SG_{type}__save_to_repo()
 * routines.
 * </li>
 *
 * <li>
 * The essense of the DAG structure is maintained by the REPO to
 * allow for efficient exchange of information between multiple
 * instances of a repository during PUSH, PULL, CLONE, and etc
 * operations.
 *
 * A CHANGESET represents the complete contents of a CHECKIN and
 * this is stored in an opaque blob.  Since a CHANGESET is the
 * fundamental unit of work, it can also be called a DAGNODE.
 * Each CHANGESET/DAGNODE can contain one or more PARENT LINKS
 * to CHANGESET(S) that it is based upon; these PARENT LINKS
 * are called EDGES in the DAG. LEAF DAGNODES are generally associated
 * with branch heads, but things are not necessarily that simple.
 * There is a single ROOT DAGNODE corresponding to the initial
 * checkin.
 *
 * The REPO will maintain tables containing the DAG's EDGES and
 * LEAVES.  This allows the DAG graph to be walked without
 * cracking open any of the opaque CHANGESET blobs.  This also
 * allows the DAG graph structure to be exchanged and compared
 * without having *any* CHANGESET blobs - that is, there are
 * several levels of sparseness possible.
 *
 * Note: Even though they are somewhat synonymous, the SG_repo API
 * will use DAG-related terms rather than CHANGESET-related terms to
 * avoid implying that it actually knows what a full CHANGESET is.
 * </li>
 * </ol>
 *
 * Note: the SG_repo API has 2 different styles of usage between
 * the non-dag-based and the dag-based routines.  In the non-dag-based
 * routines, we have SG_{type}__{load_from,store_to}_repo() and they
 * sit *ABOVE* the SG_repo layer -- those routines take care of
 * calling the SG_repo__ routines and build the in-memory data
 * structures (above the SG_repo__ layer).  In the dag-based routines,
 * we have low-level routines to build the in-memory data structures
 * that sit *BELOW* the SG_repo__ layer and let SG_repo__ build them.
 * Hence, THERE IS NO SG_dagnode__load_from_repo().  This is
 * on purpose.
 *
 *
 * To get the contents of an individual CHECKIN/COMMIT, the caller
 * needs to know the desired CHANGESET's HID.  (Handwave) They can
 * walk the DAG (probably via a HEAD or BRANCH NAME LABEL or TAG
 * of some kind) to find the DAGNODE that they want.  They use this
 * HID to fetch the actual CHANGESET from a blob.  This then contains
 * the HIDs of the root TREENODE and DBSTATE.  They can load these
 * objects (also from blobs) recursively until they find what they
 * are looking for.
 */

#ifndef H_SG_REPO_PROTOTYPES_H
#define H_SG_REPO_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Allocate a SG_repo memory object that is BOUND to a specific REPO IMPLEMENTATION.
 * We DO NOT open or create anything on disk.  You must call either
 * SG_repo__create_repo_instance() or SG_repo__open_repo_instance()
 * using the returned SG_repo pointer to actuall access the disk.
 *
 * That is, we bind pRepo->p_vtable but do not set pRepo->p_vtable_instance_data.
 *
 * You can use the returned SG_repo pointer to ask REPO IMPLEMENTATION-specific
 * questions and/or to CREATE or OPEN an actual repository on disk.
 *
 * pszStorage must be one of the defined SG_RIDESC_STORAGE__ values.
 * You can get a list of possible values by calling
 * SG_repo__query_implementation(pCtx,NULL,SG_REPO__QUESTION__VHASH__LIST_REPO_IMPLEMENTATIONS,....)
 *
 * Returns SG_ERR_UNKNOWN_STORAGE_IMPLEMENTATION when an unknown storage type requested.
 *
 */
void SG_repo__alloc(SG_context * pCtx, SG_repo ** ppRepo, const char * pszStorage);

#if defined(DEBUG)
#define SG_REPO__OPEN_REPO_INSTANCE(pCtx,pszDescriptorName,ppRepo)			SG_STATEMENT(  SG_repo * _p = NULL;										\
																   SG_repo__open_repo_instance(pCtx,pszDescriptorName,&_p);								\
																   _sg_mem__set_caller_data(_p,__FILE__,__LINE__,"SG_repo__open_repo_instance");	\
																   *(ppRepo) = _p;											)

#define SG_REPO__FETCH_DAGNODES__ONE(pCtx,pRepo,pHandle,pszidHidChangeset,ppNewDagnode)		SG_STATEMENT(  SG_dagnode * _p = NULL;										\
																   SG_repo__fetch_dagnodes__one(pCtx,pRepo,pHandle,pszidHidChangeset,&_p);								\
																   _sg_mem__set_caller_data(_p,__FILE__,__LINE__,"SG_repo__fetch_dagnodes__one");	\
																   *(ppNewDagnode) = _p;											)
#else
#define SG_REPO__OPEN_REPO_INSTANCE(pCtx,pszDescriptorName,ppRepo)			SG_repo__open_repo_instance(pCtx,pszDescriptorName,ppRepo)
#define SG_REPO__FETCH_DAGNODES__ONE(pCtx,pRepo,pHandle,pszidHidChangeset,ppNewDagnode)		SG_repo__fetch_dagnodes__one(pCtx,pRepo,pHandle,pszidHidChangeset,ppNewDagnode);
#endif


void SG_repo__validate_repo_name(
	SG_context* pCtx,
	const char* pszDescriptorName,
	char** ppszNormalizedDescriptorName // Name with whitespace trimmed. Caller owns the result and must free it.
	);

/**
 * Allocate a new SG_repo memory object and connect it to an existing on-disk REPO.
 * This is a SG_repo__alloc() and SG_repo__open_repo_instance() combination.
 */
void SG_repo__open_repo_instance(
    SG_context* pCtx,
	const char* pszDescriptorName, /**< The members of this descriptor will be treated
								   * as parameters used to locate the
								   * repo. */
	SG_repo** ppRepo
	);

/**
 * Open a repo from its descriptor.
 * This is used only in special cases, and shouldn't be used for typical repository access.
 * Try SG_repo__open_repo_instance for typical repository access.
 */
void SG_repo__open_from_descriptor(
	SG_context* pCtx,
	SG_vhash** ppvhDescriptor, /* < [in/out] Required. On success this routine takes ownership. */
	SG_repo ** ppRepo);		   /* < [out] Required. Caller should free. */

/**
 * Open a repo instance even if the descriptor status is unavailable.
 * Optionally return descriptor status information.
 */
void SG_repo__open_unavailable_repo_instance(
	SG_context* pCtx,
	const char* pszDescriptorName,
	SG_bool* pbAvailable,				/* < [out] Optional. */
	SG_closet__repo_status* pStatus,	/* < [out] Optional. */
	char** ppszStatus,					/* < [out] Optional. Caller must free. */
	SG_repo** ppRepo					/* < [out] Required. Caller must free. */
	);

/**
* Opens a copy of the provided repo.
* If the src repo is unnamed, the copy will be too.
*/
void SG_repo__open_repo_instance__copy(
    SG_context* pCtx,
	const SG_repo* pRepoSrc,
	SG_repo** ppRepo
	);

/**
 * Use an existing SG_repo memory object to create an entirely new REPO on disk.
 *
 * The SG_RIDESC_KEY__STORAGE member in the repo-descriptor must match the
 * value of pszStorage provided in SG_repo__alloc().
 *
 */
void SG_repo__create_repo_instance2(
	SG_context* pCtx,
	SG_vhash** ppvhPartialDescriptor, /**< This descriptor is used
										  * to get the protocol and
										  * any parameters it
										  * requires, but it does not
										  * have the GID. */
	SG_repo* pRepo,
    SG_bool b_indexes,
    const char* psz_hash_method,
    const char* psz_repo_id,
    const char* psz_admin_id
	);

/**
 * Allocate a new SG_repo memory object and create an entirely new REPO on disk.
 * This is a SG_repo__alloc() and SG_repo__create_repo_instance2() combination.
 */
void SG_repo__create_repo_instance(
	SG_context* pCtx,
    const char* psz_descriptor_name,
	const SG_vhash* pvhPartialDescriptor, /**< This descriptor is used
										  * to get the protocol and
										  * any parameters it
										  * requires, but it does not
										  * have the GID. */
    SG_bool b_indexes,
    const char* psz_hash_method,
    const char* psz_repo_id,
    const char* psz_admin_id,
	SG_repo** ppRepo
	);

/**
 * Remove a REPO from disk.
 */
void SG_repo__delete_repo_instance(
	SG_context* pCtx,
	const char* pszDescriptorName
	);

void SG_repo__delete_repo_instance__repo(
	SG_context * pCtx,
	SG_repo** ppRepo); // [in/out] Required. The routine will free the pointer and null caller's copy on success.

/* TODO a fair amount of stuff in this file has moved to sg_repo_misc.c
 * and should therefore get its own header file or get otherwise
 * cleaned up and put in The Right Place. */

/* TODO this routine assumes the dagnum for the version control tree.  should
 * it? */
void SG_repo__create_user_root_directory(
	SG_context* pCtx,
	SG_repo* pRepo,
	const char* pszName,
	SG_changeset** ppChangeset,
	char** ppszidGidActualRoot
	);

#ifndef SG_IOS
void SG_repo__create__completely_new__empty__closet(
	SG_context* pCtx,
    const char* psz_admin_id,
	const char* pszStorage,			// use config: new_repo/driver if null
    const char* psz_hash_method,    // If you pass in NULL, we'll look up the current setting in SG_localsettings.
    const char* pszRidescName
    );

void SG_repo__setup_basic_stuff(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_changeset** ppcs,
    char** ppsz_gid_root
    );
#endif

/**
 * Free an SG_repo object.  Nothing about the repo is altered.
 * Nothing on disk is deleted.  This simply releases the memory.
 *
 */
void SG_repo__free(SG_context* pCtx, SG_repo* pRepo);

void SG_repo__close(SG_context* pCtx, SG_repo* pRepo);

void SG_repo__begin_tx(
    SG_context* pCtx,
	SG_repo* pRepo,
	SG_repo_tx_handle** ppTx
	);

void SG_repo__begin_tx__flags(
    SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint32 flags,
	SG_repo_tx_handle** ppTx
	);

/**
* Commit the repository transaction.
*
* If iDagNum and pDagnode are provided, add the given DAGNODE to the DAG on disk in the REPO.
*
* This is completely self-contained; it will handle DB transaction details
* and update EDGES and LEAVES.
*
* The given dagnode must be frozen before it can be stored.
*
* If no dagnode is to be written, pDagnode should be NULL and iDagNum should be SG_DAGNUM__NONE.
*
* Returns SG_ERR_DAGNODE_ALREADY_EXISTS if the REPO's DAGNODE
* table already contains an entry for this HID.
*/
void SG_repo__commit_tx(
   SG_context* pCtx,
   SG_repo* pRepo,
   SG_repo_tx_handle** ppTx
   );

void SG_repo__abort_tx(
    SG_context* pCtx,
	SG_repo* pRepo,
	SG_repo_tx_handle** ppTx
	);

/**
  Every repo has a unique ID.  All instances/clones of the same
  repo have the same ID.  When you create a brand new repo, use
  SG_repo__alloc__create_new and a new GID will be created
  for it.  When you want to create a new instance/clone of
  an existing repo, call SG_repo__alloc__create_clone and
  provide the other SG_repo.  The new clone will have the
  same GID as the one you are cloning.  When you want to
  create an SG_repo object by initializing it to an existing
  instance of a repo, use SG_repo__alloc__init_existing.
*/

/**
 * returns a repo instance descriptor for this repository.  The
 * descriptor is a vhash, designed to be serializable so that it can
 * be stored.  It can also be passed to SG_repo__alloc__init_existing
 * to reopen the repository later.  The repo descriptor string is
 * analogous to an ODBC connection string.
 *
 */
void SG_repo__get_descriptor(SG_context* pCtx,
							 const SG_repo * pRepo,
							 const SG_vhash** ppvhDescriptor /**< Caller must NOT free this */
	);

/**
 * returns a repo instance descriptor name for this repository
 * if the repository was opened with a descriptor name.  If it
 * was opened by the descriptor itself (using one of the __unnamed
 * variants) NULL is returned.
 */
void SG_repo__get_descriptor_name(
	SG_context* pCtx,
	const SG_repo* pRepo,
	const char** ppszDescriptorName  /**< Caller must NOT free this */
	);

//////////////////////////////////////////////////////////////////

/**
 * Methods for storing/retrieving Blob-based information from a REPO.
 *
 * @defgroup SG_repo__blob_related (REPO) REPO routines to access Blob-based objects.
 * @{
 */

//////////////////////////////////////////////////////////////////

/**
 * Fetch the contents of the Blob identified by HID and write it to
 * the given file.  We assume that the given file is opened for writing
 * and properly positioned (opened for write+truncate).
 *
 * Normally, we think of this as being used to fetch the contents of
 * a user-file that is under version control, but it can actually be
 * used for any Blob.  We make no assumptions about the contents of
 * the given Blob -- unlike the various SG_{type}__load_from_repo()
 * routines which do various data-integrity checking.
 *
 * We optionally verify that digest/hash of the contents of the file being
 * read matches the HID used to access this.
 *
 * Upon any error, we leave the output file in an undefined state.
 */
void SG_repo__fetch_blob_into_file(SG_context* pCtx,
							 SG_repo * pRepo,
							 const char* pszidHidBlob,
							 SG_file * pFileRawData,
                             SG_uint64* pLenRawData);

void SG_repo__fetch_blob_into_file__encoded(
    SG_context* pCtx,
    SG_repo * pRepo,
    const char* pszidHidBlob,
    SG_file * pFile,
    SG_blob_encoding* p_encoding,
    char** ppsz_vcdiff_reference,
    SG_uint64* pi_len_encoded,
    SG_uint64* pi_len_full
    );

void SG_repo__verify_blob(
    SG_context* pCtx,
    SG_repo * pRepo,
    const char* pszidHidBlob,
    SG_uint64* pLenRawData
    );

/**
 * Store the contents of the given file in a Blob in the Repository.
 * We assume that the given file is open for reading.
 * (We will rewind it before reading.)
 * The file will be left at an undefined position.
 *
 * Normally, we think of this as being used to store the contents of
 * a user-file that is under version control, but it can actually be
 * used for any Blob.  We make no assumptions about the contents of
 * the given Blob -- unlike the various SG_{type}__save_to_repo()
 * routines which do various data-integrity checking.
 *
 * Optionally returns the Blob HID for the file.
 *
 * If this file is already present in the repository (a Blob exists with the
 * same HID as what we just computed for it), the HID is still returned and
 * there is no error.
 *
 * The caller must free the returned HID.
 */
void SG_repo__store_blob_from_file(SG_context* pCtx,
							 SG_repo * pRepo,
							 SG_repo_tx_handle* pTx,
                             SG_bool b_dont_bother,
							 SG_file * pFileRawData,
							 char** ppszidHidReturned,
							 SG_uint64* iBlobFullLength);

//////////////////////////////////////////////////////////////////

/**
 * Fetch the blob with the given HID and treat it as a vhash.
 *
 * We optionally verify that the digest/hash of the serialization
 * of the vhash object matches the given HID.
 *
 * The caller must free the returned object.
 *
 * You probably shouldn't use this routine; rather, use one of the
 * SG_{type}__load_from_repo() routines.  Because they will do proper
 * data-integrity checks (and make sure that the received vhash
 * represents the same type of object that we think it does).
 *
 * Note: we this to generically load serialized vhashes.
 *
 */
void SG_repo__fetch_vhash(SG_context* pCtx,
						  SG_repo * pRepo,
						  const char* pszidHidBlob,
						  SG_vhash ** ppVhashReturned);
void SG_repo__fetch_vhash__utf8_fix(SG_context* pCtx,
						  SG_repo * pRepo,
						  const char* pszidHidBlob,
						  SG_vhash ** ppVhashReturned);
void SG_repo__fetch_varray(SG_context* pCtx,
						  SG_repo * pRepo,
						  const char* pszidHidBlob,
						  SG_varray ** ppva);

//////////////////////////////////////////////////////////////////

/**
 * Fetch the blob with the given HID into a buffer.  No processing
 * or parsing is performed (other than decompression).
 *
 * We optionally verify that the digest/hash of the serialization
 * of the buffer matches the given HID.
 *
 * The caller must free the returned buffer.
 *
 * The length is returned as a SG_uint64 (because user-files can be
 * that large) but you shouldn't expect us to be able to actually
 * create a huge buffer on a 32bit machine.
 *
 * You probably shouldn't use this routine to fetch a data structure;
 * rather, use one of the SG_{type}__load_from_repo() routines.
 * Because they will do proper data-integrity checks (and make sure
 * that the received data represents the same type of object that
 * we think it does).
 *
 * You can use this routine to fetch the contents of a user-file
 * into a buffer.
 *
 * TODO we may want to have a fetch_length routine that would get
 * the length of the content of a blob so we could verify that we're
 * not asking for something huge....  or optionally allow the ppBuf
 * to be null and let that mean to just get the length.
 *
 */
void SG_repo__fetch_blob_into_memory(SG_context* pCtx,
							  SG_repo * pRepo,
							  const char* pszidHidBlob,
							  SG_byte ** ppBuf,
							  SG_uint64 * pLenRawData);

/**
 * Store the given buffer in the repository.
 *
 * We optionally return the computed HID for the buffer.
 *
 * If this buffer is already present in the repository (a Blob exists with the
 * same HID as what we just computed for it), the HID is still returned and
 * there is no error.
 *
 * If given, the caller must free the returned HID.
 *
 * You probably shouldn't use this routine to store a data structure;
 * rather, use one of the SG_{type}__save_to_repo() routines.
 * Because they will do proper data-integrity checks.
 *
 * You can use this to store a user-file that you happen to already
 * have in a buffer.
 */
void SG_repo__store_blob_from_memory(SG_context* pCtx,
									 SG_repo * pRepo,
										 SG_repo_tx_handle* pTx,
										 SG_bool b_dont_bother,
										 const SG_byte * pBufRawData,
										 SG_uint32 lenRawData,
										 char** ppszidHidReturned);

//////////////////////////////////////////////////////////////////

#if 0
// TODO this may be a handwave for now, but i was thinking that we'd
// TODO support symlinks by having a blob for the target pathname.  this
// TODO makes the treenode entries similar for files,dirs,symlinks (gid,hid,name,...)
// TODO so we might want to have an __add_symlink_target...
// TODO i was thinking that symlink blobs would have a different type.

// ERIC:  I'll duck this issue for now.  :-)

// Jeff: agreed. if/when we do get back to this we should have a SG_{type}__save_to_repo()
// Jeff: and SG_{type}__load_from_repo() class that takes care of the details and simply
// Jeff: uses the existing SG_repo__{store,fetch}_bytes() routines.  We should not have
// Jeff: SG_repo__ routines for them.
#endif

/**
 * @}
 */

//////////////////////////////////////////////////////////////////

/**
 * Methods for storing/retrieving DAG-based information from a REPO.
 *
 * Level 0 routines are basic primitives.
 * Level 1 routines are convenience routines built upon level 0 routines.
 * Level 2 routines are convenience wrappers around Level 1 and 0 routines.
 *
 * @defgroup SG_repo__dag_related (REPO) REPO routines to access DAG-based objects.
 * @{
 */

//////////////////////////////////////////////////////////////////

/**
 * Fetch an individual DAGNODE from the DAG on disk.
 *
 * This allocates a DAGNODE memory object and populates it; it
 * will be frozen when you get it. You must free the returned
 * DAGNODE.
 *
 * This will internally handle any DB transaction details necessary
 * to give a consistent view of the dagnode.  The transaction is
 * bounded by this call only -- so if you want 2 dagnode, they will
 * be fetched in 2 different transactions.  This should not be a
 * problem because dagnode are immutable and the DB either knows
 * everything or nothing about an individual dagnode.
 *
 */
void SG_repo__fetch_dagnode(SG_context* pCtx,
	SG_repo * pRepo,
	SG_uint64 iDagNum,
	const char* pszidHidChangeset,
	SG_dagnode ** ppNewDagnode);

void SG_repo__find_dag_path(
	SG_context* pCtx,
	SG_repo * pRepo, 
	SG_uint64 iDagNum,
    const char* psz_id_min,
    const char* psz_id_max,
    SG_varray** ppva
	);

void SG_repo__fetch_dagnodes__begin(
	SG_context* pCtx,
	SG_repo * pRepo, 
	SG_uint64 iDagNum,
	SG_repo_fetch_dagnodes_handle ** ppHandle);

void SG_repo__fetch_dagnodes__one(
	SG_context* pCtx,
	SG_repo * pRepo,
	SG_repo_fetch_dagnodes_handle * pHandle,
	const char * pszidHidChangeset,
	SG_dagnode ** ppNewDagnode);

void SG_repo__fetch_dagnodes__end(
	SG_context* pCtx,
	SG_repo * pRepo, 
	SG_repo_fetch_dagnodes_handle ** ppHandle);

/**
 * Fetch the HIDs of all LEAVES in the DAG on disk.
 *
 * The caller must free the returned IDSET.
 *
 * TODO should we make this return a varray containing a vhash
 * where each vhash contains the HID and a branch name?
 *
 * TODO should we define a SG_dagleaves type that wraps all this up?
 *
 * This will internally handle any DB transaction details necessary
 * to give a consistent view of the current set of leaves.  Note
 * that other processes may be adding/deleting leaves as they add
 * new dagnodes.  The set returned here represents a snapshot as
 * of the current time.  This should be safe because dagnodes are
 * immutable and can only be added to the DB.
 *
 * If you base your work on the current leaf set and then reference
 * the dagnodes (and the dagnodes referenced by them and so on), you
 * should be ok.  dagnodes currently being added by other processes
 * simply won't be referenced.
 *
 */
void SG_repo__fetch_dag_leaves(SG_context* pCtx, SG_repo * pRepo, SG_uint64 iDagNum, SG_rbtree ** ppIdsetReturned);

/**
 * WARNING!!!!!!!  Don't use this to walk the DAG!!  This is a convenience
 * method to be used on on dagnode.  Walking the dag from the leaves
 * and going up is much more efficient.
 *
 * Fetch the HIDs of all CHILDREN of the given DAGNODE in the DAG on disk.
 *
 * The caller must free the returned IDSET.
 *
 * TODO should we make this return a varray containing a vhash
 * where each vhash contains the HID and a branch name?
 *
 * This will internally handle any DB transaction details necessary
 * to give a consistent view of the current set of children.  Note
 * that other processes may be adding/deleting leaves as they add
 * new dagnodes.  The set returned here represents a snapshot as
 * of the current time.  This should be safe because dagnodes are
 * immutable and can only be added to the DB.
 *
 * If you base your work on the current children and then reference
 * the dagnodes (and the dagnodes referenced by them and so on), you
 * should be ok.  dagnodes currently being added by other processes
 * simply won't be referenced.
 *
 */
void SG_repo__fetch_dagnode_children(SG_context* pCtx, SG_repo * pRepo, SG_uint64 iDagNum, const char * pszDagnodeID, SG_rbtree ** ppIdsetReturned);

void SG_repo__find_new_dagnodes_since(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
    SG_varray* pva_starting,
    SG_ihash** ppih
	);

void SG_repo__fetch_dagnode_ids(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
    SG_int32 gen_min,
    SG_int32 gen_max,
    SG_ihash** ppih
    );

void SG_repo__fetch_chrono_dagnode_list(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
	SG_uint32 iStartRevNo,
	SG_uint32 iNumRevsRequested,
	SG_uint32* piNumRevsReturned,
	char*** ppaszHidsReturned); /**< Caller must free */

/**
 * Try to insert all of the member dagnodes in the given DAG FRAGMENT
 * into the repo's DAG.
 *
 * This can only succeed if the fragment is completely connected to the
 * dag that we have on disk.  That is, if the contents of this fragment
 * can be inserted without creating a disconnected graph.
 *
 * If the graph would be disconnected, pbConnected is false
 * ***AND*** an IDSET containing the dagnodes in the fragment end-fringe that
 * we don't have.  (the immediate parents of a fragment member that would be
 * an orphan in the graph.)  The caller must free this IDSET.  In this case
 * we do not modify the DAG on disk.
 *
 * If the graph would be connected, we try to insert each individual member
 * dagnode in the fragment.  Since we cannot (efficiently) compute the fragment
 * and dag overlap, we expect that there will be overlap and silently ignore
 * member nodes that we already have on disk.  We return an IDSET of the nodes
 * that were "new to us" and actually added.  (You might use this list to
 * pull the associated changeset blobs, for example.)  The caller must free
 * this IDSET.
 *
 * ***WARNING*** because each member is inserted in an individual transaction,
 * it is possible to do a partial insert because of errors.  If an error is
 * returned ***AND*** idset_added is non-null, then some of the nodes were
 * added.
 */
void SG_repo__check_dagfrag(SG_context* pCtx,
							SG_repo * pRepo,
							SG_dagfrag * pFrag,
							SG_bool * pbConnected,
							SG_rbtree ** ppIdsetReturned,
							SG_stringarray ** ppsaNew
                            );
void SG_repo__store_dagfrag(SG_context* pCtx,
							SG_repo * pRepo,
								 SG_repo_tx_handle* pTx,
								 SG_dagfrag * pFrag);
void SG_repo__done_with_dag(SG_context* pCtx,
							SG_repo * pRepo,
								 SG_repo_tx_handle* pTx,
                                 SG_uint64 dagnum
        );

// TODO change the API for __store_dagnode to not steal ownership of the node.
void SG_repo__store_dagnode(SG_context* pCtx,
							SG_repo * pRepo,
							SG_repo_tx_handle* pTx,
							SG_uint64 iDagNum,
							SG_dagnode * pdn);

//////////////////////////////////////////////////////////////////

#if 0
/**
 * Call SG_repo__dag1__insert_dagnode() and retry N times if the underlying
 * database is busy.
 *
 * This *MAY* reduce the odds of getting a BUSY, but it cannot prevent them,
 * since we cannot completely control the underlying database.  All we can
 * do is to try again a few times.
 */
void SG_repo__dag2__insert_dagnode(SG_context* pCtx,
								   SG_repo * pRepo,
									   SG_repo_tx_handle* pTx,
									   const SG_dagnode * pDagnode,
									   SG_bool bMarkChildAsLeaf,
									   SG_int32 nrRetries);
#endif

/**
 * @}
 */

//////////////////////////////////////////////////////////////////

/**
 * Return the dagnode HID that exactly matches the provided revision ID.
 * This calls through the vtable.
 */
void SG_repo__find_dagnode_by_rev_id(
	SG_context* pCtx,
	SG_repo * pRepo, 
	SG_uint64 iDagNum, 
	SG_uint32 iRevisionId,
	char** ppsz_hid        /**< Caller must free */
	);

/**
 * Return all dagnode HIDs that match a given prefix. 
 * Does not consider revision IDs.
 * This calls through the vtable.
 */
void SG_repo__find_dagnodes_by_prefix(SG_context* pCtx, SG_repo * pRepo, SG_uint64 iDagNum, const char* psz_hid_prefix, SG_rbtree** pprb);

/**
 * Return all blob hids that match a given prefix.
 * If no blobs match, an empty rbtree is returned.
 * This calls through the vtable.
 */
void SG_repo__find_blobs_by_prefix(SG_context* pCtx, SG_repo * pRepo, const char* psz_hid_prefix, SG_rbtree** pprb);


void SG_repo__does_blob_exist(SG_context* pCtx,
							  SG_repo* pRepo,
							  const char* psz_hid,
							  SG_bool* pbExists);
/**
 * Returns the HID of the single dagnode matching psz_hid_prefix only when it 
 * is unambiguous.
 *
 * psz_hid_prefix may be either a partial HID (prefix) or a a full revision ID.
 *
 * If psz_hid_prefix can be parsed into an SG_uint32, 
 * SG_repo__find_dagnode_by_rev_id is called and HID of the matching node, if 
 * any, is returned.
 *
 * If SG_repo__find_dagnode_by_rev_id finds no exact match, or if 
 * psz_hid_prefix cannot be parsed into an SG_uint32, 
 * SG_repo__find_blobs_by_prefix is called. 
 *
 * If there is exactly one match, its HID is returned. 
 * If there are multiple matches, SG_ERR_AMBIGUOUS_ID_PREFIX is thrown. 
 * If there are no matches, SG_ERR_NOT_FOUND is thrown.
 */
void SG_repo__hidlookup__dagnode(
		SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_hid_prefix,
        char** ppsz_hid
        );

/**
 * Given a partial HID (the prefix), if there
 * is only one BLOB in the repo which
 * begins with that prefix, find it.  Otherwise,
 * error.
 *
 * This calls find_by_hid_prefix and returns
 * success iff only one result comes back.
 */
void SG_repo__hidlookup__blob(
		SG_context* pCtx,
        SG_repo* pRepo,
        const char* psz_hid_prefix,
        char** ppsz_hid
        );

void SG_repo__get_dag_lca(
    SG_context* pCtx,
    SG_repo* pdbc,
    SG_uint64 iDagNum,
    const SG_rbtree* prbNodes,
    SG_daglca ** ppDagLca
    );

// TODO Consider also returning the strlen() of hashes computed with this hash-method.
void SG_repo__get_hash_method(
	    SG_context* pCtx,
        SG_repo* pRepo,
        char** ppsz_hash_method /**< caller must free this */
        );

void SG_repo__get_repo_id(
	    SG_context* pCtx,
        SG_repo* pRepo,
        char** ppsz_id /**< This is a GID.  caller must free this */
        );

void SG_repo__get_admin_id(
	    SG_context* pCtx,
        SG_repo* pRepo,
        char** ppsz_id /**< This is a GID.  caller must free this */
        );

void SG_repo__rebuild_indexes(
	SG_context*,
    SG_repo* pRepo
    );

void SG_repo__dbndx__query_record_history(
	SG_context*,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
    const char* psz_recid,  // this is NOT the hid of a record.  it is the zing recid
    const char* psz_rectype,
    SG_varray** ppva
	);

void SG_repo__dbndx__query_multiple_record_history(
	SG_context*,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
    const char* psz_rectype,
    SG_varray* pva_recids,
    const char* psz_field,
    SG_vhash** ppvh
	);

void SG_repo__dbndx__query__raw_history(
	SG_context* pCtx,
    SG_repo* pRepo, 
    SG_uint64 iDagNum,
    SG_int64 min_timestamp,
    SG_int64 max_timestamp,
    SG_vhash** ppvh
	);

void SG_repo__dbndx__query__recent(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
    const char* psz_rectype,
    const SG_varray* pcrit,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_fields,
    SG_varray** ppva
	);

void SG_repo__dbndx__query__fts(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
	const char* pidState,
    const char* psz_rectype,
    const char* psz_field_name,
    const char* psz_keywords,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_fields,
    SG_varray** ppva
	);

void SG_repo__dbndx__query__one(
	SG_context*,
    SG_repo* pRepo,
    SG_uint64 iDagNum,

	const char* pidState,          /**< The ID of the changeset representing
	                                 the state of the db on which you want to
	                                 filter.  If you pass NULL here, the query
	                                 will return all records that match the
	                                 criteria, without filtering on any db
	                                 state.  If you pass a changeset id, the
	                                 query will filter the results for all
	                                 records that are a member of that state.
	                                 */

    const char* psz_rectype,
    const char* psz_field_name,
    const char* psz_field_value,

    SG_varray* pva_slice_fields,
    SG_vhash** ppvh
	);

void SG_repo__dbndx__query__prep(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
	const char* pidState
	);

void SG_repo__dbndx__query__count(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
	const char* pidState,
    const char* psz_rectype,
	const SG_varray* pcrit,
    SG_uint32* pi_count
	);

void SG_repo__dbndx__query(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
	const char* pidState,
    const char* psz_rectype,
	const SG_varray* pcrit,
	const SG_varray* pSort,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_fields,
    SG_varray** ppva
	);

void SG_repo__treendx__get_paths(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        SG_vhash* pvh,
        SG_vhash** ppvh
        );

void SG_repo__treendx__get_path_in_dagnode(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_search_item_gid,
        const char* psz_changeset,
        SG_string** ppPathOfFoundObject,
        SG_treenode_entry ** ppTreeNodeEntry
        );

void SG_repo__hash__begin(
	SG_context* pCtx,
    SG_repo * pRepo,
    SG_repo_hash_handle** ppHandle
    );

void SG_repo__hash__chunk(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_hash_handle* pHandle,
    SG_uint32 len_chunk,
    const SG_byte* p_chunk
    );

void SG_repo__hash__end(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_hash_handle** pHandle,
    char** ppsz_hid_returned
    );

void SG_repo__hash__abort(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_hash_handle** pHandle
    );

void SG_repo__query_implementation(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_uint16 question,
    SG_bool* p_bool,
    SG_int64* p_int,
    char* p_buf_string,
    SG_uint32 len_buf_string,
    SG_vhash** pp_vhash
    );

void SG_repo__query_blob_existence(
	SG_context* pCtx,
	SG_repo * pRepo,
	SG_stringarray* psaBlobHids,
	SG_stringarray** ppsaNonexistentBlobs
	);

void SG_repo__replace_empty(
	SG_context* pCtx,
    const char* psz_repo_id,
    const char* psz_admin_id,
    const char* psz_hash_method,
	const char* psz_new_descriptor_name,
	SG_closet__repo_status status,
	SG_repo** ppRepo
	);
void SG_repo__create_empty(
	SG_context* pCtx,
    const char* psz_repo_id,
    const char* psz_admin_id,
    const char* psz_hash_method,
	const char* psz_new_descriptor_name,
	SG_closet__repo_status status,
	SG_repo** ppRepo
	);

void SG_repo__list_dags(SG_context* pCtx, SG_repo* pRepo, SG_uint32* piCount, SG_uint64** paDagNums);

/**
 * Returns an rbtree whose keys are hex strings of all the dagnums in the repo. 
 */
void SG_repo__list_dags__rbtree(SG_context* pCtx, SG_repo* pRepo, SG_rbtree** pprbDagnums);

void SG_repo__store_blob__begin(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    SG_blob_encoding blob_format,
    const char* psz_hid_vcdiff_reference,
    SG_uint64 lenFull,
	SG_uint64 lenEncoded,
    const char* psz_hid_known,
    SG_repo_store_blob_handle** ppHandle
    );

void SG_repo__store_blob__chunk(
	SG_context* pCtx,
    SG_repo * pRepo,
    SG_repo_store_blob_handle* pHandle,
    SG_uint32 len_chunk,
    const SG_byte* p_chunk,
    SG_uint32* piWritten
    );
void SG_repo__store_blob__end(
    SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    SG_repo_store_blob_handle** pHandle,
    char** ppsz_hid_returned
    );

void SG_repo__store_blob__abort(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    SG_repo_store_blob_handle** ppHandle
    );

void SG_repo__get_blob(
    SG_context* pCtx,
	SG_repo * pRepo,
    const char* psz_hid_blob,
    SG_blob** ppHandle
    );
             
void SG_repo__release_blob(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_blob** ppBlob
    );

void SG_repo__fetch_blob__begin(
	SG_context* pCtx,
    SG_repo * pRepo,
    const char* psz_hid_blob,
    SG_bool b_convert_to_full,
    SG_blob_encoding* pBlobFormat,
    char** ppsz_hid_vcdiff_reference,
    SG_uint64* pLenRawData,
    SG_uint64* pLenFull,
    SG_repo_fetch_blob_handle** ppHandle
    );

void SG_repo__fetch_blob__chunk(
	SG_context* pCtx,
    SG_repo * pRepo,
    SG_repo_fetch_blob_handle* pHandle,
    SG_uint32 len_buf,
    SG_byte* p_buf,
    SG_uint32* p_len_got,
    SG_bool* pb_done
    );

void SG_repo__fetch_blob__end(
    SG_context* pCtx,
    SG_repo * pRepo,
    SG_repo_fetch_blob_handle** ppHandle
    );

void SG_repo__fetch_blob__abort(
	SG_context* pCtx,
    SG_repo * pRepo,
    SG_repo_fetch_blob_handle** ppHandle
    );

void SG_repo__fetch_repo__fragball(
	SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint32 version,
	const SG_pathname* pFragballDirPathname,
	char** ppszFragballName
	);

void SG_repo__list_blobs(
	    SG_context* pCtx,
        SG_repo * pRepo,
        SG_blob_encoding blob_encoding, /**< only return blobs of this encoding */
        SG_uint32 limit,                /**< only return this many entries */
        SG_uint32 offset,               /**< skip the first group of the sorted entries */
        SG_blobset** ppbs               /**< Caller must free */
        );

void SG_repo__verify__dbndx_states(
    SG_context* pCtx,
    SG_repo * pRepo,
    SG_uint64 dagnum,
    SG_vhash** pp_vhash
    );

void SG_repo__verify__dag_consistency(
    SG_context* pCtx,
    SG_repo * pRepo,
    SG_uint64 dagnum,
    SG_vhash** pp_vhash
    );

void SG_repo__query_audits(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        SG_int64 min_timestamp,
        SG_int64 max_timestamp,
        const char* psz_userid,
        SG_varray** ppva
        );

void SG_repo__list_audits_for_given_changesets(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        SG_varray* ppva_csid_list,
        SG_varray** ppva
        );

void SG_repo__lookup_audits(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        const char* psz_hid_cs_target,
        SG_varray** ppva
        );

void SG_repo__store_audit(
    SG_context* pCtx,
    SG_repo * pRepo,
    SG_repo_tx_handle* pTx,
    const char* psz_csid,
    SG_uint64 dagnum,
    const char* psz_userid,
    SG_int64 when
    );

//////////////////////////////////////////////////////////////////

/**
 * Utility routine in sg_repo_misc.c to call __hash__begin, __hash__chunk,
 * and __hash__end for a single buffer of data and compute the hash.
 *
 * Returns an HID in a buffer that you must free.
 */
void SG_repo__alloc_compute_hash__from_bytes(SG_context * pCtx,
											 SG_repo * pRepo,
											 SG_uint32 lenBuf,
											 const SG_byte * pBuf,
											 char ** ppsz_hid_returned);

/**
 * Utility wrapper when you have a SG_string rather than a buffer and length.
 *
 * Returns an HID in a buffer that you must free.
 */
void SG_repo__alloc_compute_hash__from_string(SG_context * pCtx,
											  SG_repo * pRepo,
											  const SG_string * pString,
											  char ** ppsz_hid_returned);

/**
 * Utility wrapper when you have a file rather than a buffer or string.
 *
 * The given file must be opened for reading; we will seek to the beginning
 * and read it.  We leave the file position in undefined (probably the end,
 * but you shouldn't assume that).
 *
 * WARNING: There is an inherent race condition here -- we must assume that
 * no one is changing the file as we read it.
 *
 * Returns an HID in a buffer that you must free.
 */
void SG_repo__alloc_compute_hash__from_file(SG_context * pCtx,
											SG_repo * pRepo,
											SG_file * pFile,
											char ** ppsz_hid_returned);

void SG_repo__treendx__search_changes(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        SG_uint64 iDagNum,
        SG_stringarray* psa_gids,
        SG_vhash** ppvh
        );

void SG_repo__zip(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        const char* psz_hid_cs, 
        const char* psz_path
        );

void SG_repo__install_implementation(
        SG_context* pCtx, 
        sg_repo__vtable* pvtable
        );

void SG_repo__free_implementation_plugin_list(
        SG_context* pCtx
        );

void SG_repo__get_descriptor__ref(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_vhash** ppvh
        );

void SG_repo__get_instance_data(
        SG_context* pCtx,
        SG_repo* pRepo,
        void** pp
        );

void SG_repo__set_instance_data(
        SG_context* pCtx,
        SG_repo* pRepo,
        void* p
        );

#if defined(DEBUG)
#define SG_REPO__USER_MASTER__OPEN(pCtx, ppRepo) SG_STATEMENT(  SG_repo * _p = NULL;														\
																SG_repo__user_master__open(pCtx, &_p);										\
																_sg_mem__set_caller_data(_p,__FILE__,__LINE__,"SG_repo__user_master_open");	\
																*(ppRepo) = _p;																)
#else
#define SG_REPO__USER_MASTER__OPEN(pCtx, ppRepo) SG_repo__user_master__open(pCtx, ppRepo)
#endif


void SG_repo__user_master__open(
    SG_context* pCtx,
	SG_repo** ppRepo
	);

void SG_repo__user_master__create(
	SG_context* pCtx,
	const char* psz_admin_id,	// required
	const char* pszStorage,		// use config: new_repo/driver if null
	const char* psz_hash_method	// If you pass in NULL, we'll look up the current setting in SG_localsettings.
	);

void SG_repo__user_master__exists(SG_context* pCtx, SG_bool* pbExists);

void SG_repo__user_master__get_adminid(SG_context* pCtx, char** ppszAdminId);

void SG_repo__dbndx__make_delta_from_path(
	SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 dagnum,
    SG_varray* pva_path,
    SG_uint32 flags,
    SG_vhash* pvh_add,
    SG_vhash* pvh_remove
	);

void SG_repo__store_blob__changeset(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    const char* psz_json,
    SG_uint32 len_full,
    const char* psz_hid_known,
    SG_vhash** ppvh,
    char** ppsz_hid_returned
    );

void SG_repo__store_blob__dbrecord(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    const char* psz_json,
    SG_uint32 len_full,
    SG_uint64 dagnum,
    const char* psz_hid_known,
    SG_vhash** ppvh,
    char** ppsz_hid_returned
    );

void SG_repo__store_blob__dbtemplate(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    const char* psz_json,
    SG_uint32 len_full,
    SG_uint64 dagnum,
    const char* psz_hid_known,
    SG_vhash** ppvh,
    char** ppsz_hid_returned
    );

END_EXTERN_C;

#endif//H_SG_REPO_PROTOTYPES_H

