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

function st_Exec() {
    this.setUp = function() {
    }
    
    
    this.testExec = function() {

	try
	{
	    var oBogus = sg.exec("vvasdfBlah");
	    testlib.ok( (oBogus.exec_result != 0), "Expect exec with unknown executable to fail" );
	    // oBogus.exit_status is not valid (since there was no child to have exited).
	    print(dumpObj(oBogus, "sg.exec('vvasdfBlah') yields", "", 0));
	}
	catch (eBogus)
	{
	    testlib.ok( (0), "sg.exec() should not have thrown");
	    print(dumpObj(eBogus, "sg.exec() threw", "", 0));
	}

	try
	{
	    var oBogus = sg.exec(vv, "boguscommand");
	    testlib.ok( (oBogus.exec_result == 0), "Expect exec of valid executable to pass" );
	    testlib.ok( (oBogus.exit_status != 0), "Expect exec of valid executable with bogus arg to complain");
	    print(dumpObj(oBogus, "sg.exec(vv, 'boguscommand') yields", "", 0));
	}
	catch (eBogus)
	{
	    testlib.ok( (0), "sg.exec() should not have thrown");
	    print(dumpObj(eBogus, "sg.exec() threw", "", 0));
	}

	try
	{
	    var oHelp = sg.exec(vv, "help");
	    testlib.ok( (oHelp.exec_result == 0), "Expect exec of valid executable to pass" );
	    testlib.ok( (oHelp.exit_status == 0), "Expect exec of valid executable with valid arg to pass");
	    print(dumpObj(oHelp, "sg.exec('help') yields", "", 0));
	}
	catch (eHelp)
	{
	    testlib.ok( (0), "sg.exec() should not have thrown");
	    print(dumpObj(eHelp, "sg.exec() threw", "", 0));
	}


	// TODO 2011/02/09 What is this?  Fix the test or fix the code!
	//This fails differently on Windows than on linux and mac
	//sg.exec_nowait("asdfwer");  //unknown command should not throw in a nowait
	}
    this.testExecNoWait = function() {
	testlib.notOnDisk("asyncOutput");
	testlib.notOnDisk("asyncError");
	sg.exec_nowait(["output=asyncOutput", "error=asyncError"], "curl", "--version");
	sg.sleep_ms(5000);
	testlib.existsOnDisk("asyncOutput");
	testlib.existsOnDisk("asyncError");
	testlib.ok(sg.file.read("asyncOutput").search("http")>0, "curl should have printed 'http' in its list of protocols");
	}
}

