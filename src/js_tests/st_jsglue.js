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

function st_jsglue()
{
	
	this.testOpenRepoSanityCheck = function()
	{
		var caught = false;
		try
		{
			var repo = sg.open_repo(null);
		}
		catch (e)
		{
			caught = true;
		}
		finally
		{
			testlib.testResult(caught, "Open repo throws JS exception");
		}
	};
	
	this.testCreateZingDBSanityCheck = function()
	{
		var caught = false;
		try
		{
			var db = new zingdb(null, sg.dagnum.WORK_ITEMS);
		}
		catch(e)
		{
			caught = true;
		}
		finally
		{
			testlib.testResult(caught, "zingdb constructor thorws JS exception on null repo");
		}
		
		var caught = false;
		try
		{
			var repo = sg.open_repo(repInfo.repoName);
			var db = new zingdb(repo, null);
		}
		catch(e)
		{
			caught = true;
		}
		finally
		{
			repo.close();
			testlib.testResult(caught, "zingdb constructor throws JS exception on null dagnum");
		}
		
	};
	
	this.testUpdateZingTemplateSanityCheck = function()
	{
		var caught = false;
		try
		{
			var repo = sg.open_repo(repInfo.repoName);
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
			db.set_template(null);
		}
		catch(e)
		{
			caught = true;
		}
		finally
		{
			repo.close();
			testlib.testResult(caught, "set_template throws JS exception on null template");
		}
	};
	
	this.testZingNewRecordSanityCheck = function()
	{
		var caught = false;
		try
		{
			var repo = sg.open_repo(repInfo.repoName);
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
			var tx = db.begin_tx();
			tx.new_record(null);
		}
		catch(e)
		{
			caught = true;
		}
		finally
		{
			tx.abort();
			repo.close();
			testlib.testResult(caught, "new_record throws JS exception on null rectype");
		}
		
		var caught = false;
		try
		{
			var repo = sg.open_repo(repInfo.repoName);
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
			var tx = db.begin_tx();
			tx.new_record("item", null);
		}
		catch(e)
		{
			caught = true;
		}
		finally
		{
			tx.abort();
			repo.close();
			testlib.testResult(caught, "new_record throws JS exception on null record");
		}
	};
	
	this.testSGHashSanityCheck = function()
	{
		var caught = false;
		
		try
		{
			sg.hash(null);
		}
		catch (e)
		{
			caught = true;
		}
		finally
		{
			testlib.testResult(caught, "sg.hash throws JS exception on null argument");
		}
	};
	
	this.testSGCloneAllFullSanityCheck = function()
	{
		var caught = false;
		
		try
		{
			sg.clone__all_full(null);
		}
		catch (e)
		{
			caught = true;
		}
		finally
		{
			testlib.testResult(caught, "sg.clone__all_full throws JS exception on null argument");
		}
	};
	
	this.testSGCloneAllZlibSanityCheck = function()
	{
		var caught = false;
		
		try
		{
			sg.clone__all_zlib(null);
		}
		catch (e)
		{
			caught = true;
		}
		finally
		{
			testlib.testResult(caught, "sg.clone__all_full throws JS exception on null argument");
		}
	}

	this.testIncomingAndOutgoing = function(){
		var repo0 = repInfo.repoName;
		var repo1 = repInfo.repoName+"-testIncomingAndOutgoing";
		sg.clone__exact(repo0, repo1);

		testlib.testResult(sg.incoming(repo0, repo1)===null, "sg.incoming() return null when there's no results");
		testlib.testResult(sg.outgoing(repo0, repo1)===null, "sg.outgoing() return null when there's no results");

		sg.file.write("testIncomingAndOutgoing.txt", "Initial content.");
		sg.wc.add( { "src" : "testIncomingAndOutgoing.txt" } );
		var csid1 = sg.wc.commit( { "message" : "Initial Commit." } );

		var in1  = sg.incoming(repo0, repo1);
		testlib.equal(1, in1.length, "1 incoming changeset");
		testlib.equal(csid1, in1[0].changeset_id, "csid");

		var out1 = sg.outgoing(repo0, repo1);
		testlib.equal(1, out1.length, "1 outgoing changeset");
		testlib.equal(csid1, out1[0].changeset_id, "csid");

		sg.file.write("testIncomingAndOutgoing.txt", "Modified content.");
		var csid2 = sg.wc.commit( { "message" : "Second Commit." } );

		var in2  = sg.incoming(repo0, repo1);
		testlib.equal(2, in2.length, "2 incoming changesets");
		testlib.equal(csid2, in2[1].changeset_id, "csid");

		var out2 = sg.outgoing(repo0, repo1);
		testlib.equal(2, out2.length, "2 outgoing changesets");
		testlib.equal(csid2, out2[1].changeset_id, "csid");

	};
	
	this.testCreateUserForceRecid = function()
	{
		gid = sg.gid();
		
		try
		{
			var repo = sg.open_repo(repInfo.repoName);
			var db = new zingdb(repo, sg.dagnum.USERS);
			repo.create_user_force_recid({ "name": "forced_gid", "userid": gid } );
			
			user = db.get_record("user", gid);
			
			testlib.testResult(user != null, "User should exists with forced gid");
		}
		catch(e)
		{
			testlib.testResult(false, "create_user_force_recid should not throw");
		}
		finally
		{
			repo.close();
		}
		
	}
}
