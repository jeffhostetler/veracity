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
		testlib.addTag(obj.changesets[obj.changesets.length - 1], 
						"changeset_" + obj.changesets.length );
	   	sleepStupidly(1);
    }
    function commitDetachedWrapper(obj)
    {
    	obj.changesets.push(vscript_test_wc__commit_np( { "message" : "No message, but detached.", "detached" : true } ));
    	sleepStupidly(1);
	obj.times.push(new Date().getTime());
	sg.vv2.add_tag({ "rev" : obj.changesets[obj.changesets.length - 1], "value" : "changeset_" + obj.changesets.length });
    	sleepStupidly(1);
    }
	
	function parseSingleHistoryResult(result)
	{
		thisChangeset = {};
		var lines = result.split("\n");
		//print("changeset has " + lines.length + " lines");
		if (lines.length > 0)
		{
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
    function runCLCHistory()
    {
		var args = Array.prototype.slice.call(arguments);
		args.unshift("history");
    	print(dumpObj(args));
		var execResults = testlib.execVV.apply(testlib, args);
		
    	resultChangesets =execResults.stdout.split("revision:  ");
		resultChangesets.splice(0, 1);  //Remove the first, empty array element
		returnChangesets = [];
		for (i = 0; i < resultChangesets.length; i++)
		{
			returnChangesets.push(parseSingleHistoryResult(resultChangesets[i]));
		}
    	//print(dumpObj(execResults));
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

function st_HistoryOnRoot() {
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
    
    
    this.testRoot = function() {
    	//Verify that a limitless history on @ returns an entry for every
    	//commit.
	var historyResults = sg.vv2.history( { "src" : "./" } );
    	testlib.equal(this.changesets.length, historyResults.length, "History should return an entry for every changeset.");
   	var clchistoryResults = runCLCHistory("./");
   	testlib.equal(this.changesets.length, clchistoryResults.length, "History should return an entry for every changeset.");
	var historyResults = sg.vv2.history();
    	testlib.equal(this.changesets.length, historyResults.length, "History should return an entry for every changeset.");
   	var clchistoryResults = runCLCHistory();
   	testlib.equal(this.changesets.length, clchistoryResults.length, "History should return an entry for every changeset.");
	
	var clchistoryResults = runCLCHistory("--reverse");
   	testlib.equal(this.changesets.length, clchistoryResults.length, "History should return an entry for every changeset.");
	print(clchistoryResults[0]);
	testlib.equal(1, clchistoryResults[0].revno, "The output should be reversed.");
    }
    
    this.testFromDate = function() {
	var historyResults = sg.vv2.history( { "from" : this.times[1],
					       "src" : "./" } );
    	testlib.equal(this.changesets.length - 2, historyResults.length, "History results should not have included the first two commits");
    	var clchistoryResults = runCLCHistory("--from=" + formatDate(this.times[1], true));
   	testlib.equal(this.changesets.length - 2, clchistoryResults.length, "History results should not have included the first two commits");
    }
    
    this.testToDate = function() {
	var historyResults = sg.vv2.history( { "to" : this.times[this.times.length - 2],
					       "src" : "./" } );
    	testlib.equal(this.changesets.length - 1, historyResults.length, "History results should not have included the last commit");
    	var clchistoryResults = runCLCHistory("--to=" + formatDate(this.times[this.times.length - 2], true));
   	testlib.equal(this.changesets.length - 1, clchistoryResults.length, "History results should not have included the last commit");
    }
    
    this.testToDateAndFromDate = function() {
	var historyResults = sg.vv2.history( { "to" : this.times[this.times.length - 3],
					       "from" : this.times[1],
					       "src" : "./" } );
    	testlib.equal(this.changesets.length - 4, historyResults.length, "History results should not have included the last  two commits or the first two commits");
    	var clchistoryResults = runCLCHistory("--to=" + formatDate(this.times[this.times.length - 3], true), "--from=" +  formatDate(this.times[1], true));
   	testlib.equal(this.changesets.length - 4, clchistoryResults.length, "History results should not have included the last  two commits or the first two commits");
    }

    this.testSubFolder = function() {
        var historyResults = sg.vv2.history( { "src" : "test2" } );
    	testlib.equal(2, historyResults.length, "test2 changed in two dag nodes");
	var clchistoryResults = runCLCHistory("test2");
	testlib.equal(2, clchistoryResults.length, "test2 changed in two dag nodes");
    }

    this.testSubFolder = function() {
        var historyResults = sg.vv2.history( { "src" : "test" } );
    	testlib.equal(1, historyResults.length, "test changed in one dag nodes");
	var clchistoryResults = runCLCHistory("test");
	testlib.equal(1, clchistoryResults.length, "test changed in one dag nodes");
    } 
    
    this.testRev = function() {
    	var historyResults = sg.vv2.history( { "rev" : this.changesets[this.changesets.length - 3] } );
    	testlib.equal(1, historyResults.length, "rev should return 1 entry");    
    	var clchistoryResults = runCLCHistory("--rev=" + this.changesets[this.changesets.length - 3]);
    	testlib.equal(1, clchistoryResults.length, "rev should return 1 entry");

    	var historyResults = sg.vv2.history( { "rev" : [ this.changesets[this.changesets.length - 2],
							 this.changesets[this.changesets.length - 3] ] } );
    	testlib.equal(2, historyResults.length, "rev should return 2 entry");    
    	var clchistoryResults = runCLCHistory("--rev=" + this.changesets[this.changesets.length - 3], "--rev=" + this.changesets[this.changesets.length - 2]);
    	testlib.equal(2, clchistoryResults.length, "rev should return 2 entry");
    }       

    this.testPartialRev = function() {
    	var historyResults = sg.vv2.history( { "rev" : this.changesets[this.changesets.length - 3].substring(0, 12) } );
    	testlib.equal(1, historyResults.length, "rev should return 1 entry");    
    	var clchistoryResults = runCLCHistory("--rev=" + this.changesets[this.changesets.length - 3].substring(0, 12));
    	testlib.equal(1, clchistoryResults.length, "rev should return 1 entry");

    	var historyResults = sg.vv2.history( { "rev" : [ this.changesets[this.changesets.length - 3].substring(0, 12),
							 this.changesets[this.changesets.length - 2].substring(0, 12) ] } );
    	testlib.equal(2, historyResults.length, "rev should return 2 entries");    
    	var clchistoryResults = runCLCHistory("--rev=" + this.changesets[this.changesets.length - 2].substring(0, 12), "--rev=" + this.changesets[this.changesets.length - 3].substring(0, 12));
    	testlib.equal(2, clchistoryResults.length, "rev should return 2 entries");
    }       
    
    this.testTag = function() {
    	var historyResults = sg.vv2.history( { "tag" : "changeset_" + (this.changesets.length - 2) } );
    	testlib.equal(1, historyResults.length, "rev should return one entry");    
    	var clchistoryResults = runCLCHistory("--tag=changeset_" + (this.changesets.length - 2));
    	testlib.equal(1, clchistoryResults.length, "rev should return one entry");

    	var historyResults = sg.vv2.history( { "tag" : [ "changeset_" + (this.changesets.length - 3),
							 "changeset_" + (this.changesets.length - 2) ] } );
    	testlib.equal(2, historyResults.length, "tag should return two entries");    
    	var clchistoryResults = runCLCHistory("--tag=changeset_" + (this.changesets.length - 3), "--tag=changeset_" + (this.changesets.length - 2));
    	testlib.equal(2, clchistoryResults.length, "tag should return two entries");
    }       
    this.testMax = function() {
    	var historyResults = sg.vv2.history( { "max" : 1,
					       "src" : "./" } );
    	testlib.equal(1, historyResults.length, "max should limit this to 1 result");    
    	testlib.equal(this.changesets[4], historyResults[0].changeset_id, "Make sure only the expected changesets are returned.")
    	var clchistoryResults = runCLCHistory("./", "--max=1");
	testlib.equal(1, clchistoryResults.length, "max should limit this to 1 result");
    }       
    this.testLeaves = function() {
    	var historyResults = sg.vv2.history( { "leaves" : true,
					       "src" : "./" } );
    	testlib.equal(5, historyResults.length, "leaves should act normally when it's just a line");  
    	var clchistoryResults = runCLCHistory("./", "--leaves");
	testlib.equal(5, clchistoryResults.length, "leaves should act normally when it's just a line");  
    }      
    this.tearDown = function() {
    }
}
function st_HistoryRenamedObject() {
   this.changesets = new Array();
   this.changesets.push("initial commit");
   this.times = new Array();
	this.times.push(new Date().getTime());
    this.setUp = function() {
    	//Create an item with the correct name, but delete it, so that it's not the right GID
	sg.fs.mkdir("renameToMe");
	vscript_test_wc__addremove();
	commitWrapper(this);
	vscript_test_wc__remove("renameToMe")
	commitWrapper(this);
	sg.fs.mkdir("renameMe");
	vscript_test_wc__addremove();
	commitWrapper(this);
	vscript_test_wc__rename("renameMe", "renameToMe");
	commitWrapper(this);
    }
    
    
    this.testRenamedObject = function() {
    	var historyResults = sg.vv2.history( { "src" : "renameToMe" } );
    	testlib.equal(2, historyResults.length, "There should be only two entries for renameToMe");
    	testlib.equal(this.changesets[3], historyResults[1].changeset_id, "Make sure only the expected changesets are returned.")
    	testlib.equal(this.changesets[4], historyResults[0].changeset_id, "Make sure only the expected changesets are returned.")
    }
    
    this.testRenamedObjectExpectZeroResults = function() {
    	//This is limited to before the object in question existed.
    	var historyResults = sg.vv2.history( { "to" : this.times[1],
					       "src" : "renameToMe" } );
    	testlib.equal(0, historyResults.length, "There should be zero results");
    }
    
    this.testRenamedObjectExpectOneResult = function() {
    	//This is limited to before the object in question existed.
    	var historyResults = sg.vv2.history( { "to" : this.times[3],
					       "src" : "renameToMe" } );
    	testlib.equal(1, historyResults.length, "There should be one result");
    	testlib.equal(this.changesets[3], historyResults[0].changeset_id, "Make sure only the expected changesets are returned.")
    	
    	var historyResults = sg.vv2.history( { "to" : this.times[4],
					       "from" : this.times[3],
					       "src" : "renameToMe" } );
    	testlib.equal(1, historyResults.length, "There should be one result");
    	testlib.equal(this.changesets[4], historyResults[0].changeset_id, "Make sure only the expected changesets are returned.")
    }
    
}
function st_HistoryOnAFile() {
   this.changesets = new Array();
   this.changesets.push("initial commit");
   this.times = new Array();
	this.times.push(new Date().getTime());
    this.setUp = function() {
    	//Create an item with the correct name, but delete it, so that it's not the right GID
	sg.file.write("fileName", "initialContents");
	vscript_test_wc__addremove();
	commitWrapper(this);
	sg.file.append("fileName", "moreContents");
	commitWrapper(this);
	sg.file.write("fileName", "All New Contents");
	commitWrapper(this);
    }
    
    this.testFileHistory = function() {
    	var historyResults = sg.vv2.history( { "src" : "./fileName" } );
    	testlib.equal(3, historyResults.length, "There should be only three entries for fileName");
	testlib.equal(this.changesets[1], historyResults[2].changeset_id, "Make sure only the expected changesets are returned.");
    	testlib.equal(this.changesets[2], historyResults[1].changeset_id, "Make sure only the expected changesets are returned.");
    	testlib.equal(this.changesets[3], historyResults[0].changeset_id, "Make sure only the expected changesets are returned.");
    }
}

/* I think that these were commented out becuase the test suite
  makes it hard to switch back and forth between users.  Regardless, it looks
  like they haven't been updated since we changed from userid to whoami. -Jeremy
  
function stHistoryUserFilter() {
   this.changesets = new Array();
   this.changesets.push("initial commit");
   this.times = new Array();
   this.times.push(new Date().getTime());
   this.origUserName = "";
    this.setUp = function() {
	this.origUserName = sg.local_settings().userid;
    	//Create an item with the correct name, but delete it, so that it's not the right GID
	sg.local_settings("userid", "user1");
	sg.fs.mkdir("dir1");
	vscript_test_wc__addremove();
	commitWrapper(this);
	sg.local_settings("userid", "user2");
	sg.fs.mkdir("dir2");
	vscript_test_wc__addremove();
	commitWrapper(this);
	sg.local_settings("userid", "user1");
	sg.fs.mkdir("dir3");
	vscript_test_wc__addremove();
	commitWrapper(this);
	sg.local_settings("userid", "user2");
	sg.fs.mkdir("dir4");
	vscript_test_wc__addremove();
	commitWrapper(this);
    }
    this.tearDown = function() {
	   sg.local_settings("userid", this.origUserName);
    }
    
    
    this.testLookForUser1 = function() {
    		historyResults = sg.vv2.history( { "user" : "user1",
						   "src"  : "./"    } );
        	testlib.equal(2, historyResults.length, "There should be two results");
        	testlib.equal(this.changesets[3], historyResults[0].changeset_id, "Make sure only the expected changesets are returned.")
		testlib.equal(this.changesets[1], historyResults[1].changeset_id, "Make sure only the expected changesets are returned.")
		var clchistoryResults = runCLCHistory("./", "--user=user1");
		testlib.equal(2, clchistoryResults.length, "There should be two results"); 
				
		historyResults = sg.vv2.history( { "user" : "user1",
						   "to"   : this.times[1],
						   "src"  : "./" } );
        	testlib.equal(1, historyResults.length, "Restricted by both user and date");
        	}
    
    
    this.testRenamedObjectExpectZeroResults = function() {
    		historyResults = sg.vv2.history( { "user" : "luser",
						   "src"  : "./" } );

        	testlib.equal(0, historyResults.length, "Expect zero results");
        	var clchistoryResults = runCLCHistory("./", "--user=luser");
		testlib.equal(0, clchistoryResults.length, "There should be zero results"); 
		
    }
    
    this.testLookForUser2 = function() {
    	historyResults = sg.vv2.history( { "user" : "user2",
					   "from" : this.times[1],
					   "src"  : "./" } );

	testlib.equal(2, historyResults.length, "Restricted by both user and date");
        	testlib.equal(this.changesets[4], historyResults[0].changeset_id, "Make sure only the expected changesets are returned.")
		testlib.equal(this.changesets[2], historyResults[1].changeset_id, "Make sure only the expected changesets are returned.")
				
        }
}

function stHistoryStampFilter() {
   this.changesets = new Array();
   this.changesets.push("initial commit");
   this.times = new Array();
   this.times.push(new Date().getTime());
   this.origUserName = "";
    this.setUp = function() {
	this.origUserName = sg.local_settings().userid;
	//UNCOMMENT_IF_WE_RESTORE_THIS_TEST//testlib.execVV("stamp", "--rev=" +sg.wc.parents()[0], "--stamp=user_" +sg.local_settings().userid, "--stamp=odd","--stamp=one" );
    	//Create an item with the correct name, but delete it, so that it's not the right GID
	sg.local_settings("userid", "user1");
	sg.fs.mkdir("dir1");
	vscript_test_wc__addremove();
	commitWrapper(this);
	//UNCOMMENT_IF_WE_RESTORE_THIS_TEST//testlib.execVV("stamp", "--rev=" +sg.wc.parents()[0], "--stamp=user_" +sg.local_settings().userid, "--stamp=even","--stamp=two" );
	sg.local_settings("userid", "user2");
	sg.fs.mkdir("dir2");
	vscript_test_wc__addremove();
	commitWrapper(this);
	//UNCOMMENT_IF_WE_RESTORE_THIS_TEST//testlib.execVV("stamp", "--rev=" +sg.wc.parents()[0], "--stamp=user_" +sg.local_settings().userid, "--stamp=odd","--stamp=three" );
	sg.local_settings("userid", "user1");
	sg.fs.mkdir("dir3");
	vscript_test_wc__addremove();
	commitWrapper(this);
	//UNCOMMENT_IF_WE_RESTORE_THIS_TEST//testlib.execVV("stamp", "--rev=" +sg.wc.parents()[0], "--stamp=user_" +sg.local_settings().userid, "--stamp=even","--stamp=four" );
	sg.local_settings("userid", "user2");
	sg.fs.mkdir("dir4");
	vscript_test_wc__addremove();
	commitWrapper(this);
	//UNCOMMENT_IF_WE_RESTORE_THIS_TEST//testlib.execVV("stamp", "--rev=" +sg.wc.parents()[0], "--stamp=user_" +sg.local_settings().userid, "--stamp=odd","--stamp=five" );
    }
    this.tearDown = function() {
	   sg.local_settings("userid", this.origUserName);
    }
    
    
    this.testLookForUser1 = function() {
		var clchistoryResults = runCLCHistory("./", "--stamp=odd");
		testlib.equal(3, clchistoryResults.length, "There should be three results"); 
		var clchistoryResults = runCLCHistory("./", "--stamp=five");
		testlib.equal(1, clchistoryResults.length, "There should be one result"); 
        	}
    
    this.testRenamedObjectExpectZeroResults = function() {
        	var clchistoryResults = runCLCHistory("./", "--stamp=bogus");
		testlib.equal(0, clchistoryResults.length, "There should be zero results"); 
    }
    
    this.testLookForUser2 = function() {
        	var clchistoryResults = runCLCHistory("./", "--user=user2", "--stamp=odd");
		testlib.equal(2, clchistoryResults.length, "There should be two results"); 
        	var clchistoryResults = runCLCHistory("./", "--user=user2", "--stamp=even");
		testlib.equal(0, clchistoryResults.length, "There should be zero results"); 
				
        }
}
*/
function st_HistoryBranches() {
   this.changesets = new Array();
   this.changesets.push("initial commit");
   this.times = new Array();
   this.master = new Array();
   this.untied = new Array();
   this.otherbranch = new Array();
	this.times.push(new Date().getTime());
    this.setUp = function() {
	index = 0;
	//This commit will be on master
	sg.fs.mkdir("sub");
	sg.fs.mkdir("sub/dir" + index++);	vscript_test_wc__addremove();	commitWrapper(this);
	this.master.push(this.changesets[this.changesets.length - 1]);
	//This commit will be on master
	sg.fs.mkdir("sub/dir" + index++);	vscript_test_wc__addremove();	commitWrapper(this);
	this.master.push(this.changesets[this.changesets.length - 1]);
	//This commit will be on master
	sg.fs.mkdir("sub/dir" + index++);	vscript_test_wc__addremove();	commitWrapper(this);
	this.master.push(this.changesets[this.changesets.length - 1]);

	testlib.updateDetached(this.changesets[1]);

	//This commit will be untied to a branch
	sg.fs.mkdir("sub/dir" + index++);	vscript_test_wc__addremove();	commitDetachedWrapper(this);
	this.untied.push(this.changesets[this.changesets.length - 1]);
	//This commit will be untied to a branch
	sg.fs.mkdir("sub/dir" + index++);	vscript_test_wc__addremove();	commitDetachedWrapper(this);
	this.untied.push(this.changesets[this.changesets.length - 1]);
	testlib.updateDetached(this.changesets[1]);
	
	//This commit will eventually be on otherbranch
	sg.fs.mkdir("sub/dir" + index++);	vscript_test_wc__addremove();	commitDetachedWrapper(this);
	this.otherbranch.push(this.changesets[this.changesets.length - 1]);
	testlib.execVV("branch", "add-head", "otherbranch", "--rev=" + this.changesets[this.changesets.length - 1]);
	testlib.execVV("branch", "attach", "otherbranch");
    
	//This commit will be on otherbranch
	sg.fs.mkdir("sub/dir" + index++);	vscript_test_wc__addremove();	commitWrapper(this);
	this.otherbranch.push(this.changesets[this.changesets.length - 1]);

	testlib.updateBranch("master");
	}
    
    this.checkResultsFor = function(testArray, arrayToLookFor)
	{
		for (result in testArray)
		{
			for (rev in arrayToLookFor)
			{
				if (testArray[result].changeset_id == arrayToLookFor[rev])
					return true;
			}
		}
		return false;
	}
	
	this.checkResultsMustHave = function(testArray, arrayToLookFor)
	{
		for (rev in arrayToLookFor)
		{
			bFoundThisOne = false;
			for (result in testArray)
			{
				if (testArray[result].changeset_id == arrayToLookFor[rev])
				{
					bFoundThisOne = true;
					break;
				}
			}
			if (bFoundThisOne == false)
			{
				print("did not find: " + rev + " in results");
				print(dumpObj(testArray));
				return false;
			}
		}
		return true;
	}
    this.testHistoryFindsCurrentBranch = function() {
	//By default, it should only show master 
    	var historyResults = sg.vv2.history();
    	testlib.equal(4, historyResults.length, "There should be only four entries for history on the master branch");
		testlib.equal(this.checkResultsFor(historyResults, this.otherbranch), false, "Results for otherbranch should not be included")
		testlib.equal(this.checkResultsFor(historyResults, this.untied), false, "Results for untied should not be included")
		testlib.equal(this.checkResultsMustHave(historyResults, this.master), true, "Results for master should be included")
    }    
	
	this.testHistoryFindsOtherBranch = function() {
		var historyResults = sg.vv2.history( { "branch" : "otherbranch" } );

		testlib.equal(4, historyResults.length, "There should be only four entries for history on the other branch");
		//otherbranch will have some master nodes in it.  That's ok.
		//testlib.equal(this.checkResultsFor(historyResults, this.master), false, "Results for master should not be included")
		testlib.equal(this.checkResultsFor(historyResults, this.untied), false, "Results for untied should not be included")
		testlib.equal(this.checkResultsMustHave(historyResults, this.otherbranch), true, "Results for otherbranch should be included")
    }    
	
	this.testHistoryLeaves = function() {
	   	var historyResults = sg.vv2.history( { "leaves" : true } );

		testlib.equal(this.changesets.length, historyResults.length, "There should be only an entry for every changeset");
		
		testlib.equal(this.checkResultsMustHave(historyResults, this.master), true, "Results for master should be included")
		testlib.equal(this.checkResultsMustHave(historyResults, this.untied), true, "Results for untied should be included")
		testlib.equal(this.checkResultsMustHave(historyResults, this.otherbranch), true, "Results for otherbranch should be included")
    }
	
	this.testHistoryBranchAndFolder = function() {
	   	var historyResults = sg.vv2.history( { "branch" : "otherbranch",
						       "src" : "sub" } );
		//print(dumpObj(historyResults));
		//print("----------------------");
		//print(dumpObj(this.otherbranch));
		testlib.equal(this.checkResultsFor(historyResults, this.untied), false, "Results for untied should not be included")
		testlib.equal(this.checkResultsMustHave(historyResults, this.otherbranch), true, "Results for otherbranch should be included")    }
}

function st_HistoryFindObject() {
   this.changesets = new Array();
   this.changesets.push("initial commit");
   this.times = new Array();
   this.master = new Array();
   this.untied = new Array();
   this.otherbranch = new Array();
	this.times.push(new Date().getTime());
    this.setUp = function() {
	index = 0;
	//This commit will be on master
	sg.fs.mkdir("sub1");	vscript_test_wc__addremove();	commitWrapper(this);
	this.master.push(this.changesets[this.changesets.length - 1]);
	
	testlib.updateAttached(this.changesets[1], "master");
	
	sg.fs.mkdir("sub2");	vscript_test_wc__addremove();	commitWrapper(this);
	this.master.push(this.changesets[this.changesets.length - 1]);
	
	testlib.updateAttached(this.changesets[1], "master");

	sg.fs.mkdir("sub3");	vscript_test_wc__addremove();	commitWrapper(this);
	this.master.push(this.changesets[this.changesets.length - 1]);
	}
    
   
    this.testHistoryFindsCorrect = function() {
		var repo = sg.open_repo(repInfo.repoName);

		var historyResults = repo.history(10, null, null, null, null, null, "@/sub2");
		testlib.equal(historyResults.items.length, 1, "One result should be returned.");
		testlib.equal(historyResults.items[0].changeset_id, this.changesets[2], "The correct changeset should be returned.");
		
		var historyResults = repo.history(10, null, null, null, null, null, "@/sub3");
		testlib.equal(historyResults.items.length, 1, "One result should be returned.");
		testlib.equal(historyResults.items[0].changeset_id, this.changesets[3], "The correct changeset should be returned.");
		
		repo.close();
    }    
}

function st_HistoryFetchMore() {
   this.changesets = new Array();
   this.changesets.push("initial commit");
   this.times = new Array();
   this.master = new Array();
   this.untied = new Array();
   this.otherbranch = new Array();
	this.times.push(new Date().getTime());
    this.setUp = function() {
	index = 0;
	print("starting setup");
	sg.fs.mkdir("sub");	vscript_test_wc__addremove();	commitWrapper(this);
	for (i = 0; i < 20; i++)
	{
		//These commits will be on master
		sg.fs.mkdir("sub/dir" + index++);	vscript_test_wc__addremove();	commitWrapper(this);
		this.master.push(this.changesets[this.changesets.length - 1]);
	}
	/*print("committing detached");
	testlib.updateDetached(this.changesets[1]);
	
	for (i = 0; i < 20; i++)
	{
		//This commit will be untied to a branch
		sg.fs.mkdir("sub/dir" + index++);	vscript_test_wc__addremove();	commitWrapper(this);
		this.untied.push(this.changesets[this.changesets.length - 1]);
	}
	*/
	testlib.updateDetached(this.changesets[1]);
	
	testlib.execVV("branch", "new", "otherbranch");
	for (i = 0; i < 20; i++)
	{
		//This commit will eventually be on otherbranch
		sg.fs.mkdir("sub/dir" + index++);	vscript_test_wc__addremove();	commitWrapper(this);
		this.otherbranch.push(this.changesets[this.changesets.length - 1]);
	}

	testlib.updateBranch("master");
	}
    
    this.checkResultsFor = function(testArray, testArray2)
	{
		var hash = {};
		for (result in testArray.items)
		{
			hash[testArray.items[result].changeset_id] = true;
		}
		for (rev in testArray2.items)
		{
			if (hash[testArray2.items[rev].changeset_id] != null)
				return true;
		}
		return false;
	}
	
	this.checkResultsMustHave = function(testArray, arrayToLookFor)
	{
		for (rev in arrayToLookFor)
		{
			bFoundThisOne = false;
			for (result in testArray)
			{
				if (testArray[result].changeset_id == arrayToLookFor[rev])
				{
					bFoundThisOne = true;
					break;
				}
			}
			if (bFoundThisOne == false)
			{
				print("did not find: " + rev + " in results");
				print(dumpObj(testArray));
				return false;
			}
		}
		return true;
	}
	this.formatDate = function(epoch)
	{
		var d = new Date(epoch);
		var curr_date = d.getDate();
		if (curr_date < 10)
			curr_date = "0" + curr_date;
		var curr_month = d.getMonth();
		curr_month++;
		if (curr_month < 10)
			curr_month = "0" + curr_month;
		var curr_year = d.getFullYear();
		var curr_hour = d.getHours();
		if (curr_hour < 10)
			curr_hour = "0" + curr_hour;
		var curr_min = d.getMinutes();
		if (curr_min < 10)
			curr_min = "0" + curr_min;
		var curr_sec = d.getSeconds();
		if (curr_sec < 10)
			curr_sec = "0" + curr_sec;
		datetimeString = curr_year + "-" + curr_month + "-" + curr_date + " " + curr_hour + ":" + curr_min + ":" + curr_sec;
		return datetimeString;
	}
    this.testHistoryFetchMoreListUnfiltered = function() {
		var repo = sg.open_repo(repInfo.repoName);
    	var historyResults = repo.history(10);
		while(historyResults != null && historyResults.items != null && historyResults.next_token != null && historyResults.items.length > 0)
		{
			print("fetching 10 more");
			historyResultsNew = repo.history_fetch_more(historyResults.next_token, 10);
			if (historyResultsNew != null)
			{
				testlib.equal(this.checkResultsFor(historyResultsNew, historyResults), false, "There should be no repeats of changesets.")			
				historyResults = historyResultsNew;
			}
		}
		repo.close();
    }    
	
	this.testHistoryFetchMoreListFiltered = function() {
		var repo = sg.open_repo(repInfo.repoName);
		var datetimeString = this.times[3].toString();
		print(datetimeString);
    	var historyResults = repo.history(10, null, null, datetimeString);
		while(historyResults != null && historyResults.items != null && historyResults.next_token != null && historyResults.items.length > 0)
		{
			historyResultsNew = repo.history_fetch_more(historyResults.next_token, 10);
			if (historyResultsNew != null)
			{
				testlib.equal(this.checkResultsFor(historyResultsNew, historyResults), false, "There should be no repeats of changesets.")			
				historyResults = historyResultsNew;
			}
		}
		repo.close();
    }
	
	this.testHistoryFetchMoreDagwalkUnfiltered = function() {
		var repo = sg.open_repo(repInfo.repoName);
		var historyResults = repo.history(10, null, null, null, null, "master");
		while(historyResults != null && historyResults.items != null && historyResults.next_token != null && historyResults.items.length > 0)
		{
			historyResultsNew = repo.history_fetch_more(historyResults.next_token, 10);
			if (historyResultsNew != null)
			{
				testlib.equal(this.checkResultsFor(historyResultsNew, historyResults), false, "There should be no repeats of changesets.")			
				historyResults = historyResultsNew;
			}
		}
		repo.close();
    }    
	
	this.testHistoryFetchMoreDagwalkFiltered = function() {
		var repo = sg.open_repo(repInfo.repoName);
		var datetimeString = this.times[3].toString();
		print(datetimeString);
		var historyResults = repo.history(10, null, null, datetimeString, null, "master");
		while(historyResults != null && historyResults.items != null && historyResults.next_token != null && historyResults.items.length > 0)
		{
			historyResultsNew = repo.history_fetch_more(historyResults.next_token, 10);
			if (historyResultsNew != null)
			{
				testlib.equal(this.checkResultsFor(historyResultsNew, historyResults), false, "There should be no repeats of changesets.")			
				historyResults = historyResultsNew;
			}
		}
		repo.close();
    }
}

function st_HistoryHideMerges() {
   this.changesets = new Array();
   this.changesets.push("initial commit");
   this.times = new Array();
	this.times.push(new Date().getTime());
	this.hideableMerges = new Array();
    this.setUp = function() {
	index = 0;
	fileName = "mergeMe.txt";
	versions = new Array();
	versions.push( 			 "a\nb\nc\nd\ne\nf\ng\n"													);
	versions.push(	 		 "a\nb\nc\nd\ne\nf\ng\nh\ni\nj\n"											);
	versions.push(	 		 "a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk\nl\nm\n"									);
	versions.push(	 		 "a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk\nl\nm\nn\no\np\n"							);
	versions.push(	 		 "a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk\nl\nm\nn\no\np\nr\ns\nt\n"				);
	versions.push(	 		 "a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk\nl\nm\nn\no\np\nr\ns\nt\nu\nv\n"			);

	sg.file.write(fileName, versions[0]);	vscript_test_wc__addremove();	commitWrapper(this);
	sg.file.write(fileName, versions[1]);	vscript_test_wc__addremove();	commitWrapper(this);
	sg.file.write(fileName, versions[2]);	vscript_test_wc__addremove();	commitWrapper(this);
	
	//Update to an older changeset, then commit a nonconflicting change.
	
	testlib.updateAttached(this.changesets[1], "master");
	sg.file.write("notAConflict.txt", "hello\n");	vscript_test_wc__addremove();	commitWrapper(this);
	
//////////////////////////////////////////////////////////////////
// TODO 2012/07/02 We no longer support .manually_set_parents().
// TODO            I'm not sure what the purpose of the following
// TODO            steps are.
//
//	//Perform the merge.  Commit the identical contents to the version in the other leaf.
//	sg.XXX.manually_set_parents(this.changesets[3], this.changesets[4]);
//	sg.file.write(fileName, versions[2]);	vscript_test_wc__addremove();	commitWrapper(this);
//	this.hideableMerges.push(this.changesets[this.changesets.length - 1]);
//	
//	//Now divergent edits.
//	sg.file.write(fileName, versions[3]);	vscript_test_wc__addremove();	commitWrapper(this);
//	testlib.execVV("update", "--rev=" + this.changesets[5]);
//	sg.file.write(fileName, versions[4]);	vscript_test_wc__addremove();	commitWrapper(this);
//	sg.XXX.manually_set_parents(this.changesets[6], this.changesets[7]);
//	sg.file.write(fileName, versions[5]);	vscript_test_wc__addremove();	commitWrapper(this);
//////////////////////////////////////////////////////////////////	
	}
    
    this.checkResultsFor = function(testArray, arrayToLookFor)
	{
		for (result in testArray)
		{
			for (rev in arrayToLookFor)
			{
				if (testArray[result].changeset_id == arrayToLookFor[rev])
					return true;
			}
		}
		return false;
	}
	
	this.checkResultsMustHave = function(testArray, arrayToLookFor)
	{
		for (rev in arrayToLookFor)
		{
			bFoundThisOne = false;
			for (result in testArray)
			{
				if (testArray[result].changeset_id == arrayToLookFor[rev])
				{
					bFoundThisOne = true;
					break;
				}
			}
			if (bFoundThisOne == false)
			{
				print("did not find: " + rev + " in results");
				print(dumpObj(testArray));
				return false;
			}
		}
		return true;
	}
    this.testHistoryHideMerges = function() {
	    	//It's an error to use hide-merges without a file or folder argument.
	    	testlib.execVV_expectFailure("history", "--hide-merges");

		historyResults = sg.vv2.history( { "hide-merges" : true,
						   "src" : fileName } );
		testlib.equal(this.changesets.length - this.hideableMerges.length - 1 - 1, historyResults.length, "There should be the right number of history results");
		testlib.equal(this.checkResultsFor(historyResults, this.hideableMerges), false, "Results for hideableMerges should not be included")
		historyResults = sg.vv2.history( { "hide-merges" : true,
						   "leaves" : true,
						   "src" : fileName } );
		testlib.equal(this.checkResultsFor(historyResults, this.hideableMerges), false, "Results for hideableMerges should not be included")
	}    
}



