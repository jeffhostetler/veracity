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

function st_modules()
{
	this.setUp = function() 
	{
	};

	this.tearDown = function()
	{
	};

	this.testCheckClonedRepos = function()
	{
		var orig = sg.gid();
		var clone = sg.gid();
		var repo = null;

		try
		{
			sg.vv2.init_new_repo( { "repo" : orig, "no-wc" : true } );

			sg.clone__exact(orig, clone);

			repo = sg.open_repo(orig);
			var origDags = repo.list_dags();
			repo.close();
			repo = null;

			repo = sg.open_repo(clone);
			var cloneDags = repo.list_dags();
			repo.close();
			repo = null;

			var diffs = false;

			testlib.equal(origDags.length, cloneDags.length, "Cloned repo's DAG count");
		}
		finally
		{
			if (repo)
				repo.close();
		}
	};
}
