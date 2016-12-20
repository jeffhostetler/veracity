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
//    	sleepStupidly(1);
		obj.times.push(new Date().getTime());
		
		testlib.addTag(obj.changesets[obj.changesets.length - 1], 
						"changeset_" + obj.changesets.length );
  		
		testlib.addStamp(obj.changesets[obj.changesets.length - 1], 
						(obj.changesets.length % 2) == 0 ? "even" : "odd");
						
	//	sleepStupidly(1);
    }
	
function testJS(path, argumentsArray, expectedResultCount, verificationFunction)
{
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

	print("Javascript arguments.");
	print(dumpObj(structJS));
	
	//Force dag reassembly
	structJS["reassemble-dag"] = true;
	
	var repo = sg.open_repo(repInfo.repoName);
	var historyResults = repo.history( structJS["max"] != null ? parseInt(structJS["max"])	 : 0, 
									   structJS["user"] != null ? structJS["user"] : null, 
									   structJS["stamp"] != null ? structJS["stamp"] : null, 
									   structJS["from"] != null ? structJS["from"] : null, 
									   structJS["to"] != null ? structJS["to"] : null, 
									   structJS["branch"] != null ? structJS["branch"][0] : null, 
									   (structJS["src"] != null && structJS["src"].length > 0) ? structJS["src"][0] : null, 
									   structJS["hide-merges"] != null ? structJS["hide-merges"] : null, 
									   structJS["reassemble-dag"] != null ? structJS["reassemble-dag"] : null);
	repo.close();
	testlib.equal(expectedResultCount, historyResults.items.length, "JS history did not return the expected number results.");
	if (verificationFunction)
	{
		print("Javascript results.");
		print(dumpObj(historyResults.items));
		verificationFunction(historyResults.items, historyResults.next_token);
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

function checkParentsHashForRevno(parentsHash, revNo)
{
	for (var k in parentsHash) {
		if (parentsHash[k] == revNo)
			return true;
	}
	return false;
}

function checkParentsHashForChangesetID(parentsHash, changeset_id)
{
	for (var k in parentsHash) {
		if (k == changeset_id)
			return true;
	}
	return false;
}

function st_HistorySimpleDAG() {
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
		{
			testJS("", ["--stamp=odd"], 3, 
				function(c) { 
					testlib.equal(5, c[0].revno, "Verify history results."); 
					testlib.ok(checkParentsHashForRevno(c[0].pseudo_parents, 3), "Verify history results."); 
					
					testlib.equal(3, c[1].revno, "Verify history results."); 
					testlib.ok(checkParentsHashForRevno(c[1].pseudo_parents, 1), "Verify history results."); 
					
					testlib.equal(1, c[2].revno, "Verify history results."); 
					});
		}
	}
}

function st_HistoryMoreComplicated() {
   this.changesets = new Array();
   
   this.times = new Array();
   this.times.push(0);
    this.setUp = function() {
	this.changesets.push(sg.vv2.leaves()[0]);
	testlib.addInitialTagAndStamp();

	//2
	sg.fs.mkdir("test");
	vscript_test_wc__addremove();
	commitWrapper(this);
	
	//3
	sg.fs.mkdir("test2");
	vscript_test_wc__addremove();
	commitWrapper(this);
	
	testlib.updateAttached("2", "master");
	
	//4
	sg.fs.mkdir("test3");
	vscript_test_wc__addremove();
	commitWrapper(this);
	
	//5
	sg.fs.mkdir("test4");
	vscript_test_wc__addremove();
	commitWrapper(this);
	
	//6.  Parents are 3 and 5
	sg.wc.merge({});
	commitWrapper(this);
	
	//7
	sg.fs.mkdir("test5");
	vscript_test_wc__addremove();
	commitWrapper(this);
	
	testlib.updateAttached("5", "master");
	
	//8
	sg.fs.mkdir("test6");
	vscript_test_wc__addremove();
	commitWrapper(this);
	
	//9.  Parents are 7 and 8
	sg.wc.merge({});
	commitWrapper(this);
    }
	
	this.testRoot = function() {
		{
			testJS("", ["--stamp=odd"], 5, 
				function(c) { 
					nCount = c.length
					
					testlib.equal(9, c[nCount-5].revno, "Verify history results."); 
					testlib.ok(checkParentsHashForRevno(c[nCount-5].pseudo_parents, 7), "Verify history results."); 
					testlib.ok(!checkParentsHashForRevno(c[nCount-5].pseudo_parents, 5), "Verify history results."); 
					testlib.ok(!checkParentsHashForRevno(c[nCount-5].pseudo_parents, 8), "Verify history results."); 
					
					testlib.equal(7, c[nCount-4].revno, "Verify history results."); 
					testlib.ok(checkParentsHashForRevno(c[nCount-4].pseudo_parents, 3), "Verify history results."); 
					testlib.ok(checkParentsHashForRevno(c[nCount-4].pseudo_parents, 5), "Verify history results."); 
					
					testlib.equal(5, c[nCount-3].revno, "Verify history results."); 
					testlib.ok(checkParentsHashForRevno(c[nCount-3].pseudo_parents, 1), "Verify history results."); 
					
					testlib.equal(3, c[nCount-2].revno, "Verify history results."); 
					testlib.ok(checkParentsHashForRevno(c[nCount-2].pseudo_parents, 1), "Verify history results."); 
					
					testlib.equal(1, c[nCount-1].revno, "Verify history results."); 
					});
		}
	}
}


function st_HistoryFetchMore() {
   this.changesets = new Array();
   
   this.times = new Array();
   this.times.push(0);
    this.setUp = function() {
	this.changesets.push(sg.vv2.leaves()[0]);
	testlib.addInitialTagAndStamp();

	//2
	sg.fs.mkdir("test");
	vscript_test_wc__addremove();
	commitWrapper(this);
	
	//3
	sg.fs.mkdir("test2");
	vscript_test_wc__addremove();
	commitWrapper(this);
	
	//4
	sg.fs.mkdir("test3");
	vscript_test_wc__addremove();
	commitWrapper(this);

	testlib.updateAttached("2", "master");	
	
	//5
	sg.fs.mkdir("test4");
	vscript_test_wc__addremove();
	commitWrapper(this);
	
	//6.  Parents are 4 and 5
	sg.wc.merge({});
	commitWrapper(this);
	
	//7
	sg.fs.mkdir("test5");
	vscript_test_wc__addremove();
	commitWrapper(this);

	testlib.updateAttached("5", "master");	
	
	//8
	sg.fs.mkdir("test6");
	vscript_test_wc__addremove();
	commitWrapper(this);
	
	//9.  Parents are 7 and 8
	sg.wc.merge({});
	commitWrapper(this);

	testlib.updateAttached("4", "master");
	
	//10
	sg.fs.mkdir("test7");
	vscript_test_wc__addremove();
	commitWrapper(this);
	
	//11.  Pseudo parent is 3.
	sg.fs.mkdir("test8");
	vscript_test_wc__addremove();
	commitWrapper(this);
	
	}
	
	this.testRoot = function() {
		{
			testJS("", ["--stamp=odd", "--max=4"], 4, 
				function(c, t) { 
					nCount = c.length
					testlib.ok(t != null, "history token should not be null");
					
					testlib.equal(11, c[nCount-4].revno, "Verify history results."); 
					testlib.ok(c[nCount-4].pseudo_parents == null || c[nCount-5].pseudo_parents == 0, "Verify history results."); 
					rev11_csid = c[nCount-4].changeset_id;
					
					testlib.equal(9, c[nCount-3].revno, "Verify history results."); 
					testlib.ok(checkParentsHashForRevno(c[nCount-3].pseudo_parents, 7), "Verify history results."); 
					testlib.ok(!checkParentsHashForRevno(c[nCount-3].pseudo_parents, 5), "Verify history results."); 
					testlib.ok(!checkParentsHashForRevno(c[nCount-3].pseudo_parents, 8), "Verify history results."); 
					
					testlib.equal(7, c[nCount-2].revno, "Verify history results."); 
					testlib.ok(!checkParentsHashForRevno(c[nCount-2].pseudo_parents, 3), "Verify history results."); 
					testlib.ok(checkParentsHashForRevno(c[nCount-2].pseudo_parents, 5), "Verify history results."); 
					rev7_csid = c[nCount-2].changeset_id;
					
					testlib.equal(5, c[nCount-1].revno, "Verify history results."); 
					testlib.ok(!checkParentsHashForRevno(c[nCount-1].pseudo_parents, 1), "Verify history results."); 
					rev5_csid = c[nCount-1].changeset_id;
					
					var repo = sg.open_repo(repInfo.repoName);
					var historyResults = repo.history_fetch_more(t, 1);
					repo.close();
					c = historyResults.items;
					t = historyResults.next_token;
					testlib.ok(t != null, "history token should not be null");
					print(dumpObj(c));
					
					testlib.equal(3, c[0].revno, "Verify history results."); 
					
					testlib.ok(checkParentsHashForChangesetID(c[0].pseudo_children, rev11_csid), "Verify history results."); 
					testlib.ok(checkParentsHashForChangesetID(c[0].pseudo_children, rev7_csid), "Verify history results."); 
					rev3_csid = c[0].changeset_id;
					
					var repo = sg.open_repo(repInfo.repoName);
					var historyResults = repo.history_fetch_more(t, 1);
					repo.close();
					c = historyResults.items;
					
					print(dumpObj(c));
					
					testlib.equal(1, c[0].revno, "Verify history results."); 

					testlib.ok(checkParentsHashForChangesetID(c[0].pseudo_children, rev3_csid), "Verify history results."); 
					testlib.ok(checkParentsHashForChangesetID(c[0].pseudo_children, rev5_csid), "Verify history results."); 

					});
		}
	}
}