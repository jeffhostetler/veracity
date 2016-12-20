load("../js_test_lib/vscript_test_lib.js");

function st_list_descriptors_and_reverse_map()
{
    load("update_helpers.js");
    initialize_update_helpers(this);

    //////////////////////////////////////////////////////////////////

    this.no_setup = true;		// do not create an initial REPO and WD.

    this.loop_limit = 2;	// reverse mapping is very expensive and my test closet has 40,000 repos...

    //////////////////////////////////////////////////////////////////

    this.test_main = function()
    {
	var vhDescList = sg.list_descriptors();

	print( sg.to_json__pretty_print( vhDescList ) );

	var k = 0;
	for (var name_k in vhDescList)
	{
	    var data_k = vhDescList[name_k];
	    var dir_k  = data_k["dir_name"];
	    print( k + " desc: " + name_k + " dir: " + dir_k );

	    if (k >= this.loop_limit)
		break;
	    k = k + 1;
	}

	var k = 0;
	for (var name_k in vhDescList)
	{
	    var repo_k = sg.open_repo(name_k);
	    var repo_id_k = repo_k.repo_id;
	    repo_k.close();

	    print( k + " desc: " + name_k + " repoid: " + repo_id_k );

	    var reverse_info = sg.repoid_to_descriptors(repo_id_k);

	    print( sg.to_json__pretty_print(reverse_info) );

	    if (k >= this.loop_limit)
		break;
	    k = k + 1;
	}
    }
}
