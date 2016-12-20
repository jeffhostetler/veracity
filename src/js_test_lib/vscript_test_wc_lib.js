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

// This file contains utility routines using the new WC
// re-write of PendingTree.

//////////////////////////////////////////////////////////////////
// Some of the oldest tests use the following basic boolean
// methods to verify/dump clean/dirty status without worrying
// about the details of the dirt (or explicitly predicting
// what the dirt should be).
//
// I've replaced the guts of those routines with calls to
// the following routines, but left the existing calls so
// I don't have to re-write all of the tests.  New tests
// should avoid these.
//
//    testlib.statusEmpty()
//    testlib.statusEmptyWhenIgnoringIgnores()
//    testlib.statusDirty()
//    verifyCleanStatus()
//    verifyCleanStatusWhenIgnoringIgnores()
//    dumpTree()
//    reportTreeObjectStatus()
// 

function vscript_test_wc__statusEmpty()
{
    var status = sg.wc.status();
    var length = status.length;

    if (length > 0)
    {
	print("statusEmpty() found " + length + " dirty items:");
	print(sg.to_json__pretty_print(status));
    }

    return (testlib.ok((length==0), "status should be empty"));
}

// Is the status empty not counting any IGNORED items.
//
function vscript_test_wc__statusEmptyExcludingIgnores()
{
    var sum = 0;
    var status = sg.wc.status();
    
    for (var s_index in status)
	if (! status[s_index].status.isIgnored)
	    sum++;

    return (testlib.ok( (sum==0), "status empty excluding ignores") );
}

// Is the status empty when ignoring the ignores settings
// (that is when treating IGNORED items as if FOUND).
//
function vscript_test_wc__statusEmptyWhenIgnoringIgnores()
{
    var status = sg.wc.status({"no_ignores":true});
    var length = status.length;

    if (length > 0)
    {
	print("statusEmptyWhenIgnoringIgnores() found " + length + " dirty items:");
	print(sg.to_json__pretty_print(status));
    }

    return (testlib.ok((length==0), "status should be empty"));
}

function vscript_test_wc__statusDirty()
{
    var status = sg.wc.status();
    var length = status.length;

    return (testlib.ok((length>0), "status should show some pending changes"));
}

function vscript_test_wc__verifyCleanStatus()
{
    var status = sg.wc.status();
    var length = status.length;

    return (length == 0);
}

function vscript_test_wc__verifyCleanStatusWhenIgnoringIgnores()
{
    var status = sg.wc.status({"no_ignores":true});
    var length = status.length;

    return (length == 0);
}

//////////////////////////////////////////////////////////////////


function vscript_test_wc__dumpTree(message)
{
    var status = sg.wc.status();
    var length = status.length;

    print("DumpTree: [" + length + " dirty items]: " + message);
    if (length > 0)
	print(sg.to_json__pretty_print(status));
}

function vscript_test_wc__reportTreeObjectStatus(objPath, noPrint)
{
    var result = "";
    var fullPath = "@/" + objPath.trim("/");

    try
    {
	var status = sg.wc.status({"src":fullPath, "depth":0});
	var length = status.length;

	if (length > 0)
	    result = sg.to_json__pretty_print(status);
	else
	    result = "<objNotDirty>";
    }
    catch (e)
    {
	result = "<objNotFound>";
    }

    if (!noPrint && !testlib.failuresOnly)
        indentLine("-> (WC) PendingTree status for " + objPath + " = " + result);
    return (result);
}

//////////////////////////////////////////////////////////////////

function __vscript_test_wc__commit(message)
{
    if (message == undefined)
	message = "No Message.";

    var hid = sg.wc.commit( { "message" : message } );

    indentLine("=> WC: commit created " + hid);

    return hid;
}

// Convenience wrapper for sg.wc.commit()
// that supplies a message when called
// without an arg.  (Most of the commits
// in the PendingTree version just did:
//    sg.pending_tree.commit();
// This is a wrapper for them.
function vscript_test_wc__commit(message)
{
    var bTestOneArgStatus = testlib.defined.SG_NIGHTLY_BUILD;
    if (bTestOneArgStatus)
    {
//	print("help:");
//	print(sg.to_json__pretty_print(sg.wc.status()));

	var parents = sg.wc.parents();
	var s1a = sg.wc.status({"rev":parents[0]});
	if (parents.length > 1)
	    var s1b = sg.wc.status({"rev":parents[1]});
    }

    var hid = __vscript_test_wc__commit(message);

    if (bTestOneArgStatus)
    {
	var s2a = sg.vv2.status({"revs":[{"rev":parents[0]},{"rev":hid}]});
	vscript_test_wc__test_1_revspec_status("Parent 0", s1a, s2a);

	if (parents.length > 1)
	{
	    var s2b = sg.vv2.status({"revs":[{"rev":parents[1]},{"rev":hid}]});
	    vscript_test_wc__test_1_revspec_status("Parent 1", s1b, s2b);
	}
    }

    return hid;
}

// Wrapper for sg.wc.commit() to get
// consistent logging with the above
// wrapper for those places where there
// were more args passed to 
//    sg.pending_tree.commit();
// The caller needs to repack the args
// using the new named parameters.
function vscript_test_wc__commit_np( np )
{
    var hid = sg.wc.commit( np );

    indentLine("=> WC: commit created " + hid);

    return hid;
}

