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

load("../js_test_lib/vscript_test_wc_lib.js");
load("../js_test_lib/vscript_test_lib.js");

function loadDefaultFiles() {
    // Load all tests (st*.js)
    var scriptname = "ut_lister";
    if (sg.platform() == "WINDOWS") {
        scriptname = scriptname + ".bat";
        sg.file.write(scriptname, "dir /b st*.js >dir.txt\n");
        sg.exec("cmd", "/C", scriptname);
    } else {
        scriptname = scriptname + ".sh";
        sg.file.write(scriptname, 'ls st*.js >dir.txt\n');
        sg.exec("/bin/sh", scriptname);
    }
    var filetext = sg.file.read("dir.txt").replace(/\r/g, "");
    var files = filetext.split("\n");
    for (var i=0; i<files.length-1; i++) {
        print("Loading: " + files[i]);
        load(files[i]);
    }
    sg.fs.remove(scriptname);
    sg.fs.remove("dir.txt");
}

/***************************************************************************************************************************
*  To write new tests:
*
*       If you want to write a new set of Veracity unit tests, the test unit function should start with 'st'
*       Unit tests within that set of tests should start with 'test', for example:
*
*       function stNewTests()
*       {*
            this.setUp = function () {}  //optional
*           this.tearDown = function() {}
*
*           this.testTest1 = function(){
*
*            }
*           this.testTest2 = function(){
*
*            }
*           this.tearDown = function () {}  //optional
*        }
*
*       You can also define a setUp and tearDown within your test function that will be called at the beginning and
*       end of each test.
*
*       If you create a new .js file for your tests, make sure to load that file at the top of this page.
*
*       load("stNewTests.js");
*
*****************************************************************************************************************************/

var testlib = new TestLib();
var summary = new Summary();
var testTotal;
var testCnt = 0;
var tests = [];
var allTests = [];
var groupNames = [];
var allGroupNames = [];
var q = [];
var repInfo;
var tempDir = pathCombine(sg.fs.getcwd(),"temp");
var srcDir = sg.fs.getcwd();

// handle some command line options before we load the tests
if (arguments.length > 1) {
    for (var i = 0; i < arguments.length; i++) {
        var arg = arguments[i];
        if (arg.indexOf("-") == 0) {
            switch (arg) {
                case "--load":
                    {
                        arguments.splice(i, 1);
                        while (arguments[i]) {
                            if (arguments[i].indexOf("-") == 0) {
                                break;
                            } else {
                                if (sg.fs.exists(arguments[i])) {
                                    load(arguments[i]);
                                    print("File loaded: " + arguments[i]);
                                } else {
                                    print("File not found: " + arguments[i]);
                                    quit(1);
                                }
                                var specialFilesLoaded = true;
                                arguments.splice(i, 1);
                                break;
                            }
                        }
                        --i;
                        break;
                    }
                case "--temp":
                    {
                        arguments.splice(i, 1);
                        if (arguments[i].indexOf("-") != 0) {
                            tempDir = arguments[i].replace(/\\/g,"/");
                            arguments.splice(i, 1);
                            print("Using temp directory: " + tempDir);
                        }
                        --i;
                        break;
                    }
                case "-D":
                    {
                        arguments.splice(i, 1);
                        if (arguments[i].indexOf("-") != 0) {
                            var define_var = arguments[i];
                            arguments.splice(i, 1);

                            var pair = define_var.split("=");
                            if (pair.length == 2)
                            {
                                testlib.defined[pair[0]] = pair[1];
                                print("Defined: testlib.defined." + pair[0] + "=" + pair[1] + " (from the command line)");
                            }
                            else
                            {
                                testlib.defined[define_var] = true;
                                print("Defined: testlib.defined." + define_var + "=true (from the command line)");
                            }
                        }
                        --i;
                        break;
                    }
            }
        }
    }
}

if (!specialFilesLoaded) {
    loadDefaultFiles();
}

loadAllTests();

