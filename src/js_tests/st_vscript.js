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


function st_vscript() {

	this.testSurviveBadServerFiles = function() {
		var settings = sg.local_settings();

		var oldServerFiles = settings.machine.server.files;

		var jsfn = 'testSurviveBadServerFiles.txt';
		
		sg.file.write(jsfn, "quit(0);\n");

		try
		{
			sg.set_local_setting("server/files", pathCombine(srcDir, sg.gid()));

			var o = sg.exec('vscript', jsfn);

			testlib.equal(0, o.exit_status);
		}
		catch (e)
		{
			testlib.fail(e.toString());
		}
		finally
		{
			sg.set_local_setting("server/files", oldServerFiles); // pathCombine(getParentPath(srcDir), "server_files"));
			
		}
		
	};
}
