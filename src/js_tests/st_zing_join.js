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

function st_zing_join()
{
    this.test_query_join_1 = function()
    {
        var t =
        {
            "version" : 1,
            "rectypes" :
            {
                "environment" :
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
                        "desc" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "required" : true
                            }
                        },
                        "cool" :
                        {
                            "datatype" : "bool",
                            "constraints" :
                            {
                                "required" : true
                            }
                        }
                    }
                },
                "build" :
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
                        "environment_ref" :
                        {
                            "datatype" : "reference",
                            "constraints" :
                            {
                                "required" : true,
                                "rectype" : "environment"
                            }
                        },
                        "num" :
                        {
                            "datatype" : "int",
                            "constraints" :
                            {
                                "required" : true
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
        var recid_Linux = ztx.new_record("environment", {"desc" : "Linux", "cool":true}).recid;
        var recid_Mac = ztx.new_record("environment", {"desc" : "Mac", "cool":true}).recid;
        var recid_Windows = ztx.new_record("environment", {"desc" : "Windows", "cool":false}).recid;
        ztx.commit();

        ztx = zs.begin_tx();
        var r = ztx.new_record("build");
        r.num = 1;
        r.environment_ref = recid_Linux;

        var r = ztx.new_record("build");
        r.num = 2;
        r.environment_ref = recid_Mac;

        var r = ztx.new_record("build");
        r.num = 3;
        r.environment_ref = recid_Mac;

        var r = ztx.new_record("build");
        r.num = 4;
        r.environment_ref = recid_Windows;

        var r = ztx.new_record("build");
        r.num = 5;
        r.environment_ref = recid_Linux;

        var r = ztx.new_record("build");
        r.num = 6;
        r.environment_ref = recid_Windows;

        var r = ztx.new_record("build");
        r.num = 7;
        r.environment_ref = recid_Mac;

        var r = ztx.new_record("build");
        r.num = 8;
        r.environment_ref = recid_Linux;

        ztx.commit();

        var t1 = sg.time();
        var recs = zs.query("build", 
                [
                "num", 
                {"type":"from_me","ref":"environment_ref","field":"desc","alias":"environment"},
                {"type":"from_me","ref":"environment_ref","field":"cool","alias":"cool"}
                ] 
                );
        var t2 = sg.time();
        testlib.ok(recs.length == 8);
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");

        var t1 = sg.time();
        var recs = zs.query("environment", 
                [
                "desc", 
                "cool", 
                {"type":"to_me","rectype":"build","ref":"environment_ref","alias":"builds","fields":["num", "recid"] },
                ] 
                );
        var t2 = sg.time();
        testlib.ok(recs.length == 3);
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");

        var t1 = sg.time();
        var recs = zs.query("environment", 
                [
                "desc", 
                "cool", 
                {"type":"to_me","rectype":"build","ref":"environment_ref","alias":"builds","fields":["num", "recid"] },
                ],
                "num < 5"
                );
        var t2 = sg.time();
        testlib.ok(recs.length == 3);
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");

        var t1 = sg.time();
        var recs = zs.query("environment", 
                [
                "desc", 
                "cool", 
                {"type":"to_me","rectype":"build","ref":"environment_ref","alias":"builds","fields":["num", "recid"] },
                ],
                "num == 3"
                );
        var t2 = sg.time();
        testlib.ok(recs.length == 1);
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");

        // this one sorts on a field which came in through the from_me join

        var t1 = sg.time();
        var recs = zs.query("build", 
                [
                "num", 
                {"type":"from_me","ref":"environment_ref","field":"desc","alias":"environment"}
                ],
                null,
                "environment #DESC, num #DESC" 
                );
        var t2 = sg.time();
        testlib.ok(recs.length == 8);
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");
        testlib.ok(recs[0].environment == "Windows");
        testlib.ok(recs[0].num == 6);

        repo.close();
    }

    this.test_query_join_with_fts = function()
    {
        var t =
        {
            "version" : 1,
            "rectypes" :
            {
                "environment" :
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
                        "desc" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "required" : true,
                                "full_text_search" : true
                            }
                        },
                        "cool" :
                        {
                            "datatype" : "bool",
                            "constraints" :
                            {
                                "required" : true
                            }
                        }
                    }
                },
                "build" :
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
                        "environment_ref" :
                        {
                            "datatype" : "reference",
                            "constraints" :
                            {
                                "required" : true,
                                "rectype" : "environment"
                            }
                        },
                        "num" :
                        {
                            "datatype" : "int",
                            "constraints" :
                            {
                                "required" : true
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
        var recid_Linux = ztx.new_record("environment", {"desc" : "the passenger safety instructions", "cool":true}).recid;
        var recid_Mac = ztx.new_record("environment", {"desc" : "the dining car", "cool":true}).recid;
        var recid_Windows = ztx.new_record("environment", {"desc" : "Jerrance Howard, the One, the Only", "cool":false}).recid;
        ztx.commit();

        ztx = zs.begin_tx();
        var r = ztx.new_record("build");
        r.num = 1;
        r.environment_ref = recid_Linux;

        var r = ztx.new_record("build");
        r.num = 2;
        r.environment_ref = recid_Mac;

        var r = ztx.new_record("build");
        r.num = 3;
        r.environment_ref = recid_Mac;

        var r = ztx.new_record("build");
        r.num = 4;
        r.environment_ref = recid_Windows;

        var r = ztx.new_record("build");
        r.num = 5;
        r.environment_ref = recid_Linux;

        var r = ztx.new_record("build");
        r.num = 6;
        r.environment_ref = recid_Windows;

        var r = ztx.new_record("build");
        r.num = 7;
        r.environment_ref = recid_Mac;

        var r = ztx.new_record("build");
        r.num = 8;
        r.environment_ref = recid_Linux;

        ztx.commit();

        // plain fts with a join
        var t1 = sg.time();
        var recs = zs.query__fts("environment", "desc", "car",
                [
                "desc", 
                {"type":"to_me","rectype":"build","ref":"environment_ref","alias":"builds","fields":["num", "recid"] },
                ] 
                );
        var t2 = sg.time();
        testlib.ok(recs.length == 1);
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");

        // now on the word "the", which should return all three records
        var t1 = sg.time();
        var recs = zs.query__fts("environment", "desc", "the",
                [
                "desc", 
                {"type":"to_me","rectype":"build","ref":"environment_ref","alias":"builds","fields":["num", "recid"] },
                ] 
                );
        var t2 = sg.time();
        testlib.ok(recs.length == 3);
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");

        // same, but with a limit/skip
        var t1 = sg.time();
        var recs = zs.query__fts("environment", "desc", "the",
                [
                "desc", 
                {"type":"to_me","rectype":"build","ref":"environment_ref","alias":"builds","fields":["num", "recid"] },
                ], 5, 1 
                );
        var t2 = sg.time();
        testlib.ok(recs.length == 2);
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");

        // same, but also with history
        var t1 = sg.time();
        var recs = zs.query__fts("environment", "desc", "the",
                [
                "desc", 
                "history",
                {"type":"to_me","rectype":"build","ref":"environment_ref","alias":"builds","fields":["num", "recid"] },
                ], 5, 1 
                );
        var t2 = sg.time();
        testlib.ok(recs.length == 2);
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");
        testlib.ok(recs[0].history[0].audits.length == 1);

        repo.close();
    }

    this.test_query_join_lots = function()
    {
        var t =
        {
            "version" : 1,
            "rectypes" :
            {
                "article" :
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
                        "topic" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "required" : true
                            }
                        },
                        "author" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "required" : true
                            }
                        }
                    }
                },
                "comment" :
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
                        "article_ref" :
                        {
                            "datatype" : "reference",
                            "constraints" :
                            {
                                "required" : true,
                                "rectype" : "article"
                            }
                        },
                        "text" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "required" : true
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

        var mycomments=[];

        var myrecids = [];

        sg.sleep_ms(50);
        ztx = zs.begin_tx();
        myrecids[0] = ztx.new_record("article", {"topic" : "I warn you not to underestimate my powers", "author":"Luke"}).recid;
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx();
        myrecids[1] = ztx.new_record("article", {"topic" : "I know", "author":"Leia"}).recid;
        ztx.commit();

        var t_mark = sg.time();

        sg.sleep_ms(50);
        ztx = zs.begin_tx();
        myrecids[2] = ztx.new_record("article", {"topic" : "These aren't the droids you're looking for", "author":"Ben"}).recid;
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx();
        myrecids[3] = ztx.new_record("article", {"topic" : "Life as a giant worm", "author":"Jabba"}).recid;
        ztx.commit();

        sg.sleep_ms(50);
        ztx = zs.begin_tx();
        myrecids[4] = ztx.new_record("article", {"topic" : "No, really, I shot first", "author":"Han"}).recid;
        ztx.commit();

        mycomments[0] = "Bad driver";
        mycomments[1] = "Slave";
        mycomments[2] = "Bridge over the River Kwai";
        mycomments[3] = "You weren't actually IN the first movie you know";
        mycomments[4] = "K19 sucked";

        ztx = zs.begin_tx();
        for (i=0; i<100; i++)
        {
            var r = ztx.new_record("comment", {});
            var article_ndx = sg.rand(0,myrecids.length-1);

            r.text = mycomments[article_ndx];

            var article_recid = myrecids[article_ndx];
            r.article_ref = article_recid;
        }
        ztx.commit();

        var t1 = sg.time();
        var recs = zs.query("article", 
                [
                "topic", 
                "author", 
                "last_timestamp", 
                {"type":"to_me","rectype":"comment","ref":"article_ref","alias":"comments","fields":["text", "recid"] },
                ],
                null,
                "last_timestamp #DESC"
                );
        var t2 = sg.time();
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");

        testlib.ok(recs[0].author == "Han");
        testlib.ok(recs[1].author == "Jabba");
        testlib.ok(recs[2].author == "Ben");
        testlib.ok(recs[3].author == "Leia");
        testlib.ok(recs[4].author == "Luke");

        testlib.ok(recs[0].last_timestamp >= recs[1].last_timestamp);

        testlib.ok(recs[0].comments[0].text == "K19 sucked");

        var t1 = sg.time();
        var recs = zs.query("article", 
                [
                "topic", 
                "author", 
                "last_timestamp", 
                {"type":"to_me","rectype":"comment","ref":"article_ref","alias":"comments","fields":["text", "recid"] },
                ],
                "last_timestamp <= " + t_mark.toString(),
                "last_timestamp #DESC"
                );
        var t2 = sg.time();
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");
        testlib.ok(recs.length == 2);
        testlib.ok(recs[0].author == "Leia");
        testlib.ok(recs[1].author == "Luke");

        var t1 = sg.time();
        var recs = zs.query("article", 
                [
                "topic", 
                "author", 
                "last_timestamp", 
                {"type":"to_me","rectype":"comment","ref":"article_ref","alias":"comments","fields":["text", "recid"] },
                ],
                "author != 'Han'",
                "last_timestamp #ASC"
                );
        var t2 = sg.time();
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");
        testlib.ok(recs.length == 4);
        testlib.ok(recs[0].author == "Luke");
        testlib.ok(recs[1].author == "Leia");

        repo.close();
    }
    
    this.test_query_join_xref = function()
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
                        "title" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "required" : true
                            }
                        }
                    }
                },
                "category" :
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
                        "name" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "required" : true
                            }
                        }
                    }
                },
                "assoc" :
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
                        "item" :
                        {
                            "datatype" : "reference",
                            "constraints" :
                            {
                                "required" : true,
                                "rectype" : "item"
                            }
                        },
                        "category" :
                        {
                            "datatype" : "reference",
                            "constraints" :
                            {
                                "required" : true,
                                "rectype" : "category"
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

        var items = [];

        ztx = zs.begin_tx();
        frasier = ztx.new_record("item", {"title" : "Frasier"}).recid;
        startrek = ztx.new_record("item", {"title" : "Star Trek"}).recid;
        house = ztx.new_record("item", {"title" : "House"}).recid;
        closer = ztx.new_record("item", {"title" : "The Closer"}).recid;
        mash = ztx.new_record("item", {"title" : "M*A*S*H"}).recid;
        seinfeld = ztx.new_record("item", {"title" : "Seinfeld"}).recid;
        firefly = ztx.new_record("item", {"title" : "Firefly"}).recid;
        dollhouse = ztx.new_record("item", {"title" : "Dollhouse"}).recid;
        ztx.commit();

        ztx = zs.begin_tx();
        comedy = ztx.new_record("category", {"name" : "Comedy"}).recid;
        space = ztx.new_record("category", {"name" : "Space"}).recid;
        whedon = ztx.new_record("category", {"name" : "Whedon"}).recid;
        hour = ztx.new_record("category", {"name" : "Hour"}).recid;
        medical = ztx.new_record("category", {"name" : "Medical"}).recid;
        crime = ztx.new_record("category", {"name" : "Crime"}).recid;
        ztx.commit();

        ztx = zs.begin_tx();
        ztx.new_record("assoc", {"item" : frasier, "category" : comedy});
        ztx.new_record("assoc", {"item" : house, "category" : comedy});
        ztx.new_record("assoc", {"item" : mash, "category" : comedy});
        var recid_assoc_seinfeld_comedy = ztx.new_record("assoc", {"item" : seinfeld, "category" : comedy}).recid;

        ztx.new_record("assoc", {"item" : firefly, "category" : space});
        ztx.new_record("assoc", {"item" : startrek, "category" : space});

        ztx.new_record("assoc", {"item" : house, "category" : medical});

        ztx.new_record("assoc", {"item" : dollhouse, "category" : whedon});
        ztx.new_record("assoc", {"item" : firefly, "category" : whedon});

        ztx.new_record("assoc", {"item" : closer, "category" : crime});

        ztx.new_record("assoc", {"item" : startrek, "category" : hour});
        ztx.new_record("assoc", {"item" : house, "category" : hour});
        ztx.new_record("assoc", {"item" : closer, "category" : hour});
        ztx.new_record("assoc", {"item" : firefly, "category" : hour});
        ztx.new_record("assoc", {"item" : dollhouse, "category" : hour});
        ztx.commit();

        var t1 = sg.time();
        var recs = zs.query("item", 
                [
                "title", 
                {
                    "type":"xref",
                    "xref":"assoc",
                    "ref_to_me":"item",
                    "ref_to_other" : "category",
                    "alias":"categories",
                    "fields":["name"] },
                ] 
                );
        var t2 = sg.time();
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");

        var t1 = sg.time();
        var recs = zs.query("item", 
                [
                "title", 
                {
                    "type":"xref",
                    "xref":"assoc",
                    "ref_to_me":"item",
                    "ref_to_other" : "category",
                    "xref_recid_alias" : "assoc",
                    "alias":"categories",
                    "fields":["name"] },
                ],
               "title = 'Seinfeld'" 
                );
        var t2 = sg.time();
        print(sg.to_json__pretty_print(recs));
        print(t2 - t1, " ms");
        testlib.ok(recs.length == 1)
        testlib.ok(recs[0].title == "Seinfeld")
        testlib.ok(recs[0].categories.length == 1)
        testlib.ok(recs[0].categories[0].name == "Comedy")
        testlib.ok(recs[0].categories[0].assoc == recid_assoc_seinfeld_comedy)

        repo.close();
    }

}