if (arguments.length == 0) {

    runAllTests();
}

else {
    parseCommandLine(arguments);

}


if (testlib.numTestFails > 0) {

    quit(1);

}

function parseCommandLine(arguments) {
    //var tests = new SmokeTests();
    if (arguments.length == 1) {
        if (arguments[0].indexOf("-") == 0) {
            switch (arguments[0]) {
                case "-f":
                case "-F":
                    {
                        testlib.failuresOnly = true;
                        runAllTests();
                        break;
                    }

                case "-l":
                case "-L":
                case "--list":
                    {
                        listTests();
                        //arguments.shift();
                        return;
                    }

                case "-v":
                case "-V":
                    {
                        testlib.verbose = true;
                        runAllTests();
                        break;
                    }

                case "-h":
                case "-H":
                    {
                        usage();
                        return;
                    }

                default:
                    {
                        print("Usage\n");
                        usage();
                        return;
                    }
            }
        }
        else {
            print("Usage\n");
            usage();
        }

    }
    else
    {
        for (var i = 0; i < arguments.length; i++) {

            var arg = arguments[i];

            if (arg.indexOf("-") == 0) {

                switch (arg) {
                    case "-f":
                    case "-F":
                        {
                            testlib.failuresOnly = true;
                            break;
                        }

                    case "-v":
                    case "-V":
                        {
                            testlib.verbose = true;
                            break;
                        }

                    case "-g":
                    case "-G":
                        {
                            while (arguments[++i]) {
                                if (arguments[i].indexOf("-") == 0) {
                                    i--;
                                    break;
                                }
                                else {
                                    addTestGroup(arguments[i]);
                                }
                            }
                            break;
                        }

                    case "-t":
                    case "-T":
                        {
                            while (arguments[++i]) {
                                if (arguments[i].indexOf("-") == 0) {
                                    i--;
                                    break;
                                }
                                else {
                                    //print("arguments[i] in t is: " + arguments[i]);
                                    //print(arguments[i]);
                                    addTestToQ(arguments[i]);
                                }
                            }
                            break;
                        }

                    case "-o":
                    case "-O":
                        {
                            while (arguments[++i]) {
                                if (arguments[i].indexOf("-") == 0) {
                                    i--;
                                    break;
                                }
                                else {
                                    //print("arguments[i] in t is: " + arguments[i]);
                                    //print(arguments[i]);
                                    testlib.outputFile = arguments[i].replace(/\\/g,"/");
                                    print(testlib.outputFile);
                                    if (summary.outputFile == "") {
                                        summary.outputFile = pathCombine(getParentPath(arguments[i].replace(/\\/g,"/")), "vscript_tests_summary.txt");
                                        print(summary.outputFile);
                                    }
                                }
                            }
                            break;
                        }

                    case "--summaryFile":
                        {
                            while (arguments[++i]) {
                                if (arguments[i].indexOf("-") == 0) {
                                    i--;
                                    break;
                                }
                                else {
                                    var sumPath = getParentPath(arguments[i].replace(/\\/g,"/"));
                                    var sumFile = getFileNameFromPath(arguments[i].replace(/\\/g,"/"));
                                    // check for relative path
                                    if (sumPath == "" || (sumPath.substr(0,1)!="/" && sumPath.substr(1,1)!=":")) {
                                        sumPath = pathCombine(sg.fs.getcwd(), sumPath);
                                    }
                                    summary.outputFile = pathCombine(sumPath, sumFile);
                                    print("Summary log will be sent to: " + summary.outputFile);
                                }
                            }
                            break;
                        }

                    default:
                        {
                            print("Usage\n");
                            usage();
                            break;
                        }
                }
            }
            else {
                print("Usage\n");
                usage();
            }
        }

    }

    if (tests.length > 0) {
        if (q.length > 0)
            testTotal = q.length;
        else
            testTotal = countTests(tests);
        for (var i = 0; i < tests.length; i++) {
            if (q.length > 0) {
                testlib.runTests(tests[i], groupNames[i], q);

            }
            else
                testlib.runTests(tests[i], groupNames[i]);
        }
    }
    else if (q.length > 0) {
        testTotal = q.length;
        testlib.runTests(tests[0], groupNames[0], q);
    }
    //the only case where this will happen is
    //if there is a -o specified...i think
    //*OR* a combination of -f -v -o --summaryFile
    else if ((arguments.length > 1) && (summary.outputFile || testlib.outputFile || testlib.failuresOnly || testlib.verbose)) {
        runAllTests();
    }
    summary.wrapup();
}