function vscript_test_wc__commit_np__expect_error( np, msg )
{
    try
    {
	vscript_test_wc__commit_np( np );
	testlib.ok( (0), "Did not receive error.  Expectd '" + msg + "'." );
    }
    catch (e)
    {
	print("Received error: '" + e.toString() + "'.");
	testlib.ok( (e.toString().indexOf(msg) >= 0), "Error message should contain expected string '" + msg + "'.");
    }
}

function vscript_test_wc__move( src, destdir )
{
    indentLine("=> WC: move " + src + " " + destdir);

    sg.wc.move( { "src" : src,
		  "dest_dir" : destdir } );
}

function vscript_test_wc__move__expect_error( src, destdir, msg )
{
    try
    {
	vscript_test_wc__move( src, destdir );
	testlib( (0), "Did not receive error.  Expected '" + msg + "'." );
    }
    catch (e)
    {
	print("Received error: '" + e.toString() + "'.");
	testlib.ok( (e.toString().indexOf(msg) >= 0), "Error message should contain expected string '" + msg + "'.");
    }
}

function vscript_test_wc__move_n( src_array, destdir )
{
    indentLine("=> WC: move N to " + destdir);

    sg.wc.move( { "src" : src_array,
		  "dest_dir" : destdir } );
}

function vscript_test_wc__move__ignore_error( src, destdir )
{
    try
    {
	vscript_test_wc__move( src, destdir );
    }
    catch (e)
    {
    }
}

function vscript_test_wc__add( src )
{
    indentLine("=> WC: add " + src);

    sg.wc.add( { "src" : src } );
}

function vscript_test_wc__add_np( np )
{
    indentLine("=> WC: add np");

    sg.wc.add( np );
}

function vscript_test_wc__add__ignore_error( src )
{
    try
    {
	vscript_test_wc__add( src );
    }
    catch (e)
    {
    }
}

function vscript_test_wc__rename( src, newname )
{
    indentLine("=> WC: rename " + src + " " + newname);

    sg.wc.rename( { "src" : src,
		    "entryname" : newname } );
}

function vscript_test_wc__rename__ignore_error( src, newname )
{
    try
    {
	vscript_test_wc__rename( src, newname );
    }
    catch (e)
    {
    }
}

// The original PendingTree based helper routines
// had both a __remove_file and a __remove_dir
// wrapper.  Not sure why.  Leaving it like this
// during first port to WC.
function vscript_test_wc__remove_file( src )
{
    indentLine("=> WC: remove_file " + src );

    sg.wc.remove( { "src" : src } );
}

function vscript_test_wc__remove_dir( src )
{
    indentLine("=> WC: non-recursive remove_dir " + src );

    sg.wc.remove( { "src" : src } );
}

function vscript_test_wc__remove( src )
{
    indentLine("=> WC: remove " + src );

    sg.wc.remove( { "src" : src } );
}

function vscript_test_wc__remove_np( np )
{
    indentLine("=> WC: remove_np " + np.src );

    sg.wc.remove( np );
}

function vscript_test_wc__remove_np__expect_error( np, msg )
{
    try
    {
	vscript_test_wc__remove_np( np );
	testlib.ok( (0), "Did not receive error.  Expected '" + msg + "'." );
    }
    catch (e)
    {
	print("Received error: '" + e.toString() + "'.");
	testlib.ok( (e.toString().indexOf(msg) >= 0), "Error message should contain expected string '" + msg + "'.");
    }
}

function vscript_test_wc__remove__keep( src )
{
    indentLine("=> WC: remove__keep " + src );

    sg.wc.remove( { "src" : src, "keep" : true } );
}

function vscript_test_wc__addremove()
{
    indentLine("=> WC: addremove");

    sg.wc.addremove();
}

function vscript_test_wc__addremove_np( np )
{
    indentLine("=> WC: addremove");

    sg.wc.addremove( np );
}

function vscript_test_wc__checkout_np( np )
{
    indentLine("=> WC: checkout");

    sg.wc.checkout( np );
}

function vscript_test_wc__checkout_np__expect_error( np, msg )
{
    try
    {
	vscript_test_wc__checkout_np( np );
	testlib.ok( (0), "Did not receive error. Expected '" + msg + "'." );
    }
    catch (e)
    {
	print("Received error: '" + e.toString() + "'.");
	testlib.ok( (e.toString().indexOf(msg) >= 0), "Error message should contain expected string '" + msg + "'.");
    }
}

function vscript_test_wc__init_np( np )
{
    indentLine("=> WC: init");

    sg.vv2.init_new_repo( np );
}

function vscript_test_wc__init_np__expect_error( np, msg )
{
    try
    {
	vscript_test_wc__init_np( np );
	testlib.ok( (0), "Did not receive error. Expected '" + msg + "'." );
    }
    catch (e)
    {
	print("Received error: '" + e.toString() + "'.");
	testlib.ok( (e.toString().indexOf(msg) >= 0), "Error message should contain expected string '" + msg + "'.");
    }
}

function vscript_test_wc__merge_np( np )
{
    indentLine("=> WC: merge");

    sg.wc.merge( np );
}

function vscript_test_wc__merge_np__expect_error( np, msg )
{
    try
    {
	vscript_test_wc__merge_np( np );
	testlib.ok( (0), "Did not receive error.  Expected '" + msg + "'." );
    }
    catch (e)
    {
	print("Received error: '" + e.toString() + "'.");
	testlib.ok( (e.toString().indexOf(msg) >= 0), "Error message should contain expected string '" + msg + "'.");
    }
}

function vscript_test_wc__print_section_divider(message)
{
    print("");
    print("================================================================");
    print( message );
    print("================================================================");
    print("");
}

//////////////////////////////////////////////////////////////////

