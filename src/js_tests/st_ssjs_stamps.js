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

function st_ssjs_stamps()
{
	this.setUp = function()
	{
		this.repo = sg.open_repo(repInfo.repoName);
		var serverFiles = sg.local_settings().machine.server.files;
		var core = serverFiles + "/core";
		var scrum = serverFiles + "/modules/scrum";
		load(serverFiles + "/dispatch.js");
		load(core + "/vv.js");
		load(scrum + "/workitems.js");
		load(core + "/session.js");
		
		this.item_recid = newRecord("item", this.repo, { title: "Test Item"} );

		newRecord("stamp", this.repo, { name: "test", item: this.item_recid });
		newRecord("stamp", this.repo, { name: "ztest", item: this.item_recid });
	};
	
	this.tearDown = function()
	{
		if (this.repo)
			this.repo.close();
	};
	
	this.testSearchStamps = function()
	{
		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			headers: {
				From: this.userId
			}
		}
		
		var wir = new witRequest(request);
		
		var stamps = wir.searchStamps();
        // TODO why does searchStamps() return users as well as stamps?  weird.
		
		// Why expected 4?  2 stamps (test, ztest), 2 users (vscript_tests, nobody)
		testlib.testResult(stamps.length == 4, "Stamp count", 4, stamps.length);
		
		wir.params.prefix = "z";
		
		var stamps = wir.searchStamps();
		
		testlib.testResult(stamps.length == 1, "Filtered stamp count", 1, stamps.length);
	}
	
}
