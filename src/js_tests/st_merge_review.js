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

var myexec = function(){
	if(sg.exec.apply(null, arguments).exit_status!==0){
		var msg="EXEC FAILED:";
		for(var i=0; i<arguments.length; ++i){
			msg+=" ";
			if(arguments[i].indexOf(" ")==-1){
				msg+=arguments[i];
			}
			else if(arguments[i].indexOf('"')==-1){
				msg+='"'+arguments[i]+'"';
			}
			else{
				msg+="'"+arguments[i]+"'";
			}
		}
		testlib.log(msg);
		testlib.testResult(false);
	}
}

function st_merge_review_basic()
{
	//     
	// 8   
	// |\  
	// 7 | 
	// | | 
	// | 6 
	// |/| 
	// 5 | 
	// | | 
	// | 4 
	// | | 
	// | 3 
	// | | 
	// 2 | 
	// |/  
	// 1   
	//     
	this.testBasic = function(){
		testlib.log("Here we go...");
		
		myexec("vv", "update", "--attach=master", "-r1");
		sg.file.write("r2-file.txt", "...");
		vscript_test_wc__add("r2-file.txt");
		vscript_test_wc__commit( "Added r2-file.txt with content \"...\".");
		testlib.log("(r2 added)");
		
		myexec("vv", "update", "--attach=master", "-r1");
		sg.file.write("r3-file.txt", "...");
		vscript_test_wc__add("r3-file.txt");
		vscript_test_wc__commit( "Added r3-file.txt with content \"...\".");
		testlib.log("(r3 added)");
		
		sg.file.write("r4-file.txt", "...");
		vscript_test_wc__add("r4-file.txt");
		vscript_test_wc__commit( "Added r4-file.txt with content \"...\".");
		testlib.log("(r4 added)");
		
		myexec("vv", "update", "--attach=master", "-r2");
		sg.file.write("r5-file.txt", "...");
		vscript_test_wc__add("r5-file.txt");
		vscript_test_wc__commit( "Added r5-file.txt with content \"...\".");
		testlib.log("(r5 added)");
		
		testlib.wcMerge();
		vscript_test_wc__commit( "Merged");
		testlib.log("(r6 added)");
		
		myexec("vv", "update", "--attach=master", "-r5");
		sg.file.write("r7-file.txt", "...");
		vscript_test_wc__add("r7-file.txt");
		vscript_test_wc__commit( "Added r7-file.txt with content \"...\".");
		testlib.log("(r7 added)");
		
		testlib.wcMerge();
		vscript_test_wc__commit( "Merged");
		testlib.log("(r8 added)");
		
		mr.verify(8, {"8":7}, "8(643)7521");
		mr.verify(8, {},             "8(7)6(52)431");
		mr.verify(8, {"8":6},        "8(7)6(52)431");
		mr.verify(8, {"8":6, "6":4}, "8(7)6(52)431");
		mr.verify(8, {"8":6, "6":5}, "8(7)6(43)521");
		
		mr.verify(6, {},      "6(52)431");
		mr.verify(6, {"6":4}, "6(52)431");
		mr.verify(6, {"6":5}, "6(43)521");
		
		testlib.testResult(true);
	};
}

function st_merge_review_basic_2()
{
	// 6     
	// |\    
	// | 5   
	// | |\  
	// | | 4 
	// | | | 
	// | 3 | 
	// | | | 
	// 2 |/  
	// |/    
	// 1     
	//       
	this.testBasic = function(){
		testlib.log("Here we go...");
		
		myexec("vv", "update", "--attach=master", "-r1");
		sg.file.write("r2-file.txt", "...");
		vscript_test_wc__add("r2-file.txt");
		vscript_test_wc__commit( "Added r2-file.txt with content \"...\".");
		testlib.log("(r2 added)");
		
		myexec("vv", "update", "--attach=master", "-r1");
		sg.file.write("r3-file.txt", "...");
		vscript_test_wc__add("r3-file.txt");
		vscript_test_wc__commit( "Added r3-file.txt with content \"...\".");
		testlib.log("(r3 added)");
		
		myexec("vv", "update", "--attach=master", "-r1");
		sg.file.write("r4-file.txt", "...");
		vscript_test_wc__add("r4-file.txt");
		vscript_test_wc__commit( "Added r4-file.txt with content \"...\".");
		testlib.log("(r4 added)");
		
		myexec("vv", "merge", "-r3");
		vscript_test_wc__commit( "Merged");
		testlib.log("(r5 added)");
		
		testlib.wcMerge();
		vscript_test_wc__commit( "Merged");
		testlib.log("(r6 added)");
		
		mr.verify(6, {},             "6(5(4)3)21");
		mr.verify(6, {"6":2},        "6(5(4)3)21");
		mr.verify(6, {"6":2, "5":3}, "6(5(4)3)21");
		mr.verify(6, {"6":2, "5":4}, "6(5(3)4)21");
		mr.verify(6, {"6":5},        "6(2)5(4)31");
		mr.verify(6, {"6":5, "5":3}, "6(2)5(4)31");
		mr.verify(6, {"6":5, "5":4}, "6(2)5(3)41");
		
		mr.verify(5, {},      "5(4)31");
		mr.verify(5, {"5":3}, "5(4)31");
		mr.verify(5, {"5":4}, "5(3)41");
		
		testlib.testResult(true);
	};
}

