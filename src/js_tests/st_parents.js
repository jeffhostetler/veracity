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

load("../js_test_lib/vscript_test_lib.js");

function sleepStupidly(usec)
    {
	var endtime= new Date().getTime() + (usec * 1000);
        while (new Date().getTime() < endtime)
	    ;
    }
    function commitWrapper(obj)
    {
    	obj.changesets.push(vscript_test_wc__commit());
    	sleepStupidly(1);
	obj.times.push(new Date().getTime());
	testlib.execVV("tag", "--rev="+obj.changesets[obj.changesets.length - 1], "add", "changeset_" + obj.changesets.length);
    	sleepStupidly(1);
    }
	// Removes ending whitespaces
function RTrim( value ) {
	
	var re = /((\s*\S+)*)\s*/;
	return value.replace(re, "$1");
	
}
function parseSingleParentResult(result)
	{
		thisChangeset = {};
		var lines = result.split("\n");
		//print("changeset has " + lines.length + " lines");
		
		if (lines.length > 0)
		{
			while (lines.length > 0 && lines[0].indexOf(":") == -1)
			{
				lines.splice(0,1); //remove the first entry.
			}
			//print("revision line: " + lines[0]);
			var revSplit = lines[0].split(":");
			if (revSplit.length >= 2)
			{
				thisChangeset.revno = revSplit[0];
				thisChangeset.changeset_id = revSplit[1];
				
				//print("revno: " + thisChangeset.revno);
				//print("changeset_id: " + thisChangeset.changeset_id);
			}
		}
		thisChangeset.text = result;
		return thisChangeset;
	}
    function runCLCParents()
    {
		var args = Array.prototype.slice.call(arguments);
		args.unshift("parents");
    	print(dumpObj(args));
		var execResults = testlib.execVV.apply(testlib, args);
		print(dumpObj(execResults));
		
    	resultChangesets =execResults.stdout.split("revision:  ");
		resultChangesets.splice(0, 1);  //Remove the first, empty array element
		print(dumpObj(resultChangesets));
		returnChangesets = [];
		for (i = 0; i < resultChangesets.length; i++)
		{
			returnChangesets.push(parseSingleParentResult(resultChangesets[i]));
		}
    	
		print(dumpObj(returnChangesets));
    	return returnChangesets;
    }
	
function formatDate(ticks, includeTime)
{
	var dateObj = new Date();
	dateObj.setTime(ticks);
	var returnStr = "";
	returnStr = dateObj.getFullYear() + "-" + (dateObj.getMonth() + 1) + "-" + dateObj.getDate();
	if (includeTime != null)
		returnStr += " " + dateObj.getHours() + ":" + dateObj.getMinutes() + ":" + dateObj.getSeconds();
	return returnStr;
}

