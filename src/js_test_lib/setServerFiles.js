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
var srcDir = sg.fs.getcwd();

var sfpath = pathCombine(getParentPath(srcDir), "server_files");
sg.set_local_setting("server/files", sfpath);
sg.set_local_setting("log/level", "verbose");
sg.set_local_setting("log/path", arguments[0]);
serverFilesPath = sg.get_local_setting("server/files");
print("server/files is " + serverFilesPath);
//print("log/level is " + sg.get_local_setting("log/level"));
//print("log/path is " + sg.get_local_setting("log/path"));

if (serverFilesPath.toLowerCase().indexOf("program files") >= 0)
{
	print("server/files is set to program files");
	throw("server/files is set to program files");
}