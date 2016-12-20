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

function st_sync_closet_users()
{
	this.no_setup = true;
	this.oldCloset = "";

	this.setUp = function()
	{
		/* We're deliberately creating a closet within the closet here,
		 * so that we don't create a bunch of new "closet spam:"
		 * deleting .sgcloset-test still cleans everything up.
		 * It works fine today. Someday this could be Wrong. */
		this.oldCloset = sg.getenv("SGCLOSET");
		sg.setenv("SGCLOSET", this.oldCloset + "/" + sg.gid().substr(0,5));
		sg.refresh_closet_location_from_environment_for_testing_purposes();
		print("SGCLOSET is " + sg.getenv("SGCLOSET"));

		sg.create_user_master_repo({ "adminid": sg.gid() });
	};

	this.tearDown = function()
	{
		sg.setenv("SGCLOSET", this.oldCloset);
	};

	/**
	 * otherThing is either a second repo name or an array of user records.
	 */
	this.verify_users_match = function(repoOrRepoName, otherThing)
	{
		var users1 = null;
		var repo_mine = null;
		var repo1 = null;


		if (typeof(repoOrRepoName) == "string")
		{
			repo_mine = sg.open_repo(repoOrRepoName);
			repo1 = repo_mine;
		}
		else
		{
			repo1 = repoOrRepoName;
		}

		try
		{
			var zdb1 = new zingdb(repo1, sg.dagnum.USERS);
			users1 = zdb1.query("user", ["*"], null, "name");
		}
		finally
		{
			if (repo_mine !== null)
				repo_mine.close();
		}

		var users2 = null;
		if (Array.isArray(otherThing))
		{
			users2 = otherThing;
		}
		else
		{
			var repo2 = sg.open_repo(otherThing);
			try
			{
				var zdb2 = new zingdb(repo2, sg.dagnum.USERS);
				users2 = zdb2.query("user", ["*"], null, "name");
			}
			finally
			{
				repo2.close();
			}
		}

		testlib.testResult(users1.length == users2.length, "The number of users should match.");
		for (var i = 0; i < users1.length; i++)
		{
			testlib.testResult(users1[i].name == users2[i].name, "User #" + i + " in each repo should have the same name.");
			testlib.testResult(users1[i].prefix == users2[i].prefix, "User #" + i + " in each repo should have the same prefix.");
			testlib.testResult(users1[i].name == users2[i].name, "User #" + i + " in each repo should have the same recid.");
		}
	};

	/**
	 * otherThing is either a second repo name or an array of user records.
	 */
	this.verify_users_match_user_master = function(otherThing)
	{
		var user_master_repo = sg.open_user_master_repo();
		try
		{
			this.verify_users_match(user_master_repo, otherThing);
		}
		finally
		{
			user_master_repo.close();
		}
	};

	this.create_user = function(repoName, userName)
	{
		var repo = sg.open_repo(repoName);
		try
		{
			repo.create_user(userName);
		}
		finally
		{
			repo.close();
		}
	};

	this.test_basic = function()
	{
		// sg.vv.init_new_repo(["no-wc","from-user-master"], "repo1");
		sg.vv2.init_new_repo( { "repo" : "repo1", "no-wc" : true, "from-user-master" : true    } );
		// sg.vv.init_new_repo(["no-wc","shared-users=repo1"], "repo2");
		sg.vv2.init_new_repo( { "repo" : "repo2", "no-wc" : true,     "shared-users" : "repo1" } );

		this.create_user("repo1", "user1");
		this.create_user("repo2", "user2");

		var repo1 = sg.open_repo("repo1");
		try
		{
			var syncedUsers = repo1.sync_all_users_in_closet();
			testlib.testResult(syncedUsers.length == 3);
			this.verify_users_match(repo1, "repo2");
			this.verify_users_match("repo2", syncedUsers);
			this.verify_users_match_user_master(syncedUsers);
		}
		finally
		{
			repo1.close();
		}

	};

	this.test_conflict = function()
	{
		// sg.vv.init_new_repo(["no-wc", "from-user-master"], "repo3");
		sg.vv2.init_new_repo( { "repo" : "repo3", "no-wc" : true, "from-user-master" : true    } );
		// sg.vv.init_new_repo(["no-wc","shared-users=repo3"], "repo4");
		sg.vv2.init_new_repo( { "repo" : "repo4", "no-wc" : true,     "shared-users" : "repo3" } );

		this.create_user("repo3", "user3");
		this.create_user("repo4", "user3");

		var repo3 = sg.open_repo("repo3");
		try
		{
			var syncedUsers = repo3.sync_all_users_in_closet();
			testlib.testResult(syncedUsers.length == 5, null, 5, syncedUsers.length);
			this.verify_users_match(repo3, "repo4");
			this.verify_users_match(repo3, syncedUsers);
			this.verify_users_match_user_master(syncedUsers);
		}
		finally
		{
			repo3.close();
		}
	};

	this.test_2_usermap = function()
	{
		var t =
		{
			"version" : 1,
			"rectypes" :
			{
				"item" :
				{
					"merge" :
					{
						"merge_type" : "field",
						"auto" :
						[
							{
								"op" : "most_recent"
							}
						]
					},
					"fields" :
					{
						"val" :
						{
							"datatype" : "string",
							"constraints" :
							{
								"unique" : true,
								"defaultfunc" : "gen_userprefix_unique"
							},
							"merge" :
							{
								"uniqify" :
								{
									"which" : "last_modified",
									"op" : "redo_defaultfunc"
								}
							}
						}
					}
				}
			}
		};

		// --------
		var r2name = sg.gid();
		print("repo just to get 4 users into master: ", r2name);

		// sg.vv.init_new_repo( [ "no-wc", "from-user-master" ], r2name );
		sg.vv2.init_new_repo( { "repo" : r2name, "no-wc" : true, "from-user-master" : true    } );

		var repo = sg.open_repo(r2name);
		userid_Aragorn = repo.create_user("Aragorn");
		userid_Boromir = repo.create_user("Boromir");
		userid_Faramir = repo.create_user("Faramir");
		userid_Eomer = repo.create_user("Eomer");
		repo.set_user("Aragorn");

		var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

		// set template
		var ztx = zs.begin_tx();
		ztx.set_template(t);
		ztx.commit();

		// add records
		ztx = zs.begin_tx();
		for (i=0; i<5; i++)
		{
			ztx.new_record("item");
		}
		ztx.commit();
	  
		repo.sync_all_users_in_closet();
	   
		repo.close();

		// --------
		var r4name = sg.gid();
		print("repo with new users: ", r4name);

		// sg.vv.init_new_repo( [ "no-wc"], r4name );
		sg.vv2.init_new_repo( { "repo" : r4name, "no-wc" : true } );

		repo = sg.open_repo(r4name);
		userid_Merry = repo.create_user("Merry");
		userid_Frodo = repo.create_user("Frodo");
		userid_Sam = repo.create_user("Sam");
		repo.set_user("Frodo");

		zs = new zingdb(repo, sg.dagnum.TESTING_DB);

		// set template
		ztx = zs.begin_tx();
		ztx.set_template(t);
		ztx.commit();

		// add records
		ztx = zs.begin_tx();
		for (i=0; i<5; i++)
		{
			ztx.new_record("item");
		}
		var csid = ztx.commit().csid;
	  
		zs = new zingdb(repo, sg.dagnum.USERS);

		// set template
		ztx = zs.begin_tx();
		var rec = ztx.open_record("user", userid_Merry);
		rec.inactive = true;
		ztx.commit();

		repo.close();

		// --------
		// do usermap clone
		var r5name = sg.gid();
		print("usermap repo: ", r5name);
		sg.clone__prep_usermap(
				{
					"existing_repo" : r4name,
					"new_repo" : r5name
				}
				);
		
		/* We create a couple of "ballast" clones here to reproduce the sqlite rowid/usermap id issue described in K8549. */
		var r6name = sg.gid();
		print("'ballast' usermap repo 1: ", r6name);
		sg.clone__prep_usermap(
				{
					"existing_repo" : r4name,
					"new_repo" : r6name
				}
				);
		var r7name = sg.gid();
		print("'ballast' usermap repo 2: ", r6name);
		sg.clone__prep_usermap(
				{
					"existing_repo" : r4name,
					"new_repo" : r7name
				}
				);

		var map = {};
		map[userid_Frodo] = userid_Aragorn;
		map[userid_Sam] = userid_Faramir;
		map[userid_Merry] = "NEW";
		this.helper__finish_usermap(r5name, csid, map);

		map[userid_Merry] = userid_Eomer;
		this.helper__finish_usermap(r6name, csid, map);
		
		this.helper__finish_usermap(r7name, csid, map);
	};

	this.helper__finish_usermap = function(reponame, csid, map)
	{
		var a = sg.get_users_from_needs_usermap_repo(reponame);
		print("users from needs map" + sg.to_json__pretty_print(a));

		for (var prop in map)
		{
			print(prop + ":" + map[prop]);
			sg.usermap.add_or_update(reponame, prop, map[prop]);
		}

		sg.clone__finish_usermap(
				{
					"repo_being_usermapped" : reponame
				} );

		repo = sg.open_repo(reponame);
		try
		{
			a = repo.lookup_audits(sg.dagnum.TESTING_DB, csid);
			print(sg.to_json__pretty_print(a));

			var db = new zingdb(repo, sg.dagnum.USERS);
			var recs = db.query('user', ['*']);
			print(sg.to_json__pretty_print(recs));

			repo.sync_all_users_in_closet();
			this.verify_users_match_user_master(reponame);
		}
		finally
		{
			repo.close();
		}

		/* ensure usermap is deleted */
		var bCorrectFailure = false;
		try
		{
			sg.usermap.get_all(reponame);
		}
		catch(e)
		{
			if (e.message.indexOf("Error 62") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
				bCorrectFailure = true;
			else
				print(sg.to_json__pretty_print(e));
		}
		testlib.testResult(bCorrectFailure, "usermap.get_all should fail with sglib error 62.");
	};

/*	This test works, but it's not really testing anything and is relatively slow.

	this.test_many = function()
	{
		sg.vv.init_new_repo(["no-wc", "from-user-master"], "repo10");
		this.create_user("repo10", "user10");

		sg.vv.init_new_repo(["no-wc","shared-users=repo10"], "repo11");
		this.create_user("repo11", "user11");
		sg.vv.init_new_repo(["no-wc","shared-users=repo10"], "repo12");
		this.create_user("repo12", "user12");
		sg.vv.init_new_repo(["no-wc","shared-users=repo10"], "repo13");
		this.create_user("repo13", "user13");
		sg.vv.init_new_repo(["no-wc","shared-users=repo10"], "repo14");
		this.create_user("repo14", "user14");
		sg.vv.init_new_repo(["no-wc","shared-users=repo10"], "repo15");
		this.create_user("repo15", "user15");
		sg.vv.init_new_repo(["no-wc","shared-users=repo10"], "repo16");
		this.create_user("repo16", "user16");
		sg.vv.init_new_repo(["no-wc","shared-users=repo10"], "repo17");
		this.create_user("repo17", "user17");
		sg.vv.init_new_repo(["no-wc","shared-users=repo10"], "repo18");
		this.create_user("repo18", "user18");
		sg.vv.init_new_repo(["no-wc","shared-users=repo10"], "repo19");
		this.create_user("repo19", "user19");
		sg.vv.init_new_repo(["no-wc","shared-users=repo10"], "repo20");
		this.create_user("repo20", "user20");

		var repo10 = sg.open_repo("repo10");
		try
		{
			var syncedUsers = repo10.sync_all_users_in_closet();
	
			testlib.testResult(syncedUsers.length == 16);
			this.verify_users_match("repo10", "repo11");
			this.verify_users_match("repo11", "repo12");
			this.verify_users_match("repo12", "repo13");
			this.verify_users_match("repo13", "repo14");
			this.verify_users_match("repo14", "repo15");
			this.verify_users_match("repo15", "repo16");
			this.verify_users_match("repo16", "repo17");
			this.verify_users_match("repo17", "repo18");
			this.verify_users_match("repo18", "repo19");
			this.verify_users_match("repo19", "repo20");
			this.verify_users_match("repo20", syncedUsers);
			this.verify_users_match_user_master(syncedUsers);
		}
		finally
		{
			repo10.close();
		}
	};
*/}
