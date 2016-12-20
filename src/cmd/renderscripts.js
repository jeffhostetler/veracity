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

var serverFiles = arguments[0];
var outDir = arguments[1];

print("rendering scripts from " + serverFiles + " to " + outDir);

var core = serverFiles + "/core/";

load(serverFiles + "/dispatch.js");
load(core + "vv.js");
load(core + "textCache.js");
load(core + "session.js");
load(core + "templates.js");
load(core + "registerModule.js");

var sortDir = function(a,b) { return a.name.localeCompare(b.name); }

var moduleRoot = serverFiles + "/modules";

var modules = sg.fs.readdir(moduleRoot);
modules.sort(sortDir);

for ( var i = 0; i < modules.length; ++i )
{
	if (! modules[i].isDirectory)
		continue;

	var module = modules[i].name;

	var initfn = moduleRoot + "/" + module + "/init.js";

	var deftext = sg.file.read(initfn);
	eval(deftext);
}

renderDir(core + "templates", "core");

for ( var i = 0; i < modules.length; ++i )
{
	if (! modules[i].isDirectory)
		continue;

	var module = modules[i].name;

	if (module.match(/^\./))
		continue;
	
	template_path = moduleRoot + "/" + module + "/templates";
	if(sg.fs.exists(template_path))
		renderDir(template_path, module);
}

function renderDir(dir, module)
{
	var files = sg.fs.readdir(dir);
	files.sort(sortDir);
	
	for ( var i = 0; i < files.length; ++i )
	{
		if (! files[i].isFile)
			continue;

		var fn = files[i].name;

		if (! fn.match(/\.html$/))
			continue;

		renderScripts(module, fn, serverFiles, outDir);
	}
}

function renderScripts(module, renderFile, serverFiles, outDir)
{
	var req = {
		headers : {
			
		}
	};

	var tmp = null;

	try
	{
		tmp = new vvUiTemplateResponse(req, module, "foo", null, renderFile, serverFiles, outDir);
	}
	catch (e)
	{
		print( e.toString() );
	}
	finally
	{
		if (tmp)
		{
			tmp.finalize();
			tmp = null;
		}
	}

}
