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

function my_create_file(relPath, nlines) 
{
    var numlines = nlines || Math.floor(Math.random() * 25) + 1;
    var someText = "file contents for version 1 - " + Math.floor(Math.random() * numlines);

    if (!sg.fs.exists(relPath)) 
    {
        sg.file.write(relPath, someText + "\n");
    }

    for (var i = 0; i < numlines; i++) 
    {
        sg.file.append(relPath, someText + "\n");
    }

    return relPath;

}

function st_U4298_W5479_W2282_W0836_K1322_K2177_W6015_W0516_W8132_A3001()
{
    this.no_setup = true;

    this.test = function test() 
    {
        // -------- create r1

        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, "r1." + r1_name);
	print("r1_path is: " + r1_path);

        if (!sg.fs.exists(r1_path)) 
            sg.fs.mkdir_recursive(r1_path);
	sg.fs.cd(r1_path);

        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	sg.vv2.whoami({ "username" : "ringo", "create" : true });

        my_create_file("f0.txt", 51);
	vscript_test_wc__addremove();
	var cs0 = vscript_test_wc__commit();

        my_create_file("f1.txt", 51);
	vscript_test_wc__addremove();
	var cs1 = vscript_test_wc__commit();

        my_create_file("f2.txt", 51);
	vscript_test_wc__addremove();
	var cs2 = vscript_test_wc__commit();

	o = sg.exec(vv, "stamp", "add", "s_cs2", "--rev", cs2);
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);

	// So at this point the r1 instance has:
	//
	//   cs0
	//    |
	//   cs1
	//    |
	//   cs2  "master"

	print("After commit in r1 with whoami=" + sg.vv2.whoami());
	print("");
	print("===============");
	print("History:");
	o = sg.exec(vv,"history");
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);

	print("");
	print("===============");
	print("Branches:");
	print(sg.exec(vv, "branches", "--all").stdout);
	print("");
	print("===============");
	print("Heads:");
	print(sg.exec(vv,"heads").stdout);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Begin r2 instance");
        // -------- clone r1 r2

        var r2_name = sg.gid();
        var r2_path = pathCombine(tempDir, "r2." + r2_name);
	print("r2_path is: " + r2_path);

        o = sg.exec(vv, "clone", r1_name, r2_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- checkout r2 as of cs0

        o = sg.exec(vv, "checkout", "-r", cs0, r2_name, r2_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	sg.fs.cd(r2_path);

	sg.exec(vv, "branch", "attach", "master");

        my_create_file("f3.txt", 51);
	vscript_test_wc__addremove();
	var cs1a = vscript_test_wc__commit();

	o = sg.exec(vv, "stamp", "add", "s_cs1a", "--rev", cs1a);
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);

        my_create_file("f4.txt", 51);
	vscript_test_wc__addremove();
	var cs2a = vscript_test_wc__commit();

	o = sg.exec(vv, "stamp", "add", "s_cs2a", "--rev", cs2a);
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);

	// So at this point the r2 instance has:
	//
	//           cs0
	//          /   \
	//      cs1a     cs1
	//     /            \
	// cs2a "master"     cs2 "master"

	print("After checkout in r2 with whoami=" + sg.vv2.whoami());
	print("");
	print("===============");
	print("History:");
	o = sg.exec(vv,"history");
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);

	print("");
	print("===============");
	print("Branches:");
	o = sg.exec(vv, "branches", "--all");
	print(o.stdout);
	testlib.ok( (o.stdout.indexOf("master (needs merge)") >= 0), "Confirm needs-merge." );

	print("");
	print("===============");
	print("Heads:");
	print(sg.exec(vv,"heads").stdout);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Begin 'push -r' of cs1a but not cs2a");

        // -------- in r2, push only cs1a to r1

        o = sg.exec(vv, "push", "-r", cs1a);
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Check what r1 now has.");

	sg.fs.cd(r1_path);

	// So at this point the r1 instance has:
	//
	//           cs0
	//          /   \
	//      cs1a     cs1
	//     /            \
	// ???? "master"     cs2 "master"

	testlib.ok( (sg.wc.parents()[0] == cs2), "Confirm r1 parent is cs2.");

	// With the changes for W2282, we don't report need-merge for non-present heads.
	// We give present/non-present stats when --all is used.
	print("");
	print("===============");
	print("Branches:");
	o = sg.exec(vv, "branches");
	print(o.stdout);
	testlib.ok( (o.stdout.indexOf("master (needs merge)") < 0), "Confirm NOT needs-merge." );

	o = sg.exec(vv, "branches", "--all");
	print(o.stdout);
	testlib.ok( (o.stdout.indexOf("master (needs merge)") < 0), "Confirm NOT needs-merge." );
	testlib.ok( (o.stdout.indexOf("master (1 of 2 not present in repository)") >= 0), "Confirm not-present." );

	print("");
	print("===============");
	print("Heads:");
	o = sg.exec(vv,"heads");
	print(o.stdout);
	testlib.ok( (o.stdout.indexOf("not present in repository") >= 0), "Confirm not-present." );

	// With the fix for W0836, we now exclude the non-present heads
	// from the history query.  (Since we use no args, the history
	// code effectively does "vv history --branch `vv branch`".)
	print("");
	print("===============");
	print("History:");
	o = sg.exec(vv,"history");
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);

	// With the fix for U4298, we now exclude non-present heads
	// from consideration when looking for other heads of the
	// (implied master) branch.
	print("");
	print("===============");
	print("merge (implied master branch) test:");
	o = sg.exec(vv, "merge", "--test");    // try to merge ???? into cs2
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.stdout.indexOf("No merge is needed.") >= 0), "Confirm implied merge on master does nothing.");
        testlib.ok(o.exit_status == 0);

	// With the fix for U4298, we now exclude non-present heads
	// from consideration when looking for the target head for
	// the update.  Since cs2 is a head, nothing should happen.
	print("");
	print("===============");
	print("update:");
	o = sg.exec(vv, "update", "--test");
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.stdout.indexOf("0 deleted, 0 added") >= 0), "Confirm update would do nothing.");
        testlib.ok(o.exit_status == 0);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Update r1 back to cs1 explicitly.");

	// update back to cs1 explicitly and try the default update again.
	o = sg.exec(vv, "update", "-r", cs1, "-c");
	print(o.stdout);
	testlib.ok( (o.exit_status == 0), "Expect update to cs1 to succeed.");
	testlib.ok( (sg.wc.parents()[0] == cs1), "Confirm r1 parent is cs1.");

	// cs2 and cs2a are heads of the branch, but cs2a
	// isn't present in the r1 instance, so if we 
	// ignored the non-present head, we should be able
	// to see a simple update to cs2.

	// U4298
	print("update:");
	o = sg.exec(vv, "update", "--test");
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.exit_status == 0), "Expect test update to cs2 to succeed.");
	testlib.ok( (o.stdout.indexOf("0 deleted, 1 added") >= 0), "Expect test update to cs2 to succeed.");

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Return to r2 and create 'zzz' branch.");

	sg.fs.cd(r2_path);

	// update back to cs1a explicitly and create new branch.
	o = sg.exec(vv, "update", "-r", cs1a, "--attach-new", "zzz");
	print(o.stdout);
	testlib.ok( (o.exit_status == 0), "Expect update to cs1a to succeed.");
	testlib.ok( (sg.wc.parents()[0] == cs1a), "Confirm r2 parent is cs1a.");

        my_create_file("f5.txt", 51);
	vscript_test_wc__addremove();
	var cs2b = vscript_test_wc__commit();

	o = sg.exec(vv, "stamp", "add", "s_cs2b", "--rev", cs2b);
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);

	// So at this point the r2 instance has:
	//
	//                      cs0
	//                 ______|______
	//                /             \
	//            cs1a               cs1
	//           /    \                 \
	// cs2b "zzz"      cs2a "master"     cs2 "master"

	print("");
	print("===============");
	print("Branches:");
	o = sg.exec(vv, "branches", "--all");
	print(o.stdout);
	testlib.ok( (o.stdout.indexOf("master (needs merge)") >= 0) );
	testlib.ok( (o.stdout.indexOf("zzz") >= 0) );

	print("");
	print("===============");
	print("Heads:");
	print(sg.exec(vv,"heads").stdout);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Begin 'push -r cs1a' again");

        // -------- in r2, push only cs1a to r1

        o = sg.exec(vv, "push", "-r", cs1a);
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Check what r1 now has.");

	sg.fs.cd(r1_path);

	// So at this point the r1 instance has:
	//
	//                      cs0
	//                 ______|______
	//                /             \
	//            cs1a               cs1
	//           /    \                 \
	// ???? "zzz"      ???? "master"     cs2 "master"

	// With the changes for W2282, we don't report need-merge for non-present heads.
	// We give present/non-present stats when --all is used.
	print("");
	print("===============");
	print("Branches:");
	o = sg.exec(vv, "branches", "--all");
	print(o.stdout);
	testlib.ok( (o.stdout.indexOf("master (needs merge)") < 0), "Confirm NOT needs-merge." );
	testlib.ok( (o.stdout.indexOf("master (1 of 2 not present in repository)") >= 0), "Confirm not-present." );
	testlib.ok( (o.stdout.indexOf("zzz (1 of 1 not present in repository") >= 0), "Confirm r1 has zzz branch name.");

	print("");
	print("===============");
	print("Heads:");
	o = sg.exec(vv,"heads");
	print(o.stdout);
	testlib.ok( (o.stdout.indexOf("not present in repository") >= 0), "Confirm not-present in r1." );

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try to update/merge r1 with zzz.");

	print("");
	print("===============");
	print("merge test:");
	o = sg.exec(vv, "merge", "-b", "zzz", "--test");    // try to merge ???? into cs1 -- this SHOULD fail
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.exit_status != 0) );
	testlib.ok( (o.stderr.indexOf("The branch references a changeset which is not present: Branch 'zzz' refers") >= 0), "Confirm merge -b zzz broken.");

	// The following is NOT an example of U4298 because there is only 1 head for 'zzz'.
	print("");
	print("===============");
	print("update:");
	o = sg.exec(vv, "update", "-b", "zzz", "--test");	// this SHOULD fail
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.exit_status != 0) );
	testlib.ok( (o.stderr.indexOf("The branch references a changeset which is not present: Branch 'zzz' refers") >= 0), "Confirm update -b zzz broken.");

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Back to r2 and create a tag for W5479.");

	sg.fs.cd(r2_path);

	// put a TAG on a cset that we ARE NOT pushing.

	o = sg.exec(vv, "tag", "add", "my_tag", "-b", "zzz");
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.exit_status == 0) );

	print("");
	print("===============");
	print("Tags (as seen in r2):");
	o = sg.exec(vv,"tags");
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.exit_status == 0) );

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Begin 'push -r cs1a' again (won't push cset with tag)");

        // -------- in r2, push only cs1a to r1

        o = sg.exec(vv, "push", "-r", cs1a);
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Back to r1 and see what tags we can see.");

	sg.fs.cd(r1_path);

	// With the fix for W5479, we now report tags associated with 
	// missing csets with "?" for the revno and also label it as such.
	print("");
	print("===============");
	print("Tags (as seen in r1):");
	o = sg.exec(vv,"tags");
	print(o.stdout);
	print(o.stderr);
	var re = new RegExp("[ ]*my_tag:[ ]*\\?:[0-9a-f]* \\(not present in repository\\)");
	testlib.ok( (re.test(o.stdout)), "Confirm tag from r1.");
	testlib.ok( (o.exit_status == 0) );

	// See what happens when we use a --tag arg on a command when
	// we have the tag but not the referenced cset.
	print("");
	print("===============");
	print("Trying to use '--tag' in history in r1:");
	o = sg.exec(vv, "history", "--tag", "my_tag");		// W8132
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.stderr.indexOf("The requested changeset is not present in the repository") >= 0) );
	testlib.ok( (o.exit_status != 0) );

	// See what happens when we use a --tag arg on a command when
	// we have the tag but not the referenced cset.
	print("");
	print("===============");
	print("Trying to use '--tag' in history in r1:");
	o = sg.exec(vv, "history", "--repo", r1_name, "--tag", "my_tag");		// W8132
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.stderr.indexOf("The requested changeset is not present in the repository") >= 0) );
	testlib.ok( (o.exit_status != 0) );

	// See what happens when we use a --tag arg on a command when
	// we have the tag but not the referenced cset.
	print("");
	print("===============");
	print("Trying to use '--tag' in status in r1:");
	o = sg.exec(vv, "status", "--rev", cs0, "--tag", "my_tag");
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.stderr.indexOf("The requested changeset is not present in the repository") >= 0) );
	testlib.ok( (o.exit_status != 0) );

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try to do a checkout of non-present cset using master from r1.");

	var wd3_path = pathCombine(tempDir, "wd3.r1." + r1_name);
	print("wd3_path is: " + wd3_path);

	sg.fs.cd(tempDir);

	// In r1, the "master" branch has 1 present and 1 non-present head.
	// With the fix for W0516, we should ignore the non-present head
	// and just use the present one.
	print("");
	print("===============");
	print("checkout --branch master");
	o = sg.exec(vv, "checkout", "--branch", "master", r1_name, wd3_path);
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.exit_status == 0) );

	sg.fs.cd(wd3_path);
	testlib.ok( (sg.wc.parents()[0] == cs2), "Confirm wd3 parent is cs2.");

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try to do a checkout of non-present cset using zzz from r1.");

	var wd4_path = pathCombine(tempDir, "wd4.r1." + r1_name);
	print("wd4_path is: " + wd4_path);

	sg.fs.cd(tempDir);

	// The following is NOT an example of W0516 because the head of 
	// branch "zzz" is not present in the local repo.
	print("");
	print("===============");
	print("checkout --branch zzz");
	o = sg.exec(vv, "checkout", "--branch", "zzz", r1_name, wd4_path);
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.stderr.indexOf("The branch references a changeset which is not present: Branch 'zzz' refers") >= 0), "Confirm checkout on zzz broken.");
	testlib.ok( (o.exit_status != 0) );

	// Try checkout using tag not present in local repo.
	print("");
	print("===============");
	print("checkout --tag my_tag");
	o = sg.exec(vv, "checkout", "--tag", "my_tag", r1_name, wd4_path);
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.stderr.indexOf("The requested changeset is not present in the repository: Tag 'my_tag' refers") >= 0), "Confirm checkout on my_tag broken.");
	testlib.ok( (o.exit_status != 0) );

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Begin r3 instance");

        // -------- clone r1 r3

        var r3_name = sg.gid();
        var r3_path = pathCombine(tempDir, "r3." + r3_name);
	print("r3_path is: " + r3_path);

        o = sg.exec(vv, "clone", r1_name, r3_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	sg.fs.cd(r1_path);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try pushing current contents of r1 to r3.");

	// In r1, the "master" branch has 1 present and 1 non-present head.	
	// With the changes for W6015, we should be able to push the master
	// branch and have it only push the one we have.
	print("===============");
	print("push master from r1 to r3");
        o = sg.exec(vv, "push", "--branch", "master", r3_name);
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try to do a checkout using master from r3.");

	var wd5_path = pathCombine(tempDir, "wd5.r3." + r3_name);
	print("wd5_path is: " + wd5_path);

	sg.fs.cd(tempDir);

	// Only 1 master head should have been pushed to r3.
	// Try to do a plain checkout and confirm that.
	print("");
	print("===============");
	print("checkout --branch master");
	o = sg.exec(vv, "checkout", "--branch", "master", r3_name, wd5_path);
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.exit_status == 0) );

	sg.fs.cd(wd5_path);
	testlib.ok( (sg.wc.parents()[0] == cs2), "Confirm wd5 parent is cs2.");
	
	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Back in r2, test A3001.");

	sg.fs.cd(r2_path);

	// So at this point the r2 instance has:
	//
	//                      cs0
	//                 ______|______
	//                /             \
	//            cs1a               cs1
	//           /    \                 \
	// cs2b "zzz"      cs2a "master"     cs2 "master"

	//////////////////////////////////////////////////////////////////

	print("");
	print("===============");
	print("update back to cs0 (detached)");
	o = sg.exec(vv, "update", "-r", cs0, "--detached");
	print(o.stdout);
	testlib.ok( (o.exit_status == 0), "Expect update to cs0 to succeed.");
	testlib.ok( (sg.wc.parents()[0] == cs0), "Confirm r2 parent is cs0.");

	// cs2 and cs2a are both heads of master.
	// Try to merge master into cs0 and see if it
	// complains with a meaningful message.  In theory,
	// this could fail because both are descendants of
	// the baseline, but that check comes after we have
	// decided on a unique target.
	//
	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs0:");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. 2 are descendants of the current baseline. Consider updating to one of them. You are not attached to a branch.") >= 0), "Confirm ambiguous merge.");

	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs0 (when attached to zzz):");
	sg.exec(vv, "branch", "attach", "zzz");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. 2 are descendants of the current baseline. Consider updating to one of them. You are attached to branch 'zzz'.") >= 0), "Confirm ambiguous merge.");

	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs0 (when attached to master):");
	sg.exec(vv, "branch", "attach", "master");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. 2 are descendants of the current baseline. Consider updating to one of them and then merging the branch.") >= 0), "Confirm ambiguous merge.");

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Begin r4 instance");

        // -------- clone r2 r4

        var r4_name = sg.gid();
        var r4_path = pathCombine(tempDir, "r4." + r4_name);
	print("r4_path is: " + r4_path);

        o = sg.exec(vv, "clone", r2_name, r4_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	print("");
	print("===============");
	print("checkout --rev cs2");
	o = sg.exec(vv, "checkout", "--rev", cs2, r4_name, r4_path);
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.exit_status == 0) );

	sg.fs.cd(r4_path);
	testlib.ok( (sg.wc.parents()[0] == cs2), "Confirm r4 parent is cs2.");
	var info = sg.wc.get_wc_info();
	testlib.ok( (info.branch == undefined), "Confirm r4 not attached.");

	print("");
	print("===============");
	print("create cs3 (anonymous head)");
        my_create_file("r4_1.txt", 51);
	vscript_test_wc__addremove();
	var cs3 = vscript_test_wc__commit_np( {"detached":true, "message":"cs3 anonymous"} );

	// So at this point the r4 instance has:
	//
	//                      cs0
	//                 ______|______
	//                /             \
	//            cs1a               cs1
	//           /    \                 \
	// cs2b "zzz"      cs2a "master"     cs2 "master"
	//                                     |
	//                                   cs3 (anonymous)

	// cs2 and cs2a are both heads of master.
	// Try to merge master into cs3 and see if it
	// complains with a meaningful message.  In theory,
	// this could fail because one is an ancestor of
	// the baseline, but that check comes after we have
	// decided on a unique target.
	//
	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs3 (when detached):");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. Changeset '"+cs2+"' is an ancestor of the current baseline. Consider moving that head forward. You are not attached to a branch.") >= 0), "Confirm ambiguous merge.");
	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs3 (when attached to zzz):");
	sg.exec(vv, "branch", "attach", "zzz");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. Changeset '"+cs2+"' is an ancestor of the current baseline. Consider moving that head forward. You are attached to branch 'zzz'.") >= 0), "Confirm ambiguous merge.");
	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs3 (when attached to master):");
	sg.exec(vv, "branch", "attach", "master");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. Changeset '"+cs2+"' is an ancestor of the current baseline. Consider moving that head forward and then merging the branch.") >= 0), "Confirm ambiguous merge.");

	//////////////////////////////////////////////////////////////////

	print("");
	print("===============");
	print("update back to cs2a (detached)");
	o = sg.exec(vv, "update", "-r", cs2a, "--detached");
	print(o.stdout);
	testlib.ok( (o.exit_status == 0), "Expect update to cs2a to succeed.");
	testlib.ok( (sg.wc.parents()[0] == cs2a), "Confirm r4 parent is cs2a.");

	print("");
	print("===============");
	print("create cs3a (anonymous head)");
        my_create_file("r4_2.txt", 51);
	vscript_test_wc__addremove();
	var cs3a = vscript_test_wc__commit_np( {"detached":true, "message":"cs3a anonymous"} );

	print("");
	print("===============");
	print("merge anonymous cs3 and cs3a");
	o = sg.exec(vv, "merge", "--rev", cs3);
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);
	var cs4 = vscript_test_wc__commit_np( {"detached":true, "message":"cs4 anonymous"} );

	// So at this point the r4 instance has:
	//
	//                      cs0
	//                 ______|______
	//                /             \
	//            cs1a               cs1
	//           /    \                 \
	// cs2b "zzz"      cs2a "master"     cs2 "master"
	//                   |                 |
	//                 cs3a (anonymous)   cs3 (anonymous)
	//                    \              /
	//                     cs4 (anonymous)

	// cs2 and cs2a are both heads of master.
	// But the current baseline is cs4 which is a
	// descendant of both heads.
	//
	// Try to merge master into cs4 and see if it
	// complains with a meaningful message.  In theory,
	// this could fail because all of the heads are ancestors of
	// the baseline, but that check comes after we have
	// decided on a unique target.
	//
	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs4 (when detached):");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. All of them are ancestors of the current baseline. Consider moving one of the heads forward and removing the others.") >= 0), "Confirm ambiguous merge.");

	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs4 (when attached to zzz):");
	sg.exec(vv, "branch", "attach", "zzz");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. All of them are ancestors of the current baseline. Consider moving one of the heads forward and removing the others.") >= 0), "Confirm ambiguous merge.");

	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs4 (when attached to master (but not a head)):");
	sg.exec(vv, "branch", "attach", "master");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. All of them are ancestors of the current baseline. Consider moving one of the heads forward and removing the others.") >= 0), "Confirm ambiguous merge.");

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Back in r2, more tests for A3001.");

	sg.fs.cd(r2_path);

	print("");
	print("===============");
	print("update back to cs1a (attached to master)");
	o = sg.exec(vv, "update", "-r", cs1a, "--attach", "master");
	print(o.stdout);
	testlib.ok( (o.exit_status == 0), "Expect update to cs1a to succeed.");
	testlib.ok( (sg.wc.parents()[0] == cs1a), "Confirm r2 parent is cs1a.");

	// cs2 and cs2a are both heads of master.
	// Try to merge master into cs1a and see if it
	// complains with a meaningful message.  In theory,
	// this could fail because one is a descendant of
	// the baseline, but that check comes after we have
	// decided on a unique target.
	//
	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs1a (attached to master):");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. Only changeset '"+cs2a+"' is a descendant of the current baseline. Consider updating to it and then merging the branch.") >= 0), "Confirm ambiguous merge.");

	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs1a: (attached to zzz)");
	sg.exec(vv, "branch", "attach", "zzz");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. Only changeset '"+cs2a+"' is a descendant of the current baseline. Consider updating to it. You are attached to branch 'zzz'.") >= 0), "Confirm ambiguous merge.");

	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs1a: (detached)");
	sg.exec(vv, "branch", "detach", "zzz");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. Only changeset '"+cs2a+"' is a descendant of the current baseline. Consider updating to it. You are not attached to a branch.") >= 0), "Confirm ambiguous merge.");

	//////////////////////////////////////////////////////////////////

	print("");
	print("===============");
	print("update back to cs2a");
	o = sg.exec(vv, "update", "-r", cs2a, "--attach", "master");
	print(o.stdout);
	testlib.ok( (o.exit_status == 0), "Expect update to cs2a to succeed.");
	testlib.ok( (sg.wc.parents()[0] == cs2a), "Confirm r2 parent is cs2a.");

	// cs2 and cs2a are both heads of master.
	// Try to merge --branch master.
	// This should work because there is only 1 non-baseline head of the branch.
	//
	// A3001
	print("");
	print("===============");
	print("try to merge --branch master");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);

	//////////////////////////////////////////////////////////////////

	print("");
	print("===============");
	print("update back to cs2b (aka zzz)");
	o = sg.exec(vv, "update", "-r", cs2b, "--attach", "zzz");
	print(o.stdout);
	testlib.ok( (o.exit_status == 0), "Expect update to cs2b to succeed.");
	testlib.ok( (sg.wc.parents()[0] == cs2b), "Confirm r2 parent is cs2b.");

	// cs2 and cs2a are both heads of master.
	// Try to merge master into cs2b and see if it
	// complains with a meaningful message.
	//
	// A3001
	print("");
	print("===============");
	print("try to merge master branch into zzz:");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads. All are peers of the current baseline. Consider merging one of the other heads using --rev/--tag. You are attached to branch 'zzz'.") >= 0), "Confirm ambiguous merge.");

	//////////////////////////////////////////////////////////////////

	// create another head on master using cs2b as a starting point.

	print("");
	print("===============");
	print("create another head on master using cs2b as a starting point.");
	o = sg.exec(vv, "update", "-r", cs2b, "--attach", "master");
	print(o.stdout);
	testlib.ok( (o.exit_status == 0), "Expect update to cs2b to succeed.");
	testlib.ok( (sg.wc.parents()[0] == cs2b), "Confirm r2 parent is cs2b.");

        my_create_file("f6.txt", 51);
	vscript_test_wc__addremove();
	var cs3b = vscript_test_wc__commit();

	// So at this point the r2 instance has:
	//
	//                      cs0
	//                 ______|______
	//                /             \
	//            cs1a               cs1
	//           /    \                 \
	// cs2b "zzz"      cs2a "master"     cs2 "master"
	//   |
	// cs3b "master"

	print("");
	print("===============");
	print("Heads:");
	print(sg.exec(vv,"heads").stdout);

	// cs2 and cs2a and cs3b are all heads of master.
	// Try to merge master into cs3b and see if it
	// complains with a meaningful message.
	//
	// A3001
	print("");
	print("===============");
	print("try to merge master branch into cs3b:");
	o = sg.exec(vv, "merge", "--branch", "master", "--test");    // this should fail
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status != 0);
	testlib.ok( (o.stderr.indexOf("The branch needs to be merged: Branch 'master' has 2 heads (excluding the baseline). Consider merging one of the other heads using --rev/--tag.") >= 0), "Confirm ambiguous merge.");

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Back in r2, test W6609 (stamps).");
	
	sg.fs.cd(r2_path);

	// So at this point the r2 instance has these csets with these stamps:
	//
	//                                  cs0
	//                             ______|______
	//                            /             \
	//               cs1a 's_cs1a'               cs1
	//              /             \                 \
	// cs2b 's_cs2b'               cs2a 's_cs2a'     cs2 's_cs2'

	print("");
	print("===============");
	print("'vv stamp list' (from r2):");
	o = sg.exec(vv, "stamp", "list");
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);
	testlib.ok( (o.stdout.indexOf("s_cs1a:\t1") >= 0) );
	testlib.ok( (o.stdout.indexOf("s_cs2:\t1") >= 0) );
	testlib.ok( (o.stdout.indexOf("s_cs2a:\t1") >= 0) );
	testlib.ok( (o.stdout.indexOf("s_cs2b:\t1") >= 0) );

	print("");
	print("===============");
	print("'vv stamp list s_cs2a' (from r2):");
	o = sg.exec(vv, "stamp", "list", "s_cs2a");
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);
	testlib.ok( (o.stdout.indexOf("revision:  6:"+cs2a) >= 0) );
	testlib.ok( (o.stdout.indexOf(  "branch:  master") >= 0) );
	testlib.ok( (o.stdout.indexOf(     "who:  ringo") >= 0) );
	testlib.ok( (o.stdout.indexOf( "comment:  No Message.") >= 0) );
	testlib.ok( (o.stdout.indexOf(   "stamp:  s_cs2a") >= 0) );

	//////////////////////////////////////////////////////////////////
	// go back to r1 and see what the stamps we have.

	sg.fs.cd(r1_path);

	// W6609 doesn't matter here. We are just printing a tally of the
	// number of each stamp (without regard to whether the associated
	// cset is present or not).
	print("");
	print("===============");
	print("'vv stamp list' (from r1):");
	o = sg.exec(vv, "stamp", "list");
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);
	testlib.ok( (o.stdout.indexOf("s_cs1a:\t1") >= 0) );
	testlib.ok( (o.stdout.indexOf("s_cs2:\t1") >= 0) );
	testlib.ok( (o.stdout.indexOf("s_cs2a:\t1") >= 0) );
	testlib.ok( (o.stdout.indexOf("s_cs2b:\t1") >= 0) );

	// W6609 was to confirm that we can print what info we have
	// for a non-present cset associated with a stamp.
	print("");
	print("===============");
	print("'vv stamp list s_cs2a' (from r1):");
	o = sg.exec(vv, "stamp", "list", "s_cs2a");
	print(o.stdout);
	print(o.stderr);
        testlib.ok(o.exit_status == 0);
	testlib.ok( (o.stdout.indexOf("revision:  "+cs2a) >= 0) );
	testlib.ok( (o.stdout.indexOf(  "branch:  master") >= 0) );
	testlib.ok( (o.stdout.indexOf(           "(not present in repository)") >= 0) );
	testlib.ok( (o.stdout.indexOf(   "stamp:  s_cs2a") < 0), "No details for non-present cset." );
	testlib.ok( (o.stdout.indexOf(     "who:  ringo") < 0), "No details for non-present cset." );
    }
}
