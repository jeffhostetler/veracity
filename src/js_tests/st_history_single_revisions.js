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
    	obj.changesets.push( vscript_test_wc__commit() );
    	sleepStupidly(1);
		obj.times.push(new Date().getTime());
		testlib.addTag(obj.changesets[obj.changesets.length - 1], 
						"changeset_" + obj.changesets.length );
  		
		testlib.addStamp(obj.changesets[obj.changesets.length - 1], 
						(obj.changesets.length % 2) == 0 ? "even" : "odd");
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
    	var execResults = testlib.execVV.apply(testlib, args);
		
    	resultChangesets =execResults.stdout.split("revision:  ");
		resultChangesets.splice(0, 1);  //Remove the first, empty array element
		returnChangesets = [];
		for (i = 0; i < resultChangesets.length; i++)
		{
			returnChangesets.push(parseSingleHistoryResult(resultChangesets[i]));
		}
    	//print(dumpObj(execResults));
		//print(dumpObj(returnChangesets));
    	return returnChangesets;
    }

function testTagVsRev(changesetsArray, path, SRSargs, argumentsArray, expectedResultCount, verificationFunction)
{
	var argsTags = Array.prototype.slice.call(argumentsArray);
	for (i = 0; i < SRSargs.length; i++)
	{
		//append the tag arguments
		argsTags.push("--tag=changeset_" + SRSargs[i]);
	}
	testCLCvsJS(path, argsTags, expectedResultCount, verificationFunction);
	
	var argsRevs = Array.prototype.slice.call(argumentsArray);
	for (i = 0; i < SRSargs.length; i++)
	{
		//append the tag arguments
		argsRevs.push("--rev=" + changesetsArray[SRSargs[i] - 1]);
	}
	testCLCvsJS(path, argsRevs, expectedResultCount, verificationFunction);
	
}
function testCLCvsJS(path, argumentsArray, expectedResultCount, verificationFunction)
{
	var argsCLC = Array.prototype.slice.call(argumentsArray);
	if (path != null && path != "")
		argsCLC.push(path);
	for (i=0; i < argsCLC.length; i++)
	{
		if (argsCLC[i].search("--from=") == 0)
			argsCLC[i] = "--from=" + formatDate(argsCLC[i].split("=")[1], true);
		else if (argsCLC[i].search("--to=") == 0)
			argsCLC[i] = "--to=" + formatDate(argsCLC[i].split("=")[1], true);
	}
	print("CLC arguments.");
	print(sg.to_json__pretty_print(argsCLC));
	//print(dumpObj(argsCLC));
	
	var clchistoryResults = runCLCHistory.apply(this, argsCLC);
	testlib.equal(expectedResultCount, clchistoryResults.length, "CLC history did not return the expected number of results.");
	if (verificationFunction)
	{
		print("CLC results.");
		print(sg.to_json__pretty_print(clchistoryResults));
		//print(dumpObj(clchistoryResults));
		verificationFunction(clchistoryResults);
	}
	
	// convert [ ..., "--key=value", ... ]
	// into    { ..., "key" : "value", ... }
	//
	// convert [ ..., "--key", ... ]
	// into    { ..., "key" : true, ... }
	//
	// with special case for rev/tag/branch since for sg.vv2.history()
 	// they can appear multiple times (and mean something different than
	// most other routines).
	//
	// for them we convert [ ..., "--rev=r1", "--rev=r2", ... ]
	// into                { ..., "rev" : [ "r1", "r2" ], ... }
	//
	// and we put any paths into "src" inside the struct rather
	// than using a separate arg.

	var structJS = {};
	var argsJS = Array.prototype.slice.call(argumentsArray);
	for (i=0; i < argsJS.length; i++)
	{
	    if (argsJS[i].search("--") == 0)
	    {
		var key_value = argsJS[i].split("--")[1];
		var key       = key_value.split("=")[0];
		var value     = key_value.split("=")[1];

		if (value == undefined)
		{
		    structJS[key] = true;
		}
		else
		{
		    if ((key == "rev") || (key == "tag") || (key == "branch"))
		    {
			if (structJS[key] == undefined)
			    structJS[key] = [];
			structJS[key].push(value);
		    }
		    else
		    {
			structJS[key] = value;
		    }
		}
	    }
	}
	if (path != null && path != "")
    	{
	    structJS["src"] = [];
	    structJS["src"].push(path);
	}

	print("JS Args:");
	print(sg.to_json__pretty_print(argsJS));
	print("JS Struct");
	print(sg.to_json__pretty_print(structJS));
	
	var historyResults = sg.vv2.history( structJS );
	testlib.equal(expectedResultCount, historyResults.length, "JS history did not return the expected number results.");
	if (verificationFunction)
	{
		print("Javascript results.");
		print(sg.to_json__pretty_print(historyResults));
		//print(dumpObj(historyResults));
		verificationFunction(historyResults);
	}
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

function st_HistorySingleRevisions() {
   this.changesets = new Array();
   
   this.times = new Array();
   this.times.push(0);
    this.setUp = function() {
	this.changesets.push(sg.vv2.leaves()[0]);
	testlib.addInitialTagAndStamp();
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
		{
			testTagVsRev(this.changesets, "", [1], [], 1, 
				function(c) { 
					testlib.equal(1, c[0].revno, "Verify history results."); 
					});
			
			testTagVsRev(this.changesets, "", [2,3], [], 2, 
				function(c) { 
					testlib.equal(2, c[0].revno, "Verify history results."); 
					testlib.equal(3, c[1].revno, "Verify history results."); 
					});
			
			testTagVsRev(this.changesets, "", [3,4,5], [], 3, 
				function(c) { 
					testlib.equal(3, c[0].revno, "Verify history results."); 
					testlib.equal(4, c[1].revno, "Verify history results."); 
					testlib.equal(5, c[2].revno, "Verify history results."); 
					});
		}
		
		{
			testTagVsRev(this.changesets, "", [1], ["--reverse"], 1, 
				function(c) { 
					testlib.equal(1, c[0].revno, "Verify history results."); 
					});
			
			testTagVsRev(this.changesets, "", [2,3], ["--reverse"], 2, 
				function(c) { 
					testlib.equal(3, c[0].revno, "Verify history results."); 
					testlib.equal(2, c[1].revno, "Verify history results."); 
					});
			
			testTagVsRev(this.changesets, "", [3,4,5], ["--reverse"], 3, 
				function(c) { 
					testlib.equal(5, c[0].revno, "Verify history results."); 
					testlib.equal(4, c[1].revno, "Verify history results."); 
					testlib.equal(3, c[2].revno, "Verify history results."); 
					});
		}
		
		{
			testTagVsRev(this.changesets, "", [3], ["--from=" + this.times[1]], 1, 		
				function(c) { 
					testlib.equal(3, c[0].revno, "Verify history results."); 
					});
					
			testTagVsRev(this.changesets, "", [2], ["--from=" + this.times[1]], 0);
			
			testTagVsRev(this.changesets, "", [3,4], ["--from=" + this.times[1]], 2, 		
				function(c) { 
					testlib.equal(3, c[0].revno, "Verify history results."); 
					testlib.equal(4, c[1].revno, "Verify history results."); 
					});
					
			testTagVsRev(this.changesets, "", [1,2], ["--from=" + this.times[1]], 0);
		}

		{
			testTagVsRev(this.changesets, "", [2], ["--to=" + this.times[1]], 1, 		
				function(c) { 
					testlib.equal(2, c[0].revno, "Verify history results."); 
					});
					
			testTagVsRev(this.changesets, "", [3], ["--to=" + this.times[1]], 0);
			
			testTagVsRev(this.changesets, "", [1,2], ["--to=" + this.times[1]], 2, 		
				function(c) { 
					testlib.equal(1, c[0].revno, "Verify history results."); 
					testlib.equal(2, c[1].revno, "Verify history results."); 
					});
					
			testTagVsRev(this.changesets, "", [3,4], ["--to=" + this.times[1]], 0);
		}
		
		{
			testTagVsRev(this.changesets, "", [3], ["--stamp=odd"], 1, 		
				function(c) { 
					testlib.equal(3, c[0].revno, "Verify history results."); 
					});
					
			testTagVsRev(this.changesets, "", [3], ["--stamp=even"], 0);
			
			testTagVsRev(this.changesets, "", [1,3], ["--stamp=odd"], 2, 		
				function(c) { 
					testlib.equal(1, c[0].revno, "Verify history results."); 
					testlib.equal(3, c[1].revno, "Verify history results."); 
					});
					
			testTagVsRev(this.changesets, "", [3,5], ["--stamp=even"], 0);
		}
		
		{
			testTagVsRev(this.changesets, "test2", [3], [], 1, 		
				function(c) { 
					testlib.equal(3, c[0].revno, "Verify history results."); 
					});
					
			testTagVsRev(this.changesets, "test2", [2], [], 0);
			
			testTagVsRev(this.changesets, "test2", [3,5], [], 2, 		
				function(c) { 
					testlib.equal(3, c[0].revno, "Verify history results."); 
					testlib.equal(5, c[1].revno, "Verify history results."); 
					});
					
			testTagVsRev(this.changesets, "test2", [2,4], [], 0);
		}
	}
}
	
function st_HistorySingleRevisions_branches() {
   this.changesets = new Array();
   
   this.times = new Array();
   this.times.push(0);
    this.setUp = function() {
		this.changesets.push(sg.vv2.leaves()[0]);
		testlib.addInitialTagAndStamp();
		sg.fs.mkdir("test");
		vscript_test_wc__addremove();
		commitWrapper(this);
		sg.fs.mkdir("test2");
		vscript_test_wc__addremove();
		commitWrapper(this);
		
		testlib.updateAttached("2", "master");

		sg.fs.mkdir("test3");
		vscript_test_wc__addremove();
		commitWrapper(this);
		sg.fs.mkdir("test4");
		vscript_test_wc__addremove();
		commitWrapper(this);
		
		testlib.updateAttached("2", "master");
		testlib.execVV("branch", "new", "otherBranch");
		
		sg.fs.mkdir("test5");
		vscript_test_wc__addremove();
		commitWrapper(this);
		sg.fs.mkdir("test6");
		vscript_test_wc__addremove();
		commitWrapper(this);
    }
	
	this.testRoot = function() {
	//Verify that a limitless history on @ returns an entry for every
	//commit.
		{
			testTagVsRev(this.changesets, "", [1], ["--branch=master"], 1, 
				function(c) { 
					testlib.equal(1, c[0].revno, "Verify history results."); 
					});
			
			testTagVsRev(this.changesets, "", [2,4], ["--branch=master"], 2, 
				function(c) { 
					testlib.equal(2, c[0].revno, "Verify history results."); 
					testlib.equal(4, c[1].revno, "Verify history results."); 
					});
			
			testTagVsRev(this.changesets, "", [2,4,5], ["--branch=master"], 3, 
				function(c) { 
					testlib.equal(2, c[0].revno, "Verify history results."); 
					testlib.equal(4, c[1].revno, "Verify history results."); 
					testlib.equal(5, c[2].revno, "Verify history results."); 
					});
					
			//Rev 6 is not on master.
			testTagVsRev(this.changesets, "", [2,4,5,6], ["--branch=master"], 3, 
				function(c) { 
					testlib.equal(2, c[0].revno, "Verify history results."); 
					testlib.equal(4, c[1].revno, "Verify history results."); 
					testlib.equal(5, c[2].revno, "Verify history results."); 
					});
			
			testTagVsRev(this.changesets, "", [2,4,5,6], ["--branch=otherBranch"], 2, 
				function(c) { 
					testlib.equal(2, c[0].revno, "Verify history results."); 
					testlib.equal(6, c[1].revno, "Verify history results."); 
					});
			
			//Rev 6 is not on master.
			testTagVsRev(this.changesets, "", [2,4,5,6], ["--leaves"], 4, 
				function(c) { 
					testlib.equal(2, c[0].revno, "Verify history results."); 
					testlib.equal(4, c[1].revno, "Verify history results."); 
					testlib.equal(5, c[2].revno, "Verify history results."); 
					testlib.equal(6, c[3].revno, "Verify history results."); 
					});
			
			//Verify that if you supply a single revision, it doesn't 
			//try to automatically limit you to the current branch.
			testTagVsRev(this.changesets, "", [5], [], 1, 
				function(c) { 
					testlib.equal(5, c[0].revno, "Verify history results."); 
					});
		}
		
	}
}