function st_ParentsOnRoot() {
   this.changesets = new Array();
   this.changesets.push("initial commit");
   this.times = new Array();
   this.times.push(0);
    this.setUp = function() {
	sg.fs.mkdir("test");
	vscript_test_wc__addremove();
	commitWrapper(this);
	sg.fs.mkdir("test2");
	vscript_test_wc__addremove();
	commitWrapper(this);
	sg.fs.mkdir("test3");
	vscript_test_wc__addremove();
	commitWrapper(this);
	sg.fs.mkdir("test2/sub1");
	vscript_test_wc__addremove();
	commitWrapper(this);
    }
    
    
// TODO 2011/12/01 sg.pending_tree.parents() has been removed
// TODO            as part of the WC conversion.  The following
// TODO            routines use sg.pending_tree.parents() and/or
// TODO            'vv parents' with an argument to get the
// TODO            history of a single item.
// TODO
// TODO            This behavior is not being preserved in sg.wc.parents()
// TODO            (it only does WD-based stuff).  We need a new API if we
// TODO            want this history feature.
// TODO
// TODO            See W9964.

    this.TODO_W9964 = ((sg.pending_tree == undefined) || (sg.pending_tree.parents == undefined));

    this.testRoot = function() 
    {
//	if (this.TODO_W9964)
//	{
//    	    //Verify that a limitless parents on . returns an entry for every
//    	    //commit.
//    	    var parentsResults = sg.pending_tree.parents("./");
//    	    testlib.equal(1, parentsResults.length, "Parents should return an entry.");
//    	    testlib.equal(parentsResults[0], this.changesets[this.changesets.length - 1], "Parents should have the right parent for a revision to root.");
//   	    var clcparentsResults = runCLCParents("./");
//   	    testlib.equal(1, clcparentsResults.length, "Parents should return an entry.");
//    	    testlib.equal(clcparentsResults.changeset_id, this.changesets[this.changesets.length - 1], "Parents should have the right parent for a revision to root.");
//	}

   	var parentsResults = sg.wc.parents();
    	testlib.equal(1, parentsResults.length, "Parents should return an entry.");
    	testlib.equal(parentsResults[0], this.changesets[this.changesets.length - 1], "Parents should have the right parent for a revision to root.");
   	var clcparentsResults = runCLCParents();
   	testlib.equal(1, clcparentsResults.length, "Parents should return an entry.");
    	testlib.equal(clcparentsResults[0].changeset_id, this.changesets[this.changesets.length - 1], "Parents should have the right parent for a revision to root.");

//	if (this.TODO_W9964)
//	{
//   	    parentsResults = sg.pending_tree.parents("test");
//    	    testlib.equal(1, parentsResults.length, "Parents should return an entry.");
//    	    testlib.equal(parentsResults[0], this.changesets[1], "Parents should have the right parent for a revision to a subfolder.");
//   	    clcparentsResults = runCLCParents("test");
//   	    testlib.equal(1, clcparentsResults.length, "Parents should return an entry.");
//    	    testlib.equal(clcparentsResults[0].changeset_id, this.changesets[1], "Parents should have the right parent for a revision to a subfolder.");
//	}
    }
    
    this.testRev = function() 
    {
//	if (this.TODO_W9964)
//	{
//    	    var parentsResults = sg.pending_tree.parents(["rev=" +this.changesets[this.changesets.length - 3] ])
//    	    testlib.equal(1, parentsResults.length, "Parents should return an entry.");
//    	    testlib.equal(parentsResults[0], this.changesets[this.changesets.length - 4], "Parents should have the right parent.");
//    	    var parentsResults = sg.pending_tree.parents(["rev=" +this.changesets[this.changesets.length - 3] ], "./test")
//    	    testlib.equal(1, parentsResults.length, "Parents should return an entry.");
//    	    testlib.equal(parentsResults[0], this.changesets[1], "Parents should have the right parent for a subfolder revision.");
//
//    	    var clcparentsResults = runCLCParents("--rev=" + this.changesets[this.changesets.length - 3]);
//   	    testlib.equal(1, clcparentsResults.length, "Parents should return an entry.");
//    	    testlib.equal(clcparentsResults[0].changeset_id, this.changesets[this.changesets.length - 4], "Parents should have the right parent.");
//    	    var clcparentsResults = runCLCParents("./test", "--rev=" + this.changesets[this.changesets.length - 3]);
//   	    testlib.equal(1, clcparentsResults.length, "Parents should return an entry.");
//    	    testlib.equal(clcparentsResults[0].changeset_id, this.changesets[1], "Parents should have the right parent for a subfolder revision.");
//	}
    }       

    this.testTag = function() 
    {
//	if (this.TODO_W9964)
//	{
//    	    var parentsResults = sg.pending_tree.parents(["tag=changeset_3"] );
//    	    testlib.equal(1, parentsResults.length, "Parents should return an entry.");
//    	    testlib.equal(parentsResults[0], this.changesets[this.changesets.length - 4], "Parents should have the right parent.");
//    	    var parentsResults = sg.pending_tree.parents(["tag=changeset_3"], "./test");
//    	    testlib.equal(1, parentsResults.length, "Parents should return an entry.");
//    	    testlib.equal(parentsResults[0], this.changesets[1], "Parents should have the right parent for a subfolder revision.");
//
//    	    var clcparentsResults = runCLCParents("--tag=changeset_3");
//   	    testlib.equal(1, clcparentsResults.length, "Parents should return an entry.");
//    	    testlib.equal(clcparentsResults[0].changeset_id, this.changesets[this.changesets.length - 4], "Parents should have the right parent.");
//    	    var clcparentsResults = runCLCParents("./test", "--tag=changeset_3");
//   	    testlib.equal(1, clcparentsResults.length, "Parents should return an entry.");
//    	    testlib.equal(clcparentsResults[0].changeset_id, this.changesets[1], "Parents should have the right parent for a subfolder revision.");
//	}
    } 

    this.testWalkBackParents = function() 
    {
//	if (this.TODO_W9964)
//	{
//	    currentParent = sg.wc.parents()[0];
//	    while (true)
//	    {
//		parents = sg.pending_tree.parents(["rev=" + currentParent]);
//		if (parents == null || parents.length != 1)
//		    break;
//		currentParent = parents[0];
//	    }
//	    historyResults = sg.pending_tree.history(["rev=1"]);
//	    rootID = historyResults[historyResults.length - 1].changeset_id;
//	    testlib.equal(currentParent, rootID, "Should have walked back to the root dag node");
//	}
    }

    this.tearDown = function() 
    {
    }
}
