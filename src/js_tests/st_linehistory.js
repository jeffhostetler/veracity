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
fileVersion1 = "1\r\n2\r\n3\r\n4\r\n5\r\n6\r\n7\r\n8\r\n9\r\n10\r\n11"; //revno 2
fileVersion2 = "1\r\n2\r\n3\r\n4edited\r\n5\r\n6\r\n7\r\n8\r\n9\r\n10\r\n11"; //revno 3
fileVersion3 = "1\r\n3\r\n4edited\r\n5\r\n6\r\n6.5\r\n\6.75\r\n7\r\n8\r\n9\r\n10\r\n11"; //revno 4
fileVersion4 = "1\r\n3\r\n4edited\r\n5\r\n6\r\n6.5\r\n\6.75\r\n7\r\n8\r\n9edited\r\n10\r\n11"; //revno 5

function commitWrapper(obj)
    {
    	obj.changesets.push( vscript_test_wc__commit() );
		obj.times.push(new Date().getTime());
		testlib.addTag(obj.changesets[obj.changesets.length - 1], 
						"changeset_" + obj.changesets.length );
  		
		testlib.addStamp(obj.changesets[obj.changesets.length - 1], 
						(obj.changesets.length % 2) == 0 ? "even" : "odd");
    }
	
function st_LineHistory_wf_edited() {
	this.changesets = new Array();
	this.changesets.push("initial commit");
	this.times = new Array();
	this.times.push(0);
	this.setUp = function() {
		vscript_test_wc__write_file("line_history_file_name", fileVersion1);
		vscript_test_wc__addremove();
		vscript_test_wc__commit();
		vscript_test_wc__write_file("line_history_file_name", fileVersion2);
	}

	this.testLineHistoryErrors_WC = function() {
		
		bGotError = false;
		try
		{
			sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 4, "length" : 0 });
		}
		catch(e)
		{
			print(e.toString());
			if (e.toString().indexOf("nonzero") != -1)
				bGotError = true;
		}
		
		testlib.ok(bGotError, "Expected an error");
		
		bGotError = false;
		try
		{
			sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 40, "length" : 1 });
		}
		catch(e)
		{
			print(e.toString());
			if (e.toString().indexOf("past the end of the file") != -1)
				bGotError = true;
		}
		
		testlib.ok(bGotError, "Expected an error");
		
		bGotError = false;
		try
		{
			sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 4, "length" : 1 });
		}
		catch(e)
		{
			print(e.toString());
			if (e.toString().indexOf("have been modified in the working copy") != -1)
				bGotError = true;
		}
		
		testlib.ok(bGotError, "Expected an error");
		
		bGotError = false;
		try
		{
			sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 0, "length" : 1 });
		}
		catch(e)
		{
			print(e.toString());
			if (e.toString().indexOf("first") != -1)
				bGotError = true;
		}
		
		testlib.ok(bGotError, "Expected an error");
	}
}