function runAllTests() {

    testTotal = countTests(allTests);
    for (var i = 0; i < allTests.length; i++) {
        //print(dumpObj(allTests[i]));
        testlib.runTests(allTests[i], allGroupNames[i]);
    }
}

function usage() {
    print("To run all tests: \n"
	  + "vscript vscript_tests.js [-f (show failures only)]\n"
	  + "\n"
	  + "To run specific tests:\n"
	  + "vscript vscript_tests.js [-f (show failures only)] [-g group_name] [-t tests_name]\n"
	  + "\n"
	  + "Additional options:\n"
	  + "    [-l, --list] list tests\n"
	  + "    [--load testFile(s).js (overrides the default set)]\n"
	  + "    [--temp path_for_test_repos]\n"
	  + "    [--summaryFile summary_filePath]\n"
	  + "    [-o full_output_filePath]\n"
	  + "    [-D VARNAME] define variable: testlib.defined.VARNAME=true\n"
	  + "    [-v] verbose echoing of many commands\n"
	  + "    [-h] help"
	 );
}

function loadAllTests() {

    for (method in this) {
        if (method.substring(0, 3) == 'st_') {
            allGroupNames[allTests.length] = method;
            allTests[allTests.length] = new this[method]();
        }
    }

}
function addTestGroup(group) {

    for (method in this) {
        if (group.toLowerCase() == method.toLowerCase()) {
            //print(group);
            groupNames[tests.length] = method;
            tests[tests.length] = new this[method]();
        }
    }

    /*if (tests.length <= 0)
    tests = allTests;*/
}

function addTestToQ(t) {

    var f = testExists(t);
    if (f[0]) {

        //if the test group isn't included yet, include it
        if (tests.indexOf(f[1]) < 0) {

            groupNames[tests.length] = f[2];
            tests[tests.length] = f[1];
        }

        q[q.length] = t;
        // print(arguments[i]);
    }
    else {

        print(t + " is not a valid test");
    }



}

function testExists(test) {

    var method;
    for (var i = 0; i < allTests.length; i++) {
       // print (dumpObj(allTests[i]));
        for (method in allTests[i]) {
            if (method.substring(0, 4) == 'test') {

                if (test.toLowerCase() == method.toLowerCase()) {
                    return [true, allTests[i], allGroupNames[i]];
                }
            }
        }
    }
    return [false, null];
}

function countTests(groups) {
    var ttl = 0;
    var method;
    var methodMaxLen = 0;
    for (var i = 0; i < groups.length; i++) {
        for (method in groups[i]) {
            if (method.length > methodMaxLen)
                methodMaxLen = method.length;
            if (method.substring(0, 4) == 'test')
                ttl++;
        }
    }
    summary.setMetrics(ttl, methodMaxLen);
    return ttl;
}

function listTests() {

    var st;
    var method;

    /*for (var i = 0; i < allTests.length; i++) {
        print(getTestName(allTests[i]));
        for (method in allTests[i]) {
            if (method.substring(0, 4) == 'test') {

                indentLine(dumpObj(method));
            }
        }
    }*/
    for (st in this) {
        if (st.substring(0, 2) == 'st') {
            var foo = new this[st]();
            print(dumpObj(st));

            for (method in foo) {
                if (method.substring(0, 4) == 'test') {
                    indentLine(dumpObj(method));
                }
            }
        }
    }
}



