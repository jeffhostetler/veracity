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

function num_keys(obj)
{
    var count = 0;
    for (var prop in obj)
    {
        count++;
    }
    return count;
}

function get_keys(obj)
{
    var count = 0;
    var a = [];
    for (var prop in obj)
    {
        a[count++] = prop;
    }
    return a;
}

function get_add_from_cs(cs)
{
    var parents = get_keys(cs.db.changes);
    var hids = get_keys(cs.db.changes[parents[0]].add);
    return hids[0];
}

function get_nth(x, n)
{
    var k = get_keys(x);
    return x[k[n]];
}

function err_must_contain(err, s)
{
    testlib.ok(-1 != err.indexOf(s), s);
}

function verify(zs, where, cs, which, count)
{
    var ids = zs.query("item", ["recid"], where, null, 0, 0, cs[which]);
    var msg = "count " + where + " on " + which + " should be " + count;
    testlib.ok(count == ids.length, msg);
}

function st_zing_query()
{
    this.test_query_misc_new = function()
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
                        "Luke_Skywalker" :
                        {
                            "datatype" : "int"
                        },
                        "Han_Solo" :
                        {
                            "datatype" : "int"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        ztx.commit();

        var t2 = zs.get_template();
        testlib.ok(sg.to_json(t) == sg.to_json(t2));

        ztx = zs.begin_tx();
        ztx.new_record("item", {"Luke_Skywalker" : 76, "Han_Solo" : 22});
        ztx.new_record("item", {"Luke_Skywalker" : 11, "Han_Solo" : 33});
        ztx.new_record("item", {"Luke_Skywalker" : -5, "Han_Solo" : 44});
        ztx.new_record("item", {"Luke_Skywalker" : 50, "Han_Solo" : -11});
        ztx.new_record("item", {"Luke_Skywalker" : 22, "Han_Solo" : -33});
        ztx.new_record("item", {"Luke_Skywalker" : 0, "Han_Solo" : 55});
        ztx.new_record("item", {"Luke_Skywalker" : 1, "Han_Solo" : 66});
        ztx.new_record("item", {"Luke_Skywalker" : -40, "Han_Solo" : 99});

        ztx.commit();

        var ids = zs.q(
                {
                    "rectype" : "item",
                    "fields" : ["recid"],
                }
                );
        testlib.ok(ids.length == 8, "8 results");

        recs = zs.q(
                {
                    "rectype" : "item",
                    "fields" : ["Luke_Skywalker", "Han_Solo"],
                    "sort" : [
                                {
                                    "name" : "Luke_Skywalker",
                                    "dir" : "desc"
                                },
                                {
                                    "name" : "Han_Solo",
                                    "dir" : "asc"
                                },
                             ]
                }
                );
        testlib.ok(recs.length == 8, "8 results");
        testlib.ok(recs[0].Han_Solo == 22);

        recs = zs.q(
                {
                    "rectype" : "item",
                    "fields" : ["Luke_Skywalker", "Han_Solo"],
                    "sort" : [
                                {
                                    "name" : "Han_Solo",
                                    "dir" : "asc"
                                },
                             ],
                    "where" : [
                        "Luke_Skywalker",
                        "<=",
                        10
                    ]
                }
                );
        testlib.ok(recs.length == 4, "4 results");
        testlib.ok(recs[0].Luke_Skywalker == -5);

        repo.close();
    }

    this.test_query_misc = function()
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
                        "Luke_Skywalker" :
                        {
                            "datatype" : "int"
                        },
                        "Han_Solo" :
                        {
                            "datatype" : "int"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        ztx.commit();

        var t2 = zs.get_template();
        testlib.ok(sg.to_json(t) == sg.to_json(t2));

        ztx = zs.begin_tx();
        ztx.new_record("item", {"Luke_Skywalker" : 76, "Han_Solo" : 22});
        ztx.new_record("item", {"Luke_Skywalker" : 11, "Han_Solo" : 33});
        ztx.new_record("item", {"Luke_Skywalker" : -5, "Han_Solo" : 44});
        ztx.new_record("item", {"Luke_Skywalker" : 50, "Han_Solo" : -11});
        ztx.new_record("item", {"Luke_Skywalker" : 22, "Han_Solo" : -33});
        ztx.new_record("item", {"Luke_Skywalker" : 0, "Han_Solo" : 55});
        ztx.new_record("item", {"Luke_Skywalker" : 1, "Han_Solo" : 66});
        ztx.new_record("item", {"Luke_Skywalker" : -40, "Han_Solo" : 99});

        ztx.commit();

        var ids = zs.query("item", ["recid"]);
        testlib.ok(ids.length == 8, "8 results");

        recs = zs.query("item", ["Luke_Skywalker", "Han_Solo"], null, "Luke_Skywalker #DESC, Han_Solo #ASC");
        testlib.ok(recs.length == 8, "8 results");
        testlib.ok(recs[0].Han_Solo == 22)

        recs = zs.query("item", ["Luke_Skywalker", "Han_Solo"], "Luke_Skywalker <= 10", "Han_Solo #ASC");
        testlib.ok(recs.length == 4, "4 results");
        testlib.ok(recs[0].Luke_Skywalker == -5)

        repo.close();
    }

    this.test_query_reference = function()
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
                        "x" :
                        {
                            "datatype" : "reference",
                            "constraints" :
                            {
                                "rectype" : "item"
                            }
                        },
                        "y" :
                        {
                            "datatype" : "reference",
                            "constraints" :
                            {
                                "rectype" : "item"
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        ztx.commit();

        ztx = zs.begin_tx();
        recid1 = ztx.new_record("item").recid;
        recid2 = ztx.new_record("item").recid;
        recid3 = ztx.new_record("item").recid;
        recid4 = ztx.new_record("item").recid;
        ztx.commit();

        ztx = zs.begin_tx();
        r = ztx.open_record("item", recid1);
        r.x = recid2;
        r.y = recid3;
        r = ztx.open_record("item", recid2);
        r.x = recid2;
        r.y = recid1;
        r = ztx.open_record("item", recid4);
        r.x = recid4;
        r.y = recid1;
        ztx.commit();

        var ids = zs.query("item", ["recid"]);
        testlib.ok(ids.length == 4, "4 results");

        recs = zs.query("item", ["x", "y"], "x == '"+recid2+"'");
        testlib.ok(recs.length == 2, "2 results");

        recs = zs.query("item", ["x", "y"], "y == '"+recid1+"'");
        testlib.ok(recs.length == 2, "2 results");

        recs = zs.query("item", ["x", "y"], "(x == '"+recid2+"') && (y == '"+recid1+"')");
        testlib.ok(recs.length == 1, "1 results");

        recs = zs.query("item", ["x", "y"], "(x == '"+recid2+"') || (y == '"+recid1+"')");
        testlib.ok(recs.length == 3, "3 results");

        repo.close();
    }

    this.test_query_sort_allowed_regular = function()
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
                        "foo" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "allowed" :
                                [
                                    "charlie",
                                    "bravo",
                                    "alpha",
                                    "foxtrot",
                                    "echo",
                                    "delta"
                                ]
                            }
                        },
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        ztx.commit();

        ztx = zs.begin_tx();
        ztx.new_record("item").foo = "foxtrot";
        ztx.new_record("item").foo = "echo";
        ztx.new_record("item").foo = "delta";
        ztx.new_record("item").foo = "charlie";
        ztx.new_record("item").foo = "bravo";
        ztx.new_record("item").foo = "alpha";

        ztx.commit();

        var ids;
        var recs;

        recs = zs.query("item", ["hidrec", "foo"], null, "foo #ASC");
        testlib.ok(recs.length == 6, "6 results");
        testlib.ok(recs[0].foo == "alpha")
        testlib.ok(recs[1].foo == "bravo")
        testlib.ok(recs[2].foo == "charlie")
        testlib.ok(recs[3].foo == "delta")
        testlib.ok(recs[4].foo == "echo")
        testlib.ok(recs[5].foo == "foxtrot")

        repo.close();
    }
    
    this.test_query_sort_by_allowed = function()
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
                        "foo" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "allowed" :
                                [
                                    "charlie",
                                    "bravo",
                                    "alpha",
                                    "foxtrot",
                                    "echo",
                                    "delta"
                                ],
                                "sort_by_allowed" : true
                            }
                        },
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        ztx.commit();

        ztx = zs.begin_tx();
        ztx.new_record("item").foo = "foxtrot";
        ztx.new_record("item").foo = "echo";
        ztx.new_record("item").foo = "delta";
        ztx.new_record("item").foo = "charlie";
        ztx.new_record("item").foo = "bravo";
        ztx.new_record("item").foo = "alpha";

        ztx.commit();

        var ids;
        var recs;

        recs = zs.query("item", ["hidrec", "foo"], null, "foo #ASC");
        testlib.ok(recs.length == 6, "6 results");
        testlib.ok(recs[0].foo == "charlie")
        testlib.ok(recs[1].foo == "bravo")
        testlib.ok(recs[2].foo == "alpha")
        testlib.ok(recs[3].foo == "foxtrot")
        testlib.ok(recs[4].foo == "echo")
        testlib.ok(recs[5].foo == "delta")

        repo.close();
    }

    this.test_query_sort_by_allowed_change = function()
    {
        var t1 =
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
                        "foo" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "allowed" :
                                [
                                    "charlie",
                                    "bravo",
                                    "alpha",
                                    "foxtrot",
                                    "echo",
                                    "delta"
                                ],
                                "sort_by_allowed" : true
                            }
                        },
                    }
                }
            }
        }
        
        var t2 =
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
                        "foo" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "allowed" :
                                [
                                    "charlie",
                                    "foxtrot",
                                    "bravo",
                                    "alpha",
                                    "echo",
                                    "delta"
                                ],
                                "sort_by_allowed" : true
                            }
                        },
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t1);
        ztx.commit();

        ztx = zs.begin_tx();
        ztx.new_record("item").foo = "foxtrot";
        ztx.new_record("item").foo = "echo";
        ztx.new_record("item").foo = "delta";
        ztx.new_record("item").foo = "charlie";
        ztx.new_record("item").foo = "bravo";
        ztx.new_record("item").foo = "alpha";

        var cs0 = ztx.commit();

        var recs;

        recs = zs.query("item", ["hidrec", "foo"], null, "foo #ASC");
        testlib.ok(recs.length == 6, "6 results");
        testlib.ok(recs[0].foo == "charlie")
        testlib.ok(recs[1].foo == "bravo")
        testlib.ok(recs[2].foo == "alpha")
        testlib.ok(recs[3].foo == "foxtrot")
        testlib.ok(recs[4].foo == "echo")
        testlib.ok(recs[5].foo == "delta")

        var ztx = zs.begin_tx();
        ztx.set_template(t2);
        ztx.commit();

        recs = zs.query("item", ["hidrec", "foo"], null, "foo #ASC");
        testlib.ok(recs.length == 6, "6 results");
        testlib.ok(recs[0].foo == "charlie")
        testlib.ok(recs[1].foo == "foxtrot")
        testlib.ok(recs[2].foo == "bravo")
        testlib.ok(recs[3].foo == "alpha")
        testlib.ok(recs[4].foo == "echo")
        testlib.ok(recs[5].foo == "delta")

        recs = zs.query("item", ["hidrec", "foo"], null, "foo #ASC", 0, 0, cs0.csid);
        testlib.ok(recs.length == 6, "6 results");
        testlib.ok(recs[0].foo == "charlie")
        testlib.ok(recs[1].foo == "bravo")
        testlib.ok(recs[2].foo == "alpha")
        testlib.ok(recs[3].foo == "foxtrot")
        testlib.ok(recs[4].foo == "echo")
        testlib.ok(recs[5].foo == "delta")

        repo.close();
    }

    this.test_query_sort_string = function()
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
                        "foo" :
                        {
                            "datatype" : "string"
                        },
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);

        ztx.new_record("item").foo = "charlie";
        ztx.new_record("item").foo = "delta";
        ztx.new_record("item").foo = "foxtrot";
        ztx.new_record("item").foo = "alpha";
        ztx.new_record("item").foo = "bravo";
        ztx.new_record("item").foo = "echo";

        ztx.commit();

        var ids = zs.query("item", ["recid"]);
        testlib.ok(ids.length == 6, "6 results");

        recs = zs.query("item", ["hidrec", "foo"], null, "foo #DESC");
        testlib.ok(recs.length == 6, "6 results");
        testlib.ok(recs[0].foo == "foxtrot")

        recs = zs.query("item", ["hidrec", "foo"], null, "foo #ASC", 10, 2);
        testlib.ok(recs.length == 4, "4 results");
        testlib.ok(recs[0].foo == "charlie")

        repo.close();
    }

    this.test_query_limit_without_sort = function()
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
                        "foo" :
                        {
                            "datatype" : "int"
                        },
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);

        ztx.new_record("item").foo = 5;
        ztx.new_record("item").foo = 8;
        ztx.new_record("item").foo = 11;
        ztx.new_record("item").foo = 17;
        ztx.new_record("item").foo = 26;
        ztx.new_record("item").foo = 33;
        ztx.new_record("item").foo = 34;
        ztx.new_record("item").foo = 65;
        ztx.new_record("item").foo = 77;
        ztx.new_record("item").foo = 104;

        ztx.commit();

        var ids = zs.query("item", ["recid"]);
        testlib.ok(ids.length == 10, "10 results");

        recs = zs.query("item", ["hidrec", "foo"], null, null, 1);
        print(sg.to_json__pretty_print(recs));
        testlib.ok(recs.length == 1, "1 results");

        repo.close();
    }
    
    this.test_query_sort_limit = function()
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
                        "foo" :
                        {
                            "datatype" : "int"
                        },
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);

        ztx.new_record("item").foo = 5;
        ztx.new_record("item").foo = 8;
        ztx.new_record("item").foo = 11;
        ztx.new_record("item").foo = 17;
        ztx.new_record("item").foo = 26;
        ztx.new_record("item").foo = 33;
        ztx.new_record("item").foo = 34;
        ztx.new_record("item").foo = 65;
        ztx.new_record("item").foo = 77;
        ztx.new_record("item").foo = 104;

        ztx.commit();

        var ids = zs.query("item", ["recid"]);
        testlib.ok(ids.length == 10, "10 results");

        recs = zs.query("item", ["hidrec", "foo"], null, "foo #DESC", 10);
        testlib.ok(recs.length == 10, "10 results");
        testlib.ok(recs[0].foo == 104);

        recs = zs.query("item", ["hidrec", "foo"], null, "foo #DESC", 11);
        testlib.ok(recs.length == 10, "10 results");
        testlib.ok(recs[2].foo == 65);

        recs = zs.query("item", ["hidrec", "foo"], null, "foo #DESC", 4);
        testlib.ok(recs.length == 4, "4 results");
        testlib.ok(recs[2].foo == 65);

        repo.close();
    };

	/* Make sure you can use fields you didn't ask for in both WHERE and SORT clauses */
	this.test_query_where_sort_unrequested_fields = function()
	{
		var recs, reponame, repo, zdb, ztx, ids, rec;

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
                        "foo" :
                        {
                            "datatype" : "int"
                        },
						"bar" :
						{
							"datatype" : "string"
						}
                    }
                }
            }
        };

        reponame = sg.gid();
        new_zing_repo(reponame);

        repo = sg.open_repo(reponame);
        zdb = new zingdb(repo, sg.dagnum.TESTING_DB);

        ztx = zdb.begin_tx();
        ztx.set_template(t);

        rec = ztx.new_record("item");
		rec.foo = 5;
		rec.bar = "five";
        rec = ztx.new_record("item");
		rec.foo = 8;
		rec.bar = "eight";
        rec = ztx.new_record("item");
		rec.foo = 11;
		rec.bar = "eleven";
        rec = ztx.new_record("item");
		rec.foo = 17;
		rec.bar = "seventeen";
        rec = ztx.new_record("item");
		rec.foo = 26;
		rec.bar = "twenty-six";
        rec = ztx.new_record("item");
		rec.foo = 33;
		rec.bar = "thirty-three";
        rec = ztx.new_record("item");
		rec.foo = 34;
		rec.bar = "thirty-four";
        rec = ztx.new_record("item");
		rec.foo = 65;
		rec.bar = "sixty-five";
        rec = ztx.new_record("item");
		rec.foo = 77;
		rec.bar = "seventy-seven";
        rec = ztx.new_record("item");
		rec.foo = 104;
		rec.bar = "one hundred four";

        ztx.commit();

        ids = zdb.query("item", ["bar"]);
        testlib.ok(ids.length == 10, ids.length + " results");

        recs = zdb.query("item", ["hidrec","bar"], null, "foo #DESC", 10, 3);
        testlib.ok(recs.length == 7, "7 results");
        testlib.ok(recs[0].bar == "thirty-four");

        recs = zdb.query("item", ["hidrec","bar"], null, "foo #ASC", 10, 9);
        testlib.ok(recs.length == 1, recs.length + " results");
        testlib.ok(recs[0].bar == "one hundred four");

        recs = zdb.query("item", ["hidrec","bar"], "foo >= 11", "foo #ASC", 10, 3);
        testlib.ok(recs.length == 5, "5 results");
        testlib.ok(recs[0].bar == "thirty-three");

        recs = zdb.query("item", ["hidrec","bar"], "foo == 33", "foo #ASC");
        testlib.ok(recs.length == 1, "1 results");
        testlib.ok(recs[0].bar == "thirty-three");

        recs = zdb.query("item", ["hidrec","bar"], "(foo < 10) || (foo > 90)", "foo #DESC");
        testlib.ok(recs.length == 3, "3 results");
        testlib.ok(recs[1].bar == "eight");

        ztx = zdb.begin_tx();
        ztx.set_template(t);

        rec = ztx.new_record("item");
		rec.foo = 0;
		rec.bar = "zero";
        rec = ztx.new_record("item");
		rec.foo = 0;
		rec.bar = "zero zero";
        rec = ztx.new_record("item");
		rec.foo = 0;
		rec.bar = "zero zero zero";
        rec = ztx.new_record("item");
		rec.foo = 0;
		rec.bar = "zero zero zero zero";

        ztx.commit();

		recs = zdb.query("item", ["hidrec","bar"], null, "foo, bar");
		testlib.ok(recs.length == 14, recs.length + " results");
		testlib.ok(recs[0].bar == "zero");
		testlib.ok(recs[3].bar == "zero zero zero zero");

		recs = zdb.query("item", ["hidrec","bar"], null, "foo #ASC, bar #DESC");
		testlib.ok(recs.length == 14, recs.length + " results");
		testlib.ok(recs[3].bar == "zero");
		testlib.ok(recs[0].bar == "zero zero zero zero");

		recs = zdb.query("item", ["hidrec","bar"], null, "foo #DESC, bar #ASC");
		testlib.ok(recs.length == 14, recs.length + " results");
		testlib.ok(recs[10].bar == "zero");
		testlib.ok(recs[13].bar == "zero zero zero zero");

		recs = zdb.query("item", ["hidrec","bar"], null, "foo #DESC, bar #DESC");
		testlib.ok(recs.length == 14, recs.length + " results");
		testlib.ok(recs[13].bar == "zero");
		testlib.ok(recs[10].bar == "zero zero zero zero");

        repo.close();
	};

    this.test_query_sort_skip_new = function()
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
                        "foo" :
                        {
                            "datatype" : "int"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);

        ztx.new_record("item").foo = 5;
        ztx.new_record("item").foo = 8;
        ztx.new_record("item").foo = 11;
        ztx.new_record("item").foo = 17;
        ztx.new_record("item").foo = 26;
        ztx.new_record("item").foo = 33;
        ztx.new_record("item").foo = 34;
        ztx.new_record("item").foo = 65;
        ztx.new_record("item").foo = 77;
        ztx.new_record("item").foo = 104;

        ztx.commit();

        var recs;

        recs = zs.q(
                {
                    "rectype" : "item",
                    "fields" : ["hidrec", "foo"]
                }
                );
        testlib.ok(recs.length == 10, "10 results");

        recs = zs.q(
                {
                    "rectype" : "item",
                    "fields" : ["hidrec", "foo"],
                    "sort" : [
                                {
                                    "name" : "foo",
                                    "dir" : "desc"
                                },
                             ],
                    "limit" : 10,
                    "skip" : 3
                }
                );
        testlib.ok(recs.length == 7, "7 results");
        testlib.ok(recs[0].foo == 34);

        recs = zs.q(
                {
                    "rectype" : "item",
                    "fields" : ["hidrec", "foo"],
                    "sort" : [
                                {
                                    "name" : "foo",
                                    "dir" : "asc"
                                },
                             ],
                    "limit" : 10,
                    "skip" : 9
                }
                );
        testlib.ok(recs.length == 1, "1 results");
        testlib.ok(recs[0].foo == 104);

        recs = zs.q(
                {
                    "rectype" : "item",
                    "fields" : ["hidrec", "foo"],
                    "where" :
                    [   
                        "foo",
                        ">=",
                        11
                    ],
                    "sort" : [
                                {
                                    "name" : "foo",
                                    "dir" : "asc"
                                },
                             ],
                    "limit" : 10,
                    "skip" : 3
                }
                );
        testlib.ok(recs.length == 5, "5 results");
        testlib.ok(recs[0].foo == 33);

        recs = zs.q(
                {
                    "rectype" : "item",
                    "fields" : ["hidrec", "foo"],
                    "where" :
                    [   
                        "foo",
                        "==",
                        33
                    ],
                    "sort" : [
                                {
                                    "name" : "foo",
                                    "dir" : "asc"
                                },
                             ]
                }
                );
        testlib.ok(recs.length == 1, "1 results");
        testlib.ok(recs[0].foo == 33);

        recs = zs.q(
                {
                    "rectype" : "item",
                    "fields" : ["hidrec", "foo"],
                    "where" :
                    [
                    [   
                        "foo",
                        "<",
                        10
                    ],
                    "||",
                    [   
                        "foo",
                        ">",
                        90
                    ]
                    ],
                    "sort" : [
                                {
                                    "name" : "foo",
                                    "dir" : "desc"
                                },
                             ]
                }
                );
        testlib.ok(recs.length == 3, "3 results");
        testlib.ok(recs[1].foo == 8);

        repo.close();
    };

    this.test_query_sort_skip = function()
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
                        "foo" :
                        {
                            "datatype" : "int"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);

        ztx.new_record("item").foo = 5;
        ztx.new_record("item").foo = 8;
        ztx.new_record("item").foo = 11;
        ztx.new_record("item").foo = 17;
        ztx.new_record("item").foo = 26;
        ztx.new_record("item").foo = 33;
        ztx.new_record("item").foo = 34;
        ztx.new_record("item").foo = 65;
        ztx.new_record("item").foo = 77;
        ztx.new_record("item").foo = 104;

        ztx.commit();

        var ids = zs.query("item", ["recid"]);
        testlib.ok(ids.length == 10, "10 results");

        recs = zs.query("item", ["hidrec", "foo"], null, "foo #DESC", 10, 3);
        testlib.ok(recs.length == 7, "7 results");
        testlib.ok(recs[0].foo == 34);

        recs = zs.query("item", ["hidrec", "foo"], null, "foo #ASC", 10, 9);
        testlib.ok(recs.length == 1, "1 results");
        testlib.ok(recs[0].foo == 104);

        recs = zs.query("item", ["hidrec", "foo"], "foo >= 11", "foo #ASC", 10, 3);
        testlib.ok(recs.length == 5, "5 results");
        testlib.ok(recs[0].foo == 33);

        recs = zs.query("item", ["hidrec", "foo"], "foo == 33", "foo #ASC");
        testlib.ok(recs.length == 1, "1 results");
        testlib.ok(recs[0].foo == 33);

        recs = zs.query("item", ["hidrec", "foo"], "(foo < 10) || (foo > 90)", "foo #DESC");
        testlib.ok(recs.length == 3, "3 results");
        testlib.ok(recs[1].foo == 8);

        repo.close();
    };

    this.test_queries_or = function()
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
                        "foo" :
                        {
                            "datatype" : "int"
                        },
                        "bar" :
                        {
                            "datatype" : "string"
                        },
                        "yum" :
                        {
                            "datatype" : "int"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);

        var rec = ztx.new_record("item");
        rec.foo = 5;
        rec.bar = "red";
        rec.yum = 72;

        var rec = ztx.new_record("item");
        rec.foo = 7;
        rec.bar = "red";
        rec.yum = 21;

        var rec = ztx.new_record("item");
        rec.foo = 7;
        rec.bar = "red";
        rec.yum = 61;

        var rec = ztx.new_record("item");
        rec.foo = 7;
        rec.bar = "yellow";
        rec.yum = 0;

        var rec = ztx.new_record("item");
        rec.foo = 9;
        rec.bar = "red";
        rec.yum = 15;

        var rec = ztx.new_record("item");
        rec.foo = 11;
        rec.bar = "blue";
        rec.yum = 44;

        var rec = ztx.new_record("item");
        rec.foo = 31;
        rec.bar = "blue";
        rec.yum = 33;

        ztx.commit();

        var ids = zs.query("item", ["recid"]);
        testlib.ok(ids.length == 7, "7 results");

        ids = zs.query("item", ["hidrec"], "(foo == 7) || (yum > 5000)");
        testlib.ok(ids.length == 3, "3 results");

        ids = zs.query("item", ["hidrec"], "(foo == 238746) || (bar == 'yellow')");
        testlib.ok(ids.length == 1, "1 results");

        repo.close();
    }

    this.test_queries_1 = function()
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
                        "foo" :
                        {
                            "datatype" : "int"
                        },
                        "bar" :
                        {
                            "datatype" : "string"
                        },
                        "yum" :
                        {
                            "datatype" : "int"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);

        var rec = ztx.new_record("item");
        rec.foo = 5;
        rec.bar = "red";
        rec.yum = 72;

        var rec = ztx.new_record("item");
        rec.foo = 7;
        rec.bar = "red";
        rec.yum = 21;

        var rec = ztx.new_record("item");
        rec.foo = 7;
        rec.bar = "red";
        rec.yum = 61;

        var rec = ztx.new_record("item");
        rec.foo = 7;
        rec.bar = "yellow";
        rec.yum = 0;

        var rec = ztx.new_record("item");
        rec.foo = 9;
        rec.bar = "red";
        rec.yum = 15;

        var rec = ztx.new_record("item");
        rec.foo = 11;
        rec.bar = "blue";
        rec.yum = 44;

        var rec = ztx.new_record("item");
        rec.foo = 31;
        rec.bar = "blue";
        rec.yum = 33;

        ztx.commit();

        var ids = zs.query("item", ["recid"]);
        testlib.ok(ids.length == 7, "7 results");

        recs = zs.query("item", ["bar", "yum"], "foo < 10", "foo,yum");
        print(sg.to_json(recs));
        testlib.ok(recs.length == 5, "5 results");
        testlib.ok(recs[1].bar == "yellow")
        testlib.ok(recs[0].yum == 72)
        testlib.ok(recs[3].yum == 61)

        recs = zs.query("item", ["hidrec", "yum"], "(foo == 7) && (yum > 50)", "yum #DESC");
        testlib.ok(recs.length == 1, "1 results");
        testlib.ok(recs[0].yum == 61)

        repo.close();
    }

    this.test_query_squares = function()
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
                        "n" :
                        {
                            "datatype" : "int"
                        },
                        "nsquared" :
                        {
                            "datatype" : "int"
                        },
                        "color":
                        {
                            "datatype" : "string"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        ztx.commit();

        ztx = zs.begin_tx();
        for (i=1; i<=27; i++)
        {
            var rec = ztx.new_record("item");
            rec.n = i;
            rec.nsquared = i*i;
            if (0 == (i % 4))
            {
                rec.color = "red";
            }
            else
            {
                rec.color = "white";
            }
        }
        ztx.commit();

        var ids = zs.query("item", ["recid"]);
        testlib.ok(ids.length == 27, "27 results");

        ids = zs.query("item", ["hidrec"], "nsquared > 190", "n");
        testlib.ok(ids.length == 14, "14 results");

        recs = zs.query("item", ["hidrec", "n"], "nsquared==225");
        testlib.ok(recs.length == 1, "1 results");
        testlib.ok(recs[0].n == 15)

        ids = zs.query("item", ["hidrec"], "(n>16) && (color=='red')");
        print(ids.length);
        testlib.ok(ids.length == 2, "2 results");

        repo.close();
    }

    this.test_query_bools = function()
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
                        "n" :
                        {
                            "datatype" : "int"
                        },
                        "b" :
                        {
                            "datatype" : "bool"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        ztx.commit();

        var rec;

        ztx = zs.begin_tx();

        rec = ztx.new_record("item");
        rec.n = 2;
        rec.b = true;

        rec = ztx.new_record("item");
        rec.n = 3;
        rec.b = true;

        rec = ztx.new_record("item");
        rec.n = 4;
        rec.b = false;

        rec = ztx.new_record("item");
        rec.n = 5;
        rec.b = true;

        rec = ztx.new_record("item");
        rec.n = 6;
        rec.b = false;

        rec = ztx.new_record("item");
        rec.n = 7;
        rec.b = true;

        rec = ztx.new_record("item");
        rec.n = 8;
        rec.b = false;

        rec = ztx.new_record("item");
        rec.n = 9;
        rec.b = false;

        rec = ztx.new_record("item");
        rec.n = 10;
        rec.b = false;

        rec = ztx.new_record("item");
        rec.n = 11;
        rec.b = true;

        rec = ztx.new_record("item");
        rec.n = 12;
        rec.b = false;

        rec = ztx.new_record("item");
        rec.n = 13;
        rec.b = true;

        rec = ztx.new_record("item");
        rec.n = 14;
        rec.b = false;

        ztx.commit();

        var ids = zs.query("item", ["recid"]);
        testlib.ok(ids.length == 13, "13 results");

        ids = zs.query("item", ["hidrec"], "b == #T");
        testlib.ok(ids.length == 6, "6 results");

        ids = zs.query("item", ["hidrec"], "b == #TRUE");
        testlib.ok(ids.length == 6, "6 results");

        ids = zs.query("item", ["hidrec"], "b == #F");
        testlib.ok(ids.length == 7, "7 results");

        ids = zs.query("item", ["hidrec"], "b == #FALSE");
        testlib.ok(ids.length == 7, "7 results");

        ids = zs.query("item", ["hidrec"], "b != #F");
        testlib.ok(ids.length == 6, "6 results");

        ids = zs.query("item", ["hidrec"], "b != #T");
        testlib.ok(ids.length == 7, "7 results");

        ids = zs.query("item", ["hidrec"], "(b == #T) && (n < 4)");
        testlib.ok(ids.length == 2, "2 results");

        repo.close();
    }

    this.test_u0018_multiple_states = function()
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
                        "r" :
                        {
                            "datatype" : "int"
                        },
                        "x" :
                        {
                            "datatype" : "int"
                        },
                        "y" :
                        {
                            "datatype" : "int"
                        },
                        "z" :
                        {
                            "datatype" : "int"
                        },
                        "w" :
                        {
                            "datatype" : "int"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        ztx.commit();

        ztx = zs.begin_tx();

        var rec1 = ztx.new_record("item");
        var recid1 = rec1.recid;
        rec1.r = 1;
        rec1.x = 4;
        rec1.y = 1;
        rec1.z = -4;

        var rec2 = ztx.new_record("item");
        var recid2 = rec2.recid;
        rec2.r = 2;
        rec2.x = 4;
        rec2.y = 0;
        rec2.z = 24;

        var rec3 = ztx.new_record("item");
        var recid3 = rec3.recid;
        rec3.r = 3;
        rec3.x = 4;
        rec3.y = 1;
        rec3.z = 11;

        var cs = [];
        cs[0] = null;
        cs[1] = ztx.commit().csid;

        ztx = zs.begin_tx(cs[1]);

        var rec4 = ztx.new_record("item");
        var recid4 = rec4.recid;
        rec4.r = 4;
        rec4.x = 4;
        rec4.y = 0;
        rec4.z = -4;

        var rec5 = ztx.new_record("item");
        var recid5 = rec5.recid;
        rec5.r = 5;
        rec5.x = 4;
        rec5.y = 1;
        rec5.z = 24;

        var rec6 = ztx.new_record("item");
        var recid6 = rec6.recid;
        rec6.r = 6;
        rec6.x = 4;
        rec6.y = 0;
        rec6.z = 11;

        cs[2] = ztx.commit().csid;

        ztx = zs.begin_tx(cs[1]);

        var rec7 = ztx.new_record("item");
        var recid7 = rec7.recid;
        rec7.r = 7;
        rec7.x = 4;
        rec7.y = 1;
        rec7.z = -4;

        var rec8 = ztx.new_record("item");
        var recid8 = rec8.recid;
        rec8.r = 8;
        rec8.x = 4;
        rec8.y = 0;
        rec8.z = 24;

        cs[3] = ztx.commit().csid;

        ztx = zs.begin_tx(cs[2]);

        var rec9 = ztx.new_record("item");
        var recid9 = rec9.recid;
        rec9.r = 9;
        rec9.x = 4;
        rec9.y = 1;
        rec9.z = -4;

        ztx.delete_record("item", recid3);

        rec4 = ztx.open_record("item", recid4);
        rec4.w = 1968;

        cs[4] = ztx.commit().csid;

        //verify(zs, "r==4", cs, 0, 2);
        verify(zs, "r==4", cs, 1, 0);
        verify(zs, "r==4", cs, 2, 1);
        verify(zs, "r==4", cs, 3, 0);
        verify(zs, "r==4", cs, 4, 1);

        //verify(zs, "r==7", cs, 0, 1);
        verify(zs, "r==7", cs, 1, 0);
        verify(zs, "r==7", cs, 2, 0);
        verify(zs, "r==7", cs, 3, 1);
        verify(zs, "r==7", cs, 4, 0);

        //verify(zs, "r==3", cs, 0, 1);
        verify(zs, "r==3", cs, 1, 1);
        verify(zs, "r==3", cs, 2, 1);
        verify(zs, "r==3", cs, 3, 1);
        verify(zs, "r==3", cs, 4, 0);

        //verify(zs, "y==0", cs, 0, 5);
        verify(zs, "y==0", cs, 1, 1);
        verify(zs, "y==0", cs, 2, 3);
        verify(zs, "y==0", cs, 3, 2);
        verify(zs, "y==0", cs, 4, 3);

        //verify(zs, "y==1", cs, 0, 5);
        verify(zs, "y==1", cs, 1, 2);
        verify(zs, "y==1", cs, 2, 3);
        verify(zs, "y==1", cs, 3, 3);
        verify(zs, "y==1", cs, 4, 3);

        repo.close();
    }

    this.test_u0033_lots = function()
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
                        "k" :
                        {
                            "datatype" : "int"
                        },
                        "i" :
                        {
                            "datatype" : "int"
                        },
                        "n" :
                        {
                            "datatype" : "int"
                        },
                        "q" :
                        {
                            "datatype" : "int"
                        },
                        "which" :
                        {
                            "datatype" : "string"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        ztx.commit();

        var k;

        for (k=0; k<10; k++)
        {
            ztx = zs.begin_tx();

            var i;

            for (i=0; i<10; i++)
            {
                var n = k * 1000 + i;

                ztx.new_record("item", {"k":k, "i":i, "n":n, "q": n%13, "which": (n%2 ? "odd" : "even")});
                /*
                var recid = rec.recid;

                rec.k = k;
                rec.i = i;
                rec.n = n;
                rec.q = n % 13;
                rec.which = n%2 ? "odd" : "even";
                */
            }

            ztx.commit();
        }

        ids = zs.query("item", ["hidrec"], "(q == 7) && (k == 8)");
        testlib.ok(ids.length == 1, "1 results");

        repo.close();
    }

    this.test_history = function()
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
                            "datatype" : "int"
                        },
                        "name" :
                        {
                            "datatype" : "string"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        ztx.commit();
        // 0

        ztx = zs.begin_tx();
        var recid_a = ztx.new_record("item", {"val" : 5, "name" : "a"}).recid;
        var recid_b = ztx.new_record("item", {"val" : 21, "name" : "b"}).recid;
        var cs1 = ztx.commit();
        print("cs1:");
        print(sg.to_json__pretty_print(cs1));

        ztx = zs.begin_tx();
        var recid_c = ztx.new_record("item", {"val" : 11, "name" : "c"}).recid;
        var recid_d = ztx.new_record("item", {"val" : 13, "name" : "d"}).recid;
        r = ztx.open_record("item", recid_a);
        r.val = 15;
        var cs2 = ztx.commit();
        print("cs2:");
        print(sg.to_json__pretty_print(cs2));

        ztx = zs.begin_tx();
        r = ztx.open_record("item", recid_a);
        r.val = 6;
        r = ztx.open_record("item", recid_c);
        r.val = 6;
        var cs3 = ztx.commit();
        print("cs3:");
        print(sg.to_json__pretty_print(cs3));

        ztx = zs.begin_tx();
        r = ztx.open_record("item", recid_a);
        r.val = 87;
        r = ztx.open_record("item", recid_b);
        r.val = 543;
        var cs4 = ztx.commit();
        print("cs4:");
        print(sg.to_json__pretty_print(cs4));

        ztx = zs.begin_tx();
        r = ztx.open_record("item", recid_a);
        r.val = 11;
        r = ztx.open_record("item", recid_b);
        r.val = 43;
        var cs5 = ztx.commit();
        print("cs5:");
        print(sg.to_json__pretty_print(cs5));

        ztx = zs.begin_tx();
        r = ztx.open_record("item", recid_a);
        r.val = 876;
        var cs6 = ztx.commit();
        print("cs6:");
        print(sg.to_json__pretty_print(cs6));

        var res = zs.query("item", ["val", "name", "recid", "history"], null, "name #DESC");
        print(sg.to_json__pretty_print(res));
        testlib.ok(res.length == 4);
        testlib.ok(res[0].recid == recid_d);
        testlib.ok(res[1].recid == recid_c);
        testlib.ok(res[2].recid == recid_b);
        testlib.ok(res[3].recid == recid_a);

        testlib.ok(res[0].history.length == 1);
        testlib.ok(res[1].history.length == 2);
        testlib.ok(res[2].history.length == 3);
        testlib.ok(res[3].history.length == 6);

        testlib.ok(res[0].history[0].csid == cs2.csid);

        testlib.ok(res[1].history[0].csid == cs3.csid);
        testlib.ok(res[1].history[1].csid == cs2.csid);

        testlib.ok(res[2].history[0].csid == cs5.csid);
        testlib.ok(res[2].history[1].csid == cs4.csid);
        testlib.ok(res[2].history[2].csid == cs1.csid);

        testlib.ok(res[3].history[0].csid == cs6.csid);
        testlib.ok(res[3].history[1].csid == cs5.csid);
        testlib.ok(res[3].history[2].csid == cs4.csid);
        testlib.ok(res[3].history[3].csid == cs3.csid);
        testlib.ok(res[3].history[4].csid == cs2.csid);
        testlib.ok(res[3].history[5].csid == cs1.csid);

        print("-------- last_timestamp");
        var t1 = sg.time();
        var res_last = zs.query("item", ["val", "name", "recid", "last_timestamp"], null, "last_timestamp #ASC");
        var t2 = sg.time();
        print(sg.to_json__pretty_print(res_last));
        print(t2 - t1, " ms");

        repo.close();
    }

    this.test_slice = function()
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
                        "foo" :
                        {
                            "datatype" : "int"
                        },
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        ztx.commit();
        // 0

        ztx = zs.begin_tx();
        var recid_small = ztx.new_record("item", {"foo" : 5}).recid;
        ztx.new_record("item").foo = 16;
        ztx.new_record("item").foo = 23;
        var cs1 = ztx.commit();
        // + 1 = 1

        ztx = zs.begin_tx();
        ztx.new_record("item").foo = 16;
        ztx.new_record("item").foo = 16;
        ztx.new_record("item").foo = 17;
        ztx.new_record("item").foo = 15;
        var cs2 = ztx.commit();
        // + 2 = 3

        ztx = zs.begin_tx();
        var recid_deleteme = ztx.new_record("item", {"foo" : 16}).recid;
        var cs3 = ztx.commit();
        // + 1 = 4

        ztx = zs.begin_tx();
        ztx.new_record("item").foo = 15;
        ztx.new_record("item").foo = 12;
        var cs4 = ztx.commit();
        // + 0 = 4

        ztx = zs.begin_tx();
        ztx.delete_record("item", recid_deleteme);
        var cs5 = ztx.commit();
        // - 1 = 3

        ztx = zs.begin_tx();
        ztx.new_record("item").foo = 11;
        ztx.new_record("item").foo = 16;
        ztx.new_record("item").foo = 16;
        var cs6 = ztx.commit();
        // + 2 = 5

        var res = zs.query("item", ["foo", "recid", "history"], "foo < 9");
        testlib.ok(res.length == 1);
        testlib.ok(res[0].recid == recid_small);

        repo.close();
    }

    this.test_history_for_something_that_already_existed = function()
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
                        "foo" :
                        {
                            "datatype" : "int"
                        },
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        ztx.commit();

        ztx = zs.begin_tx();
        var recid = ztx.new_record("item", {"foo" : 5}).recid;
        var cs1 = ztx.commit();

        var res = zs.query("item", ["foo", "recid", "history"]);
        testlib.ok(res.length == 1);
        testlib.ok(res[0].recid == recid);
        testlib.ok(res[0].foo == 5);
        testlib.ok(res[0].history.length == 1);

        ztx = zs.begin_tx();
        r = ztx.open_record("item", recid);
        r.foo = 7;
        ztx.commit();

        var res = zs.query("item", ["foo", "recid", "history"]);
        testlib.ok(res.length == 1);
        testlib.ok(res[0].recid == recid);
        testlib.ok(res[0].foo == 7);
        testlib.ok(res[0].history.length == 2);

        ztx = zs.begin_tx();
        r = ztx.open_record("item", recid);
        r.foo = 5;
        ztx.commit();

        var res = zs.query("item", ["foo", "recid", "history"]);
        print(sg.to_json__pretty_print(res));
        testlib.ok(res.length == 1);
        testlib.ok(res[0].recid == recid);
        testlib.ok(res[0].foo == 5);
        testlib.ok(res[0].history.length == 3);

        repo.close();
    }
    
    this.test_history_and_audits_issues = function()
    {
        var t =
        {
            "version" : 1,
            "rectypes" :
            {
                "item" :
                {
                    "fields" :
                    {
                        "foo" :
                        {
                            "datatype" : "int"
                        },
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var dagnum = sg.make_dagnum(
                    {
                        "type" : "db",
                        "vendor" : 1,
                        "grouping" : 7,
                        "id" : 7,
                        "trivial" : true
                    }
                    );
        print("dagnum: ", dagnum);

        var zs = new zingdb(repo, dagnum);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var cs0 = ztx.commit();

        ztx = zs.begin_tx(cs0.csid);
        ztx.new_record("item", {"foo" : 5});
        var cs1 = ztx.commit();

        var res = zs.query("item", ["hidrec", "foo", "history"]);
        testlib.ok(res.length == 1);
        var hidrec = res[0].hidrec;
        testlib.ok(res[0].foo == 5);
        testlib.ok(res[0].history.length == 1);
        testlib.ok(res[0].history[0].csid == cs1.csid);
        testlib.ok(res[0].history[0].audits.length ==1);

        ztx = zs.begin_tx(cs0.csid);
        ztx.new_record("item", {"foo" : 11});
        var cs2 = ztx.commit();

        testlib.ok(zs.get_leaves().length==2, "two leaves");
        var cs3 = zs.merge();
        testlib.ok(zs.get_leaves().length==1, "one leaf");

        var res = zs.query("item", ["hidrec", "foo", "history"], "foo==5");
        print(sg.to_json__pretty_print(res));
        testlib.ok(res.length == 1);
        testlib.ok(res[0].hidrec == hidrec);
        testlib.ok(res[0].history.length == 1);

        print("-------- hidrec: ", hidrec);

        var cset1 = repo.fetch_json(cs1.csid);
        print("-------- cs1");
        print(sg.to_json__pretty_print(cset1));

        var cset2 = repo.fetch_json(cs2.csid);
        print("-------- cs2");
        print(sg.to_json__pretty_print(cset2));

        var cset3 = repo.fetch_json(cs3.csid);
        print("-------- cs3");
        print(sg.to_json__pretty_print(cset3));

        testlib.ok(num_keys(cset1.db.changes[cs0.csid].add) == 1);
        testlib.ok(hidrec in cset1.db.changes[cs0.csid].add);

        testlib.ok(num_keys(cset2.db.changes[cs0.csid].add) == 1);
        testlib.ok((!(hidrec in cset2.db.changes[cs0.csid].add)));

        repo.close();
    }
    
    this.test_multiple_changesets_trivial = function()
    {
        var t =
        {
            "version" : 1,
            "rectypes" :
            {
                "item" :
                {
                    "fields" :
                    {
                        "foo" :
                        {
                            "datatype" : "int"
                        },
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var dagnum = sg.make_dagnum(
                    {
                        "type" : "db",
                        "vendor" : 1,
                        "grouping" : 7,
                        "id" : 7,
                        "trivial" : true
                    }
                    );
        var zs = new zingdb(repo, dagnum); 

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var cs0 = ztx.commit();

        // create two records
        ztx = zs.begin_tx(cs0.csid);
        ztx.new_record("item", {"foo" : 5});
        ztx.new_record("item", {"foo" : 7});
        var cs1 = ztx.commit();

        // branch the dag, creating 2 records, one of which is the same
        ztx = zs.begin_tx(cs0.csid);
        ztx.new_record("item", {"foo" : 5});
        ztx.new_record("item", {"foo" : 13});
        var cs2 = ztx.commit();

        // merge it
        testlib.ok(zs.get_leaves().length==2, "two leaves");
        var cs3 = zs.merge();
        testlib.ok(zs.get_leaves().length==1, "one leaf");

        // query
        var res = zs.query("item", ["foo", "history"], null, "foo #ASC");
        testlib.ok(res.length == 3);
        print("-------- res: ");
        print(sg.to_json__pretty_print(res));

        testlib.ok(res[0].foo == 5);
        testlib.ok(res[0].history.length == 2);
        testlib.ok(res[0].history[0].audits.length = 1);
        testlib.ok(res[0].history[1].audits.length = 1);

        testlib.ok(res[1].foo == 7);
        testlib.ok(res[1].history.length == 1);
        testlib.ok(res[1].history[0].audits.length = 1);

        testlib.ok(res[2].foo == 13);
        testlib.ok(res[2].history.length == 1);
        testlib.ok(res[2].history[0].audits.length = 1);

        // redo the query with a skip
        var res = zs.query("item", ["foo", "history"], null, "foo #ASC", 5, 1);
        testlib.ok(res.length == 2);
        print("-------- res: ");
        print(sg.to_json__pretty_print(res));

        testlib.ok(res[0].foo == 7);
        testlib.ok(res[0].history.length == 1);

        testlib.ok(res[1].foo == 13);
        testlib.ok(res[1].history.length == 1);

        // query but sort the other way
        var res = zs.query("item", ["foo", "history"], null, "foo #DESC");
        testlib.ok(res.length == 3);
        print("-------- res: ");
        print(sg.to_json__pretty_print(res));

        testlib.ok(res[2].foo == 5);
        testlib.ok(res[2].history.length == 2);

        testlib.ok(res[1].foo == 7);
        testlib.ok(res[1].history.length == 1);

        testlib.ok(res[0].foo == 13);
        testlib.ok(res[0].history.length == 1);

        // query but skip them all
        var res = zs.query("item", ["foo", "history"], null, "foo #DESC", 5, 3);
        testlib.ok(res.length == 0);
        print("-------- res: ");
        print(sg.to_json__pretty_print(res));

        // query but skip them all, overdoing it
        var res = zs.query("item", ["foo", "history"], null, "foo #DESC", 5, 83);
        testlib.ok(res.length == 0);
        print("-------- res: ");
        print(sg.to_json__pretty_print(res));

        // re-do the main query
        var res = zs.query("item", ["foo", "history"], null, "foo #ASC");
        testlib.ok(res.length == 3);
        print("-------- res: ");
        print(sg.to_json__pretty_print(res));

        testlib.ok(res[0].foo == 5);
        testlib.ok(res[0].history.length == 2);
        testlib.ok(res[0].history[0].audits.length = 1);
        testlib.ok(res[0].history[1].audits.length = 1);

        testlib.ok(res[1].foo == 7);
        testlib.ok(res[1].history.length == 1);
        testlib.ok(res[1].history[0].audits.length = 1);

        testlib.ok(res[2].foo == 13);
        testlib.ok(res[2].history.length == 1);
        testlib.ok(res[2].history[0].audits.length = 1);

        // re-do the main query
        var res = zs.query("item", ["foo", "history"], null, "foo #ASC");
        testlib.ok(res.length == 3);
        print("-------- res: ");
        print(sg.to_json__pretty_print(res));

        testlib.ok(res[0].foo == 5);
        testlib.ok(res[0].history.length == 2);
        testlib.ok((res[0].history[0].audits.length + res[0].history[1].audits.length) == 2);

        testlib.ok(res[1].foo == 7);
        testlib.ok(res[1].history.length == 1);
        testlib.ok(res[1].history[0].audits.length = 1);

        testlib.ok(res[2].foo == 13);
        testlib.ok(res[2].history.length == 1);
        testlib.ok(res[2].history[0].audits.length = 1);

        // re-do the main query, skip the first one
        var res = zs.query("item", ["foo", "history"], null, "foo #ASC", 2, 1);
        testlib.ok(res.length == 2);
        print("-------- res: ");
        print(sg.to_json__pretty_print(res));

        testlib.ok(res[0].foo == 7);
        testlib.ok(res[0].history.length == 1);
        testlib.ok(res[0].history[0].audits.length = 1);

        testlib.ok(res[1].foo == 13);
        testlib.ok(res[1].history.length == 1);
        testlib.ok(res[1].history[0].audits.length = 2);

        repo.close();
    }
    
    this.test_record_modified_multiple_times = function()
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
                        "foo" :
                        {
                            "datatype" : "int"
                        },
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var dagnum = sg.dagnum.TESTING_DB;
        var zs = new zingdb(repo, dagnum); 

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var cs0 = ztx.commit();

        ztx = zs.begin_tx(cs0.csid);
        var recid = ztx.new_record("item", {"foo" : 5}).recid;
        var cs1 = ztx.commit();

        // now modify the record
        ztx = zs.begin_tx();
        r = ztx.open_record("item", recid);
        r.foo = 31;
        var cs_mod = ztx.commit();

        ztx = zs.begin_tx();
        r = ztx.open_record("item", recid);
        r.foo = 4;
        var cs_mod = ztx.commit();

        ztx = zs.begin_tx();
        r = ztx.open_record("item", recid);
        r.foo = 88;
        var cs_mod = ztx.commit();

        ztx = zs.begin_tx();
        r = ztx.open_record("item", recid);
        r.foo = 33;
        var cs_mod = ztx.commit();

        print("-------- history");
        var res = zs.query("item", ["recid", "foo", "history"]);
        testlib.ok(res.length == 1);
        print(sg.to_json__pretty_print(res));
        testlib.ok(res[0].history.length == 5);

        var res = zs.query__recent("item", ["hidrec", "*"]);
        print(sg.to_json__pretty_print(res));
        testlib.ok(res.length == 5);
        testlib.ok(res[0].foo == 33);
        testlib.ok(res[1].foo == 88);
        testlib.ok(res[2].foo == 4);
        testlib.ok(res[3].foo == 31);
        testlib.ok(res[4].foo == 5);

        var res = zs.query__recent("item", ["hidrec", "*"], "foo > 30");
        print(sg.to_json__pretty_print(res));
        testlib.ok(res.length == 3);
        testlib.ok(res[0].foo == 33);
        testlib.ok(res[1].foo == 88);
        testlib.ok(res[2].foo == 31);

        repo.close();
    }
    
    this.test_single_rectype = function()
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
                        "foo" :
                        {
                            "datatype" : "int"
                        },
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var dagnum = sg.make_dagnum(
                    {
                        "type" : "db",
                        "vendor" : 1,
                        "grouping" : 7,
                        "id" : 7,
                        "single_rectype" : true
                    }
                    );
        var zs = new zingdb(repo, dagnum); 

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var cs0 = ztx.commit();

        ztx = zs.begin_tx(cs0.csid);
        var recid = ztx.new_record("item", {"foo" : 5}).recid;
        var cs1 = ztx.commit();

        var res = zs.query("item", ["recid", "foo"]);
        testlib.ok(res.length == 1);
        print(sg.to_json__pretty_print(res));

        var res = zs.query(null, ["recid", "foo"]);
        testlib.ok(res.length == 1);
        print(sg.to_json__pretty_print(res));

        var err = "";
        try
        {
            zs.query("wrong", ["recid", "foo"]);
        }
        catch (e)
        {
            err = e.toString();
        }
        testlib.ok(err.length > 0);

        repo.close();
    }

    this.test_username = function()
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
                        "who" :
                        {
                            "datatype" : "userid"
                        },
                        "desc" :
                        {
                            "datatype" : "string"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
	sg.vv2.init_new_repo({"repo":reponame, "no-wc":true});

        var repo = sg.open_repo(reponame);
        var elrond = repo.create_user("Elrond");
        repo.set_user("Elrond");
        repo.close();

        var repo = sg.open_repo(reponame);

        var gandalf = repo.create_user("Gandalf");
        var aragorn = repo.create_user("Aragorn");
        var frodo = repo.create_user("Frodo");

        var dagnum = sg.dagnum.TESTING_DB;
        var zs = new zingdb(repo, dagnum); 

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var cs0 = ztx.commit();

        ztx = zs.begin_tx(cs0.csid);
        ztx.new_record("item", {"who" : gandalf, "desc" : "Wizard"});
        ztx.new_record("item", {"who" : aragorn, "desc" : "Man"});
        ztx.new_record("item", {"who" : frodo, "desc" : "Hobbit"});
        var cs1 = ztx.commit();

        var res = zs.query("item", [
                {
                    "type":"username",
                    "userid":"who",
                    "alias":"uname"
                },
                "desc",
                "history"
                ],
                null,
                "uname #ASC"
                );
        testlib.ok(res.length == 3);
        testlib.ok(res[0].uname == "Aragorn");
        testlib.ok(res[1].uname == "Frodo");
        testlib.ok(res[2].uname == "Gandalf");
        print(sg.to_json__pretty_print(res));

        var res = zs.query("item", [
                {
                    "type":"alias",
                    "field":"desc",
                    "alias":"type"
                },
                "desc",
                ],
                null,
                "type #DESC"
                );
        testlib.ok(res[0].type == "Wizard");
        testlib.ok(res[0].desc == "Wizard");
        testlib.ok(res[1].type == "Man");
        testlib.ok(res[1].desc == "Man");
        testlib.ok(res[2].type == "Hobbit");
        testlib.ok(res[2].desc == "Hobbit");
        print(sg.to_json__pretty_print(res));
        repo.close();
    }

}

