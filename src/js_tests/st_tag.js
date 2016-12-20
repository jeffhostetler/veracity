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
		sleepStupidly(1);
}

function st_TagMain() {
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

	this.testTagMain = function() {
		//Make sure that there are no tags yet
		var output = testlib.execVV_lines("tag", "list");
		testlib.equal(0, output.length,  "haven't applied any tags yet");
		//Apply one
		var output = testlib.execVV_lines("tag", "add", "first Tag");
		//Make sure that it's there
		var output = testlib.execVV_lines("tag", "list");
		testlib.equal(1, output.length,  "applied one tag");
		testlib.stringContains(output[0], "first Tag",  "applied one tag");
		testlib.stringContains(output[0], this.changesets[this.changesets.length - 1],  "applied one tag");
		//apply a new tag to an old revision
		var output = testlib.execVV_lines("tag", "add", "--rev=" + this.changesets[2], "second Tag");
		//Verify that both tags to applied, and to the correct revisions
		var output = testlib.execVV_lines("tag", "list");
		testlib.equal(2, output.length,  "applied one tag");
		testlib.stringContains(output[0], "first Tag",  "applied one tag");
		testlib.stringContains(output[0], this.changesets[this.changesets.length - 1],  "applied one tag");
		testlib.stringContains(output[1], "second Tag",  "applied two tags");
		testlib.stringContains(output[1], this.changesets[2],  "applied two tags");
		//attempt to apply a tag to a different changeset, verify that it fails.
		var execResultObject = testlib.execVV_expectFailure("tag", "add", "--rev=" + this.changesets[2], "first Tag");
		//manually remove a tag, and apply it to a different changeset
		var output = testlib.execVV_lines("tag", "remove", "first Tag");
		var output = testlib.execVV("tag", "add", "--rev=" + this.changesets[2], "first Tag");
		var output = testlib.execVV_lines("tag", "list");
		testlib.equal(2, output.length,  "applied one tag");
		testlib.stringContains(output[0], "first Tag",  "applied one tag");
		testlib.stringContains(output[0], this.changesets[2],  "applied one tag");
		testlib.stringContains(output[1], "second Tag",  "applied two tags");
		testlib.stringContains(output[1], this.changesets[2],  "applied two tags");
		//Remove all tags, so as not to interfere with other tests
		var output = testlib.execVV_lines("tag", "remove", "first Tag");
		var output = testlib.execVV_lines("tag", "remove", "second Tag");
		var output = testlib.execVV_lines("tag", "list");
		testlib.equal(0, output.length,  "removed all tags");
	}
	this.testTagForce = function() {
		//Make sure that there are no tags yet
		testlib.equal(0, testlib.execVV_lines("tag", "list").length,  "haven't applied any tags yet");
		var output = testlib.execVV_lines("tag", "add", "replaceMe");
		var output = testlib.execVV_lines("tag", "list");
		testlib.equal(1, output.length,  "applied one tag");
		//Try to reapply the same tag to the current changeset, assert that the CLC complains
		var outpu = testlib.execVV_expectFailure("tag", "add", "replaceMe");
		//Try to remove a tag that doesn't exist.  This is a failure.
		var outpu = testlib.execVV_expectFailure("tag", "remove", "not there");
		//Use --force to reapply a tag
		var output = testlib.execVV_lines("tag", "move", "--rev=" + this.changesets[3], "replaceMe");
		var output = testlib.execVV_lines("tag", "list");
		testlib.equal(1, output.length,  "applied one tag");
		testlib.stringContains(output[0], "replaceMe",  "applied one tag");
		testlib.stringContains(output[0], this.changesets[3],  "applied one tag");
		var output = testlib.execVV_lines("tag", "remove", "replaceMe");
		testlib.equal(0, testlib.execVV_lines("tag", "list").length,  "removed all tags");
	}
	this.testTagErrors = function() {
		//Make sure that there are no tags yet
		testlib.equal(0, testlib.execVV_lines("tag", "list").length,  "haven't applied any tags yet");
		var output = testlib.execVV_expectFailure("tag", "add", "--rev=blah", "should fail");
		var output = testlib.execVV_expectFailure("tag", "add", "--tag=blah", "should fail");
		var output = testlib.execVV("tag", "add", "tryToReapply");

		var output = testlib.execVV_expectFailure("tag", "add", "--rev=" + this.changesets[1], "tryToReapply");
		var output = testlib.execVV_lines("tag", "remove", "tryToReapply");
		testlib.equal(0, testlib.execVV_lines("tag", "list").length,  "removed all tags");
	}

	this.testTagMainJS = function() {
		//Make sure that there are no tags yet
		var output = sg.vv2.tags();
		testlib.equal(0, output.length,  "haven't applied any tags yet");
		//Apply one
	    sg.vv2.add_tag( { "value" : "first Tag" } );

		//Make sure that it's there
		var output = sg.vv2.tags();
		testlib.equal(1, output.length,  "applied one tag");
		testlib.equal(output[0].tag, "first Tag",  "applied one tag");
		testlib.equal(output[0].csid, this.changesets[this.changesets.length - 1],  "applied one tag");
		//apply a new tag to an old revision
	    sg.vv2.add_tag( { "rev" : this.changesets[2], "value" : "second Tag" } );

		//Verify that both tags to applied, and to the correct revisions
		var output = sg.vv2.tags();
		testlib.equal(2, output.length,  "applied one tag");
		testlib.equal(output[0].tag, "first Tag",  "applied one tag");
		testlib.equal(output[0].csid, this.changesets[this.changesets.length - 1],  "applied one tag");
		testlib.equal(output[1].tag, "second Tag",  "applied two tags");
		testlib.equal(output[1].csid, this.changesets[2],  "applied two tags");

		//attempt to apply a tag to a different changeset, verify that it fails.
	    var execResultObject = testlib.verifyFailure( "sg.vv2.add_tag( { \"rev\" : "+this.changesets[2]+", \"value\" : \"first Tag\" } )" );

		//manually remove a tag, and apply it to a different changeset
	    sg.vv2.remove_tag( { "value" : "first Tag" } );
	    sg.vv2.add_tag( { "rev" : this.changesets[2], "value" : "first Tag" } );

		var output = sg.vv2.tags();
		testlib.equal(2, output.length,  "applied one tag");
		testlib.equal(output[0].tag, "first Tag",  "applied one tag");
		testlib.equal(output[0].csid, this.changesets[2],  "applied one tag");
		testlib.equal(output[1].tag, "second Tag",  "applied two tags");
		testlib.equal(output[1].csid, this.changesets[2],  "applied two tags");

		//Remove all tags, so as not to interfere with other tests
	    sg.vv2.remove_tag( { "value" : "first Tag" } );
	    sg.vv2.remove_tag( { "value" : "second Tag" } );

		var output = sg.vv2.tags();
		testlib.equal(0, output.length,  "removed all tags");
	}
	this.testTagForceJS = function() {
		//Make sure that there are no tags yet
		testlib.equal(0, sg.vv2.tags().length,  "haven't applied any tags yet");
	    sg.vv2.add_tag( { "value" : "replaceMe" } );
		var output = sg.vv2.tags();
		testlib.equal(1, output.length,  "applied one tag");

		//Try to reapply the same tag to the current changeset, assert that we complain
	    var output = testlib.verifyFailure( "sg.vv2.add_tag( { \"value\" : \"replaceMe\" } )" );

		//Try to remove a tag that doesn't exist.
	    var output = testlib.verifyFailure( "sg.vv2.remove_tag( { \"value\" : \"not there\" } )" );

		//Use --force to reapply a tag
	    sg.vv2.add_tag( { "rev" : this.changesets[3], "force" : true, "value" : "replaceMe" } );

		var output = sg.vv2.tags();
		testlib.equal(1, output.length,  "applied one tag");
		testlib.equal(output[0].tag, "replaceMe",  "applied one tag");
		testlib.equal(output[0].csid, this.changesets[3],  "applied one tag");

		//Remove all tags, so as not to interfere with other tests
	    sg.vv2.remove_tag( { "value" : "replaceMe" } );

		testlib.equal(0, sg.vv2.tags().length,  "removed all tags");
	}

	this.testTagValidity = function () {
		var invalidChars = "#%/?`~!$^&*()=[]{}\\|;:'\"<>";
		var minLength = 1;
		var maxLength = 256;
		var lengthInterval = 5; // ensures that maxLength is hit exactly
		var invalidCount = 5;
		var changeset = this.changesets[this.changesets.length - 1];

		// try making tags with various invalid characters
		for (var i = 0; i < invalidChars.length; ++i) {
			var currentChar = invalidChars[i];
			if (currentChar == "\\") {
				currentChar = "\\\\";
			}

		    var sz = "abc" + currentChar + "123";
		    var output = testlib.verifyFailure(  "sg.vv2.add_tag( { \"rev\" : "+changeset+", \"value\" : "+sz+" } )"  );
		    var sz = currentChar + "abc123";
		    var output = testlib.verifyFailure(  "sg.vv2.add_tag( { \"rev\" : "+changeset+", \"value\" : "+sz+" } )"  );
		    var sz = "abc123" + currentChar;
		    var output = testlib.verifyFailure(  "sg.vv2.add_tag( { \"rev\" : "+changeset+", \"value\" : "+sz+" } )"  );
		}

		// try making tags of various valid lengths
		var tagCount = sg.vv2.tags().length;
		for (var i = minLength; i <= maxLength; i = i + lengthInterval) {
			var currentName = new Array(i + 1).join("a");
		    sg.vv2.add_tag( { "rev" : changeset, "value" : currentName } );

			testlib.equal(tagCount + 1, sg.vv2.tags().length,  "tag applied: " + currentName);
		    sg.vv2.remove_tag( { "value" : currentName } );

			testlib.equal(tagCount, sg.vv2.tags().length,  "tag removed: " + currentName);
		}

		// try making tags of various invalid lengths
	    var sz = "";
	    var output = testlib.verifyFailure(  "sg.vv2.add_tag( { \"rev\" : "+changeset+", \"value\" : "+sz+" } )" );

		for (var i = maxLength + 1; i <= maxLength + 1 + invalidCount; ++i) {
			var currentName = new Array(i + 1).join("a");

		    var sz = currentName;
		    var output = testlib.verifyFailure( "sg.vv2.add_tag( { \"rev\" : "+changeset+", \"value\" : "+sz+" } )" );
		}
	}
}
