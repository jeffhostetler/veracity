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

function st_FsTests()
{
	this.fillDir = function(name, files) 
	{
		if (! testlib.testResult(sg.fs.mkdir(name), "mkdir " + name))
			return(false);

		for ( var i = 0; i < files.length; ++i )
		{
			var fn = name + "/" + files[i];
			sg.file.write(fn, fn);
		}

		return(true);
	};

	this.testReadEmptyDir = function()
	{
		var dir = "testReadEmptyDir";

		if (! this.fillDir(dir, []))
			return;

		var results = sg.fs.readdir(dir);

		testlib.equal(0, results.length, "expected 0 files on empty dir");
	};

	this.testReadOneFile = function()
	{
		var dir = 'testReadOneFile';
		if (! this.fillDir(dir, [ 'file.txt' ]))
			return;

		var results = sg.fs.readdir(dir);

		if (! testlib.equal(1, results.length, "expected 1 file"))
			return;

		var o = results[0];

		testlib.testResult(o.isFile, "is file");
		testlib.testResult(! o.isDirectory, "is not directory");
		testlib.equal(24, o.size, "file size"); // dir + / + name
		testlib.equal('file.txt', o.name, "name");
	}

	this.testReadFiles = function()
	{
		var dir = 'testReadFiles';
		if (! this.fillDir(dir, [ 'file.txt', 'file2.txt' ]))
			return;

		var results = sg.fs.readdir(dir);

		results.sort(
			     function(a,b) {
				 if (a.name > b.name)
				     return(1);
				 else if (a.name < b.name)
				     return(-1);
				 else
				     return(0);
			     }
			     );

		if (! testlib.equal(2, results.length, "expected 2 files"))
			return;

		var o = results[0];

		testlib.testResult(o.isFile, "is file");
		testlib.testResult(! o.isDirectory, "is not directory");
		testlib.equal(22, o.size, "file size"); // dir + / + name
		testlib.equal('file.txt', o.name, "name");

		o = results[1];

		testlib.testResult(o.isFile, "is file");
		testlib.testResult(! o.isDirectory, "is not directory");
		testlib.equal(23, o.size, "file size"); // dir + / + name
		testlib.equal('file2.txt', o.name, "name");
	};

	this.testReadWithDir = function()
	{
		var dir = 'testReadWithDir';
		var subdir = dir + "/sub";

		if ((! this.fillDir(dir, [ 'file.txt' ])) ||
			(! this.fillDir(subdir, ['subfile.txt']))
		   )
			return;
		
		var results = sg.fs.readdir(dir);

		if (! testlib.equal(2, results.length))
			return;

		results.sort(
			     function(a,b) {
				 if (a.name > b.name)
				     return(1);
				 else if (a.name < b.name)
				     return(-1);
				 else
				     return(0);
			     }
			     );
		
		var o = results[0];
		testlib.testResult(o.isFile, "is file");
		testlib.testResult(! o.isDirectory, "is not directory");
		testlib.equal('file.txt', o.name, "name");

		o = results[1];
		testlib.testResult(! o.isFile, "is not file");
		testlib.testResult(o.isDirectory, "is directory");
		testlib.equal('sub', o.name, "name");

	};
}
