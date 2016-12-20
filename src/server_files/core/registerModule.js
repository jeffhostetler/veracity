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

var vvModules = [];
var knownDags = {};

var checkedRepos = {};
var cmdInProgress = false;
var vvUserIdNOBODY = "g3141592653589793238462643383279502884197169399375105820974944592";

function registerModule(def)
{
	vvModules.push(def);
}

function initModules()
{
	for ( var i = 0; i < vvModules.length; ++i )
	{
		var def = vvModules[i];

		for ( var dagname in def.dagnums )
		{
			var details = def.dagnums[dagname];

			var newdag = {
				type: "db",
				vendor: def.vendor,
				grouping: def.grouping,
				id: details.dagid
			};

			var dagnum = sg.make_dagnum(newdag);

			sg.dagnum[dagname] = dagnum;
		}
	}
}

function moduleEnabled(mn)
{
	for ( var i = 0; i < vvModules.length; ++i )
	{
		var def = vvModules[i];

		if (def.area == mn)
			return(! def.disabled);
	}

	return(false);
}

function checkModuleDags(reponame, modulesPath)
{
	if (cmdInProgress)
		return;

	var uid = vvUserIdNOBODY;

	cmdInProgress = true;

	try
	{
		initModules();

		var repo = false;

		try
		{
			repo = sg.open_repo(reponame);
		}
		catch (e)
		{
			repo = false;
		}

		if (! repo)
			return;

		var rkey = reponame + '-' + repo.repo_id;

		if (checkedRepos[rkey])
		{
			repo.close();
			repo = null;
			return;						
		}

		try
		{
			for ( var i = 0; i < vvModules.length; ++i )
			{
				var def = vvModules[i];

				for ( var dagname in def.dagnums )
				{
					var details = def.dagnums[dagname];

					var newdag = {
						type: "db",
						vendor: def.vendor,
						grouping: def.grouping,
						id: details.dagid
					};

					var dagnum = sg.make_dagnum(newdag);

					var a = false;

					try
					{
						a = areaExists(repo, dagnum);
					}
					catch(e)
					{
						repo.close();
						repo = null;
						return;
					}

					if (! a)
					{
						if (! makeArea(repo, def.area, dagnum, uid))
						{
							repo.close();
							repo = null;
							return;
						}
					}

					if (! dagnumExists(reponame, repo, dagnum))
					{
						var tfn = modulesPath + "/" + def.area + "/zing/" + details.template;

						var tf = sg.file.read(tfn);
						var t;

						eval('t = ' + tf);

						if (! setTemplate(repo, dagnum, t, uid))
						{
							repo.close();

							repo = null;
							return;
						}

						knownDags[reponame] = null;
					}
				}

				var hooks = def.hooks || {};

				for ( var iface in hooks )
				{
					var hook = hooks[iface];

					var needed = true;

					var installed = repo.vc_hooks_installed(iface);

					for ( var j = 0; j < installed.length; ++j )
					{
						var oldHook = installed[j];

						if ((oldHook.version >= def.hook_code_sync_rev) && (oldHook.module == def.area))
						{
							needed = false;
							break;
						}
					}

					if (needed)
					{
						var tfn = modulesPath + "/" + def.area + "/scripts/" + hook.file;

						var tf = sg.file.read(tfn);

						var installRequest = {
								"interface": iface,
								"js": tf,
								"module": def.area,
								"version": def.hook_code_sync_rev,
								"replace": hook.replace || false
							};

						if (uid)
							installRequest.userid = uid;

						repo.install_vc_hook( installRequest );
					}
				}
			}

			checkedRepos[rkey] = true;
		}
		finally
		{
			if (repo)
				repo.close();
		}
	}
	catch(ex)
	{
		sg.log("urgent", "Exception in checkModuleDags: " + ex);
		throw ex;
	}
	finally
	{
		cmdInProgress = false;
	}
}


function areaExists(repo, dagnum)
{
	var areas = repo.list_areas();
	var aid = sg.get_area_id(dagnum);

	for ( var an in areas )
		if (areas[an] == aid)
			return(true);

	return(false);
}


function makeArea(repo, areaname, dagnum, uid)
{
	var aid = sg.get_area_id(dagnum);

	try
	{
		repo.area_add(areaname, aid, uid || null);
		return(true);
	}
	catch(e)
	{
		return(false);
	}
}


function dagnumExists(reponame, repo, dagnum)
{
	if (! knownDags[reponame])
	{
		var kd = {};

		var d = repo.list_dags();

		for ( var i = 0; i < d.length; ++i )
			kd[d[i]] = true;

		knownDags[reponame] = kd;
	}

	return( !! knownDags[reponame][dagnum] );
}

function setTemplate(repo, dagnum, template, uid)
{
	var ztx = false;

	try
	{
		var db = new zingdb(repo, dagnum);
		ztx = db.begin_tx(null, uid || null);
		ztx.set_template(template);

		var ct = ztx.commit();

		if (ct.errors)
		{
		    var msg = sg.to_json(ct);

		    throw (msg);
		}

		ztx = false;

		return(true);
	}
	catch (e)
	{
		return(false);
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}
}

