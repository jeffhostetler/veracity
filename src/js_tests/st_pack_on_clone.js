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
    var all = bs.get({ "limit" : 3000, "skip" : 0});
    for (var hid in all)
    {
        ba.push(hid);
    }
    bs.cleanup();

    return ba;
}

function st_pack_on_clone() 
{
    // Helper functions are in update_helpers.js
    // There is a reference list at the top of the file.
    // After loading, you must call initialize_update_helpers(this)
    // before you can use the helpers.
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    this.test_1 = function()
    {
        var r1name = repInfo.repoName;

        this.do_fsobj_mkdir("dir_0");
        for (var i=10; i<=49; i++)
        {
            this.do_fsobj_create_file("file_" + i.toString());
        }
        vscript_test_wc__addremove();
        this.do_commit("initial");

        for (i=0; i<42; i++)
        {
            for (var j=10; j<=49; j++)
            {
                this.do_fsobj_append_file("file_" + j.toString());
            }
            this.do_commit("mod transaction " + i.toString());
        }

        // open r1, then close it
        var repo = sg.open_repo(r1name);
        var blobs1 = get_blob_stats(repo);
        repo.close();

        // throwaway clone
        var r2name = sg.gid();
        sg.clone__exact(r1name, r2name); 

        //repInfo.repoName

        var repo = sg.open_repo(r2name);
        var blobs1 = get_blob_stats(repo);
        var ba = get_blobs(repo);
        var count_total = get_count(blobs1.zlib)
                          + get_count(blobs1.full)
                          + get_count(blobs1.alwaysfull)
                          + get_count(blobs1.vcdiff);
        repo.close();

        print("count_total: ", count_total);
        print("ba.length: ", ba.length);
        testlib.ok(count_total == ba.length);

        // ----------------------------------------------------------------
        // clone it exact
        var r2name = sg.gid();
        sg.clone__exact(r1name, r2name); 

        // fetch the blob list from the clone
        var repo = sg.open_repo(r2name);
        var blobs2 = get_blob_stats(repo);
        repo.close();

        testlib.equal(get_count(blobs1.zlib), get_count(blobs2.zlib), "zlib count");
        testlib.equal(get_count(blobs1.vcdiff), get_count(blobs2.vcdiff), "vcdiff count");
        testlib.equal(get_count(blobs1.full), get_count(blobs2.full), "full count");
        testlib.equal(get_count(blobs1.alwaysfull), get_count(blobs2.alwaysfull), "alwaysfull count");

        // ----------------------------------------------------------------
        // clone it, PACK
        var r4name = sg.gid();
        sg.clone__pack(
                { "existing_repo" : r1name, "new_repo" : r4name }
                );

        // fetch the blob list from the clone
        var repo = sg.open_repo(r4name);
        var blobs4 = get_blob_stats(repo);
        repo.close();

        // make sure everything is right
        testlib.equal(ba.length, (get_count(blobs4.zlib) + get_count(blobs4.vcdiff) + get_count(blobs4.full) + get_count(blobs4.alwaysfull)), "total count");
        testlib.ok(get_count(blobs4.vcdiff)>0);

    }

}