// Wrapper to confirm the contents of a STATUS
// using an old-style expect-array.  
//
// The caller does the status and therefore sets
// the arguments to the status call.  YOU PROBABLY
// WANT TO CALL: vscript_test_wc__confirm_wii()
// instead of this one.
//
// This version does what used to be known as:
//    this.confirm__when_ignoring_ignores(expected)
//    this.confirm(expected)
//    this.confirm_expectations()
//    this.confirm_all_claimed()
//
// We are given something of the form:
//    var expect = new Array;
//    expect["Added"] = [ "@/dir_A", "@/file_0" ];
//    expect["Found"] = [ "@/dir_C" ];
//
// We get new canonical status in the form of:
//    var status = [ { "status" : { "isFile" : true,
//                                  "isNonDirModified":true },
//                     "path" : "@/file_0" },
//                   { "status" : { "isDirectory" : true,
//                                  "isAdded" : true },
//                     "path" : "@/dir_A/" } ];
// (I've omitted a bunch of fields we don't care about here.)
//
//
// 2012/02/09 STATUS now reports deleted items using the baselien
//            ref_path (or in the case of something merge created,
//            the other ref_path).  So all EXPECT paths in the
//            "Removed" section should be 'b' domain or 'o' domain
//            and not live ('/' domain).
//
// We do not deal with the full generality provided by the domain
// prefix concept for repo-paths.
//
function vscript_test_wc__confirm(expected, status)
{
    indentLine("=> (WC) confirm");

    if (status == undefined)
	status = sg.wc.status( );

    var table =
	[ 
	    //////////////////////////////////////////////////////////////////
	    // 'vv status' of WC can create these:
	    //////////////////////////////////////////////////////////////////
	    { "s" : "Added",                          "k" : [ "isAdded" ] },
	    { "s" : "Added (Merge)",                  "k" : [ "isMergeCreated" ] },
	    { "s" : "Added (Update)",                 "k" : [ "isUpdateCreated" ] },
	    { "s" : "Removed",                        "k" : [ "isRemoved" ] },
	    { "s" : "Modified",                       "k" : [ "isNonDirModified" ] },
	    { "s" : "Attributes",                     "k" : [ "isModifiedAttributes" ] },

	    { "s" : "Renamed",                        "k" : [ "isRenamed" ] },
	    { "s" : "Moved",                          "k" : [ "isMoved" ] },
	    { "s" : "Lost",                           "k" : [ "isLost" ] },
	    { "s" : "Found",                          "k" : [ "isFound" ] },
	    { "s" : "Ignored",                        "k" : [ "isIgnored" ] },
	    { "s" : "Sparse",                         "k" : [ "isSparse" ] },
	    { "s" : "Existence (B,!C,WC)",            "k" : [ "isExistence_ABM" ] },
	    { "s" : "Existence (!B,C,WC)",            "k" : [ "isExistence_ACM" ] },
	    { "s" : "Auto-Merged",                    "k" : [ "isAutoMerged" ] },
	    { "s" : "Auto-Merged (Edited)",           "k" : [ "isAutoMergedEdited" ] },
	    { "s" : "Resolved",                       "k" : [ "isResolved" ] },
	    { "s" : "Unresolved",                     "k" : [ "isUnresolved" ] },

	    { "s" : "Choice Resolved (Existence)",           "k" : [ "isResolvedExistence" ] },
	    { "s" : "Choice Resolved (Name)",                "k" : [ "isResolvedName" ] },
	    { "s" : "Choice Resolved (Location)",            "k" : [ "isResolvedLocation" ] },
	    { "s" : "Choice Resolved (Attributes)",          "k" : [ "isResolvedAttributes" ] },
	    { "s" : "Choice Resolved (Contents)",            "k" : [ "isResolvedContents" ] },

	    { "s" : "Choice Unresolved (Existence)",         "k" : [ "isUnresolvedExistence" ] },
	    { "s" : "Choice Unresolved (Name)",              "k" : [ "isUnresolvedName" ] },
	    { "s" : "Choice Unresolved (Location)",          "k" : [ "isUnresolvedLocation" ] },
	    { "s" : "Choice Unresolved (Attributes)",        "k" : [ "isUnresolvedAttributes" ] },
	    { "s" : "Choice Unresolved (Contents)",          "k" : [ "isUnresolvedContents" ] },

	    { "s" : "Reserved",                       "k" : [ "isReserved" ] },

	    { "s" : "Locked (by You)",                "k" : [ "isLockedByUser" ] },
	    { "s" : "Locked (by Others)",             "k" : [ "isLockedByOther" ] },
	    { "s" : "Locked (Waiting)",               "k" : [ "isLockedWaiting" ] },
	    { "s" : "Locked (Pending Violation)",     "k" : [ "isPendingLockViolation" ] },
	    { "s" : "Locked (Comitted Violation)",    "k" : [ "isComittedLockViolation" ] },

	    // "Unchanged".  Don't care right now.

	    //////////////////////////////////////////////////////////////////
	    // In addition to *some* of the above, 'vv mstatus' of WC can create these:
	    //////////////////////////////////////////////////////////////////
	    { "s" : "Added (WC)",                     "k" : [ "isAdded" ] },
	    { "s" : "Added (B)",                      "k" : [ "isAdded" ] },
	    { "s" : "Added (C)",                      "k" : [ "isAdded" ] },
	    { "s" : "Added (B,C)",                    "k" : [ "isAdded" ] },
	    { "s" : "Removed (WC)",                   "k" : [ "isRemoved" ] },
	    { "s" : "Removed (B,WC)",                 "k" : [ "isRemoved" ] },
	    { "s" : "Removed (C,WC)",                 "k" : [ "isRemoved" ] },
	    { "s" : "Removed (B,C,WC)",               "k" : [ "isRemoved" ] },

	    { "s" : "Modified (WC)",                  "k" : [ "isNonDirModified" ] },
	    // TestAndSet_BCM()
	    { "s" : "Modified (WC!=B,WC!=C,B!=C)",    "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (WC!=B,WC==C)",         "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (WC==B,WC!=C)",         "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (WC!=B,WC!=C,B==C)",    "k" : [ "isNonDirModified" ] },
	    // TestAndSet_AxM(B)
	    { "s" : "Modified (WC!=A,B!=A,WC!=B)",    "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (B!=A,WC==B)",          "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (B==A,WC!=B)",          "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (WC!=B,WC==A)",         "k" : [ "isNonDirModified" ] },
	    // TestAndSet_AxM(C)
	    { "s" : "Modified (WC!=A,C!=A,WC!=C)",    "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (C!=A,WC==C)",          "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (C==A,WC!=C)",          "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (WC!=C,WC==A)",         "k" : [ "isNonDirModified" ] },
	    // TestAndSet_ABCM()
	    { "s" : "Modified (WC!=A,WC!=B,WC!=C)",   "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (WC!=A,WC!=B,WC==C)",   "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (WC!=A,WC==B,WC!=C)",   "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (WC!=A,WC==B,WC==C)",   "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (WC==A,WC!=B,WC!=C)",   "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (WC==A,WC!=B,WC==C)",   "k" : [ "isNonDirModified" ] },
	    { "s" : "Modified (WC==A,WC==B,WC!=C)",   "k" : [ "isNonDirModified" ] },

	    { "s" : "Renamed (WC)",                   "k" : [ "isRenamed" ] },
	    // TestAndSet_BCM()
	    { "s" : "Renamed (WC!=B,WC!=C,B!=C)",    "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (WC!=B,WC==C)",         "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (WC==B,WC!=C)",         "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (WC!=B,WC!=C,B==C)",    "k" : [ "isRenamed" ] },
	    // TestAndSet_AxM(B)
	    { "s" : "Renamed (WC!=A,B!=A,WC!=B)",    "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (B!=A,WC==B)",          "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (B==A,WC!=B)",          "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (WC!=B,WC==A)",         "k" : [ "isRenamed" ] },
	    // TestAndSet_AxM(C)
	    { "s" : "Renamed (WC!=A,C!=A,WC!=C)",    "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (C!=A,WC==C)",          "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (C==A,WC!=C)",          "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (WC!=C,WC==A)",         "k" : [ "isRenamed" ] },
	    // TestAndSet_ABCM()
	    { "s" : "Renamed (WC!=A,WC!=B,WC!=C)",   "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (WC!=A,WC!=B,WC==C)",   "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (WC!=A,WC==B,WC!=C)",   "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (WC!=A,WC==B,WC==C)",   "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (WC==A,WC!=B,WC!=C)",   "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (WC==A,WC!=B,WC==C)",   "k" : [ "isRenamed" ] },
	    { "s" : "Renamed (WC==A,WC==B,WC!=C)",   "k" : [ "isRenamed" ] },

	    { "s" : "Moved (WC)",                     "k" : [ "isMoved" ] },
	    // TestAndSet_BCM()
	    { "s" : "Moved (WC!=B,WC!=C,B!=C)",      "k" : [ "isMoved" ] },
	    { "s" : "Moved (WC!=B,WC==C)",           "k" : [ "isMoved" ] },
	    { "s" : "Moved (WC==B,WC!=C)",           "k" : [ "isMoved" ] },
	    { "s" : "Moved (WC!=B,WC!=C,B==C)",      "k" : [ "isMoved" ] },
	    // TestAndSet_AxM(B)
	    { "s" : "Moved (WC!=A,B!=A,WC!=B)",      "k" : [ "isMoved" ] },
	    { "s" : "Moved (B!=A,WC==B)",            "k" : [ "isMoved" ] },
	    { "s" : "Moved (B==A,WC!=B)",            "k" : [ "isMoved" ] },
	    { "s" : "Moved (WC!=B,WC==A)",           "k" : [ "isMoved" ] },
	    // TestAndSet_AxM(C)
	    { "s" : "Moved (WC!=A,C!=A,WC!=C)",      "k" : [ "isMoved" ] },
	    { "s" : "Moved (C!=A,WC==C)",            "k" : [ "isMoved" ] },
	    { "s" : "Moved (C==A,WC!=C)",            "k" : [ "isMoved" ] },
	    { "s" : "Moved (WC!=C,WC==A)",           "k" : [ "isMoved" ] },
	    // TestAndSet_ABCM()
	    { "s" : "Moved (WC!=A,WC!=B,WC!=C)",     "k" : [ "isMoved" ] },
	    { "s" : "Moved (WC!=A,WC!=B,WC==C)",     "k" : [ "isMoved" ] },
	    { "s" : "Moved (WC!=A,WC==B,WC!=C)",     "k" : [ "isMoved" ] },
	    { "s" : "Moved (WC!=A,WC==B,WC==C)",     "k" : [ "isMoved" ] },
	    { "s" : "Moved (WC==A,WC!=B,WC!=C)",     "k" : [ "isMoved" ] },
	    { "s" : "Moved (WC==A,WC!=B,WC==C)",     "k" : [ "isMoved" ] },
	    { "s" : "Moved (WC==A,WC==B,WC!=C)",     "k" : [ "isMoved" ] },

	    { "s" : "Attributes (WC)",                  "k" : [ "isModifiedAttributes" ] },
	    // TestAndSet_BCM()
	    { "s" : "Attributes (WC!=B,WC!=C,B!=C)",    "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (WC!=B,WC==C)",         "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (WC==B,WC!=C)",         "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (WC!=B,WC!=C,B==C)",    "k" : [ "isModifiedAttributes" ] },
	    // TestAndSet_AxM(B)
	    { "s" : "Attributes (WC!=A,B!=A,WC!=B)",    "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (B!=A,WC==B)",          "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (B==A,WC!=B)",          "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (WC!=B,WC==A)",         "k" : [ "isModifiedAttributes" ] },
	    // TestAndSet_AxM(C)
	    { "s" : "Attributes (WC!=A,C!=A,WC!=C)",    "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (C!=A,WC==C)",          "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (C==A,WC!=C)",          "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (WC!=C,WC==A)",         "k" : [ "isModifiedAttributes" ] },
	    // TestAndSet_ABCM()
	    { "s" : "Attributes (WC!=A,WC!=B,WC!=C)",   "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (WC!=A,WC!=B,WC==C)",   "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (WC!=A,WC==B,WC!=C)",   "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (WC!=A,WC==B,WC==C)",   "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (WC==A,WC!=B,WC!=C)",   "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (WC==A,WC!=B,WC==C)",   "k" : [ "isModifiedAttributes" ] },
	    { "s" : "Attributes (WC==A,WC==B,WC!=C)",   "k" : [ "isModifiedAttributes" ] },

	    { "s" : "Existence (A,B,!C,WC)",            "k" : [ "isExistence_ABM" ] },
	    { "s" : "Existence (A,!B,C,WC)",            "k" : [ "isExistence_ACM" ] },

	];

    var matched = new Array();

    for (var e_section in expected)
    {
	var table_row = -1;
	for (var t_index in table)
	{
	    if (table[t_index].s == e_section)
	    {
		table_row = t_index;
		break;
	    }
	}
	if (table_row == -1)
	{
	    testlib.ok( (0), "WC: Unknown section '" + e_section + "' in EXPECT." );
	    continue;
	}
	// print("Inspecting expect['" + e_section + "']:");
	var keys = table[table_row].k;

	matched[e_section] = {};

	for (var e_index in expected[e_section])
	{
	    matched[e_section][e_index] = false;

	    var e_path = expected[e_section][e_index];
	    if (e_path == undefined)
	    {
		testlib.ok( (0), "WC: Expected['" + e_section + "']['" + e_index + "'] is undefined.");
		continue;
	    }
	    if (e_path[0] != '@')
		testlib.ok( (0), "WC: Expect path must begin with @: " + e_path);

	    var bWildcards = (e_path.indexOf("*") >= 0);
	    if (bWildcards)
		var re = new RegExp( "^" + e_path + "$" );
	    // print("Inspecting expect['" + e_section + "']['" + e_index + "']: " + expected[e_section][e_index]);

	    for (var s_index in status)
	    {
		var s_path = status[s_index].path.rtrim("/");
		if (e_path[1] != '/')
		{
		    if (e_path[1] == 'a')
		    {
			if (status[s_index].A != undefined)
			    var s_path = status[s_index].A.path.rtrim("/");
		    }
		    else if (e_path[1] == 'b')
		    {
			if (status[s_index].B != undefined)
			    var s_path = status[s_index].B.path.rtrim("/");
		    }
		    else if (e_path[1] == 'c')
		    {
			if (status[s_index].C != undefined)
			    var s_path = status[s_index].C.path.rtrim("/");
		    }
		    else if (e_path[1] == 'm')
		    {
			if (status[s_index].M != undefined)
			    var s_path = status[s_index].M.path.rtrim("/");
		    }
		    else if (e_path[1] == 'g')
		    {
			var s_path = '@' + status[s_index].gid;
		    }
		    else if (e_path[1] == '0')
		    {
			if (status[s_index].A != undefined)
			    var s_path = status[s_index].A.path.rtrim("/");
		    }
		    else if (e_path[1] == '1')
		    {
			if (status[s_index].B != undefined)
			    var s_path = status[s_index].B.path.rtrim("/");
		    }
		    else
			testlib.ok( (0), "WC: Unsupported prefix in: " + e_path);
		}

		var bMatchPaths = false;
		if (bWildcards)
		    bMatchPaths = re.test( s_path );
		else
		    bMatchPaths = (s_path == e_path);
		
		if (bMatchPaths)
		{
		    // do exact match on the heading/section.
		    var claimed_heading = false;
		    for (var h_index in status[s_index].headings)
		    {
			if (status[s_index].headings[h_index] == e_section)
			    claimed_heading = true;
		    }
		    testlib.ok( claimed_heading,
				"WC: EXPECTED[" + e_section + "][" + e_index + "]: heading found: " + s_path);

		    if (claimed_heading)
		    {
			// verify that all of the status/property bits 
			// that this header should have are set.
			var claimed_all = true;
			for (var k_index in keys)
			{
			    var key = keys[k_index];

			    if (status[s_index].status[key])
			    {
				// We have something like:
				//     "@/dir_A" <<==>> expect["Added"][0]
				// and "@/dir_A" <<==>> status[1].path
				// and      true <<==>> status[1].status.isAdded

				if (status[s_index].claimed == undefined)
				    status[s_index].claimed = {};
				status[s_index].claimed[key] = true;
				// print(dumpObj(status, "TEST1", "", 0));
			    }
			    else
			    {
				claimed_all = false;
			    }
			}
			if (claimed_all)
			    matched[e_section][e_index] = true;
		    }
		}
	    }
	}
    }

    // see if every item in "expected" was seen.

    for (var m_section in matched)
    {
	for (var m_index in matched[m_section])
	{
	    testlib.ok( matched[m_section][m_index],
			"WC: EXPECTED[" + m_section + "][" + m_index + "]: " + expected[m_section][m_index] );
	}
    }

    // for every item in the status, see if every
    // "isBit" that we care about was claimed.

    for (var s_index in status)
    {
	for (var s_bit in status[s_index].status)
	{
	    if (   (s_bit == "flags")
		|| (s_bit == "isFile")
		|| (s_bit == "isDirectory")
		|| (s_bit == "isSymlink")
		|| (s_bit == "isMultiple")
	       )
		continue;

	    testlib.ok( ((status[s_index].claimed != undefined) && (status[s_index].claimed[s_bit])),
			"WC: claimed bit '" + s_bit + "': " + status[s_index].path );
	}
    }

    // print(dumpObj(status, "STATUS_DUMP", "", 0));
}

