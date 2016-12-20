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

function st_builds()
{
	var repo;

	var environmentRecId;
	var seriesRecId;
	var statusRecId;

	/* In case tests aren't run in the order they're defined */
	this.my_setup_done = false;

	this.test_setup = function()
	{
		if (this.my_setup_done)
			return;
		this.my_setup_done = true;

		repo = sg.open_repo(repInfo.repoName);
		repo.create_user("st_builds@sourcegear.com");
		repo.set_user("st_builds@sourcegear.com");

		// Set up link data: environments, status, series
		var buildsDagnum = sg.dagnum.BUILDS;
		print("builds dagnum: " + sg.to_json__pretty_print(buildsDagnum));
		var zdb = new zingdb(repo, buildsDagnum);
		var ztx = zdb.begin_tx();

		var zrec = ztx.new_record("environment");
		environmentRecId = zrec.recid;
		zrec.name = "Linux 64-bit Debug";
		zrec.nickname = "L6D";
		zrec = ztx.new_record("environment");
		zrec.name = "Windows 32-bit Release";
		zrec.nickname = "W3R";

		zrec = ztx.new_record("series");
		seriesRecId = zrec.recid;
		zrec.name = "Continuous";
		zrec.nickname = "C";
		zrec = ztx.new_record("series");
		zrec.name = "Nightly";
		zrec.nickname = "N";

		zrec = ztx.new_record("status");
		statusRecId = zrec.recid;
		zrec.name = "Success";
		zrec.nickname = "S";
		zrec.color = "green";
		zrec.temporary = false;
		zrec.successful = true;

		zrec = ztx.new_record("status");
		zrec.name = "Building";
		zrec.nickname = "B";
		zrec.color = "orange";
		zrec.temporary = true;
		zrec.successful = false;

		zrec = ztx.new_record("status");
		zrec.name = "Build Failed";
		zrec.nickname = "BF";
		zrec.color = "red";
		zrec.temporary = false;
		zrec.successful = false;

		zrec = ztx.new_record("status");
		zrec.name = "Tests Failed";
		zrec.nickname = "TF";
		zrec.color = "red";
		zrec.temporary = false;
		zrec.successful = false;

		var r = ztx.commit();
		if (r.errors != null)
		{
			ztx.abort();
			testlib.ok(false, sg.to_json__pretty_print(r));
		}
		else
			testlib.ok(true, "Basic build link data added.");
	};

	this.tearDown = function()
	{
		repo.close();
	};

	this.test_simple_build = function()
	{
		this.test_setup();

		var buildsDagnum = sg.dagnum.BUILDS;
		print("builds dagnum: " + sg.to_json__pretty_print(buildsDagnum));
		var zdb = new zingdb(repo, buildsDagnum);
		var r;

		var ztx = zdb.begin_tx();
		var buildNameRec = ztx.new_record("buildname");
		var buildRec = ztx.new_record("build");

		try
		{
			buildNameRec.name = "0.5.0.1234";
			buildNameRec.csid = sg.vv2.leaves()[0];
			buildNameRec.series_ref = seriesRecId;

			print(sg.to_json__pretty_print(buildNameRec));

			buildRec.buildname_ref = buildNameRec.recid;
			buildRec.environment_ref = environmentRecId;
			buildRec.series_ref = seriesRecId;
			buildRec.status_ref = statusRecId;

			r = ztx.commit();
		}
		finally
		{
			if (r != null && r.errors != null)
			{
				ztx.abort();
				testlib.ok(false, sg.to_json__pretty_print(r));
			}
			else
				testlib.ok(true, "Basic build added.");
		}
	};
}
