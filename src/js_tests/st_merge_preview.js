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

function st_merge_preview_basic()
{
	//     
	// 7   
	// |   
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
		var expected = {
			"1<-1": ["no-op",        [] ],
			"1<-2": ["fast-forward", [2] ],
			"1<-3": ["fast-forward", [3] ],
			"1<-4": ["fast-forward", [4, 3] ],
			"1<-5": ["fast-forward", [5, 2] ],
			"1<-6": ["fast-forward", [6, 5, 4, 3, 2] ],
			"1<-7": ["fast-forward", [7, 5, 2] ],
			"2<-1": ["no-op",        [] ],
			"2<-2": ["no-op",        [] ],
			"2<-3": ["regular",      [3] ],
			"2<-4": ["regular",      [4, 3] ],
			"2<-5": ["fast-forward", [5] ],
			"2<-6": ["fast-forward", [6, 5, 4, 3] ],
			"2<-7": ["fast-forward", [7, 5] ],
			"3<-1": ["no-op",        [] ],
			"3<-2": ["regular",      [2] ],
			"3<-3": ["no-op",        [] ],
			"3<-4": ["fast-forward", [4] ],
			"3<-5": ["regular",      [5, 2] ],
			"3<-6": ["fast-forward", [6, 5, 4, 2] ],
			"3<-7": ["regular",      [7, 5, 2] ],
			"4<-1": ["no-op",        [] ],
			"4<-2": ["regular",      [2] ],
			"4<-3": ["no-op",        [] ],
			"4<-4": ["no-op",        [] ],
			"4<-5": ["regular",      [5, 2] ],
			"4<-6": ["fast-forward", [6, 5, 2] ],
			"4<-7": ["regular",      [7, 5, 2] ],
			"5<-1": ["no-op",        [] ],
			"5<-2": ["no-op",        [] ],
			"5<-3": ["regular",      [3] ],
			"5<-4": ["regular",      [4, 3] ],
			"5<-5": ["no-op",        [] ],
			"5<-6": ["fast-forward", [6, 4, 3] ],
			"5<-7": ["fast-forward", [7] ],
			"6<-1": ["no-op",        [] ],
			"6<-2": ["no-op",        [] ],
			"6<-3": ["no-op",        [] ],
			"6<-4": ["no-op",        [] ],
			"6<-5": ["no-op",        [] ],
			"6<-6": ["no-op",        [] ],
			"6<-7": ["regular",      [7] ],
			"7<-1": ["no-op",        [] ],
			"7<-2": ["no-op",        [] ],
			"7<-3": ["regular",      [3] ],
			"7<-4": ["regular",      [4, 3] ],
			"7<-5": ["no-op",        [] ],
			"7<-6": ["regular",      [6, 4, 3] ],
			"7<-7": ["no-op",        [] ]
		};
		
		testlib.log("Here we go...");
		
		//mp.ok(sg.exec("vv", "update", "-r2").exit_status!==0);
		testlib.updateAttached("1", "master");
		
		mp.verifyAll(expected, 1, 1);
		
		sg.file.write("r2-file.txt", "...");
		vscript_test_wc__add("r2-file.txt");
		vscript_test_wc__commit( "Added r2-file.txt with content \"...\".");
		testlib.log("(r2 added)");
		
		mp.verifyAll(expected, 2, 2);
		
		testlib.updateAttached("1", "master");
		
		mp.verifyAll(expected, 2, 1);
		
		sg.file.write("r3-file.txt", "...");
		vscript_test_wc__add("r3-file.txt");
		vscript_test_wc__commit( "Added r3-file.txt with content \"...\".");

		testlib.log("(r3 added)");
		
		mp.verifyAll(expected, 3, 3, 2);
		
		testlib.updateAttached("2", "master");
		
		mp.verifyAll(expected, 3, 2, 3);
		
		testlib.updateAttached("3", "master");
		
		sg.file.write("r4-file.txt", "...");
		vscript_test_wc__add("r4-file.txt");
		vscript_test_wc__commit( "Added r4-file.txt with content \"...\".");
		testlib.log("(r4 added)");
		
		mp.verifyAll(expected, 4, 4, 2);
		
		testlib.updateAttached("2", "master");
		
		mp.verifyAll(expected, 4, 2, 4);
		
		sg.file.write("r5-file.txt", "...");
		vscript_test_wc__add("r5-file.txt");
		vscript_test_wc__commit( "Added r5-file.txt with content \"...\".");
		testlib.log("(r5 added)");
		
		mp.verifyAll(expected, 5, 5, 4);
		
		testlib.updateAttached("4", "master");
		
		mp.verifyAll(expected, 5, 4, 5);
		
		testlib.wcMerge();
		vscript_test_wc__commit( "Merged");
		testlib.log("(r6 added)");
		
		mp.verifyAll(expected, 6, 6);
		
		testlib.updateAttached("5", "master");
		
		sg.file.write("r7-file.txt", "...");
		vscript_test_wc__add("r7-file.txt");
		vscript_test_wc__commit( "Added r7-file.txt with content \"...\".");
		testlib.log("(r7 added)");
		
		mp.verifyAll(expected, 7, 7, 6);
		
		testlib.updateAttached("6", "master");
		
		mp.verifyAll(expected, 7, 6, 7);
		
		testlib.testResult(true);
	};
}