// Confirm that a full status reports everything
// that we expec it to.
//
// We "ignore-ignores" and treat (what are normally) 
// IGNORED items as FOUND.
//
function vscript_test_wc__confirm_wii(expected)
{
    var status = sg.wc.status( { "no_ignores" : true } );

    vscript_test_wc__confirm(expected, status);
}

// Confirm that a full status reports everything
// that we expec it to.  INCLUDING RESERVED ITEMS.
//
// We "ignore-ignores" and treat (what are normally) 
// IGNORED items as FOUND.
//
function vscript_test_wc__confirm_wii_res(expected)
{
    var status = sg.wc.status( { "no_ignores" : true, "list_reserved" : true } );

    vscript_test_wc__confirm(expected, status);
}

// Use MSTATUS (merge-status) rather than STATUS
// to compute the current state of the WD and
// verify that we observe what we expected.
//
// We "ignore-ignores" and treat (what are normally) 
// IGNORED items as FOUND.
//
function vscript_test_wc__mstatus_confirm_wii(expected)
{
    var status = sg.wc.mstatus( { "no_ignores" : true } );

    vscript_test_wc__confirm(expected, status);
}

function vscript_test_wc__mstatus_confirm(expected)
{
    var status = sg.wc.mstatus( );

    vscript_test_wc__confirm(expected, status);
}

