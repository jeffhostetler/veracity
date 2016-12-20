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

function printUsageError()
{
	print("Usage: init_builds.js REPO_NAME");
}

function init_builds(repo)
{
	var manualSeriesRecId;
	var manualStatusRecId;

	// Set up link data: environments, status, series
	var zdb = new zingdb(repo, sg.dagnum.BUILDS);
	var ztx = zdb.begin_tx();

	var zrec = ztx.new_record("environment");
	zrec.name = "Windows 32-bit Debug";
	zrec.nickname = "W3D";
	zrec = ztx.new_record("environment");
	zrec.name = "Windows 32-bit Release";
	zrec.nickname = "W3R";
	zrec = ztx.new_record("environment");
	zrec.name = "Windows 64-bit Debug";
	zrec.nickname = "W6D";
	zrec = ztx.new_record("environment");
	zrec.name = "Windows 64-bit Release";
	zrec.nickname = "W6R";

	zrec = ztx.new_record("environment");
	zrec.name = "Linux 32-bit Debug";
	zrec.nickname = "L3D";
	zrec = ztx.new_record("environment");
	zrec.name = "Linux 32-bit Release";
	zrec.nickname = "L3R";
	zrec = ztx.new_record("environment");
	zrec.name = "Linux 64-bit Debug";
	zrec.nickname = "L6D";
	zrec = ztx.new_record("environment");
	zrec.name = "Linux 64-bit Release";
	zrec.nickname = "L6R";

	zrec = ztx.new_record("environment");
	zrec.name = "Linux 64-bit Public (source tarball)";
	zrec.nickname = "L6P";

	zrec = ztx.new_record("environment");
	zrec.name = "Mac 32-bit Debug";
	zrec.nickname = "M3D";
	zrec = ztx.new_record("environment");
	zrec.name = "Mac 32-bit Release";
	zrec.nickname = "M3R";
	zrec = ztx.new_record("environment");
	zrec.name = "Mac 64-bit Debug";
	zrec.nickname = "M6D";
	zrec = ztx.new_record("environment");
	zrec.name = "Mac 64-bit Release";
	zrec.nickname = "M6R";

	zrec = ztx.new_record("series");
	manualSeriesRecId = zrec.recid;
	zrec.name = "Manual";
	zrec.nickname = "M";
	zrec = ztx.new_record("series");
	zrec.name = "Continuous";
	zrec.nickname = "C";
	zrec = ztx.new_record("series");
	zrec.name = "Nightly";
	zrec.nickname = "N";
	zrec = ztx.new_record("series");
	zrec.name = "Long Tests";
	zrec.nickname = "L";

	zrec = ztx.new_record("status");
	manualStatusRecId = zrec.recid;
	zrec.name = "Queued (Do Not Skip)";
	zrec.nickname = "QS";
	zrec.color = "white";
	zrec.temporary = true;
	zrec.successful = false;
	zrec.use_for_eta = false;
	zrec.show_in_activity = false;
	zrec.icon = "queued";

	zrec = ztx.new_record("status");
	zrec.name = "Queued";
	zrec.nickname = "Q";
	zrec.color = "white";
	zrec.temporary = true;
	zrec.successful = false;
	zrec.use_for_eta = false;
	zrec.show_in_activity = false;
	zrec.icon = "queued";

	zrec = ztx.new_record("status");
	zrec.name = "Success";
	zrec.nickname = "S";
	zrec.color = "green";
	zrec.temporary = false;
	zrec.successful = true;
	zrec.show_in_activity = false;
	zrec.icon = "success";

	zrec = ztx.new_record("status");
	zrec.name = "Building";
	zrec.nickname = "B";
	zrec.color = "yellow";
	zrec.temporary = true;
	zrec.successful = false;
	zrec.show_in_activity = false;
	zrec.icon = "build";

	zrec = ztx.new_record("status");
	zrec.name = "Testing";
	zrec.nickname = "T";
	zrec.color = "yellow";
	zrec.temporary = true;
	zrec.successful = false;
	zrec.show_in_activity = false;
	zrec.icon = "testing";

	zrec = ztx.new_record("status");
	zrec.name = "Build Failed";
	zrec.nickname = "BF";
	zrec.color = "red";
	zrec.temporary = false;
	zrec.successful = false;
	zrec.use_for_eta = false;
	zrec.show_in_activity = true;
	zrec.icon = "buildfailed";

	zrec = ztx.new_record("status");
	zrec.name = "Tests Failed";
	zrec.nickname = "TF";
	zrec.color = "red";
	zrec.temporary = false;
	zrec.successful = false;
	zrec.use_for_eta = false;
	zrec.show_in_activity = true;
	zrec.icon = "testfailed";

	zrec = ztx.new_record("status");
	zrec.name = "Skipped";
	zrec.nickname = "K";
	zrec.color = "gray";
	zrec.temporary = false;
	zrec.successful = false;
	zrec.use_for_eta = false;
	zrec.show_in_activity = false;
	zrec.icon = "skip";

	var recs = zdb.query("config", ["recid"]);
	if (recs == null || recs.length == 0)
	{
		zrec = ztx.new_record("config");
		zrec.manual_build_series = manualSeriesRecId;
		zrec.manual_build_status = manualStatusRecId;
	}

	var r = ztx.commit();
	if (r.errors != null)
	{
		ztx.abort();
		print(sg.to_json__pretty_print(r));
	}
	else
	{
		print("Builds initialized.");
	}
}

/******************************************************************
 * MAIN
 ******************************************************************/
var repo;
if (arguments.length == 1)
{
	repo = sg.open_repo(arguments[0]);
	try
	{
		init_builds(repo);
	}
	finally
	{
		repo.close();
	}

}
else
{
	printUsageError();
}
