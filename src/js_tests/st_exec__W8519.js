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

function st_exec__W8519()
{
    this.no_setup = true;		// do not create an initial REPO and WD.

    this.u0111 = "u0111_echo_argv";

    this.check_ok = function(r)
    {
	    print("stdout from command:");
	    print(r.stdout);
		
	    print("stderr from command:");
	    print(r.stderr);
		
		print("exec_result: " + r.exec_result);
	testlib.ok( (r.exec_result == 0), "r.exec_result" );
	if (r.exec_result == 0)
	{
	    print("stdout from command:");
	    print(r.stdout);
	    testlib.ok( (r.exit_status == 0), "r.exit_status" );
	}
    }

    this.check_not_ok = function(r)
    {
	testlib.ok( (r.exec_result == 0), "r.exec_result" );
	if (r.exec_result == 0)
	{
	    print("stdout from command:");
	    print(r.stdout);
	    testlib.ok( (r.exit_status != 0), "r.exit_status" );
	}
    }

    this.check_ok__verify = function(r, v)
    {
	testlib.ok( (r.exec_result == 0), "r.exec_result" );
	if (r.exec_result == 0)
	{
	    print("stdout from command:");
	    print(r.stdout);
	    testlib.ok( (r.exit_status == 0), "r.exit_status" );

	    for (var k in v)
	    {
		if (r.stdout.indexOf( v[k] ) < 0)
		    testlib.ok( (0), "Expected: " + v[k] );
		else
		    testlib.ok( (1), "Found:    " + v[k] );
	    }
	}
    }

    this.test_1 = function()
    {
	print("Starting in: " + sg.fs.getcwd());
	print("running: " + this.u0111);

	this.check_ok(  sg.exec(this.u0111)  );
	this.check_ok(  sg.exec(this.u0111, "0")  );
	this.check_ok(  sg.exec(this.u0111, "1", "hello")  );
	this.check_ok(  sg.exec(this.u0111, "2", "hello", "there")  );

	this.check_not_ok(  sg.exec(this.u0111, "3", "hello", "there")  );

	// See W8519.

	this.check_ok(  sg.exec(this.u0111, "2", "\\hello", "there")  );
	this.check_ok(  sg.exec(this.u0111, "2", "hel\\lo", "there")  );
	this.check_ok(  sg.exec(this.u0111, "2", "hello\\", "there")  );	// fails

	// This is a bit overkill, but make sure that the
	// backslash arrives as a backslash in the called
	// program (and didn't get interpreted as a control char).
	// And make sure that we didn't unnecessarily double it.
	//
	// 5c is a backslash
	// 22 is a doublequote
	//
	// Note that since JavaScript is eating some of these backslashes,
	// it's hard to tell how much should/did make it through and how
	// much we can test.
	//
	// There are 2 ways that these tests can/do fail with W8519.
	// [] argc wrong -- the exec'd program gets "hello" and "there"
	//                  combined in one argv cell.
	// [] argv[2] wrong -- one or more backslashes not present in value.

	this.check_ok__verify(  sg.exec(this.u0111, "2", "hello", "there"),		[ "argv[2]: 68 65 6c 6c 6f",    "argv[3]: 74 68 65 72 65" ] );
	this.check_ok__verify(  sg.exec(this.u0111, "2", "\\hello", "there"),		[ "argv[2]: 5c 68 65 6c 6c 6f", "argv[3]: 74 68 65 72 65" ] );
	this.check_ok__verify(  sg.exec(this.u0111, "2", "hel\\lo", "there"),		[ "argv[2]: 68 65 6c 5c 6c 6f", "argv[3]: 74 68 65 72 65" ] );
	this.check_ok__verify(  sg.exec(this.u0111, "2", "hello\\", "there"),		[ "argv[2]: 68 65 6c 6c 6f 5c", "argv[3]: 74 68 65 72 65" ] );	// fail

	// see if there is any difference in JS with single- vs double-quotes.
	this.check_ok__verify(  sg.exec(this.u0111, "2", "hello\\\\hello", "there"),	[ "argv[2]: 68 65 6c 6c 6f 5c 5c 68 65 6c 6c 6f",    "argv[3]: 74 68 65 72 65" ] );
	this.check_ok__verify(  sg.exec(this.u0111, "2", 'hello\\\\hello', "there"),	[ "argv[2]: 68 65 6c 6c 6f 5c 5c 68 65 6c 6c 6f",    "argv[3]: 74 68 65 72 65" ] );

	// use single-quotes to embed a double-quote (without needing to use \")
	this.check_ok__verify(  sg.exec(this.u0111, "2", 'hello"hello', "there"),	[ "argv[2]: 68 65 6c 6c 6f 22 68 65 6c 6c 6f",       "argv[3]: 74 68 65 72 65" ] );
	this.check_ok__verify(  sg.exec(this.u0111, "2", 'hello\"hello', "there"),	[ "argv[2]: 68 65 6c 6c 6f 22 68 65 6c 6c 6f",       "argv[3]: 74 68 65 72 65" ] );
	this.check_ok__verify(  sg.exec(this.u0111, "2", 'hello\\"hello', "there"),	[ "argv[2]: 68 65 6c 6c 6f 5c 22 68 65 6c 6c 6f",    "argv[3]: 74 68 65 72 65" ] );	// fail
	this.check_ok__verify(  sg.exec(this.u0111, "2", 'hello\\\"hello', "there"),	[ "argv[2]: 68 65 6c 6c 6f 5c 22 68 65 6c 6c 6f",    "argv[3]: 74 68 65 72 65" ] );	// fail
	this.check_ok__verify(  sg.exec(this.u0111, "2", 'hello\\\\"hello', "there"),	[ "argv[2]: 68 65 6c 6c 6f 5c 5c 22 68 65 6c 6c 6f", "argv[3]: 74 68 65 72 65" ] );	// fail

	// use double-quotes and embed a double-quote within using \"
	this.check_ok__verify(  sg.exec(this.u0111, "2", "hello\"hello", "there"),	[ "argv[2]: 68 65 6c 6c 6f 22 68 65 6c 6c 6f",       "argv[3]: 74 68 65 72 65" ] );
	this.check_ok__verify(  sg.exec(this.u0111, "2", "hello\\\"hello", "there"),	[ "argv[2]: 68 65 6c 6c 6f 5c 22 68 65 6c 6c 6f",    "argv[3]: 74 68 65 72 65" ] );	// fail
	this.check_ok__verify(  sg.exec(this.u0111, "2", "hello\\\\\"hello", "there"),	[ "argv[2]: 68 65 6c 6c 6f 5c 5c 22 68 65 6c 6c 6f", "argv[3]: 74 68 65 72 65" ] );	// fail

    }
}