//////////////////////////////////////////////////////////////////

function vscript_test_wc__write_file(pathname, content)
{
    indentLine("=> (WC) write_file: " + pathname);

    sg.file.write(pathname, content);
}

//////////////////////////////////////////////////////////////////

function vscript_test_wc__lock_np( np, b_expect_pass )
{
    indentLine("=> WC: lock");

    try
    {
	sg.wc.lock( np );
	testlib.ok(b_expect_pass);
    }
    catch (e)
    {
	testlib.ok(!b_expect_pass);
	if (b_expect_pass)
	    print(sg.to_json__pretty_print(e));
    }
}

function vscript_test_wc__unlock_np( np, b_expect_pass )
{
    indentLine("=> WC: unlock");

    try
    {
	sg.wc.unlock( np );
	testlib.ok(b_expect_pass);
    }
    catch (e)
    {
	testlib.ok(!b_expect_pass);
	if (b_expect_pass)
	    print(sg.to_json__pretty_print(e));
    }
}

//////////////////////////////////////////////////////////////////

function vscript_test_wc__resolve__list_all()
{
    indentLine("=> (WC) resolve list --all:");

    var r = sg.exec(vv, "resolve", "list", "--all");

    print(r.stdout);
    print(r.stderr);

    testlib.ok((r.exit_status == 0), "Exit status from RESOLVE LIST ALL.");
}

