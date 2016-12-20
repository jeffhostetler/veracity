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

//////////////////////////////////////////////////////////////////
// Basic MERGE/RESOLVE tests that DO NOT require UPDATE nor REVERT.
//////////////////////////////////////////////////////////////////

function st_wc_merge_resolve_000__W8185()
{
    var my_group = "st_wc_merge_resolve_000__W8185";	// this variable must match the above group name.

    this.no_setup = true;		// do not create an initial REPO and WD.

    //////////////////////////////////////////////////////////////////

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////

    this.test_it = function()
    {
	var unique = my_group + "_" + new Date().getTime();

	var repoName = unique;
	var rootDir = pathCombine(tempDir, unique);
	var wdDirs = [];
	var csets = {};
	var nrWD = 0;

	var my_make_wd = function(obj)
	{
	    var new_wd = pathCombine(rootDir, "wd_" + nrWD);

	    vscript_test_wc__print_section_divider("Creating WD[" + nrWD + "]: " + new_wd);

	    obj.do_fsobj_mkdir_recursive( new_wd );
	    obj.do_fsobj_cd( new_wd );
	    wdDirs.push( new_wd );
	    
	    nrWD += 1;

	    return new_wd;
	}

	//////////////////////////////////////////////////////////////////
	// Create the first WD and initialize the REPO.

	my_make_wd(this);
	sg.vv2.init_new_repo( { "repo" : repoName,
				"hash" : "SHA1/160",
				"path" : "."
			      } );
	whoami_testing(repoName);

	//////////////////////////////////////////////////////////////////
	// Build a sequence of CSETs using current WD.

	this.do_fsobj_mkdir("dir_A");
	this.do_fsobj_mkdir("dir_A/dir_AA");
	this.do_fsobj_mkdir("dir_A/dir_AA/dir_AAA");
	vscript_test_wc__addremove();
	csets.A = vscript_test_wc__commit("A");

	//////////////////////////////////////////////////////////////////
	// Start a sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	this.do_fsobj_mkdir("dir_X");
	this.do_fsobj_mkdir("dir_X/dir_BB");
	vscript_test_wc__addremove();
	this.gid_BX = sg.wc.get_item_gid_path({"src":"dir_X"});
	csets.B = vscript_test_wc__commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	this.do_fsobj_mkdir("dir_X");
	this.do_fsobj_mkdir("dir_X/dir_CC");
	vscript_test_wc__addremove();
	this.gid_CX = sg.wc.get_item_gid_path({"src":"dir_X"});
	csets.C = vscript_test_wc__commit("C");

	//////////////////////////////////////////////////////////////////
	// Now start some MERGES using a fresh WD for each.
	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Checkout B and merge C into it.");

	my_make_wd(this);
	vscript_test_wc__checkout_np( { "repo"   : repoName,
					"attach" : "master",
					"rev"    : csets.B
				      } );
	vscript_test_wc__merge_np( { "rev" : csets.C } );

	// see what a regular STATUS says.

	var expect_test = new Array;
	expect_test["Added (Merge)"]                 = [ this.gid_CX, "@/dir_X~g[0-9a-f]*/dir_CC" ];
	expect_test["Renamed"]                       = [ this.gid_BX ];
	expect_test["Unresolved"]                    = [ this.gid_BX, this.gid_CX ];
	expect_test["Choice Unresolved (Existence)"] = [ this.gid_BX, this.gid_CX ];
        vscript_test_wc__confirm_wii(expect_test);

	// see what the new MSTATUS says.

	var expect_test = new Array;
	expect_test["Added (B)"]                     = [ this.gid_BX, "@/dir_X~g[0-9a-f]*/dir_BB" ];
	expect_test["Added (C)"]                     = [ this.gid_CX, "@/dir_X~g[0-9a-f]*/dir_CC" ];
	expect_test["Renamed (WC)"]                  = [ this.gid_BX, this.gid_CX ];
	expect_test["Unresolved"]                    = [ this.gid_BX, this.gid_CX ];
	expect_test["Choice Unresolved (Existence)"] = [ this.gid_BX, this.gid_CX ];
        vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE 1:");

	vscript_test_wc__resolve__list_all();
	// TODO            Existence: this.gid_BX
	// TODO            Existence: this.gid_CX

	// "accept baseline" on dir_X created in baseline B means
	// that we should keep the directory.
	//
	// Use @b syntax to avoid needing to use wildcards in command.
	vscript_test_wc__resolve__accept("baseline", "@b/dir_X");
	vscript_test_wc__resolve__list_all();
	// TODO            Existence: @/dir_X (this.gid_BX)
	// TODO            Existence: this.gid_CX

	var expect_test = new Array;
	expect_test["Added (B)"]                     = [ "@/dir_X",   "@/dir_X/dir_BB" ];
	expect_test["Added (C)"]                     = [ this.gid_CX, "@/dir_X~g[0-9a-f]*/dir_CC" ];
	expect_test["Renamed (WC)"]                  = [ this.gid_CX ];
	expect_test["Resolved"]                      = [ "@/dir_X" ];
	expect_test["Choice Resolved (Existence)"]   = [ "@/dir_X" ];
	expect_test["Unresolved"]                    = [ this.gid_CX ];
	expect_test["Choice Unresolved (Existence)"] = [ this.gid_CX ];
        vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE 2:");

	// "accept baseline" on dir_X created in C means that it
	// should not exist in the result.
	//
	// TODO 2012/04/12 See W8185.  By accepting that dir_X (from C) should not
	// TODO            exist, what happens to dir_X/dir_CC?  That is, should the
	// TODO            attempted remove of dir_X be recursive/forced or should
	// TODO            it throw because there is a controlled item within it?
	// TODO            For now I'm going to assume fully recursive because that
	// TODO            agrees with what the PendingTree version of the code did.

	vscript_test_wc__resolve__accept("baseline", "@c/dir_X");
	vscript_test_wc__resolve__list_all();
	// TODO            Existence: @/dir_X  -- from B
	// TODO            Existence: non-existence @/dir_X  -- from C

	var expect_test = new Array;
	expect_test["Added (B)"]                   = [ "@/dir_X", "@/dir_X/dir_BB" ];
	expect_test["Added (C)"]                   = [ "@c/dir_X", "@c/dir_X/dir_CC" ];
	expect_test["Removed (WC)"]                = [ "@c/dir_X", "@c/dir_X/dir_CC" ];	// See W8185.
	expect_test["Resolved"]                    = [ "@/dir_X", "@c/dir_X" ];
	expect_test["Choice Resolved (Existence)"] = [ "@/dir_X", "@c/dir_X" ];
        vscript_test_wc__mstatus_confirm_wii(expect_test);

    }

}