function TestLib() {
    this.numTestPasses = 0;
    this.numTestFails = 0;
    this.numUnitPasses = 0;
    this.numUnitFails = 0;
    this.failuresOnly = false;
    this.verbose = false;    // used by some helper functions
    this.queueResults = [];
    this.shouldFail = false;
    this.outputFile = null;
    this.failedTests = [];
    this.defined = new Array;

    this.setUp = function(grpname) {

	// TODO 2010/10/26 We should change this function to not always create
	// TODO            a full REPO with WD when starting a test.  Yes, it
	// TODO            is nice to have it available when the actual test
	// TODO            starts, but we only need these for VC-based tests.
	// TODO            All of the zing-based tests create their own WD-less
	// TODO            REPO.

		/* server/files is set by the calling batch/bash script. This is how it needs to be, so that
		 * this currently running JS context has the correct modules initialized. If we're setting
		 * server/files here, we're Doing It Wrong. */

        sg.set_local_setting("log/path", tempDir);

        
        repInfo = createNewRepo(grpname);
        
        var rmsg = "Test Repo Folder is: " + repInfo.folderName + "\n";
        print(rmsg);
        if (this.outputFile)
            this.logToFullOutput(rmsg);

		sg.set_local_setting("server/cache", pathCombine(tempDir, "vvcache"));
		sg.set_local_setting("server/session_storage", pathCombine(tempDir, "vvsession"));
		sg.set_local_setting("log/level", "verbose");

        repInfo.userName = "vscript_tests@buildsystem";

		sg.vv2.whoami({"repo":repInfo.repoName, "username":repInfo.userName, "create":true});
    };

    this.tearDown = function() {



    };
    this.testResult = function(pass, msg, expected, actual) {

        //result.failOnly = this.failuresOnly;
        if (pass)
            this.numTestPasses++;
        else
            this.numTestFails++;

        if (this.failuresOnly) {
            this.printResult(!! pass, msg || null, expected, actual, !pass);
        }
        else
            this.printResult(!! pass, msg || null, expected, actual, true);

        return !! pass;
    };

    this.timestamp = function() {
        var date = new Date();
        var YYYY = date.getFullYear();
        var MM = ("00" + (date.getMonth() + 1)).substr(-2);
        var DD = ("00" + date.getDate()).substr(-2);
        var hh = ("00" + date.getHours()).substr(-2);
        var mm = ("00" + date.getMinutes()).substr(-2);
        var ss = ("00" + date.getSeconds()).substr(-2);
        var mss = ("00" + date.getMilliseconds()).substr(-3);
        var uofs = (date.getTimezoneOffset() ? "-" : "+") + ("0000" + (date.getTimezoneOffset() / 60 * 100 + date.getTimezoneOffset() % 60)).substr(-4);
        var tstamp = "[" + YYYY + "/" + MM + "/" + DD + " " + hh + ":" + mm + ":" + ss + "." + mss + " " + uofs + "]";
        return tstamp;
    };

    this.linestamp = function() {
        var stack = new Error().stack.split('\n');
        for (var n = stack.length - 2; n >= 0; n--) {
            if (stack[n].indexOf("vscript_test") < 0)
                break;
        }
        return (n == 0 ? "" : stack[n]);
    };

    this.printResult = function(pass, msg, expected, actual, printResult) {
        var output = indentLine(this.timestamp(), true) + " " + this.linestamp() + "\n";
        if (msg) {
            output += indentLine(msg, true);
            output += "\n";
        }
        output += indentLine("result: " + (pass ? "PASSED" : "FAILED"), true);
        output += "\n";
        if (typeof(expected) != 'undefined') {
            output += indentLine("expected: " + expected, true);
            output += "\n";
        }
        if (typeof(actual) != 'undefined') {
            output += indentLine("actual: " + actual, true);
            output += "\n";
        }

		// Add stack trace for failures
		if (!pass)
		{
			try { i.dont.exist++; }
			catch (er)
			{
				var stackLines = er.stack.split('\n');
				for (var i = 3; i < stackLines.length; i++)
					output += "\t\t" + stackLines[i] + '\n';
			}
		}

		if (printResult) {
			print(output + "\n");
		}
		if (this.outputFile) {
			this.logToFullOutput(output);
		}
    };
    this.runTests = function(rtests, grpname, args) {

		//Now we check the temp dir to see if it is part of a working copy
		currentParent = tempDir;
		while (currentParent.length > 3 )
		{
			if (sg.fs.exists(currentParent + "/.sgdrawer"))
				throw "We can't run the unit tests inside a Veracity working copy. " + currentParent + "/.sgdrawer";
			currentParent = currentParent.substring(0, currentParent.lastIndexOf("/"));
		}
        var output;
        //this.testQueue = args;
        if (rtests["no_setup"])
        {
        }
        else
        {
            this.setUp(grpname);
        }

        try
        {
			if (rtests["setUp"]) {
				rtests["setUp"]();
			}
			if (args) {
				for (var i = 0; i < args.length; i++) {
					for (method in rtests) {
						if (method.toLowerCase() == args[i].toLowerCase()) {
							testCnt++;
							output = testCnt + "/" + testTotal + " " + grpname + "." + method;
							print(output);
							summary.setInfo(testCnt, grpname, method);
							if (this.outputFile)
								this.logToFullOutput(output);

							this.execute(rtests, method, grpname);
						}
					}
				}

			}
			else {
				var method;

				for (method in rtests) {
					// print(dumpObj(method));
					if (method.substring(0, 4) == 'test') {
						//print(getTestName(tests));
						var t = dumpObj(method);
						testCnt++;
						output = testCnt + "/" + testTotal + " " + grpname + "." + t;
						print(output);
						summary.setInfo(testCnt, grpname, t);
						if (this.outputFile)
							this.logToFullOutput(output);

						this.execute(rtests, method, grpname);

					}
				}
			}
		}
		finally
		{
			if (rtests["tearDown"]) {
				rtests["tearDown"]();
			}
			this.testDone();
			this.tearDown();
		}
    };



    this.testDone = function() {

        this.appendSummary();
    };

    this.appendSummary = function() {
        var outmsg;
        outmsg = "\n--TEST RESULTS--\n";
        outmsg += "Tests passed: " + this.numTestPasses + "\n";
        outmsg += "Tests failed: " + this.numTestFails + "\n";
        outmsg += "\n\n--UNIT RESULTS--\n";
        outmsg += "Units passed: " + this.numUnitPasses + "\n";
        outmsg += "Units failed: " + this.numUnitFails;
        if (this.failedTests.length > 0) {
            outmsg += "\n\n--FAILED UNITS--\n";
            outmsg += this.failedTests.join("\n");
        }
        print(outmsg);
        if (this.outputFile) {
            this.logToFullOutput(outmsg);
        }
    };

    //pass failTest args surrounded by brackets
    this.verifyFailure = function(failTestString, msg, expectedError) {
        var failed = false;
        if (this.verbose && !this.failuresOnly) {
            indentLine("=> " + failTestString);
        }
        try {
            //this.shouldFail = true;
            eval(failTestString);

        }
        catch (err) {
            //this.log(err);
            failed = true;
            if (expectedError) {
                this.ok(err.toString().indexOf(expectedError) >= 0, "Error message should contain expected string", expectedError, err);

            }
        }
        // this.shouldFail = false;
        return( this.ok(!! failed, msg) );
    };

    this.execute = function(rtests, method, grpname) {
        var fails = this.numTestFails;
        var tstart;
        var tstop;

        tstart = sg.time();

        try {

            rtests[method]();

        }
        catch (er) {

            if (!this.shouldFail) 
            {
                this.numTestFails++;
                var o = this.timestamp() + '\n';

                o += "result: FAILED with exception: " + er + '\n\n';

				if (er && er.stack)
				{
					var stackLines = er.stack.split('\n');
					for (var i = 0; i < stackLines.length; i++)
						o += "\t\t" + stackLines[i] + '\n';
				}
                
                if (this.outputFile) {
                    this.logToFullOutput(o);
                }
                this.log(o);
            }
        }
        tstop = sg.time();
        summary.postResult((this.numTestFails > fails), tstop - tstart);
        if (this.numTestFails > fails) {
            this.failedTests.push(testCnt + " - " + grpname + "." + method);
            this.numUnitFails++;
        } else {
            this.numUnitPasses++;
        }
    };
    this.execVV = function()
    {
	return this.execVV_private(arguments);
    };
    //Executes the command line client, expecting success.
    //Returns the output of the CLC, split on lines.
    this.execVV_lines = function()
    {
	var lines = this.execVV_private(arguments).stdout.split("\n");
	if (lines[lines.length - 1] == "")
		lines.splice(lines.length - 1, 1); //Remove the last, empty array object
	return lines;
    };
    this.execVV_expectFailure = function()
    {
	return this.execVV_private(arguments, true);
    };
    this.execVV_private = function(arrayOfArgs, failureIsExpected)
    {
	var args = Array.prototype.slice.call(arrayOfArgs);
	//put the command line client name at the beginning of the array.
	args.unshift("vv");
	return this.execCommand(args, failureIsExpected);
    };
    this.execCommand = function(args, failureIsExpected)
    {
        args = "\"" + args.join('", "') + "\"";
        print("executing: " + args);
        execResults = eval("sg.exec(" + args + ")");
	if (failureIsExpected == null || failureIsExpected == false)
		this.assertSuccessfulExec(execResults);
	else
		this.assertFailedExec(execResults);
	//Replace all windows line endings
	execResults.stdout = execResults.stdout.replace(/\r/g, "");
	execResults.stderr = execResults.stderr.replace(/\r/g, "");
        return execResults;
    };

	this.addTag = function(revString, tagString)
	{
		sg.vv2.add_tag( { "rev" : revString,
		      "value" : tagString  } );
	}
	
	this.addStamp = function(revString, stampString)
	{
		sg.vv2.add_stamp( { "rev" : revString,
		      "stamp" : stampString  } );
	}
	
	this.updateDetached = function(revString)
	{
		sg.wc.update( { "detached" : true, "rev" : revString } );
	}
	
	this.updateAttached = function(revString, attachString)
	{
		sg.wc.update( { "rev" : revString,
		      "attach" : attachString  } );
	}
	
	this.updateBranch = function(branchString)
	{
		sg.wc.update( { "branch" : branchString } );
	}
	
	this.addInitialTagAndStamp = function addInitialTagAndStamp()
	{
		testlib.addTag("1", "changeset_1" );
  		testlib.addStamp("1", "odd");
	}
	
	this.wcAdd = function(fileOrArrayOfFiles)
	{
		sg.wc.add({ "src" : fileOrArrayOfFiles});
	}

	this.wcMerge = function(revString, branchString)
	{
		if (revString != null)
			sg.wc.merge({ "rev" : revString });
		else if (branchString != null)
			sg.wc.merge({ "branch" : branchString });
		else
			sg.wc.merge({});
	}

	
	this.wcCommit= function(fileOrArrayOfFiles, message)
	{
		if (fileOrArrayOfFiles != null)
			return sg.wc.commit({ "src" : fileOrArrayOfFiles, "message" : message });
		else
			return sg.wc.commit({ "message" : message });
	}

	this.wcCommitDetached= function(fileOrArrayOfFiles, message)
	{
		if (fileOrArrayOfFiles != null)
			return sg.wc.commit({ "detached" : true, "src" : fileOrArrayOfFiles, "message" : message });
		else
			return sg.wc.commit({ "detached" : true, "message" : message });
	}
	
    //
    //the following functions can be used by the tests for verification and logging
    //
    this.ok = function(pass, msg, expected, actual) {
	if (arguments.length > 2)
	    return(this.testResult(pass, msg, expected, actual));
	else
	    return(this.testResult(pass, msg));
    };

    this.equal = function(expected, actual, msg) {
        var pass = (expected == actual);
        return(this.ok(pass, msg, expected, actual));
    };

	this.fail = function(msg, expected, actual) {
		return( this.ok(false, msg, expected, actual) );
	};

    this.notEqual = function(expected, actual, msg) {
        var pass = (expected != actual);
        return( this.ok(pass, msg, "!= " + expected, actual) );

    };

    this.stringContains = function(fullString, searchString, msg) {
        var pass = fullString.indexOf(searchString) != -1;
        return(this.ok(pass, msg, searchString, fullString));
    };
    this.isNull = function(val, msg) {
        pass = (val == null);
        return( this.ok(pass, msg) );
    };

    this.isNotNull = function(val, msg) {
        var pass = (val != null);
        return( this.ok(pass, msg) );
    };

    this.existsOnDisk = function(path) {
        return( testlib.ok(sg.fs.exists(path), path + " should exist on disk") );
    };

    this.notOnDisk = function(path) {
        return( testlib.ok(!sg.fs.exists(path), path + " should not exist on disk") );
    };

    this.inRepo = function(path) {
        return( this.ok(verifyInRepo(repInfo.repoName, path), path + " should be in repo") );
    };

    this.assertSuccessfulExec = function(returnObj) {
	if (returnObj.exit_status == -1)
		this.ok( (0), "Exec'd program CRASHED." );
	if (returnObj.existStatus != 0)
		return( this.ok(returnObj.exit_status == 0, "Testing to see if exec failed...  \nexit_status: " + returnObj.exit_status + "\nstderr:\n" + returnObj.stderr + "\nstdout:\n" + returnObj.stdout) );
    };

    this.assertFailedExec = function(returnObj) {
	if (returnObj.exit_status == -1)
		this.ok( (0), "Exec'd program CRASHED." );
	return( this.ok(returnObj.exit_status > 0, "Testing to see if exec succeeded when it shouldn't have.  \nexit_status: " + returnObj.exit_status + "\nstderr:\n" + returnObj.stderr + "\nstdout:\n" + returnObj.stdout) );
    };

    this.allInRepo = function(fsObjs) {
        var failures = [];
        for (var i = 0; i < fsObjs.length; i++) {
            if (!verifyInRepo(repInfo.repoName, fsObjs[i])) {
                failures.push(fsObjs[i]);
            }
        }
        return( this.ok(failures.length == 0, ("Should be in the repo:\n" + (failures ? failures.join("\n") : "all OK")) ) );
    };

    this.noneInRepo = function(fsObjs) {
        var failures = [];
        for (var i = 0; i < fsObjs.length; i++) {
            if (verifyInRepo(repInfo.repoName, fsObjs[i])) {
                failures.push(fsObjs[i]);
            }
        }
        return( this.ok(failures.length == 0, ("Should not be in the repo:\n" + (failures ? failures.join("\n") : "all OK"))) );
    };

    this.notInRepo = function(path) {
        return( this.ok((!verifyInRepo(repInfo.repoName, path)), path + " should not be in repo") );
    };

    this.log = function(msg) {
        var output = indentLine(msg);
        if (this.outputFile)
            this.logToFullOutput(output);
    };

    this.logToFullOutput = function(line) {

        if (!sg.fs.exists(this.outputFile)) {
            sg.file.write(this.outputFile, line + "\n");
        }
        else
            sg.file.append(this.outputFile, line + "\n");
    };

};

