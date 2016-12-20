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

var run_this_test = testlib.defined.SG_NIGHTLY_BUILD;
//var run_this_test = true;

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

function count_vcdiff_blobs(reponame)
{
    var repo = sg.open_repo(reponame);
    var blobs = get_blob_stats(repo);
    repo.close();
    return blobs.vcdiff.count;
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

function sleepStupidly(usec)
{
    var endtime= new Date().getTime() + (usec * 1000);
    while (new Date().getTime() < endtime)
    {
    }
}

function commitWrapper(obj)
{
    obj.changesets.push(vscript_test_wc__commit());
    sleepStupidly(1);
    //obj.times.push(new Date().getTime());
    //sleepStupidly(1);
}
	
function st_PackMain() 
{
    this.changesets = new Array();
    this.changesets.push("initial commit");
    this.times = new Array();
    this.times.push(0);

    this.setUp = function() 
    {
        if (!run_this_test) return;

		sg.fs.mkdir("test");
		for (i = 2 ; i < 26; i++)
		{
			sg.fs.mkdir("test" + i);
		}
		vscript_test_wc__addremove();
		commitWrapper(this);

		sg.file.write("file", "contents");
		vscript_test_wc__addremove();
		commitWrapper(this);

		for (i = 0 ; i < 12; i++)
		{
			sg.file.append("file", "contents");
			commitWrapper(this);
		}
    }
    
    this.testPackMain = function() 
    {
        if (!run_this_test) return;

        testlib.ok(count_vcdiff_blobs(repInfo.repoName) == 0);

        var newname = sg.gid();
   		var output = testlib.execVV_lines("clone", repInfo.repoName, newname, "--pack");

        testlib.ok(count_vcdiff_blobs(newname) > 0);
    }
}

function st_PackFullRevisions() 
{
    this.changesets = new Array();
    this.changesets.push("initial commit");
    this.times = new Array();
    this.times.push(0);

    this.setUp = function() 
    {
        if (!run_this_test) return;

        sg.fs.mkdir("test");
        for (i = 2 ; i < 26; i++)
        {
            sg.fs.mkdir("test" + i);
        }
        vscript_test_wc__addremove();
        commitWrapper(this);
        sg.file.write("file", "contents");
        vscript_test_wc__addremove();
        commitWrapper(this);
        for (i = 0 ; i < 12; i++)
        {
            sg.file.append("file", "contents");
            commitWrapper(this);
        }
    }
    
    this.testPackMain = function() 
    {
        if (!run_this_test) return;

        testlib.ok(count_vcdiff_blobs(repInfo.repoName) == 0);

        var newname = sg.gid();
   		var output = testlib.execVV_lines("clone", repInfo.repoName, newname, "--pack", "--full-revisions=3");

        testlib.ok(count_vcdiff_blobs(newname) > 0);
    }
}

function st_PackKeyframeDensity() 
{
    this.changesets = new Array();
    this.changesets.push("initial commit");
    this.times = new Array();
    this.times.push(0);

    this.setUp = function() 
    {
        if (!run_this_test) return;

        sg.fs.mkdir("test");
        for (i = 2 ; i < 26; i++)
        {
            sg.fs.mkdir("test" + i);
        }
        vscript_test_wc__addremove();
        commitWrapper(this);
        sg.file.write("file", "contents");
        vscript_test_wc__addremove();
        commitWrapper(this);
        for (i = 0 ; i < 12; i++)
        {
            sg.file.append("file", "contents");
            commitWrapper(this);
        }
    }
    
    
    this.testPackMain = function() 
    {
        if (!run_this_test) return;

        testlib.ok(count_vcdiff_blobs(repInfo.repoName) == 0);

        var newname = sg.gid();
   		var output = testlib.execVV_lines("clone", repInfo.repoName, newname, "--pack", "--keyframe-density=3");

        testlib.ok(count_vcdiff_blobs(newname) > 0);
    }
}

function st_PackMinRevisions() 
{
    this.changesets = new Array();
    this.changesets.push("initial commit");
    this.times = new Array();
    this.times.push(0);

    this.setUp = function() 
    {
        if (!run_this_test) return;

        sg.fs.mkdir("test");
        for (i = 2 ; i < 26; i++)
        {
            sg.fs.mkdir("test" + i);
        }
        sg.file.write("file", "contents");
        vscript_test_wc__addremove();
        commitWrapper(this);
        for (i = 0 ; i < 12; i++)
        {
            sg.file.append("file", "contents");
            commitWrapper(this);
        }
    }
        
    
    this.testPackMain = function() 
    {
        if (!run_this_test) return;

        testlib.ok(count_vcdiff_blobs(repInfo.repoName) == 0);

        var newname = sg.gid();
   		var output = testlib.execVV_lines("clone", repInfo.repoName, newname, "--pack", "--min-revisions=14");

        testlib.ok(count_vcdiff_blobs(newname) > 0);
    }
}