function vscript_test_wc__resolve__list_all_verbose()
{
    indentLine("=> (WC) resolve list --all --verbose:");

    var r = sg.exec(vv, "resolve", "list", "--all", "--verbose");

    print(r.stdout);
    print(r.stderr);

    testlib.ok((r.exit_status == 0), "Exit status from RESOLVE LIST ALL.");
}

function vscript_test_wc__resolve__check_if_resolved( file, bExpected )
{
    indentLine("=> resolve list --all --verbose '" + file + "':");

    var r = sg.exec(vv, "resolve", "list", "--all", "--verbose", file);

    print(r.stdout);
    print(r.stderr);

    // TODO 2012/08/16 This only allows for a single conflict on an item.
    // TODO            An item could have both a RENAME and a CONTENT
    // TODO            conflict and we'd get 2 stanzas from RESOLVE.
    var regex = new RegExp("Status:[ \t]*Resolved");
    var bIsResolved = regex.test( r.stdout );
    
    testlib.ok( (bIsResolved == bExpected),
		("CheckIfResolved: Resolved=" + bIsResolved + " Expected=" + bExpected + " : " + file) );
    return (bIsResolved);
}

function vscript_test_wc__resolve__accept(value, path, optional_condition)
{
    if (optional_condition == undefined)
    {
	indentLine("=> (WC) resolve accept: ["
		   + value
		   + "] '"
		   + path
		   + "'");
	var r = sg.exec(vv, "resolve", "accept", value, path);
    }
    else
    {
	indentLine("=> (WC) resolve accept: ["
		   + value
		   + "] '"
		   + path
		   + "' ("
		   + optional_condition
		   + ")");
	var r = sg.exec(vv, "resolve", "accept", value, path, optional_condition);
    }

    print(r.stdout);
    print(r.stderr);

    testlib.ok((r.exit_status == 0), "Exit status from RESOLVE ACCEPT.");
}