function Summary() {
//    this.outputFile = pathCombine(sg.fs.getcwd(),"vscript_tests_summary.txt");
    this.outputFile = "";
    var testNumber;
    var testTotal;
    var leftColWid;
    var leftColPad;
    var testColWid;
    var midColWid;
    var midColPad;
    var timeColWid;
    var timeColPad;
    var groupPad;
    var groupName = "";
    var testName;
    var totalTime = 0;
    var numUnitFails = 0;
    var numUnitPasses = 0;
    var failedGroupName = "";
    var failedTests = [];

    this.setMetrics = function(testTot, testMaxLen) {
        if (this.outputFile != "") {
            //if (sg.fs.exists(this.outputFile))
            //    sg.fs.remove(this.outputFile);
            this.logToSummary("Test directory " + sg.fs.getcwd());
            testTotal = testTot;
            leftColWid = (testTotal + "").length * 2 + 2;
            leftColPad = ("          ").substr(0, leftColWid + 1);
            testColWid = (testTotal + "").length + 1;
            midColWid = ((testMaxLen < 34) ? 35 : testMaxLen + 1);
            midColPad = " ........................................................";
            timeColWid = 8;
            timeColPad = "        ";
            groupPad = leftColPad + ((".................").substr(0, testColWid + 5)) + "  ";
        }
    };

    this.setInfo = function(testNum, grpname, method) {
        if (this.outputFile != "") {
            testNumber = testNum;
            if (grpname != groupName) {
                groupName = grpname;
                this.logToSummary(groupPad + groupName);
            }
            testName = method;
    //        this.logToSummary(leftColPad + "Start" + strPadLeft("      ", testNumber, testColWid) + ": " + testName);
        }
    };

    this.postResult = function(failed, msec) {
        if (this.outputFile != "") {
            var passfail;
            var fmtTime;
            var s;
            totalTime += msec;
            fmtTime = formatTimePt2(msec);
            if (failed) {
                passfail = "***Failed";
                if (groupName != failedGroupName) {
                    failedGroupName = groupName;
                     failedTests.push("    - " + failedGroupName + ":");
                }
                failedTests.push("         " + testNumber + " - " + testName);
                numUnitFails++;
            } else {
                passfail = "   Passed";
                numUnitPasses++;
            }
            this.logToSummary(strPadLeft("       ", testNumber + "/" + testTotal, leftColWid) + " Test " + strPadLeft("      #", testNumber, testColWid) + ": " + strPadRight(testName, midColPad, midColWid) + passfail + strPadLeft(timeColPad, fmtTime, timeColWid) + " sec");
        }
    };

    this.wrapup = function() {
        if (this.outputFile != "") {
            var totalUnits;
            var passPct;
            var output;

            totalUnits = numUnitPasses + numUnitFails;
            passPct = Math.round((numUnitPasses * 100) / totalUnits);
            output = "\n" + passPct + "% tests passed, " + numUnitFails + " tests failed out of " + totalUnits + "\n";
            output += "\nTotal Test time (real) = " + formatTimePt2(totalTime) + " sec";

            if (numUnitFails > 0) {
                output += "\n\nThe following tests FAILED:\n";
                output += failedTests.join("\n");
            }

            this.logToSummary(output);
        }
    };

    function formatTimePt2(milliseconds) {
        var lenTime;
        var pad = "";
        var fmtTime = Math.round(milliseconds / 10);
        if (fmtTime < 100) {
            pad = ((fmtTime < 10) ? "00" : "0");
        }
        fmtTime = pad + fmtTime;
        lenTime = fmtTime.length;
        return fmtTime.substr(0, lenTime - 2) + "." + fmtTime.substr(lenTime - 2);
    }

    function strPadLeft(strPad, str, len) {
        str = strPad + str;
        return str.substr(str.length - len);
    }

    function strPadRight(str, strPad, len) {
        return (str + strPad).substr(0, len);
    }

    this.logToSummary = function(line) {
        if (this.outputFile != "") {
            if (!sg.fs.exists(this.outputFile))
                sg.file.write(this.outputFile, line + "\n");
            else
                sg.file.append(this.outputFile, line + "\n");
        }
    };

};

