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

///////////////////////////////////////////////////////////////////////////////////////
// Tests involving updates between changesets with various patterns of path swapping.
// There are 10 tests here, any combination of which will fail on a given run.
// The failures indicate pathcycle problems.
///////////////////////////////////////////////////////////////////////////////////////

var globalThis;  // var to hold "this" pointers

function st_update_path_permutations() 
{
    // Helper functions are in update_helpers.js
    // There is a reference list at the top of the file.
    // After loading, you must call initialize_update_helpers(this)
    // before you can use the helpers.
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

//    this.verbose = true; // add this wherever you want it.  It'll stick through the end of the stGroup.

    //////////////////////////////////////////////////////////////////

    this.homedir = "dir_permute/";
    this.parkroot = "park/p";
    this.permutations = [];
    this.usedChars = [];
    this.startPath;
    this.startArray;

    this.permute_sub = function(orderStr)
    {
        var chars = orderStr.split("");
        for (var i = 0; i < chars.length; i++) {
            var ch = chars.splice(i, 1);
            this.usedChars.push(ch);
            if (chars.length == 0) 
                this.permutations[this.permutations.length] = { "array" : this.usedChars.slice(0), "str" : this.usedChars.join("") }
            this.permute_sub(chars.join(""));
            chars.splice(i, 0, ch);
            this.usedChars.pop();
        }
    }

    this.permute = function(depth)
    {
        var allPos = "0123456789";
        var allPath = "x/x/x/x/x/f/g/h/i/j";
        this.startPath = allPath.substr(0, depth * 2 - 1);
        this.startArray = this.startPath.split("/");
        this.permute_sub(allPos.substr(0, depth));
    }

    this.reorder_path = function(orderStr)
    {
        var orderArray = orderStr.split("");
        var destArray = [];
        for (var i = this.srcArray.length - 1; i >= 0; i--) {
            var srcPath = this.srcArray.slice(0, i+1).join("/");
            vscript_test_wc__move(this.homedir+srcPath, this.parkroot + i);
        }
        var destPath = this.homedir;
        for (i = 0; i < orderArray.length; i++) {
            var n = orderArray[i];
            vscript_test_wc__move(this.parkroot + n + "/" + this.srcArray[n], destPath);
            destPath = destPath + this.srcArray[n] + "/";
            destArray[i] = this.srcArray[n];
        }
        this.srcArray = destArray.slice(0);
    }


    //////////////////////////////////////////////////////////////////
    // This is the depth we will be testing.

    if (testlib.defined.SG_NIGHTLY_BUILD)
    {
        this.depth = 6;
    }
    else
    {
        this.depth = 4;
    }

    //////////////////////////////////////////////////////////////////

    this.setUp = function()
    {
        this.workdir_root = sg.fs.getcwd();     // save this for later

        // initialise the permutations array for the selected depth

        this.permute(this.depth);

        // we begin with a clean WD in a fresh REPO.
        // and build a sequence of changesets to use

        //////////////////////////////////////////////////////////////////
        // fetch the HID of the initial commit.

        this.csets.push( sg.wc.parents()[0] );
        this.names.push( "*Initial Commit*" );

        //////////////////////////////////////////////////////////////////
        // build the initial file system for permutation tests

        this.do_fsobj_mkdir(this.homedir);
        this.do_fsobj_mkdir_recursive(this.homedir + this.startPath);
        var destPath = this.homedir;
        for (var i = 0; i < this.startArray.length; i++) {
            destPath = destPath + this.startArray[i] + "/";
            sg.file.write(destPath + this.startArray[i] + ".txt", this.startArray[i] + ".txt");
            this.do_fsobj_mkdir_recursive(this.parkroot + i);
        }
        vscript_test_wc__addremove();
        this.do_commit("Order 0: " + this.permutations[0].str);

        //////////////////////////////////////////////////////////////////
        // Now move the dirs through all the permuted patterns, committing
        // at each step.  Note that we'll be doing progressive moves,
        // so while a/b/c(021)->a/c/b, the next step will modify the previous
        // result:  a/c/b(102)->c/a/b - not a/b/c(102)->b/a/c
        // This is so we can step through the changesets and be sure to
        // test all the correct permutations.

        this.srcArray = this.startArray.slice(0);

        for (var i = 1; i < this.permutations.length; i++)
        {
            this.reorder_path(this.permutations[i].str);
            this.do_commit("Order " + i + ": " + this.permutations[i].str);
        }

        this.list_csets("After setup (from update_helpers.generate_test_changesets())");
                  print("------------------------------------------------------");
    }

    ////////////////////////////////////////////////////////////////////

    this.update_down = function()
    {
        // start by jumping to the initial commit, then back to the head and step our way down
        this.do_update_when_clean(0);
        for (var i = this.csets.length - 1; i >= 0; i--) {
            this.do_update_when_clean(i);
        }
    }

    this.update_up = function()
    {
        // start by jumping to the initial commit, then step our way up to the top
        for (var i = 0; i < this.csets.length; i++) {
            this.do_update_when_clean(i);
        }
    }

    this.tearDown = function() 
    {

    }
}
