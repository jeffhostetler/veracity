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

function err_must_contain(err, s)
{
    testlib.ok(-1 != err.indexOf(s), s);
}

function st_zing_merge_concat()
{
    this.t =
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
							"datatype": "int"
						},
                        "text" :
                        {
                            "datatype" : "string",
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "concat"
                                    }
                                ]
                            }
						}
                    }
                }
            }
        };

	this.txcommit = function(ztx, desc)
	{
		var cs = ztx.commit();

		var msg = "Eror committing";

		if (desc)
			msg += " " + desc;

		if (cs.errors)
			throw(msg);

		return(cs);
	};

	// make sure op:concat doesn't blow anything up
    this.test_trivial = function()
    {
        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);
		var ztx = null;

		try
		{
			ztx = zs.begin_tx();
			ztx.set_template(this.t);
			var cs = this.txcommit(ztx, "set template");
			ztx = null;
			var csid = cs.csid;

			ztx = zs.begin_tx(csid);
			var zrec = ztx.new_record("item");
			var recid1 = zrec.recid;
			zrec.text = "My cat's name is mittens.";
			this.txcommit(ztx, "add record");
			ztx = null;
		}
		finally
		{
			if (ztx)
				ztx.abort();
			repo.close();
		}
    };

    this.test_no_changes = function()
    {
        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);
		var ztx = null;

		try
		{
			ztx = zs.begin_tx();
			ztx.set_template(this.t);
			var cs = this.txcommit(ztx, "set template");
			ztx = null;
			var csid = cs.csid;

			ztx = zs.begin_tx(csid);
			var zrec = ztx.new_record("item");
			var recid1 = zrec.recid;
			zrec.text = "My cat's name is mittens.";
			zrec.val = 1;
			cs = this.txcommit(ztx, "add record");
			csid = cs.csid;
			ztx = null;

			ztx = zs.begin_tx(csid);
			zrec = ztx.open_record('item', recid1);
			zrec.val = 2;
			this.txcommit(ztx, "first mod");
			ztx = null;

			ztx = zs.begin_tx(csid);
			zrec = ztx.open_record('item', recid1);
			zrec.val = 3;
			this.txcommit(ztx, "second mod");
			ztx = null;

			zrec = zs.get_record('item', recid1);

			testlib.equal("My cat's name is mittens.", zrec.text, "text after merge");
		}
		finally
		{
			if (ztx)
				ztx.abort();
			repo.close();
		}
    };

    this.test_left_side_change = function()
    {
        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);
		var ztx = null;

		try
		{
			ztx = zs.begin_tx();
			ztx.set_template(this.t);
			var cs = this.txcommit(ztx, "set template");
			ztx = null;
			var csid = cs.csid;

			ztx = zs.begin_tx(csid);
			var zrec = ztx.new_record("item");
			var recid1 = zrec.recid;
			zrec.text = "My cat's name is mittens.";
			zrec.val = 1;
			cs = this.txcommit(ztx, "add record");
			csid = cs.csid;
			ztx = null;

			ztx = zs.begin_tx(csid);
			zrec = ztx.open_record('item', recid1);
			zrec.val = 2;
			zrec.text = "left side";
			this.txcommit(ztx, "first mod");
			ztx = null;

			ztx = zs.begin_tx(csid);
			zrec = ztx.open_record('item', recid1);
			zrec.val = 3;
			this.txcommit(ztx, "second mod");
			ztx = null;

			zrec = zs.get_record('item', recid1);

			testlib.equal("left side", zrec.text, "text after merge");
		}
		finally
		{
			if (ztx)
				ztx.abort();
			repo.close();
		}
    };

    this.test_right_side_change = function()
    {
        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);
		var ztx = null;

		try
		{
			ztx = zs.begin_tx();
			ztx.set_template(this.t);
			var cs = this.txcommit(ztx, "set template");
			ztx = null;
			var csid = cs.csid;

			ztx = zs.begin_tx(csid);
			var zrec = ztx.new_record("item");
			var recid1 = zrec.recid;
			zrec.text = "My cat's name is mittens.";
			zrec.val = 1;
			cs = this.txcommit(ztx, "add record");
			csid = cs.csid;
			ztx = null;

			ztx = zs.begin_tx(csid);
			zrec = ztx.open_record('item', recid1);
			zrec.val = 2;

			this.txcommit(ztx, "first mod");
			ztx = null;

			ztx = zs.begin_tx(csid);
			zrec = ztx.open_record('item', recid1);
			zrec.val = 3;
			zrec.text = "right side";
			this.txcommit(ztx, "second mod");
			ztx = null;

			zrec = zs.get_record('item', recid1);

			testlib.equal("right side", zrec.text, "text after merge");
		}
		finally
		{
			if (ztx)
				ztx.abort();
			repo.close();
		}
    };


    this.test_simple_merge = function()
    {
        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);
		var ztx = null;

		try
		{
			ztx = zs.begin_tx();
			ztx.set_template(this.t);
			var cs = this.txcommit(ztx, "set template");
			ztx = null;
			var csid = cs.csid;

			ztx = zs.begin_tx(csid);
			var zrec = ztx.new_record("item");
			var recid1 = zrec.recid;
			zrec.text = "line 1\n" + "line 2\n" + "line 3\n";
			zrec.val = 1;
			cs = this.txcommit(ztx, "add record");
			csid = cs.csid;
			ztx = null;

			ztx = zs.begin_tx(csid);
			zrec = ztx.open_record('item', recid1);
			zrec.val = 2;
			zrec.text = "line 1\n" + "line 1.5\n" + "line 2\n" + "line 3\n";
			this.txcommit(ztx, "first mod");
			ztx = null;

			ztx = zs.begin_tx(csid);
			zrec = ztx.open_record('item', recid1);
			zrec.val = 3;
			zrec.text = "line 1\n" + "line 2\n" + "line 2.5\n" + "line 3\n";
			this.txcommit(ztx, "second mod");
			ztx = null;

			zrec = zs.get_record('item', recid1);

			testlib.equal("line 1\n" + "line 1.5\n" + "line 2\n" + "line 2.5\n" + "line 3\n",
						  zrec.text, "text after merge");
		}
		finally
		{
			if (ztx)
				ztx.abort();
			repo.close();
		}
    };

    this.test_conflicts = function()
    {
        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);
		var ztx = null;

		var lines = function(larr)
		{
			return(larr.join("\n"));
		};

		var separator = "\n== ZING_MERGE_CONCAT ==\n";

		var before = lines(["line 1", "line 2", "line 3"]);
		var left = lines(["line 1", "line 5", "line 3"]);
		var right = lines(["line 1", "line 7", "line 3"]);

		try
		{
			ztx = zs.begin_tx();
			ztx.set_template(this.t);
			var cs = this.txcommit(ztx, "set template");
			ztx = null;
			var csid = cs.csid;

			ztx = zs.begin_tx(csid);
			var zrec = ztx.new_record("item");
			var recid1 = zrec.recid;
			zrec.text = before;
			zrec.val = 1;
			cs = this.txcommit(ztx, "add record");
			csid = cs.csid;
			ztx = null;

			ztx = zs.begin_tx(csid);
			zrec = ztx.open_record('item', recid1);
			zrec.val = 2;
			zrec.text = left;
			this.txcommit(ztx, "first mod");
			ztx = null;

			ztx = zs.begin_tx(csid);
			zrec = ztx.open_record('item', recid1);
			zrec.val = 3;
			zrec.text = right;
			this.txcommit(ztx, "second mod");
			ztx = null;

			zrec = zs.get_record('item', recid1);

			if (! testlib.testResult((zrec.text == left + separator + right) || (zrec.text == right + separator + left), "text after merge"))
				testlib.log(zrec.text);
		}
		finally
		{
			if (ztx)
				ztx.abort();
			repo.close();
		}
    };

}

