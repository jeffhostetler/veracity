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

//////////////////////////////////////////////////////////////////

function st_P4986()
{
    this.no_setup = true;

    this.test_1 = function()
    {
	var outer_path  = pathCombine(tempDir, "outer");
	var middle_path = pathCombine(outer_path, "middle");
	var inner_path  = pathCombine(middle_path, "inner");

	sg.fs.mkdir_recursive(inner_path);
	sg.fs.cd(inner_path);
	var o = sg.exec(vv, "init", "inner", ".");
	testlib.ok( (o.exit_status == 0) );

	sg.fs.cd(middle_path);
	var o = sg.exec(vv, "init", "middle", ".");
	testlib.ok( (o.exit_status != 0) );
	testlib.ok( (o.stderr.indexOf("The entry is already under version control") >= 0) );

	sg.fs.cd(outer_path);
	var o = sg.exec(vv, "init", "outer", ".");
	testlib.ok( (o.exit_status != 0) );
	testlib.ok( (o.stderr.indexOf("The entry is already under version control") >= 0) );

    }

}
