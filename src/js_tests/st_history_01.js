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

function st_history_01()
{
    this.changesets = new Array();
    this.changesets.push("initial commit");
    this.times = new Array();
    this.times.push(0);
    this.test_history_01 = function() 
    {
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

	// history with no <<rev-spec>> args should implicitly get
	// info from current branch head(s) and/or parents.
	expect_in_history_by_tag( sg.vv2.history(),
				  [ "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );

	// history with branch name should give (branch-head...initial)
	expect_in_history_by_tag( sg.vv2.history( { "branch" : "master" } ),
				  [ "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );

	// history of specific rev/tag should give just it
	expect_in_history_by_tag( sg.vv2.history( { "tag" : "CS_2" } ),
				  [ "CS_2" ] );

	// history of set of rev/tag should give each.
	expect_in_history_by_tag( sg.vv2.history( { "tag" : [ "CS_2", "CS_4" ] } ),
				  [ "CS_2", "CS_4" ] );

	//////////////////////////////////////////////////////////////////
	// update back to CS_3 and see what happens
	// when we give implicit history commands.

	sg.wc.update( { "tag" : "CS_3", "attach" : "master" } );

	// when no <<rev-spec>> given and the  WD is attached
	// to an existing branch, we treat it as an implicit --branch <current_branch>
	// TODO 2012/07/05 This feels weird (I guess I'm thinking that history should
	// TODO            always go backwards from the current WD, rather than the
	// TODO            head of our branch.)
	expect_in_history_by_tag( sg.vv2.history(),
				  [ "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );

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

	// history of specific rev/tag should give just it
	expect_in_history_by_tag( sg.vv2.history( { "tag" : "CS_6" } ),
				  [ "CS_6" ] );

	// history with branch name should give (branch-head...initial)
	// over all heads.
	expect_in_history_by_tag( sg.vv2.history( { "branch" : "master" } ),
				  [ "CS_6", "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );

	// when no <<rev-spec>> given and the WD is attached
	// to an existing branch, we treat it as an implicit --branch <current_branch>
	// TODO 2012/07/05 This feels weird (I guess I'm thinking that history should
	// TODO            always go backwards from the current WD, rather than the
	// TODO            head of our branch.)
	expect_in_history_by_tag( sg.vv2.history(),
				  [ "CS_6", "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );

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

	// history of specific rev/tag should give just it
	expect_in_history_by_tag( sg.vv2.history( { "tag" : "CS_7" } ),
				  [ "CS_7" ] );

	sg.fs.mkdir("test_CS8");
	vscript_test_wc__addremove();
	commitWrapper_detached(this);

	// history of specific rev/tag should give just it
	expect_in_history_by_tag( sg.vv2.history( { "tag" : "CS_8" } ),
				  [ "CS_8" ] );

	// when no <<rev-spec>> and the WD is NOT attached, we treat
	// is as an implicit complete history
	expect_in_history_by_tag( sg.vv2.history(),
				  [ "CS_8", "CS_7", "CS_6", "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );

	// history with branch name should give (branch-head...initial)
	// over all heads.
	expect_in_history_by_tag( sg.vv2.history( { "branch" : "master" } ),
				  [ "CS_6", "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );

	//////////////////////////////////////////////////////////////////
	// attach back to the master branch in anticipation of creating
	// a new cset in master.

	sg.exec(vv, "branch", "attach", "master");

	// when no <<rev-spec>> and the WD is attached (out next commit
	// will be in master, but the current parent isn't),
	// back to implicit --branch *PLUS* parents
	expect_in_history_by_tag( sg.vv2.history(),
				  [ "CS_8", "CS_7", "CS_6", "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );

	//////////////////////////////////////////////////////////////////
	// update back to anon CS_7 while still attached and see what happens.

	sg.wc.update( { "tag": "CS_7", "attach" : "master" } );

	// when no <<rev-spec>> and the WD is attached (out next commit
	// will be in master, but the current parent isn't),
	// back to implicit --branch *PLUS* parents
	expect_in_history_by_tag( sg.vv2.history(),
				  [ "CS_7", "CS_6", "CS_5", "CS_4", "CS_3", "CS_2", "CS_1" ] );

	//////////////////////////////////////////////////////////////////
	// try the fake branch trick described in J8493.

	sg.exec(vv, "branch", "new", "foobar");

	// when no <<rev-spec>> and the WD is attached (out next commit
	// will be in foobar, but the current parent isn't),
	// back to implicit --branch *PLUS* parents
	expect_in_history_by_tag( sg.vv2.history(),
				  [ "CS_7", "CS_2", "CS_1" ] );

	// The answer to the following changed with the fix for X9117.
	//     CS_6 does not appear in the history
	//     between CS_7 and CS_1 (along the path
	//     implied by the implicit "--branch foobar")
	//     so it makes sense that it might not appear
	//     but it took me by surprise).  With the fix
	//     for X9117 it should (and effectively ignore
	//     the implied attachment).
	// (When we detach, we get CS_6.)
	expect_in_history_by_tag( sg.vv2.history( { "tag" : "CS_6" } ),
				  [ "CS_6" ] );
	sg.exec(vv, "branch", "detach");
	expect_in_history_by_tag( sg.vv2.history( { "tag" : "CS_6" } ),
				  [ "CS_6" ] );

	//////////////////////////////////////////////////////////////////
	// With W7543, we can request a full history from a --rev or --tag.
	// These should be completely independent of attachment/detachment.

	expect_in_history_by_tag( sg.vv2.history( { "tag" : "CS_7", "list-all" : true } ),
				  [ "CS_7", "CS_2", "CS_1" ] );

	expect_in_history_by_tag( sg.vv2.history( { "tag" : "CS_6", "list-all" : true } ),
				  [ "CS_6", "CS_3", "CS_2", "CS_1" ] );

	sg.exec(vv, "branch", "new", "foobar");

	expect_in_history_by_tag( sg.vv2.history( { "tag" : "CS_7", "list-all" : true } ),
				  [ "CS_7", "CS_2", "CS_1" ] );

	expect_in_history_by_tag( sg.vv2.history( { "tag" : "CS_6", "list-all" : true } ),
				  [ "CS_6", "CS_3", "CS_2", "CS_1" ] );
    }
}