function st_merge_review_crisscross()
{
	//     (8)     
	//     / \     
	//    /   \    
	//   /     \   
	// (6)     (7) 
	//  |\     /|  
	//  | \   / |  
	//  |  \ /  |  
	// (5)  X  (3) 
	//  |  / \  |  
	//  | /   \ |  
	//  |/     \|  
	// (4)     (2) 
	//   \     /   
	//    \   /    
	//     \ /     
	//     (1)     
	//             
	this.testCrisscross = function(){
		testlib.log("Here we go...");
		
		myexec("vv", "update", "--attach=master", "-r1");
		sg.file.write("r2-file.txt", "...");
		vscript_test_wc__add("r2-file.txt");
		vscript_test_wc__commit( "Added r2-file.txt with content \"...\".");
		testlib.log("(r2 added)");
		
		sg.file.write("r3-file.txt", "...");
		vscript_test_wc__add("r3-file.txt");
		vscript_test_wc__commit( "Added r3-file.txt with content \"...\".");
		testlib.log("(r3 added)");
		
		myexec("vv", "update", "--attach=master", "-r1");
		sg.file.write("r4-file.txt", "...");
		vscript_test_wc__add("r4-file.txt");
		vscript_test_wc__commit( "Added r4-file.txt with content \"...\".");
		testlib.log("(r4 added)");
		
		sg.file.write("r5-file.txt", "...");
		vscript_test_wc__add("r5-file.txt");
		vscript_test_wc__commit( "Added r5-file.txt with content \"...\".");
		testlib.log("(r5 added)");
		
		myexec("vv", "merge", "-r2");
		vscript_test_wc__commit( "Merged.");
		testlib.log("(r6 added)");
		
		myexec("vv", "update", "--attach=master", "-r3");
		myexec("vv", "merge", "-r4");
		vscript_test_wc__commit( "Merged.");
		testlib.log("(r7 added)");
		
		testlib.wcMerge();
		vscript_test_wc__commit( "Merged.");
		testlib.log("(r8 added)");
		
		mr.verify(8, {},             "8(73)6(54)21");
		mr.verify(8, {"8":6       }, "8(73)6(54)21");
		mr.verify(8, {"8":6, "6":2}, "8(73)6(54)21");
		mr.verify(8, {"8":6, "6":5}, "8(73)6(2)541");
		mr.verify(8, {"8":7       }, "8(65)7(4)321");
		mr.verify(8, {"8":7, "7":3}, "8(65)7(4)321");
		mr.verify(8, {"8":7, "7":4}, "8(65)7(32)41");
		
		mr.verify(7, {}, "7(4)321");
		mr.verify(6, {}, "6(54)21");
		
		testlib.testResult(true);
	};
}

// merge_review test helpers
var mr = {
	check: function(what, passed){
		if(!passed){
			testlib.log("FAILED: "+what);
			testlib.testResult(false);
		}
		return passed;
	},
	expect: function(what, expected, actual){
		mr.check((what+" (expected: '"+expected+"', actual: '"+actual+"')"), expected===actual);
	},
	
	verify: function(head, baselines, expected){
		var r = sg.open_repo(repInfo.repoName);
		try{
			var history = r.history().items;
			head = history[history.length-head].changeset_id;
			actual = r.merge_review(head, false, 0, true, baselines); // Test the results having fetched the whole thing at once.
			actual2 = [null, head]; // Test chunking by getting results 1 row at a time.
			var iActual = 0;
			var expectedIndentation = 0;
			var expectedDisplayParent = [null];
			for(var iExpected=0; iExpected<expected.length; ++iExpected){
				if(expected[iExpected]=="("){
					++expectedIndentation;
					expectedDisplayParent.push(expectedDisplayParent[expectedDisplayParent.length-1]);
				}
				else if(expected[iExpected]==")"){
					--expectedIndentation;
					expectedDisplayParent.pop();
				}
				else{
					mr.expect("indentation", expectedIndentation, actual[iActual].indent);
					mr.expect("revno", parseInt(expected[iExpected], 10), actual[iActual].revno);
					
					actual2 = r.merge_review(actual2[1], false, 1, true, baselines);
					mr.check("chunk...", sg.to_json(actual[iActual])===sg.to_json(actual2[0]));
					
					if(expectedIndentation>0){
						mr.expect("displayParent", expectedDisplayParent[expectedDisplayParent.length-1], actual[iActual].displayParent);
					}
					expectedDisplayParent[expectedDisplayParent.length-1] = parseInt(expected[iExpected]);
					
					++iActual;
				}
			}
			mr.check("full traversal", iActual===actual.length-1);
		}
		finally{
			r.close();
		}
	}
};
