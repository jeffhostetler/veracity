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
function commitWrapper(obj, stampToApply1, stampToApply2)
{
    if (stampToApply1 != null && stampToApply2 != null)
	obj.changesets.push(vscript_test_wc__commit_np( { "stamp" : [ stampToApply1, stampToApply2],
							  "message" : "No Message." } ));
    else if (stampToApply1 != null)
	obj.changesets.push(vscript_test_wc__commit_np( { "stamp" : stampToApply1,
							  "message" : "No Message." } ));
    else
	obj.changesets.push(vscript_test_wc__commit_np( { "message" : "No Message." } ));
    sleepStupidly(1);
    obj.times.push(new Date().getTime());
    runCLCstamp("tag", "add", "changeset_" + obj.changesets.length, "--rev="+obj.changesets[obj.changesets.length - 1]);
    sleepStupidly(1);
}
function runCLCstamp(arg1, arg2, arg3, arg4, arg5, arg6, arg7)
{
	var execResults = null;
	var vv = "vv"
	if (arg7 != null)
	{
		print("executing:  " + vv + " " + arg1 + " " + arg2 + " " + arg3 + " " + arg4 + " " + arg5 + " " + arg6 + " " + arg7);
		execResults = sg.exec(vv, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	}
	else if (arg6 != null)
	{
		print("executing:  " + vv + " " + arg1 + " " + arg2 + " " + arg3 + " " + arg4 + " " + arg5 + " " + arg6);
		execResults = sg.exec(vv, arg1, arg2, arg3, arg4, arg5, arg6);
	}
	else if (arg5 != null)
	{
		print("executing:  " + vv + " " + arg1 + " " + arg2 + " " + arg3 + " " + arg4 + " " + arg5);
		execResults = sg.exec(vv, arg1, arg2, arg3, arg4, arg5);
	}
	else if (arg4 != null)
	{
		print("executing:  " + vv + " " + arg1 + " " + arg2 + " " + arg3 + " " + arg4);
		execResults = sg.exec(vv, arg1, arg2, arg3, arg4);
	}
	else if (arg3 != null)
	{
		print("executing:  " + vv + " " + arg1 + " " + arg2 + " " + arg3);
		execResults = sg.exec(vv, arg1, arg2, arg3);
	}
	else if (arg2 != null)
	{
		print("executing:  " + vv + " " + arg1 + " " + arg2);
		execResults = sg.exec(vv, arg1, arg2);
	}
	else if (arg1 != null)
	{
		print("executing:  " + vv + " " + arg1);
		execResults = sg.exec(vv, arg1);
	}
	else
	{
		print("executing:  " + vv);
		execResults = sg.exec(vv);
	}

	if (arg1 == "history")
	{
		changesets =execResults.stdout.split("revision:");
		changesets.splice(0, 1);  //Remove the first, empty array element
	}
	else
	{
		changesets =execResults.stdout.replace(/\r/g, "").split("\n");
		changesets.splice(changesets.length - 1, 1);  //Remove the last, empty array element
	}

	print(dumpObj(execResults));
	return changesets;
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

function st_StampMain() {
	this.changesets = new Array();
	this.changesets.push("initial commit");
	this.times = new Array();
	this.times.push(0);
	this.setUp = function() {
		runCLCstamp("stamp", "add", "odd", "--rev=" +sg.wc.parents()[0]);
		runCLCstamp("stamp", "add", "one", "--rev=" +sg.wc.parents()[0]);
		sg.fs.mkdir("test");
		vscript_test_wc__addremove();
		commitWrapper(this);
		runCLCstamp("stamp", "add", "even", "--rev=" +sg.wc.parents()[0]);
		runCLCstamp("stamp", "add", "two", "--rev=" +sg.wc.parents()[0]);
		sg.fs.mkdir("test2");
		vscript_test_wc__addremove();
		commitWrapper(this);
		runCLCstamp("stamp", "add", "odd", "--rev=" +sg.wc.parents()[0]);
		runCLCstamp("stamp", "add", "three", "--rev=" +sg.wc.parents()[0]);
		sg.fs.mkdir("test3");
		vscript_test_wc__addremove();
		commitWrapper(this);
		runCLCstamp("stamp", "add", "even", "--rev=" +sg.wc.parents()[0]);
		runCLCstamp("stamp", "add", "four", "--rev=" +sg.wc.parents()[0]);
		sg.fs.mkdir("test2/sub1");
		vscript_test_wc__addremove();
		commitWrapper(this);
		runCLCstamp("stamp", "add", "odd", "--rev=" +sg.wc.parents()[0]);
		runCLCstamp("stamp", "add", "five", "--rev=" +sg.wc.parents()[0]);
	}

	this.testStampMain = function() {
		//Verify that a limitless stamp on @ returns an entry for every
		//commit.
		var clcstampResults = runCLCstamp("stamp", "list");
		testlib.equal(7, clcstampResults.length, "stamp should return the right count.");
		testlib.equal("even:	2", clcstampResults[0], "stamp should return results with the right count and order.");
		testlib.equal("five:	1", clcstampResults[1], "stamp should return results with the right count and order.");
		testlib.equal("four:	1", clcstampResults[2], "stamp should return results with the right count and order.");
		testlib.equal("odd:	3", clcstampResults[3], "stamp should return results with the right count and order.");
		testlib.equal("one:	1", clcstampResults[4], "stamp should return results with the right count and order.");
		testlib.equal("three:	1", clcstampResults[5], "stamp should return results with the right count and order.");
		testlib.equal("two:	1", clcstampResults[6], "stamp should return results with the right count and order.");
		var clcstampResults = runCLCstamp("stamp", "remove", "five", "--rev=" + this.changesets[this.changesets.length - 1]);
		var clcstampResults = runCLCstamp("stamp", "remove", "odd", "--rev=" + this.changesets[this.changesets.length - 1]);
		var clcstampResults = runCLCstamp("stamp", "list");
		testlib.equal(6, clcstampResults.length, "stamp should return the right count.");
		testlib.equal("even:	2", clcstampResults[0], "stamp should return results with the right count and order.");
		testlib.equal("four:	1", clcstampResults[1], "stamp should return results with the right count and order.");
		testlib.equal("odd:	2", clcstampResults[2], "stamp should return results with the right count and order.");
		testlib.equal("one:	1", clcstampResults[3], "stamp should return results with the right count and order.");
		testlib.equal("three:	1", clcstampResults[4], "stamp should return results with the right count and order.");
		testlib.equal("two:	1", clcstampResults[5], "stamp should return results with the right count and order.");
		runCLCstamp("stamp", "add", "five", "--rev=" +this.changesets[this.changesets.length - 1]);
		runCLCstamp("stamp", "add", "odd",  "--rev=" +this.changesets[this.changesets.length - 1]);
		var clcstampResults = runCLCstamp("stamp", "remove", "even", "--rev=" + this.changesets[this.changesets.length - 2]);
		var clcstampResults = runCLCstamp("stamp", "list");
		testlib.equal(7, clcstampResults.length, "stamp should return the right count.");
		testlib.equal("even:	1", clcstampResults[0], "stamp should return results with the right count and order.");
		testlib.equal("five:	1", clcstampResults[1], "stamp should return results with the right count and order.");
		testlib.equal("four:	1", clcstampResults[2], "stamp should return results with the right count and order.");
		testlib.equal("odd:	3", clcstampResults[3], "stamp should return results with the right count and order.");
		testlib.equal("one:	1", clcstampResults[4], "stamp should return results with the right count and order.");
		testlib.equal("three:	1", clcstampResults[5], "stamp should return results with the right count and order.");
		testlib.equal("two:	1", clcstampResults[6], "stamp should return results with the right count and order.");
	}
}


function st_StampJavascript() {
	this.changesets = new Array();
	this.changesets.push("initial commit");
	this.times = new Array();
	this.times.push(0);
	this.setUp = function() {
		sg.vv2.add_stamp( {"rev" : sg.wc.parents()[0], "stamp" : "odd" } );
		sg.vv2.add_stamp( {"rev" : sg.wc.parents()[0], "stamp" : "one" } );
		sg.fs.mkdir("test");
		vscript_test_wc__addremove();
		commitWrapper(this);
		sg.vv2.add_stamp( {"rev" : sg.wc.parents()[0], "stamp" : "even" } );
		sg.vv2.add_stamp( {"rev" : sg.wc.parents()[0], "stamp" : "two" } );
		sg.fs.mkdir("test2");
		vscript_test_wc__addremove();
		commitWrapper(this);
		sg.vv2.add_stamp( {"rev" : sg.wc.parents()[0], "stamp" : "odd" } );
		sg.vv2.add_stamp( {"rev" : sg.wc.parents()[0], "stamp" : "three" } );
		sg.fs.mkdir("test3");
		vscript_test_wc__addremove();
		commitWrapper(this, "even");
		sg.vv2.add_stamp( {"rev" : sg.wc.parents()[0], "stamp" : "four" } );
		sg.fs.mkdir("test2/sub1");
		vscript_test_wc__addremove();
		commitWrapper(this, "odd", "five");
	}

	this.testStampMain = function() {
		//Verify that a limitless stamp on @ returns an entry for every
		//commit.
		var clcstampResults = sg.vv2.stamps();
		testlib.equal(7, clcstampResults.length, "stamp should return the right count.");
		testlib.equal("even", clcstampResults[0].stamp, "stamp should return results with the right count and order.");
		testlib.equal("five", clcstampResults[1].stamp, "stamp should return results with the right count and order.");
		testlib.equal("four", clcstampResults[2].stamp, "stamp should return results with the right count and order.");
		testlib.equal("odd", clcstampResults[3].stamp, "stamp should return results with the right count and order.");
		testlib.equal("one", clcstampResults[4].stamp, "stamp should return results with the right count and order.");
		testlib.equal("three", clcstampResults[5].stamp, "stamp should return results with the right count and order.");
		testlib.equal("two", clcstampResults[6].stamp, "stamp should return results with the right count and order.");

		testlib.equal(2, clcstampResults[0].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[1].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[2].count, "stamp should return results with the right count and order.");
		testlib.equal(3, clcstampResults[3].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[4].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[5].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[6].count, "stamp should return results with the right count and order.");

	    sg.vv2.remove_stamp( {"rev" : this.changesets[this.changesets.length - 1] } ); // remove all the stamps on this cset
		var clcstampResults = sg.vv2.stamps();
		testlib.equal(6, clcstampResults.length, "stamp should return the right count.");
		testlib.equal("even", clcstampResults[0].stamp, "stamp should return results with the right count and order.");
		testlib.equal("four", clcstampResults[1].stamp, "stamp should return results with the right count and order.");
		testlib.equal("odd", clcstampResults[2].stamp, "stamp should return results with the right count and order.");
		testlib.equal("one", clcstampResults[3].stamp, "stamp should return results with the right count and order.");
		testlib.equal("three", clcstampResults[4].stamp, "stamp should return results with the right count and order.");
		testlib.equal("two", clcstampResults[5].stamp, "stamp should return results with the right count and order.");

		testlib.equal(2, clcstampResults[0].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[1].count, "stamp should return results with the right count and order.");
		testlib.equal(2, clcstampResults[2].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[3].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[4].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[5].count, "stamp should return results with the right count and order.");

	    sg.vv2.add_stamp(    {"rev" : this.changesets[this.changesets.length - 1], "stamp" : "odd"  } );
	    sg.vv2.add_stamp(    {"rev" : this.changesets[this.changesets.length - 1], "stamp" : "five" } );
	    sg.vv2.remove_stamp( {"rev" : this.changesets[this.changesets.length - 2], "stamp" : "even" } );
		var clcstampResults = sg.vv2.stamps();
		testlib.equal(7, clcstampResults.length, "stamp should return the right count.");
		testlib.equal("even", clcstampResults[0].stamp, "stamp should return results with the right count and order.");
		testlib.equal("five", clcstampResults[1].stamp, "stamp should return results with the right count and order.");
		testlib.equal("four", clcstampResults[2].stamp, "stamp should return results with the right count and order.");
		testlib.equal("odd", clcstampResults[3].stamp, "stamp should return results with the right count and order.");
		testlib.equal("one", clcstampResults[4].stamp, "stamp should return results with the right count and order.");
		testlib.equal("three", clcstampResults[5].stamp, "stamp should return results with the right count and order.");
		testlib.equal("two", clcstampResults[6].stamp, "stamp should return results with the right count and order.");

		testlib.equal(1, clcstampResults[0].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[1].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[2].count, "stamp should return results with the right count and order.");
		testlib.equal(3, clcstampResults[3].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[4].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[5].count, "stamp should return results with the right count and order.");


	    sg.vv2.add_stamp(    {"rev" : this.changesets[this.changesets.length - 2], "stamp" : "even"  } );
	    sg.vv2.remove_stamp( {"rev" : this.changesets[this.changesets.length - 2], "stamp" : "even" } );
	    sg.vv2.remove_stamp( {"rev" : this.changesets[this.changesets.length - 2], "stamp" : "four" } );
		var clcstampResults = sg.vv2.stamps();
		testlib.equal(6, clcstampResults.length, "stamp should return the right count.");
		testlib.equal("even", clcstampResults[0].stamp, "stamp should return results with the right count and order.");
		testlib.equal("five", clcstampResults[1].stamp, "stamp should return results with the right count and order.");
		testlib.equal("odd", clcstampResults[2].stamp, "stamp should return results with the right count and order.");
		testlib.equal("one", clcstampResults[3].stamp, "stamp should return results with the right count and order.");
		testlib.equal("three", clcstampResults[4].stamp, "stamp should return results with the right count and order.");
		testlib.equal("two", clcstampResults[5].stamp, "stamp should return results with the right count and order.");

		testlib.equal(1, clcstampResults[0].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[1].count, "stamp should return results with the right count and order.");
		testlib.equal(3, clcstampResults[2].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[3].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[4].count, "stamp should return results with the right count and order.");
		testlib.equal(1, clcstampResults[5].count, "stamp should return results with the right count and order.");

	}
	this.testStampExpectFailure = function() {
	    testlib.verifyFailure('sg.vv2.add_stamp( {"rev" : "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb", "stamp" : "hiThere" } )', "Stamp with bogus revision", "Changeset not found or none found with that prefix");
	    testlib.verifyFailure('sg.vv2.add_stamp( {"tag" : "bogus", "stamp" : "hiThere" })', "Stamp with bogus tag", "Tag not found");
	}
}