function vscript_test_wc__resolve__merge(tool, path)
{
    indentLine("=> (WC) resolve merge: [" + tool + "] '" + path + "'");
    var r = sg.exec(vv, "resolve", "merge", path, "--tool", tool);

    print(r.stdout);
    print(r.stderr);

    testlib.ok((r.exit_status == 0), "Exit status from RESOLVE MERGE.");
}

//////////////////////////////////////////////////////////////////

    function vscript_test_wc__set_debug_port_mask(mask)
    {
	indentLine("=> (WC) set debug port mask: " + mask);

	sg.exec(vv, "config", "set", "debug_port_mask", mask.toString());
    }
    function vscript_test_wc__set_debug_port_mask__off()
    {
	vscript_test_wc__set_debug_port_mask( 0 );
    }
    function vscript_test_wc__set_debug_port_mask__on()
    {
	vscript_test_wc__set_debug_port_mask( 0xffffffff );
    }

    function vscript_test_wc__set_debug_port_mask2( bIndividual, bCollision )
    {
	var mask = 0;

	if (bIndividual)
	    mask = mask + 0xffff;
	if (bCollision)
	    mask = mask + 0xffff0000;

	vscript_test_wc__set_debug_port_mask( mask );
    }

//////////////////////////////////////////////////////////////////

function vscript_test_wc__update__simple( rev )
{
    indentLine("=> WC: update__simple to rev " + rev);
    sg.wc.update( { "rev" : rev,
		    "detached" : true } );
    testlib.ok( (sg.wc.parents()[0] == rev), "Expect (parent[0]=="+rev+") after UPDATE.");
}

function vscript_test_wc__update__expect_error( rev, msg )
{
    try
    {
	vscript_test_wc__update__simple( rev );
	testlib.ok( (0), "Did not receive error.  Expected '" + msg + "'." );
    }
    catch (e)
    {
	print("Received error: '" + e.toString() + "'.");
	testlib.ok( (e.toString().indexOf(msg) >= 0), "Error message should contain expected string '" + msg + "'.");
    }
}

function vscript_test_wc__update__test( rev, bTest )
{
    indentLine("=> WC: update rev="+rev + " test="+bTest);
    var result = sg.wc.update( { "rev" : rev,
				 "detached" : true,
				 "test" : bTest,
				 "status" : true } );
    
    print("UPDATE returned:");
    print(sg.to_json__pretty_print(result));

    testlib.ok( (result.hid == rev), "Expect chosen-hid to match request.");
    if (!bTest)
	testlib.ok( (sg.wc.parents()[0] == rev), "Expect (parent[0]=="+rev+") after UPDATE.");

    return result;
}

function vscript_test_wc__update( rev )
{
    var rt = vscript_test_wc__update__test(rev, true);	// do "update --test"
    var rr = vscript_test_wc__update__test(rev, false);	// do "update"

    testlib.ok( (rt.hid            == rr.hid),            "Expect HIDs to match.");
    testlib.ok( (rt.stats.synopsys == rr.stats.synopsys), "Expect SYNOPSYS to match.");

    for (rt_ndx in rt.status)
    {
	for (var rr_ndx in rr.status)
	{
	    if (rr.status[rr_ndx].gid == rt.status[rt_ndx].gid)
	    {
		rt.status[rt_ndx].MATCHED = true;
		rr.status[rr_ndx].MATCHED = true;

		// test a couple of specific fields directly.
		testlib.ok( (rr.status[rr_ndx].path == rt.status[rt_ndx].path),
			    "Expect PATH to match: " + rr.status[rr_ndx].path + " vs " + rt.status[rt_ndx].path);
		testlib.ok( (rr.status[rr_ndx].status.flags == rt.status[rt_ndx].status.flags),
			    "Expect FLAGS to match: " + rr.status[rr_ndx].path + " vs " + rt.status[rt_ndx].path);
		// test all fields in a clump
		var st = sg.to_json(rt.status[rt_ndx]);
		var sr = sg.to_json(rr.status[rr_ndx]);
		if (sr == st)
		{
		    testlib.ok( (1), "Expect full item match.");
		}
		else
		{
		    // Note that we will get failures here if there are automerge failures
		    // and the _delete_automerge_tempfiles_on_abort code cannot rm-rf the
		    // temp dirs (because the second update attempt will generate a fresh
		    // temp dir and the name of the dir is in the diff/conflict headers 
		    // that the difftools write into the files, so the HIDs won't match).
		    testlib.ok( (0), "Expect full item match.");
		    print("st: " + st);
		    print("sr: " + sr);
		}
	    }
	}
    }

    for (rt_ndx in rt.status)
    {
	if (rt.status[rt_ndx].MATCHED != true)
	{
	    if (rt.status[rt_ndx].gid[0] == 't')
		print("Skipping matchup on TID: " + rt.status[rt_ndx].gid);
	    else
		testlib.ok( (0), "Expected match on rt.status["+rt_ndx+"].gid="+rt.status[rt_ndx].gid);
	}
    }
    for (var rr_ndx in rr.status)
    {
	if (rr.status[rr_ndx].MATCHED != true)
	{
	    if (rr.status[rr_ndx].gid[0] == 't')
		print("Skipping matchup on TID: " + rr.status[rr_ndx].gid);
	    else
		testlib.ok( (0), "Expected match on rr.status["+rr_ndx+"].gid="+rr.status[rr_ndx].gid);
	}
    }
}