function st_LineHistory_wf() {
	this.changesets = new Array();
	this.changesets.push("initial commit");
	this.times = new Array();
	this.times.push(0);
	this.setUp = function() {
		vscript_test_wc__write_file("line_history_file_name", fileVersion1);
		vscript_test_wc__addremove();
		commitWrapper(this);
		vscript_test_wc__write_file("line_history_file_name", fileVersion2);
		commitWrapper(this);
		vscript_test_wc__write_file("line_history_file_name", fileVersion3);
		commitWrapper(this);
		vscript_test_wc__write_file("line_history_file_name", fileVersion4);
		commitWrapper(this);
		print(sg.to_json__pretty_print(this.changesets));
	}

	this.testLineHistory_WC = function() {		
		//This finds 4edited
		results = sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 3, "length" : 1 });
		
		testlib.equal(this.changesets[2], results[0].child_csid, "check for correct changeset");
		testlib.equal(3, results[0].child_revno, "check for correct changeset");
		testlib.equal(4, results[0].child_start_line, "check for correct child line");
		testlib.equal(4, results[0].parent_start_line, "check for correct parent line");
		testlib.equal(1, results[0].child_length, "check for correct child length");
		testlib.equal(1, results[0].parent_length, "check for correct parent length");
		
		//This line has been unchanged since the first version
		//of the file
		results = sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 11, "length" : 1 });
		
		testlib.equal(2, results[0].child_revno, "check for correct changeset");
		testlib.equal(1, results[0].child_start_line, "check for correct child line");
		testlib.equal(1, results[0].parent_start_line, "check for correct parent line");
		testlib.equal(0, results[0].parent_length, "check for correct parent length");;
		
		//Should find the deleted line 2
		results = sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 1, "length" : 2 });
		
		testlib.equal(4, results[0].child_revno, "check for correct changeset");
		testlib.equal(2, results[0].child_start_line, "check for correct child line");
		testlib.equal(2, results[0].parent_start_line, "check for correct parent line");
		testlib.equal(0, results[0].child_length, "check for correct child length");
		testlib.equal(1, results[0].parent_length, "check for correct parent length");
		
		//Should find the '9edited' change
		results = sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 9, "length" : 4 });
		
		testlib.equal(5, results[0].child_revno, "check for correct changeset");
		testlib.equal(10, results[0].child_start_line, "check for correct line");
		testlib.equal(10, results[0].parent_start_line, "check for correct line");
	}
}

fileVersion_commonAncestor = "1\r\n2\r\n3\r\n4\r\n5\r\n6\r\n7\r\n8\r\n9\r\n10\r\n11"; //revno 2
fileVersion_version2 = "1\r\n2edited\r\n3\r\n4\r\n5\r\n6\r\n7\r\n8\r\n9\r\n10\r\n11"; //revno 3
fileVersion_longRunningBranch = "1\r\n2edited\r\n3\r\n4\r\n5\r\n6\r\n7\r\n8\r\n9\r\n10edited\r\n11"; //revno 6
fileVersion_fork1 = "1\r\n2edited\r\n3\r\n4edited\r\n5\r\n6\r\n7\r\n8\r\n9\r\n10\r\n11"; //revno 7
fileVersion_fork2 = "1\r\n2edited\r\n3\r\n4\r\n5\r\n6edited\r\n7\r\n8\r\n9\r\n10\r\n11"; //revno 8
fileVersion_merged = "1\r\n2edited\r\n3\r\n4edited\r\n5\r\n6edited\r\n7\r\n8\r\n9\r\n10\r\n11"; //revno 9
fileVersion_fully_merged = "1\r\n2edited\r\n3\r\n4edited\r\n5\r\n6edited\r\n7\r\n8\r\n9\r\n10edited\r\n11"; //revno 10