function st_merge_preview_crisscross()
{
	//             
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
		var expected = {
			"1<-1": ["no-op",        [] ],
			"1<-2": ["fast-forward", [2] ],
			"1<-3": ["fast-forward", [3, 2] ],
			"1<-4": ["fast-forward", [4] ],
			"1<-5": ["fast-forward", [5, 4] ],
			"1<-6": ["fast-forward", [6, 5, 4, 2] ],
			"1<-7": ["fast-forward", [7, 4, 3, 2] ],
			"2<-1": ["no-op",        [] ],
			"2<-2": ["no-op",        [] ],
			"2<-3": ["fast-forward", [3] ],
			"2<-4": ["regular",      [4] ],
			"2<-5": ["regular",      [5, 4] ],
			"2<-6": ["fast-forward", [6, 5, 4] ],
			"2<-7": ["fast-forward", [7, 4, 3] ],
			"3<-1": ["no-op",        [] ],
			"3<-2": ["no-op",        [] ],
			"3<-3": ["no-op",        [] ],
			"3<-4": ["regular",      [4] ],
			"3<-5": ["regular",      [5, 4] ],
			"3<-6": ["regular",      [6, 5, 4] ],
			"3<-7": ["fast-forward", [7, 4] ],
			"4<-1": ["no-op",        [] ],
			"4<-2": ["regular",      [2] ],
			"4<-3": ["regular",      [3, 2] ],
			"4<-4": ["no-op",        [] ],
			"4<-5": ["fast-forward", [5] ],
			"4<-6": ["fast-forward", [6, 5, 2] ],
			"4<-7": ["fast-forward", [7, 3, 2] ],
			"5<-1": ["no-op",        [] ],
			"5<-2": ["regular",      [2] ],
			"5<-3": ["regular",      [3, 2] ],
			"5<-4": ["no-op",        [] ],
			"5<-5": ["no-op",        [] ],
			"5<-6": ["fast-forward", [6, 2] ],
			"5<-7": ["regular",      [7, 3, 2] ],
			"6<-1": ["no-op",        [] ],
			"6<-2": ["no-op",        [] ],
			"6<-3": ["regular",      [3] ],
			"6<-4": ["no-op",        [] ],
			"6<-5": ["no-op",        [] ],
			"6<-6": ["no-op",        [] ],
			"6<-7": ["regular",      [7, 3] ],
			"7<-1": ["no-op",        [] ],
			"7<-2": ["no-op",        [] ],
			"7<-3": ["no-op",        [] ],
			"7<-4": ["no-op",        [] ],
			"7<-5": ["regular",      [5] ],
			"7<-6": ["regular",      [6, 5] ],
			"7<-7": ["no-op",        [] ]
		};
		
		testlib.log("Here we go...");
		
		//mp.ok(sg.exec("vv", "update", "-r2").exit_status!==0);
		testlib.updateAttached("1", "master");
		
		sg.file.write("r2-file.txt", "...");
		vscript_test_wc__add("r2-file.txt");
		vscript_test_wc__commit( "Added r2-file.txt with content \"...\".");
		testlib.log("(r2 added)");
		
		sg.file.write("r3-file.txt", "...");
		vscript_test_wc__add("r3-file.txt");
		vscript_test_wc__commit( "Added r3-file.txt with content \"...\".");
		testlib.log("(r3 added)");
		
		testlib.updateAttached("1", "master");
		
		sg.file.write("r4-file.txt", "...");
		vscript_test_wc__add("r4-file.txt");
		vscript_test_wc__commit( "Added r4-file.txt with content \"...\".");
		testlib.log("(r4 added)");
		
		sg.file.write("r5-file.txt", "...");
		vscript_test_wc__add("r5-file.txt");
		vscript_test_wc__commit( "Added r5-file.txt with content \"...\".");
		testlib.log("(r5 added)");
		
		testlib.wcMerge("2");
		vscript_test_wc__commit( "Merged");
		testlib.log("(r6 added)");
		
		testlib.updateAttached("3", "master");
		
		testlib.wcMerge("4");
		vscript_test_wc__commit( "Merged");
		testlib.log("(r7 added)");
		
		mp.verifyAll(expected, 7, 7, 6);
		
		testlib.updateAttached("6", "master");
		
		mp.verifyAll(expected, 7, 6, 7);
		
		testlib.testResult(true);
	};
}

