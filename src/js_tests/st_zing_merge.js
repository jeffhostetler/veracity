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

function err_must_contain(err, s)
{
    testlib.ok(-1 != err.indexOf(s), s);
}

function st_zing_merge()
{
    this.test_L0542 = function()
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
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var csid_0 = ztx.commit().csid;
        print("L0542 csid_0");
        print(sg.to_json__pretty_print(repo.fetch_json(csid_0)));

        ztx = zs.begin_tx(csid_0);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.val = 17;
        var csid_1 = ztx.commit().csid;
        print("L0542 csid_1");
        var cs_1 = repo.fetch_json(csid_1);
        print(sg.to_json__pretty_print(cs_1));
        var hidrec = get_keys(cs_1.db.changes[csid_0].add)[0];
        print("L0542 hidrec: ", hidrec);

        ztx = zs.begin_tx(csid_1);
        zrec = ztx.open_record("item", recid);
        zrec.val = 34;
        var csid_2a = ztx.commit().csid;
        print("L0542 csid_2a");
        var cs_2a = repo.fetch_json(csid_2a);
        print(sg.to_json__pretty_print(cs_2a));

        testlib.ok(hidrec in cs_2a.db.changes[csid_1].remove);

        ztx = zs.begin_tx(csid_1);
        ztx.delete_record("item", recid);
        var csid_2b = ztx.commit().csid;
        print("L0542 csid_2b");
        var cs_2b = repo.fetch_json(csid_2b);
        print(sg.to_json__pretty_print(cs_2b));

        testlib.ok(hidrec in cs_2b.db.changes[csid_1].remove);

        testlib.ok(zs.get_leaves().length==2, "two leaves");
        var csid_merge = zs.merge().csid;
        print("L0542 merge");
        var cs_merge = repo.fetch_json(csid_merge);
        print(sg.to_json__pretty_print(cs_merge));
        testlib.ok(zs.get_leaves().length==1, "one leaf");

        testlib.ok(!("add" in cs_merge.db.changes[csid_2a]));
        testlib.ok(!("add" in cs_merge.db.changes[csid_2b]));

        repo.close();
        testlib.ok(true, "No errors");
    }

    this.test_both_add_one_record = function()
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
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        var zrec = ztx.new_record("item");
        var recid1 = zrec.recid;
        zrec.val = 17;
        ztx.commit();

        ztx = zs.begin_tx(csid);
        zrec = ztx.new_record("item");
        var recid2 = zrec.recid;
        zrec.val = 13;
        ztx.commit();

        testlib.ok(zs.get_leaves().length==2, "two leaves");
        zs.merge();
        testlib.ok(zs.get_leaves().length==1, "one leaf");

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid1);
        testlib.ok(17 == zrec.val, "val check");
        zrec = ztx.open_record("item", recid2);
        testlib.ok(13 == zrec.val, "val check");
        ztx.abort();

        repo.close();
        testlib.ok(true, "No errors");
    };

    this.test_both_add_one_record_explicit_merge = function()
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
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        var zrec = ztx.new_record("item");
        var recid1 = zrec.recid;
        zrec.val = 17;
        ztx.commit();

        ztx = zs.begin_tx(csid);
        zrec = ztx.new_record("item");
        var recid2 = zrec.recid;
        zrec.val = 13;
        ztx.commit();

        var leaves = zs.get_leaves();
        testlib.ok(leaves.length==2, "two leaves");
        zs.merge(null, leaves[0], leaves[1]);
        testlib.ok(zs.get_leaves().length==1, "one leaf");

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid1);
        testlib.ok(17 == zrec.val, "val check");
        zrec = ztx.open_record("item", recid2);
        testlib.ok(13 == zrec.val, "val check");
        ztx.abort();

        repo.close();
        testlib.ok(true, "No errors");
    };

    this.test_each_modify_different_field = function()
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
                        "foo" : { "datatype" : "int" },
                        "bar" : { "datatype" : "int" }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = 13;
        zrec.bar = 16;
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 26;
        ztx.commit();

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.bar = 32;
        ztx.commit();

        zs.merge();

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid);
        testlib.ok(26 == zrec.foo, "26");
        testlib.ok(32 == zrec.bar, "32");
        testlib.ok(4 == zrec.history.length);
        ztx.abort();

        var h = zs.get_history("item", recid);
        testlib.ok(4 == h.length);

        repo.close();
    };

    this.test_automerge_string_del_mod_most_recent_fallback = function()
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
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = "begin";
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = "left";
        ztx.commit();

        sg.sleep_ms(50);

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = null;
        ztx.commit();

        zs.merge();

        zr = zs.get_record("item", recid);
        testlib.ok(!("foo" in zr));

        repo.close();
    }
    
    this.test_automerge_string_del_mod_least_recent = function()
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
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "least_recent"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = "begin";
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = "left";
        ztx.commit();

        sg.sleep_ms(50);

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = null;
        ztx.commit();

        zs.merge();

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid);
        testlib.ok("left" == zrec.foo, "foo");
        ztx.abort();

        repo.close();
    }
    
    this.test_automerge_string_least_recent = function()
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
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "least_recent"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = "begin";
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = "left";
        ztx.commit();

        sg.sleep_ms(50);

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = "right";
        ztx.commit();

        zs.merge();

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid);
        testlib.ok("left" == zrec.foo, "foo");
        ztx.abort();

        repo.close();
    };

    this.test_automerge_string_most_recent = function()
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
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "most_recent"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = "begin";
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = "left";
        ztx.commit();

        sg.sleep_ms(50);

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = "right";
        ztx.commit();

        zs.merge();

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid);
        testlib.ok("right" == zrec.foo, "foo");
        ztx.abort();

        repo.close();
    };

    this.test_automerge_int_least_recent = function()
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
                            "datatype" : "int",
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "least_recent"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = 31;
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 35;
        ztx.commit();

        sg.sleep_ms(50);

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 44;
        ztx.commit();

        zs.merge();

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid);
        testlib.ok(35 == zrec.foo, "foo");
        ztx.abort();

        repo.close();
    };

    this.test_automerge_int_most_recent = function()
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
                            "datatype" : "int",
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "most_recent"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = 31;
        var csid = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 35;
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 44;
        ztx.commit();

        zs.merge();

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid);
        testlib.ok(44 == zrec.foo, "foo");
        ztx.abort();

        repo.close();
    };

    this.test_automerge_int_min = function()
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
                            "datatype" : "int",
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "min"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = 31;
        var csid = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 35;
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 44;
        ztx.commit();

        var res = zs.merge();
        print(sg.to_json__pretty_print(res));

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid);
        print("foo == ", zrec.foo);
        testlib.ok(35 == zrec.foo, "foo");
        ztx.abort();

        repo.close();
    };

    this.test_automerge_int_max = function()
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
                            "datatype" : "int",
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "max"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = 31;
        var csid = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 35;
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 44;
        ztx.commit();

        zs.merge();

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid);
        testlib.ok(44 == zrec.foo, "foo");
        ztx.abort();

        repo.close();
    };

    this.test_automerge_int_sum = function()
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
                            "datatype" : "int",
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "sum"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = 31;
        var csid = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 40;
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 60;
        ztx.commit();

        zs.merge();

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid);
        print("foo == ", zrec.foo);
        testlib.ok(100 == zrec.foo, "foo");
        ztx.abort();

        repo.close();
    };

    this.test_automerge_int_average = function()
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
                            "datatype" : "int",
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "average"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = 31;
        var csid = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 40;
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = 60;
        ztx.commit();

        zs.merge();

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid);
        print("foo == ", zrec.foo);
        testlib.ok(50 == zrec.foo, "foo");
        ztx.abort();

        repo.close();
    };

    this.test_automerge_string_longest = function()
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
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "longest"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = "plok";
        var csid = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = "wilbur";
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = "joe";
        ztx.commit();

        zs.merge();

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid);
        testlib.ok("wilbur" == zrec.foo, "foo");
        ztx.abort();

        repo.close();
    };

    this.test_automerge_string_shortest = function()
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
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "shortest"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = "plok";
        var csid = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = "wilbur";
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = "joe";
        ztx.commit();

        zs.merge();

        ztx = zs.begin_tx();
        zrec = ztx.open_record("item", recid);
        testlib.ok("joe" == zrec.foo, "foo");
        ztx.abort();

        repo.close();
    };

    this.test_automerge_string_unique_no_uniqify = function()
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
                                "unique" : true
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        var err = "";
        try
        {
            ztx.set_template(t);
            var csid = ztx.commit().csid;
        }
        catch (e)
        {
            err = e.toString();
        }
        testlib.ok(err.length > 0);
        ztx.abort();

        repo.close();
    };
    
    this.test_automerge_uniqify_last_modified = function()
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
                        "label" :
                        {
                            "datatype" : "string"
                        },
                        "val" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "unique" : true
                            },
                            "merge" :
                            {
                                "uniqify" : 
                                {
                                    "which" : "last_modified",
                                    "op" : "append_userprefix_unique"
                                }
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var csid = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        var recid1 = ztx.new_record("item", {"val":"z1", "label":"one"}).recid;
        var leaf1 = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        var recid2 = ztx.new_record("item", {"val":"z1", "label" : "two"}).recid;
        var leaf2 = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(leaf2);
        ztx.open_record("item", recid2).label = "TWO";
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx(leaf1);
        ztx.open_record("item", recid1).label = "ONE";
        ztx.commit();

        // recid2 was last created
        // recid1 was last modified

        testlib.ok(zs.get_leaves().length==2, "two leaves");
        var res = zs.merge();
        testlib.ok(zs.get_leaves().length==1, "one leaves");

        zrec = zs.get_record("item", recid1);
        testlib.ok("ONE" == zrec.label);
        //testlib.ok("z2" == zrec.val);

        zrec = zs.get_record("item", recid2);
        testlib.ok("TWO" == zrec.label);
        testlib.ok("z1" == zrec.val);

        repo.close();
    };
    
    this.test_automerge_uniqify_last_created = function()
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
                        "label" :
                        {
                            "datatype" : "string"
                        },
                        "val" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "unique" : true
                            },
                            "merge" :
                            {
                                "uniqify" : 
                                {
                                    "which" : "last_created",
                                    "op" : "append_userprefix_unique"
                                }
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var csid = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        var recid1 = ztx.new_record("item", {"val":"z1", "label":"one"}).recid;
        var leaf1 = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        var recid2 = ztx.new_record("item", {"val":"z1", "label" : "two"}).recid;
        var leaf2 = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(leaf2);
        ztx.open_record("item", recid2).label = "TWO";
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx(leaf1);
        ztx.open_record("item", recid1).label = "ONE";
        ztx.commit();

        // recid2 was last created
        // recid1 was last modified

        testlib.ok(zs.get_leaves().length==2, "two leaves");
        var res = zs.merge();
        testlib.ok(zs.get_leaves().length==1, "one leaves");

        zrec = zs.get_record("item", recid1);
        testlib.ok("ONE" == zrec.label);
        testlib.ok("z1" == zrec.val);

        zrec = zs.get_record("item", recid2);
        testlib.ok("TWO" == zrec.label);
        //testlib.ok("z2" == zrec.val);

        repo.close();
    },
    
    this.test_automerge_both_delete = function()
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
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = 31;
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        ztx.delete_record("item", recid);
        ztx.commit();

        ztx = zs.begin_tx(csid);
        ztx.delete_record("item", recid);
        ztx.commit();

        zs.merge();

        var err = "";
        try
        {
            zrec = zs.get_record("item", recid1);
        }
        catch (e)
        {
            err = e.toString();
        }
        testlib.ok(0 < err.length, "get_record should not have succeeded");

        repo.close();
    };

    this.test_uniqify_gen_random_unique = function()
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
                                "minlength" : 1,
                                "maxlength" : 4,
                                "defaultfunc" : "gen_random_unique",
                                "genchars" : "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            },
                            "merge" :
                            {
                                "uniqify" : 
                                {
                                    "op" : "redo_defaultfunc",
                                    "which" : "last_created"
                                }
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var csid = ztx.commit().csid;

        var zrec;

        ztx = zs.begin_tx(csid);
        zrec = ztx.new_record("item");
        var recid1 = zrec.recid;
        zrec.val = "CQ";
        var leaf1 = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.new_record("item");
        var recid2 = zrec.recid;
        zrec.val = "CQ";
        var leaf2 = ztx.commit().csid;

        // recid2 was last created

        testlib.ok(zs.get_leaves().length==2, "two leaves");
        var res = zs.merge();
        testlib.ok(zs.get_leaves().length==1, "one leaves");

        zrec = zs.get_record("item", recid1);
        testlib.ok("CQ" == zrec.val);

        zrec = zs.get_record("item", recid2);
        testlib.ok("CQ" != zrec.val);

        repo.close();
    };

    this.test_merge_three_leaves = function()
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
                        "a" : 
                        {
                            "datatype" : "string"
                        },
                        "b" : 
                        {
                            "datatype" : "string"
                        },
                        "c" : 
                        {
                            "datatype" : "string"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.a = "alpha";
        zrec.b = "bravo";
        zrec.c = "charlie";
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.a = "uno";
        ztx.commit();

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.b = "dos";
        ztx.commit();

        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.c = "tres";
        ztx.commit();

        testlib.ok(3 == zs.get_leaves().length);
        zs.merge();
        testlib.ok(1 == zs.get_leaves().length);

        repo.close();
    };

    this.test_automerge_uniqify_least_impact__history_entries = function()
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
                        "label" :
                        {
                            "datatype" : "string"
                        },
                        "val" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "unique" : true
                            },
                            "merge" :
                            {
                                "uniqify" : 
                                {
                                    "which" : "least_impact",
                                    "op" : "append_userprefix_unique"
                                }
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        var recid1 = ztx.new_record("item", {"val":"z1", "label" : "one"}).recid;
        var leaf1 = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        var recid2 = ztx.new_record("item", {"val":"z1", "label" : "two"}).recid;
        var leaf2 = ztx.commit().csid;

        ztx = zs.begin_tx(leaf2);
        ztx.open_record("item", recid2).label = "TWO";
        var leaf2b = ztx.commit().csid;

        testlib.ok(zs.get_leaves().length==2, "two leaves");
        var res = zs.merge();
        testlib.ok(zs.get_leaves().length==1, "one leaves");

        // record TWO has more history entries, so record ONE should
        // be the one that gets changed
  
        zrec = zs.get_record("item", recid1);
        testlib.ok("one" == zrec.label);
        print(sg.to_json(zrec));
        //testlib.ok("z2" == zrec.val);

        zrec = zs.get_record("item", recid2);
        testlib.ok("TWO" == zrec.label);
        print(sg.to_json(zrec));
        testlib.ok("z1" == zrec.val);

        repo.close();
    };
    
    this.test_automerge_uniqify_least_impact__most_recent = function()
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
                        "label" :
                        {
                            "datatype" : "string"
                        },
                        "val" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "unique" : true
                            },
                            "merge" :
                            {
                                "uniqify" : 
                                {
                                    "which" : "least_impact",
                                    "op" : "append_userprefix_unique"
                                }
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        var recid1 = ztx.new_record("item", {"val":"z1", "label" : "one"}).recid;
        var leaf1 = ztx.commit().csid;

        sg.sleep_ms(50);

        ztx = zs.begin_tx(csid);
        var recid2 = ztx.new_record("item", {"val":"z1", "label" : "two"}).recid;
        var leaf2 = ztx.commit().csid;

        testlib.ok(zs.get_leaves().length==2, "two leaves");
        var res = zs.merge();
        testlib.ok(zs.get_leaves().length==1, "one leaves");

        // record TWO is the most recent, so it should
        // be the one that gets changed
  
        zrec = zs.get_record("item", recid1);
        testlib.ok("one" == zrec.label);
        print(sg.to_json(zrec));
        testlib.ok("z1" == zrec.val);

        zrec = zs.get_record("item", recid2);
        testlib.ok("two" == zrec.label);
        print(sg.to_json(zrec));
        //testlib.ok("z2" == zrec.val);

        repo.close();
    };
    
    /* TODO enable this test once the least_impact code can handle it
    this.test_automerge_uniqify_least_impact__dag_changesets = function()
    {
        var t =
        {
            "version" : 1,
            "rectypes" :
            {
                "item" :
                {
                    "merge_type" : "field",
                    "fields" :
                    {
                        "label" :
                        {
                            "datatype" : "string",
                        },
                        "val" :
                        {
                            "datatype" : "int",
                            "constraints" :
                            {
                                "unique" : true
                            },
                            "merge" :
                            {
                                "uniqify" : 
                                {
                                    "op" : "add",
                                    "which" : "least_impact",
                                    "addend" : 5
                                }
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
	 whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var csid = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        var recid1 = ztx.new_record("item", {"val":31, "label" : "one"}).recid;
        var leaf1 = ztx.commit().csid;

        ztx = zs.begin_tx(csid);
        var recid2 = ztx.new_record("item", {"val":31, "label" : "two"}).recid;
        var leaf2 = ztx.commit().csid;

        ztx = zs.begin_tx(leaf2);
        var recid2 = ztx.new_record("item", {"val":42, "label" : "three"}).recid;
        var leaf2b = ztx.commit().csid;

        testlib.ok(zs.get_leaves().length==2, "two leaves");
        var res = zs.merge();
        testlib.ok(zs.get_leaves().length==1, "one leaves");

        // record TWO is on a leaf with more children, so
        // record ONE should be the one that gets changed
  
        zrec = zs.get_record("item", recid1);
        testlib.ok("one" == zrec.label);
        print(sg.to_json(zrec));
        testlib.ok(36 == zrec.val);

        zrec = zs.get_record("item", recid2);
        testlib.ok("two" == zrec.label);
        print(sg.to_json(zrec));
        testlib.ok(31 == zrec.val);

        repo.close();
    }
    */
    
    // TODO multiple autos, first one fails
    // TODO int most recent (need sleep?)
    // TODO int least recent (need sleep?)
    // TODO string most recent (need sleep?)
    // TODO string least recent (need sleep?)
    // TODO int allowed last
    // TODO int allowed first
    // TODO string allowed last
    // TODO string allowed first
    // TODO string longest fail because they're equal
    // TODO string shortest fail because they're equal

    // TODO try to merge with just one leaf, fail
    // TODO try to merge with three leaves, fail
    // TODO each leaf modify a different record
    // TODO merge_type="record"
    // TODO conflict on int field with no resolver, fail

    this.test_automerge_uniqify_userprefix_1_instance = function()
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
                                "defaultfunc" : "gen_userprefix_unique",
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

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        // -------- initialize the repo with a template
        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);
        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var csid = ztx.commit().csid;

        // add 500 records
        ztx = zs.begin_tx(csid);
        for (i=0; i<500; i++)
        {
            ztx.new_record("item");
        }
        var leaf1 = ztx.commit().csid;

        // add 500 records more, parenting from the same changeset
        ztx = zs.begin_tx(csid);
        for (i=0; i<500; i++)
        {
            ztx.new_record("item");
        }
        var leaf2 = ztx.commit().csid;

        testlib.ok(zs.get_leaves().length==2, "two leaves");
        res = zs.merge();
        testlib.ok(zs.get_leaves().length==1, "one leaf");

        print(sg.to_json__pretty_print(res));

        // there will be no uniqify stuff in the log.  all the
        // record were generated in the same instance, so gen_userprefix_unique
        // made sure every string was unique across ALL records, not just in
        // the current state.

        // TODO if we had a uniqify log, we would check it here.
        //testlib.ok(res.log.length == 0);

        repo.close();
    }
    
    this.test_automerge_uniqify_userprefix_2_instances = function()
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
                                "defaultfunc" : "gen_userprefix_unique",
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

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        // -------- initialize the repo with a template
        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);
        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var csid = ztx.commit().csid;
        repo.close();

        // clone it
        var r2name = sg.gid();
        o = sg.exec(vv, "clone", reponame, r2name);
        testlib.ok(o.exit_status == 0, "exit status should be 0");

        // add 500 records in the original instance
        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);
        ztx = zs.begin_tx(csid);
        for (i=0; i<500; i++)
        {
            ztx.new_record("item");
        }
        var leaf1 = ztx.commit().csid;
        repo.close();

        // add 500 records in the clone
        var repo = sg.open_repo(r2name);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);
        ztx = zs.begin_tx(csid);
        for (i=0; i<500; i++)
        {
            ztx.new_record("item");
        }
        var leaf2 = ztx.commit().csid;
        repo.close();

        // pull from clone to original
        o = sg.exec(vv, "pull", "--dest", reponame, r2name);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print(sg.to_json__pretty_print(o));
        print(o.stdout);
    }
    
    this.test_automerge_journal = function()
    {
        var t =
        {
            "version" : 1,
            "rectypes" :
            {
                "journal" :
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
                        "record" : { "datatype" : "string" },
                        "fred" : { "datatype" : "string" },
                        "op" : { "datatype" : "string" },
                        "val0" : { "datatype" : "string" },
                        "val1" : { "datatype" : "string" },
                        "merged" : { "datatype" : "string" },
                        "field" : { "datatype" : "string" }
                    }
                },
                "item" :
                {
                    "merge" :
                    {
                        "merge_type" : "field",
                        "auto" : 
                        [
                            {
                                "op" : "most_recent",
                                "journal" :
                                {
                                    "rectype" : "journal",
                                    "fields" :
                                    {
                                        "record" : "#RECID#",
                                        "fred" : "barney",
                                        "op" : "#OP#",
                                        "val0" : "#VAL0#",
                                        "val1" : "#VAL1#",
                                        "merged" : "#MERGED_VALUE#",
                                        "field" : "#FIELD_NAME#"
                                    }
                                }
                            }
                        ]
                    },
                    "fields" :
                    {
                        "foo" : 
                        {
                            "datatype" : "string"
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var zrec = ztx.new_record("item");
        var recid = zrec.recid;
        zrec.foo = "begin";
        var csid = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = "left";
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        zrec = ztx.open_record("item", recid);
        zrec.foo = "right";
        ztx.commit();

        zs.merge();

        var r1 = zs.get_record("item", recid);
        print(sg.to_json__pretty_print(r1));

        var res = zs.query(
                "journal", 
                [ "*" ]
                );
        print(sg.to_json__pretty_print(res));
        testlib.ok(res.length == 1);
        testlib.ok(res[0].field == "foo");
        testlib.ok(res[0].fred == "barney");
        testlib.ok(res[0].merged == "right");
        testlib.ok(res[0].op == "most_recent");
        testlib.ok(res[0].record == recid);
        testlib.ok((res[0].val0 == "left") || (res[0].val0 == "right"));
        testlib.ok((res[0].val1 == "left") || (res[0].val1 == "right"));

        repo.close();
    };

    this.test_uniqify_journal = function()
    {
        var t =
        {
            "version" : 1,
            "rectypes" :
            {
                "journal" :
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
                        "record" : { "datatype" : "string" },
                        "fred" : { "datatype" : "string" },
                        "op" : { "datatype" : "string" },
                        "old_value" : { "datatype" : "string" },
                        "new_value" : { "datatype" : "string" },
                        "field" : { "datatype" : "string" }
                    }
                },
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
                        "label" :
                        {
                            "datatype" : "string"
                        },
                        "val" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "unique" : true
                            },
                            "merge" :
                            {
                                "uniqify" : 
                                {
                                    "which" : "last_modified",
                                    "op" : "append_userprefix_unique",
                                    "journal" :
                                    {
                                        "rectype" : "journal",
                                        "fields" :
                                        {
                                            "record" : "#RECID#",
                                            "fred" : "wilma",
                                            "op" : "#OP#",
                                            "old_value" : "#OLD_VALUE#",
                                            "new_value" : "#NEW_VALUE#",
                                            "field" : "#FIELD_NAME#",
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var csid = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        var recid1 = ztx.new_record("item", {"val":"z1", "label":"one"}).recid;
        var leaf1 = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(csid);
        var recid2 = ztx.new_record("item", {"val":"z1", "label" : "two"}).recid;
        var leaf2 = ztx.commit().csid;

        sg.sleep_ms(50);
        ztx = zs.begin_tx(leaf2);
        ztx.open_record("item", recid2).label = "TWO";
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx(leaf1);
        ztx.open_record("item", recid1).label = "ONE";
        ztx.commit();

        // recid2 was last created
        // recid1 was last modified

        testlib.ok(zs.get_leaves().length==2, "two leaves");
        var res = zs.merge();
        testlib.ok(zs.get_leaves().length==1, "one leaves");

        zrec = zs.get_record("item", recid1);
        testlib.ok("ONE" == zrec.label);
        //testlib.ok("z2" == zrec.val);

        zrec = zs.get_record("item", recid2);
        testlib.ok("TWO" == zrec.label);
        testlib.ok("z1" == zrec.val);

        var res = zs.query(
                "journal", 
                [ "*" ]
                );
        print(sg.to_json__pretty_print(res));
        testlib.ok(res.length == 1);
        testlib.ok(res[0].fred == "wilma");
        testlib.ok(res[0].old_value == "z1");
        testlib.ok(res[0].new_value != "z1");
        testlib.ok(res[0].op == "append_userprefix_unique");
        testlib.ok(res[0].record == recid1);

        repo.close();
    };
    
    this.test_dag_since = function()
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
                            "merge" : 
                            {
                                "auto" : 
                                [
                                    {
                                        "op" : "least_recent"
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);
		whoami_testing(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);
        var csid_0 = ztx.commit().csid;

        var leafset_0 = zs.get_leaves();

        var ztx = zs.begin_tx(csid_0);
        ztx.new_record("item");
        var csid_1 = ztx.commit().csid;

        var ztx = zs.begin_tx(csid_0);
        ztx.new_record("item");
        var csid_2 = ztx.commit().csid;

        var ztx = zs.begin_tx(csid_1);
        ztx.new_record("item");
        var csid_3 = ztx.commit().csid;

        var leafset_1 = zs.get_leaves();

        var ztx = zs.begin_tx(csid_0);
        ztx.new_record("item");
        var csid_4 = ztx.commit().csid;

        var ztx = zs.begin_tx(csid_3);
        ztx.new_record("item");
        var csid_5 = ztx.commit().csid;

        var leafset_2 = zs.get_leaves();

        var since_leafset_0 = repo.find_new_dagnodes_since(
                {
                    "dagnum" : sg.dagnum.TESTING_DB,
                    "leafset" : leafset_0
                }
                );
        print(sg.to_json__pretty_print(since_leafset_0));
        testlib.ok(!(csid_0 in since_leafset_0));
        testlib.ok(csid_1 in since_leafset_0);
        testlib.ok(csid_2 in since_leafset_0);
        testlib.ok(csid_3 in since_leafset_0);
        testlib.ok(csid_4 in since_leafset_0);
        testlib.ok(csid_5 in since_leafset_0);

        var since_leafset_1 = repo.find_new_dagnodes_since(
                {
                    "dagnum" : sg.dagnum.TESTING_DB,
                    "leafset" : leafset_1
                }
                );
        print(sg.to_json__pretty_print(since_leafset_1));
        testlib.ok(!(csid_0 in since_leafset_1));
        testlib.ok(!(csid_1 in since_leafset_1));
        testlib.ok(!(csid_2 in since_leafset_1));
        testlib.ok(!(csid_3 in since_leafset_1));
        testlib.ok(csid_4 in since_leafset_1);
        testlib.ok(csid_5 in since_leafset_1);

        var since_leafset_2 = repo.find_new_dagnodes_since(
                {
                    "dagnum" : sg.dagnum.TESTING_DB,
                    "leafset" : leafset_2
                }
                );
        print(sg.to_json__pretty_print(since_leafset_2));
        testlib.ok(!(csid_0 in since_leafset_2));
        testlib.ok(!(csid_1 in since_leafset_2));
        testlib.ok(!(csid_2 in since_leafset_2));
        testlib.ok(!(csid_3 in since_leafset_2));
        testlib.ok(!(csid_4 in since_leafset_2));
        testlib.ok(!(csid_5 in since_leafset_2));

        repo.close();
    };
}

