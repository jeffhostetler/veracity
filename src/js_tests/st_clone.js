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

function get_count(x)
{
    return x.count;
}

function get_blob_stats(repo)
{
    var bs = repo.list_blobs();
    var s = {};
    s.full = bs.get_stats({"encoding" : sg.blobencoding.FULL});
    s.alwaysfull = bs.get_stats({"encoding" : sg.blobencoding.ALWAYSFULL});
    s.zlib = bs.get_stats({"encoding" : sg.blobencoding.ZLIB});
    s.vcdiff = bs.get_stats({"encoding" : sg.blobencoding.VCDIFF});
    bs.cleanup();

    return s;
}

function get_blobs(repo)
{
    var ba = [];
    var bs = repo.list_blobs();
    var all = bs.get({ "limit" : 1000, "skip" : 0});
    for (var hid in all)
    {
        ba.push(hid);
    }
    bs.cleanup();

    return ba;
}

function st_clone()
{
    this.test_1 = function()
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

        var r1name = sg.gid();
        new_zing_repo(r1name);
		whoami_testing(r1name);
        repo = sg.open_repo(r1name);
		repo.close();

        // open repo
        var repo = sg.open_repo(r1name);
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

        // do a throwaway clone
        var r2name = sg.gid();
        sg.clone__exact(r1name, r2name);

		repo.close();
		repo = sg.open_repo(r2name);

        // fetch the blob list
        var blobs1 = get_blob_stats(repo);
		var blobdump = get_blobs(repo);

        var count_total = get_count(blobs1.zlib)
                          + get_count(blobs1.full)
                          + get_count(blobs1.alwaysfull)
                          + get_count(blobs1.vcdiff);
        repo.close();

        print(sg.to_json__pretty_print(blobs1));
        print("count_total: ", count_total);

        // ----------------------------------------------------------------
        // clone it, converting everything to full
        var r2name = sg.gid();
        sg.clone__all_full(
                { 
                    "existing_repo" : r1name, 
                    "new_repo" : r2name,
                    "override_alwaysfull" : true
                }
                );

        // fetch the blob list from the clone
        var repo = sg.open_repo(r2name);
        var blobs2 = get_blob_stats(repo);
        var ba = get_blobs(repo);

		print("stats before:\n" + sg.to_json__pretty_print(blobs1));
		print("stats after:\n" + sg.to_json__pretty_print(blobs2));

        repo.close();

        testlib.equal(count_total, ba.length, "ba length");

        /*
        // ----------------------------------------------------------------
        // build a chgs object to convert some of the blobs to zlib
        var specific = {};
        for (i=0; i<ba.length; i++)
        {
            specific[ba[i]] = {};
            if (i < 8)
            {
                specific[ba[i]].encoding = sg.blobencoding.ZLIB;
            }
            else
            {
                specific[ba[i]].encoding = sg.blobencoding.KEEPFULLFORNOW;
            }
        }

        print(sg.to_json__pretty_print(specific));

        // clone again
        var r3name = sg.gid();
        sg.clone__specific_changes(
                { 
                    "existing_repo" : r2name, 
                    "new_repo" : r3name, 
                    "specific" : specific,
                    "demand_zlib_savings_over_full" : 0 
                }
                );
        
        // fetch the blob list from the clone
        var repo = sg.open_repo(r3name);
        var blobs3 = get_blob_stats(repo);
        repo.close();

        print(sg.to_json__pretty_print(blobs3));

        // make sure everything is alwaysfull except 8 zlibs
        testlib.equal(8, get_count(blobs3.zlib), "zlib count (specific)");
        testlib.equal(0, get_count(blobs3.vcdiff), "vcdiff count (specific)");
        testlib.equal(ba.length - 8, get_count(blobs3.full), "full count (specific)");
        testlib.equal(0, get_count(blobs3.alwaysfull), "alwaysfull count (specific)");

        // ----------------------------------------------------------------
        // build a chgs object to convert ALL of the blobs to zlib
        var specific = {};
        for (i=0; i<ba.length; i++)
        {
            specific[ba[i]] = {};
            specific[ba[i]].encoding = sg.blobencoding.ZLIB;
        }

        // clone again
        var r4name = sg.gid();
        sg.clone__specific_changes(
                { 
                    "existing_repo" : r3name, 
                    "new_repo" : r4name, 
                    "specific" : specific,
                    "demand_zlib_savings_over_full" : 0 
                }
                );
        
        // fetch the blob list from the clone
        var repo = sg.open_repo(r4name);
        var blobs4 = get_blob_stats(repo);
        repo.close();

        // make sure everything is zlib
        testlib.equal(ba.length, get_count(blobs4.zlib), "zlib count");
        testlib.equal(0, get_count(blobs4.vcdiff), "vcdiff count");
        testlib.equal(0, get_count(blobs4.full), "full count");
        testlib.equal(0, get_count(blobs4.alwaysfull), "alwaysfull count");
        */

        // ----------------------------------------------------------------
        // use all_zlib to convert ALL of the blobs to zlib
        var r4name2 = sg.gid();
        sg.clone__all_zlib(
                { "existing_repo" : r2name, "new_repo" : r4name2, "override_alwaysfull" : true, "demand_zlib_savings_over_full" : 0 }
                );
        
        // fetch the blob list from the clone
        var repo = sg.open_repo(r4name2);
        var blobs4_2 = get_blob_stats(repo);
        repo.close();

        // make sure everything is zlib
        testlib.equal(count_total, get_count(blobs4_2.zlib), "zlib count");
        testlib.equal(0, get_count(blobs4_2.vcdiff), "vcdiff count");
        testlib.equal(0, get_count(blobs4_2.full), "full count");
        testlib.equal(0, get_count(blobs4_2.alwaysfull), "alwaysfull count");

        /*
        // ----------------------------------------------------------------
        // build a chgs object to convert deltify everything against the first blob
        var specific = {};
        for (i=0; i<count_total; i++)
        {
            specific[ba[i]] = {};
            if (0 == i)
            {
                specific[ba[i]].encoding = sg.blobencoding.ALWAYSFULL;
            }
            else
            {
                specific[ba[i]].encoding = sg.blobencoding.VCDIFF;
                specific[ba[i]].hid_vcdiff = ba[0];
            }
        }

        // clone again
        var r5name = sg.gid();
        sg.clone__specific_changes(
                { 
                    "existing_repo" : r4name2, 
                    "new_repo" : r5name, 
                    "specific" : specific,
                    "demand_vcdiff_savings_over_full" : 0,
                    "demand_vcdiff_savings_over_zlib" : 0 
                }
                );
        
        // fetch the blob list from the clone
        var repo = sg.open_repo(r5name);
        var blobs5 = get_blob_stats(repo);
        repo.close();

        // make sure everything is right
        testlib.equal(0, get_count(blobs5.zlib), "zlib count");
        testlib.equal(count_total - 1, get_count(blobs5.vcdiff), "vcdiff count");
        testlib.equal(0, get_count(blobs5.full), "full count");
        testlib.equal(1, get_count(blobs5.alwaysfull), "alwaysfull count");

        // ----------------------------------------------------------------
        // build a chgs object to convert deltify everything against the FIFTH blob
        var specific = {};
        for (i=0; i<count_total; i++)
        {
            specific[ba[i]] = {};
            if (5 == i)
            {
                specific[ba[i]].encoding = sg.blobencoding.ALWAYSFULL;
            }
            else
            {
                specific[ba[i]].encoding = sg.blobencoding.VCDIFF;
                specific[ba[i]].hid_vcdiff = ba[5];
            }
        }

        // clone again
        var r6name = sg.gid();
        sg.clone__specific_changes(
                { 
                    "existing_repo" : r5name, 
                    "new_repo" : r6name, 
                    "specific" : specific,
                    "demand_vcdiff_savings_over_full" : 0,
                    "demand_vcdiff_savings_over_zlib" : 0 
                }
                );
        
        // fetch the blob list from the clone
        var repo = sg.open_repo(r6name);
        var blobs6 = get_blob_stats(repo);
        repo.close();

        // make sure everything is ok
        testlib.equal(0, get_count(blobs6.zlib), "zlib count");
        testlib.equal(count_total - 1, get_count(blobs6.vcdiff), "vcdiff count");
        testlib.equal(0, get_count(blobs6.full), "full count");
        testlib.equal(1, get_count(blobs6.alwaysfull), "alwaysfull count");
        // ----------------------------------------------------------------
        // build a chgs object to convert everything to zlib
        var specific = {};
        var demands = {};
        demands.zlib_savings_over_full = 0;
        for (i=0; i<ba.length; i++)
        {
            specific[ba[i]] = {};
            specific[ba[i]].encoding = sg.blobencoding.ZLIB;
        }

        // clone again
        var r7name = sg.gid();
        sg.clone__specific_changes(
                { 
                    "existing_repo" : r6name, 
                    "new_repo" : r7name, 
                    "specific" : specific,
                    "demand_zlib_savings_over_full" : 0 
                }
                );
        
        // fetch the blob list from the clone
        var repo = sg.open_repo(r7name);
        var blobs7 = get_blob_stats(repo);
        repo.close();

        // make sure everything is zlib
        testlib.equal(ba.length, get_count(blobs7.zlib), "zlib count");
        testlib.equal(0, get_count(blobs7.vcdiff), "vcdiff count");
        testlib.equal(0, get_count(blobs7.full), "full count");
        testlib.equal(0, get_count(blobs7.alwaysfull), "alwaysfull count");

        // ----------------------------------------------------------------
        // build a chgs object to convert everything to full
        var specific = {};
        for (i=0; i<ba.length; i++)
        {
            specific[ba[i]] = {};
            specific[ba[i]].encoding = sg.blobencoding.ALWAYSFULL;
        }

        // clone again
        var r8name = sg.gid();
        sg.clone__specific_changes(
                { 
                    "existing_repo" : r7name, 
                    "new_repo" : r8name,
                    "specific" : specific
                }
                );
        
        // fetch the blob list from the clone
        var repo = sg.open_repo(r8name);
        var blobs8 = get_blob_stats(repo);
        repo.close();

        // make sure everything is zlib
        testlib.equal(0, get_count(blobs8.zlib), "zlib count");
        testlib.equal(0, get_count(blobs8.vcdiff), "vcdiff count");
        testlib.equal(0, get_count(blobs8.full), "full count");
        testlib.equal(ba.length, get_count(blobs8.alwaysfull), "alwaysfull count");
        */
    };
     
    //test_2_usermap moved to st_sync_closet_users.js so it can use the user master repo already set up there

    this.test_name_conflict = function()
    {
        var correctErr = false;
        try
        {
            sg.clone__exact(repInfo.repoName, repInfo.repoName);
        }
        catch(e)
        {
                if (e.message.indexOf("Error 233") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
                    bCorrectFailure = true;
                else
                    print(sg.to_json__pretty_print(e));
        }
        testlib.testResult(bCorrectFailure, "Clone should fail with sglib error 233.");

        var descriptor = sg.get_descriptor(repInfo.repoName);
        testlib.testResult(descriptor !== null && descriptor !== undefined, "descriptor should exist");
        var repo = sg.open_repo(repInfo.repoName);
        try
        {
            testlib.testResult(repo !== null && repo !== undefined, "repo should exist");
        }
        finally
        {
            repo.close();
        }
    };
}