// merge_preview test helpers
var mp = {
	ok: function(ok_param, msg){
		msg = msg || "--";
		if(!ok_param){
			testlib.log("FAILED: "+msg);
			testlib.testResult(false);
		}
		return ok_param;
	},
	eq: function(expected, actual, msg){
		mp.ok(expected==actual, msg+" (expected: '"+expected+"', actual: '"+actual+"')");
	},

	verify: function(type, expected, rev1, rev2){
		// First make sure 'vv merge-preview' returns the expected results.
		var actual;
		if(rev2){
			actual = sg.exec("vv", "merge-preview", "-r"+rev1, "-r"+rev2);
		}
		else if(rev1){
			actual = sg.exec("vv", "merge-preview", "-r"+rev1)
		}
		else{
			actual = sg.exec("vv", "merge-preview")
		}
		
		if(type=="no-op"){
			mp.ok(actual.stdout.indexOf("The baseline already includes the merge target. No merge is needed.")>-1, "no-op message");
		}
		else{
			var count = 0;
			mp.eq(0, actual.exit_status, "exit status");
			lines = actual.stdout.split("\n");
			for(var i=0; i<lines.length; ++i){
				var line = lines[i];
				if(line.slice(0,12)=="\trevision:  "){
					if(count==expected.length){
						throw "too many revs returned by merge-preview";
					}
					mp.eq(expected[count], line.slice(12, line.lastIndexOf(":")), "rev #"+count);
					++count;
				}
			}
			mp.eq(expected.length, count, "count");
			if(type=="fast-forward"){
				mp.ok(actual.stdout.indexOf("Fast-Forward Merge to")>-1, "Fast-Forward Merge to");
			}
			else{
				mp.ok(actual.stdout.indexOf("Merge with")>-1, "Merge with");
			}
			mp.ok(actual.stdout.indexOf("brings in "+expected.length+ " changeset")>-1, "tallyhoo");
		}
		
		// Then, if applicable, make sure merge_preview() jsglue returns the expected results too.
		if(rev2){
			var r = sg.open_repo(repInfo.repoName);
			try{
				var history = r.history().items;
				var csid1 = history[history.length-rev1].changeset_id;
				var csid2 = history[history.length-rev2].changeset_id;
				actual = r.merge_preview(csid1, csid2);
				mp.eq(expected.length, actual.length, "length");
				for(var i=0; i<expected.length && i<actual.length; ++i){
					mp.eq(history[history.length-expected[i]].changeset_id, actual[i].changeset_id, "["+i+"]");
				}
				mp.eq(expected.length, r.count_new_since_common(csid1, csid2), "count_new_since_common()");
			}
			finally{
				r.close();
			}
		}
	},
	
	verifyAll: function(expected, maxRev, implicitBaseline, implicitTarget){
		if(implicitTarget){
			var key = implicitBaseline.toString()+"<-"+implicitTarget.toString();
			testlib.log(key+" (implicit)");
			mp.verify(expected[key][0], expected[key][1])
		}
		for(var target=1; target<=maxRev; ++target){
			var key = implicitBaseline.toString()+"<-"+target.toString();
			testlib.log(key+" (implicit baseline)");
			mp.verify(expected[key][0], expected[key][1], target)
		}
		for(var baseline=1; baseline<=maxRev; ++baseline){
			for(var target=1; target<=maxRev; ++target){
				var key = baseline.toString()+"<-"+target.toString();
				testlib.log(key);
				mp.verify(expected[key][0], expected[key][1], baseline, target);
			}
		}
	}
};