function st_LineHistory_ignorable() {
	this.changesets = new Array();
	this.changesets.push("initial commit");
	this.times = new Array();
	this.times.push(0);
	this.setUp = function() {
		vscript_test_wc__write_file("line_history_file_name", fileVersion_commonAncestor);
		vscript_test_wc__addremove();
		commitWrapper(this);
		vscript_test_wc__write_file("line_history_file_name", fileVersion_version2);
		commitWrapper(this);
		
		//Update back to the common ancestor, and create branch that will merge back in
		//There are no edits to the file we are tracking.
		testlib.updateAttached("2", "master");
		vscript_test_wc__write_file("other_file", "unimportant");
		vscript_test_wc__addremove();
		commitWrapper(this);
		
		sg.wc.merge({}); //Merge revno 3 and 4 to create 5
		commitWrapper(this);
		
		vscript_test_wc__write_file("line_history_file_name", fileVersion_longRunningBranch);
		commitWrapper(this); //revno 6
		
		//Go back to revno 5.  We're creating three descendants off of 5.
		//They will be 6, 7, and 8
		testlib.updateAttached("5", "master");
		vscript_test_wc__write_file("line_history_file_name", fileVersion_fork1);
		commitWrapper(this); //revno 7
		
		testlib.updateAttached("5", "master");
		vscript_test_wc__write_file("line_history_file_name", fileVersion_fork2);
		commitWrapper(this); //revno 8
		
		sg.wc.merge({ "rev" : "7"}); //Merge revno 7 and 8 to create 9
		print(sg.exec("cat", "line_history_file_name").stdout);
		commitWrapper(this); //revno 9
		
		sg.wc.merge({ "rev" : "6"}); //Merge revno 6 and 9 to create 10
		commitWrapper(this); //revno 10
	
		print(sg.to_json__pretty_print(this.changesets));
	}

	this.findRevInResults = function(arrayToCheck, revno, parent_revno, ignorable)
	{
		for (key in arrayToCheck)
		{
			if (arrayToCheck[key].child_revno == revno)
			{
				if (arrayToCheck[key].parent_revno != parent_revno)
					continue;
				
				if (ignorable)
					return ignorable == arrayToCheck[key].probably_ignorable;
				else
					return arrayToCheck[key].probably_ignorable == null;
			}
		}
		return false;
	}
	this.testLineHistory_Ignorable = function() {		
		//Line 1 hasn't been changed at all, in any versions.
		results = sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 1, "length" : 1 });
		testlib.equal(2, results[0].child_revno, "check for correct changeset");
		testlib.equal(1, results[0].child_start_line, "check for correct child line");
		testlib.equal(1, results[0].parent_start_line, "check for correct parent line");
		testlib.equal(0, results[0].parent_length, "check for correct parent length");;
		
		//Line 2 was edited in revno 3
		results = sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 2, "length" : 1 });
		testlib.equal(3, results[0].child_revno, "check for correct changeset");
		testlib.equal(2, results[0].child_start_line, "check for correct child line");
		testlib.equal(2, results[0].parent_start_line, "check for correct parent line");
		testlib.equal(1, results[0].parent_length, "check for correct parent length");;
		testlib.equal(1, results[0].child_length, "check for correct child length");;
		
		//Line 4 was originally edited in revno 7. That edit was reapplied with revno 9 and 10
		results = sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 4, "length" : 1 });
		testlib.equal(3, results.length, "expected three results");
		testlib.ok(this.findRevInResults(results, 10, 6, true), "expected revno 10 to be ignorable");
		
		testlib.ok(this.findRevInResults(results, 9, 8, true), "expected revno 9 to be ignorable");
		
		testlib.ok(this.findRevInResults(results, 7, 5, false), "expected revno 7 to be in the results");
		
		//Line 4 and 6 were originally edited in revno 7 and 8, respectively. They were merged with revno 9, and reapplied in 10
		results = sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 4, "length" : 3 });
		testlib.equal(3, results.length, "expected three results");
		//print(sg.to_json__pretty_print(results));
		testlib.ok(this.findRevInResults(results, 10, 6, true), "expected revno 10 to be in the results");		
		
		testlib.ok(this.findRevInResults(results, 9, 7, false), "expected revno 9 to be in the results");		
		
		testlib.ok(this.findRevInResults(results, 9, 8, false), "expected revno 9 to be in the results");		
		
		//Line 10 was originally edited in revno 6. That edit was reapplied with revno 10
		results = sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 10, "length" : 1 });
		testlib.equal(2, results.length, "expected two results");
		testlib.ok(this.findRevInResults(results, 10, 9, true), "expected revno 10 to be in the results");		
		
		testlib.ok(this.findRevInResults(results, 6, 5, false), "expected revno 6 to be in the results");		
		
		
		results = sg.wc.line_history({ "src" : "line_history_file_name", "start_line" : 1, "length" : 11 });
		testlib.equal(2, results.length, "expected two results");
		testlib.ok(this.findRevInResults(results, 10, 9, false), "expected revno 10 to be in the results");		
		testlib.ok(this.findRevInResults(results, 10, 6, false), "expected revno 10 to be in the results");		
	}
}