function vscript_test_wc__update_attached( rev, branch )
{
    var r = vscript_test_wc__update( rev );
    sg.exec(vv, "branch", "attach", branch);
    return r;
}

function vscript_test_wc__revert_all( label )
{
    if (label != undefined)
	indentLine("=> WC: revert_all [" + label + "]");
    else
	indentLine("=> WC: revert_all");

    var result = sg.wc.revert_all();

    print("REVERT_ALL returned:");
    print(sg.to_json__pretty_print(result));

    return result;
}

function vscript_test_wc__revert_all__expect_error( label, msg )
{
    try
    {
	vscript_test_wc__revert_all( label );
	testlib.ok( (0), "Did not receive error.  Expected '" + msg + "'." );
    }
    catch (e)
    {
	print("Received error: '" + e.toString() + "'.");
	testlib.ok( (e.toString().indexOf(msg) >= 0), "Error message should contain expected string '" + msg + "'.");
    }
}

function vscript_test_wc__revert_all__np( np, label )
{
    if (label != undefined)
	indentLine("=> WC: revert_all [" + label + "]");
    else
	indentLine("=> WC: revert_all");

    var result = sg.wc.revert_all( np );

    print("REVERT_ALL returned:");
    print(sg.to_json__pretty_print(result));

    return result;
}

function vscript_test_wc__revert_all__test( label )
{
    if (label != undefined)
	indentLine("=> WC: revert_all test [" + label + "]");
    else
	indentLine("=> WC: revert_all");

    var result = sg.wc.revert_all( { "test" : true } );

    print("REVERT_ALL returned:");
    print(sg.to_json__pretty_print(result));

    return result;
}

//////////////////////////////////////////////////////////////////

function vscript_test_wc__revert_items( sz_or_array )
{
    indentLine("=> WC: revert_items");

    var result = sg.wc.revert_items( { "src" : sz_or_array, "verbose" : true } );

    print("REVERT_ITEMS returned:");
    print(sg.to_json__pretty_print(result));

    return result;
}

function vscript_test_wc__revert_items__np( np )
{
    indentLine("=> WC: revert_items");

    var result = sg.wc.revert_items( np );

    print("REVERT_ITEMS returned:");
    print(sg.to_json__pretty_print(result));

    return result;
}

function vscript_test_wc__revert_items__expect_error( sz_or_array, msg )
{
    try
    {
	vscript_test_wc__revert_items( sz_or_array );
	testlib.ok( (0), "Did not receive error.  Expected '" + msg + "'." );
    }
    catch (e)
    {
	print("Received error: '" + e.toString() + "'.");
	testlib.ok( (e.toString().indexOf(msg) >= 0), "Error message should contain expected string '" + msg + "'.");
    }
}

//////////////////////////////////////////////////////////////////

// Build the "predicted" backup name of the given file.
// The caller must pass in the expected sequence number "sg00".
//
// We create something like: "<gid7>~<entryname>~<seq>~".
// This is the ENTRYNAME ONLY.
//
// See sg_wc_db__path__generate_backup_path().
//
function vscript_test_wc__backup_name_of_file( file, seq )
{
    var gid = sg.wc.get_item_gid( { "src" : file } );
    var lastSlash = file.lastIndexOf("/");
    if (lastSlash == -1)
	var entryname = file;
    else
	var entryname = file.substr(lastSlash+1);

    var backup = gid.substr(0,7) + "~" + entryname + "~" + seq + "~";

    return backup;
}

//////////////////////////////////////////////////////////////////
// Function to help test 1-rev-spec version of STATUS.
// In theory, we can do a 1-rev-spec status of a dirty WD
// with either parent, do a complete (non-partial) commit,
// and then do a 2-rev-spec status with the previous parent
// and the newly created cset.  The 2 statuses should be
// very similar.

function vscript_test_wc__test_1_revspec_status(label, s1, s2)
{
//    print("Test-1-rev-spec: " + label);
//    print(sg.to_json__pretty_print(s1));
//    print(sg.to_json__pretty_print(s2));

    // s1 is from a 1-revspec status.
    // it has sections "B" and "WC".
    // it has repo-paths "@0/..." and "@/...".
    //
    // s2 is from a 2-revspec status.
    // it has sections "A" and "B".
    // it has repo-paths "@0/..." and "@1/...".

    var tbl = {};
    for (k in s1)
    {
	if (s1[k].status.isFound || s1[k].status.isIgnored || s1[k].status.isReserved)
	    continue;

	var g = s1[k].gid;
	var f = s1[k].status.flags;
	var p = s1[k].path;

	if (tbl[g] == undefined)
	    tbl[g] = {};
	tbl[g]["s1"] = p + ":" + f;
    }

    for (k in s2)
    {
	var g = s2[k].gid;
	var f = s2[k].status.flags;
	var p = s2[k].path.replace(/@1/,"@");

	if (tbl[g] == undefined)
	    tbl[g] = {};
	tbl[g]["s2"] = p + ":" + f;
    }

    for (gid in tbl)
    {
//	print("tbl[" + gid + "]: " + sg.to_json__pretty_print(tbl[gid]));
	testlib.ok(  (  (tbl[gid]["s1"] != undefined)
			&& (tbl[gid]["s2"] != undefined)
			&& (tbl[gid]["s1"] == tbl[gid]["s2"])  ),
		     ("Expect match for: " + gid)  );
    }

}
