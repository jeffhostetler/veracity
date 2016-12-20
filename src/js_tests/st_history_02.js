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
    obj.changesets.push(vscript_test_wc__commit_np( { "message" : "No Message." } ));
    sleepStupidly(1);
    obj.times.push(new Date().getTime());

    sg.vv2.add_tag( { "rev" : obj.changesets[obj.changesets.length - 1],
		      "value" : "CS_" + obj.changesets.length  } );

    sleepStupidly(1);
}
function commitWrapper_detached(obj)
{
    obj.changesets.push(vscript_test_wc__commit_np( { "message" : "No Message.", "detached" : true } ));
    sleepStupidly(1);
    obj.times.push(new Date().getTime());

    sg.vv2.add_tag( { "rev" : obj.changesets[obj.changesets.length - 1],
		      "value" : "CS_" + obj.changesets.length  } );

    sleepStupidly(1);
}

// See if the given history contains *exactly* the set
// of csets expected.
//
// 'history' should be the output of sg.vv2.history()
// 'expect'  should be an array of tags (applied above by commitWrapper()).
//
function expect_in_history_by_tag( history, expect )
{
    for (var e in expect)
    {
	var found_e = false;

	print("E:Looking for tag: " + expect[e]);
	for (var h in history)
	{
	    if (history[h].tags[0] == expect[e])
	    {
		found_e = true;
		print("E:Found tag in history: " + expect[e]);
		break;
	    }
	}

	testlib.ok( (found_e), "E:Looking for tag in history: " + expect[e]);
    }

    for (var h in history)
    {
	var found_h = false;

	print("H:Looking for tag: " + history[h].tags[0]);
	for (var e in expect)
	{
	    if (expect[e] == history[h].tags[0])
	    {
		found_h = true;
		print("H:Found tag in expect: " + history[h].tags[0]);
		break;
	    }
	}

	testlib.ok( (found_h), "H:Looking for tag in expect: " + history[h].tags[0]);
    }
}

function st_history_02()
{
    this.changesets = new Array();
    this.changesets.push("initial commit");
    this.times = new Array();
    this.times.push(0);
    this.test_history_01 = function() 
    {
	print("Repo is: " + repInfo.repoName);

	//////////////////////////////////////////////////////////////////
	// tag the initial commit to keep the pattern
	//
	//     CS_1:m

	sg.vv2.add_tag( { "rev" : "1",
			  "value" : "CS_1" } );

	//////////////////////////////////////////////////////////////////
	// build initial chain on 'master'.
	// 
	//                        CS_1
	//                       /
	//                   CS_2
	//                  /
	//              CS_3
	//             /
	//         CS_4
	//        /
	//    CS_5:m

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

	//////////////////////////////////////////////////////////////////
	// fork the master branch
	// 
	//                        CS_1
	//                       /
	//                   CS_2
	//                  /
	//              CS_3
	//             /    \
	//         CS_4      CS_6:m
	//        /
	//    CS_5:m

	sg.wc.update( { "tag" : "CS_3", "attach" : "master" } );
	sg.fs.mkdir("test_CS6");
	vscript_test_wc__addremove();
	commitWrapper(this);

	//////////////////////////////////////////////////////////////////
	// create a cset as an anonymous leaf
	// 
	//                        CS_1
	//                       /
	//                   CS_2
	//                  /    \
	//              CS_3      CS_7:anon
	//             /    \         \
	//         CS_4      CS_6:m    CS_8:anon
	//        /
	//    CS_5:m

	sg.wc.update( { "tag" : "CS_2", "detached" : true } );
	sg.fs.mkdir("test_CS7");
	vscript_test_wc__addremove();
	commitWrapper_detached(this);

	sg.fs.mkdir("test_CS8");
	vscript_test_wc__addremove();
	commitWrapper_detached(this);

	//////////////////////////////////////////////////////////////////
	// cd out of the WD just to be sure that we are 100% repo-based.

	sg.fs.cd("..");

	//////////////////////////////////////////////////////////////////

	// full history of everything
	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName } ),
				  [ "CS_8", "CS_7", "CS_6", "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );

	//////////////////////////////////////////////////////////////////

	// single-item history using --tag
	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "tag"  : "CS_5" } ),
				  [ "CS_5" ] );

	// list-all on a single-item using --tag
	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "tag"  : "CS_5", "list-all" : true } ),
				  [ "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );


	//////////////////////////////////////////////////////////////////
	// branch

	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "branch" : "master" } ),
				  [ "CS_6", "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );

	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "branch" : "master", "tag" : "CS_5" } ),
				  [ "CS_5" ] );


	// TODO 2012/07/05 Should the following inlude CS_6" ?  Makes sense when you
	// TODO            think about it as returning everything in the branch.
	// TODO            (And unioning it CS_5 which is one of the heads.)
	// TODO            But it feels a little wrong.
	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "branch" : "master", "tag" : "CS_5", "list-all" : true } ),
				  [ "CS_6", "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );


	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "branch" : "master", "tag" : "CS_7" } ),
				  [ ] );

	// TODO 2012/07/05 Again, this gets the union treatment and feels a bit too big.
	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "branch" : "master", "tag" : "CS_7", "list-all" : true } ),
				  [ "CS_7", "CS_6", "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );

	//////////////////////////////////////////////////////////////////
	// anonymous

	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "tag" : "CS_7" } ),
				  [ "CS_7" ] );

	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "tag" : "CS_7", "list-all" : true } ),
				  [ "CS_7", "CS_2", "CS_1" ] );

	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "tag" : "CS_8" } ),
				  [ "CS_8" ] );

	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "tag" : "CS_8", "list-all" : true } ),
				  [ "CS_8", "CS_7", "CS_2", "CS_1" ] );

	//////////////////////////////////////////////////////////////////

	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "tag" : [ "CS_7", "CS_6" ] } ),
				  [ "CS_7", "CS_6" ] );

	expect_in_history_by_tag( sg.vv2.history( { "repo" : repInfo.repoName, "tag" : [ "CS_7", "CS_6" ], "list-all" : true } ),
				  [ "CS_7", "CS_6", "CS_3", "CS_2", "CS_1" ] );

	//////////////////////////////////////////////////////////////////
	// TODO 2012/07/05 Create another branch using one of the existing csets
	// TODO            and then do a history query using '... "branch" : [ "master", "xyz" ], ...'

    }
}
