/*
Copyright 2011-2013 SourceGear, LLC

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

/**
 * Create and fully initialize a response object based on a template
 * file. All include processing and variable expansion happens here.
 * 
 * @param request request object describing our context
 * @param packageName the package within which this request is being handled
 * @param title the page title to be used
 * @param replacer (optional) keyword replacement function
 *                 overrides the default replacements. The function takes
 *                 two parameters : keyword and request object. It should
 *                 return nothing if it has no match for the keyword; the
 *                 default replacement fills in the gap.
 */
function vvUiTemplateResponse(request, packageName, title, replacer, templateFilename, serverFiles, outDir)
{
	this._request = request;
	this._packageName = packageName;
	this._replacer = replacer || false;
	this._title = title;
	this._scriptFiles = [];
	this._scriptHashes = [];
	this._cssFiles = [];
	this._cssHashes = [];
	this._outDir = outDir || false;

	this._serverFiles = serverFiles || serverConf.files;

	this._cdnRoot = serverConf.cdnroot || "/ui";

	this._coreTemplateDir = this._serverFiles + "/core/templates";

	if (packageName == "core")
		this._templateDir = this._coreTemplateDir;
	else
		this._templateDir = this._serverFiles + "/modules/" + packageName + "/templates";

	this._fn = templateFilename || false;
	this.statusCode = STATUS_CODE__NOT_FOUND;
	this._str = null;

	if (!this._fn)
	{
		var uri = trimSlashes(request.uri);
		var matches = uri.match(/([^\/]+\.html)$/);
		if(matches)
			this._fn = matches[1];
		else
		{
			matches = uri.match(/([^\/]+)$/);
			if(matches)
				this._fn = matches[1]+".html";
		}
	}

	if(this._fn)
	{
		if (this._fn.match(/^([a-z]:)?\//i))
			this._fullPath = this._fn;
		else
			this._fullPath = this._templateDir + "/" + this._fn;

		if (sg.fs.exists(this._fullPath))
		{
			var text = sg.file.read(this._fullPath);

			if (!! text)
			{
				this.statusCode = STATUS_CODE__OK;
				this._str = this.fillTemplate(text);
			}
		}
	}

	if(this._str===null)
		throw "The template file was not found. TemplateFilename: " + this._fn + " templateDir: " + this._templateDir;

	this._str = sg.to_sz(this._str);
	this.headers = {
		"Content-Length" : this._str.length,
		"Content-Type" : htmlContentTypeForUserAgent(request.headers["User-Agent"])
	};

	return(this);
}

vvUiTemplateResponse.prototype.onChunk = function () {
	var ret = this._str;
	this._str = null;
	return ret;
};
vvUiTemplateResponse.prototype.finalize = function () {
	this._str.fin();
	this._str = null;
};


vvUiTemplateResponse.prototype.expandTemplateFile = function(fn) {
	var tpath = this._templateDir + "/" + fn;
	
	if (! sg.fs.exists(tpath))
		tpath = this._coreTemplateDir + "/" + fn;

	var text = sg.file.read(tpath);
	if (!! text)
		return(this.fillTemplate(text));
	else
		return("");
};


vvUiTemplateResponse.prototype.fillTemplate = function(templateText) {
	var result = "";
	var matches;
	var len = templateText.length;
	var remainder = 0;
	var skipping = 0;

	var inactiveUser = !! this._request.inactiveUser;

	while (remainder < len)
	{
		var p = templateText.indexOf("{{{", remainder);

		if (p < 0)
		{
			result += templateText.substring(remainder);
			remainder = len;
		}
		else
		{
			if(!skipping)
				result += templateText.substring(remainder, p);

			var endp = templateText.indexOf("}}}", p);

			if (endp < 0)
			{
				result += templateText.substring(p);
				remainder = len;
			}
			else
			{
				var key = templateText.substring(p + 3, endp);
				remainder = endp + 3;

				if ((key.length > 0) && (key.charAt(0) == '<'))
				{
					result += this.expandTemplateFile(key.substring(1));
				}
				else if ((key.length >= 3) && (key[0]=='I') && (key[1]=='F') && ((key[2]==' ')||(key[2]=='(')))
				{
					if(skipping>0)
						skipping = skipping+1;
					else
					{
						var if_statement_result = false;
						try
						{
							if_statement_result = eval(key.slice(2));
						}
						catch(ex)
						{
						}

						if(!if_statement_result)
							skipping = 1;
					}
				}
				else if(key=="ENDIF")
				{
					if(skipping>0)
						skipping = skipping - 1;
				}
				else if(!skipping)
				{
					var replacement = undefined;
					var jsEscape = false;
					var urlencode = false;

					matches = key.match(/^js:(.+)/);

					if (matches)
					{
						key = matches[1];
						jsEscape = true;
					}
					
					matches = key.match(/^u:(.+)/);
					if (matches)
					{
						key = matches[1];
						urlencode = true;
					}

					if (this._replacer)
						replacement = this._replacer(key, this._request);

					if (replacement===undefined)
						replacement = this.defaultReplacer(key, this._request, jsEscape, urlencode);
					else if(Array.isArray(replacement)){
						if(replacement.length==2 && replacement[0]=="raw:"){
							replacement = replacement[1];
						}
						else{
							throw "Replacer returned invalid array!"
						}
					}
					else
						replacement = this.escape(replacement, jsEscape, urlencode);

					result += replacement;
				}
			}
		}
	}

	return(result);
};

vvUiTemplateResponse.prototype.escape = function(text, jsEscape, urlencode)
{
	var esc = text || '';

	if (jsEscape)
	{
		esc = esc.
			replace(/\\/g, "\\\\").
			replace(/"/g, '\\"').
			replace(/'/g, "\\'").
			replace(/\n/g, "\\n").
			replace(/\r/g, "\\r").
			replace(/\t/g, "\\t").
			replace(/\v/g, "\\v");
	}
	else if (urlencode)
		esc = encodeURIComponent(esc);
	else
	{
		esc = esc.
			replace(/\&/g, '&amp;').
			replace(/"/g, '&quot;').
			replace(/'/g, "&#39;").
			replace(/</g, '&lt;').
			replace(/>/g, '&gt;');
	}
	
	return( esc );
};


vvUiTemplateResponse.prototype.scriptTags = function(scriptList, linkRoot)
{
	var base = linkRoot + "/ui/";
	var scripts = scriptList.split(/[ \t\r\n,]+/);
	var results = [];

	if (serverConf.enable_diagnostics)
	{
		for ( var i = 0; i < scripts.length; ++i )
		{
			results.push("<script type='text/javascript' src='" + base + scripts[i] + "'></script>");
		}

		return(results.join("\r\n"));
	}
	else
	{
		var scriptRoot = this._serverFiles + "/ui/";

		for ( var i = 0; i < scripts.length; ++i )
		{
			var fn = scriptRoot + scripts[i];

			if (sg.fs.exists(fn))
			{
				this._scriptFiles.push(fn);
				this._scriptHashes.push(sg.file.hash(fn));
			}
  	    }

		return("");
	}
};


vvUiTemplateResponse.prototype.emitScripts = function(outDir) {
	if (this._scriptFiles.length == 0)
		return("");

	var res = "";
	
	var db = function() {
        var msg = '';

        for (var i = 0; i < arguments.length; ++i)
        {
            var txt = arguments[i] || '';

            if (typeof (txt) == 'object')
                txt = sg.to_json(txt);
            else if (typeof (txt) != 'string')
                txt = txt.toString();

            msg += txt + " ";
        }

		if (outDir)
			print(msg);
		else
			res += "<!-- " + msg + "-->\n";
	};

	var scriptHash = sg.hash(this._scriptHashes);

//	db(this._scriptFiles, this._scriptHashes);

	if (outDir)
	{
		if (! sg.fs.exists(outDir))
			if (! sg.fs.mkdir(outDir))
				throw "unable to create output directory " + outDir;

		var outfn = outDir + "/" + scriptHash + ".js";
		
		if (!sg.fs.exists(outfn))
		{
			sg.file.write(outfn, "/*\n" + sg.to_json(this._scriptFiles) + sg.to_json(this._scriptHashes) + "\n*/");
			for ( var i = 0; i < this._scriptFiles.length; ++i )
			{
				var fn = this._scriptFiles[i];
				var txt = sg.file.read(fn);
			
				var min = sg.jsmin(txt);

				sg.file.append(outfn, "\n");
				sg.file.append(outfn, min);
			}
		}
		
		res += outfn;
	}
	else
	{
		res += "<script type='text/javascript' src='" + this._cdnRoot + "/min/" + scriptHash + ".js'></script>";
	}

	this._scriptFiles = [];
	this._scriptHashes = [];

	return(res);
};

vvUiTemplateResponse.prototype.cssTags = function(cssList, linkRoot)
{
	var base = linkRoot + "/ui/";
	var scripts = cssList.split(/[ \t\r\n,]+/);
	var results = [];

	if (serverConf.enable_diagnostics)
	{
		for ( var i = 0; i < scripts.length; ++i )
		{
			results.push("<link rel='stylesheet' type='text/css' href='" + base + scripts[i] + "' />");
		}

		return(results.join("\r\n"));
	}
	else
	{
		var scriptRoot = this._serverFiles + "/ui/";

		for ( var i = 0; i < scripts.length; ++i )
		{
			var fn = scriptRoot + scripts[i];

			if (sg.fs.exists(fn))
			{
				this._cssFiles.push(fn);
				this._cssHashes.push(sg.file.hash(fn));
			}
  	    }

		return("");
	}
};


vvUiTemplateResponse.prototype.emitCss = function(outDir) {
	var db = function() {
        var msg = '';

        for (var i = 0; i < arguments.length; ++i)
        {
            var txt = arguments[i] || '';

            if (typeof (txt) == 'object')
                txt = sg.to_json(txt);
            else if (typeof (txt) != 'string')
                txt = txt.toString();

            msg += txt + " ";
        }

		if (outDir)
			print(msg);
		else
			res += "<!-- " + msg + "-->\n";
	};

	var res = "";

//	db("emitCss", this._fn || "", this._cssFiles, this._cssHashes);
	
	if (this._cssFiles.length == 0)
		return("");

	var cssHash = sg.hash(this._cssHashes);

	if (outDir)
	{
		if (! sg.fs.exists(outDir))
			if (! sg.fs.mkdir(outDir))
				throw "unable to create output directory " + outDir;

		var outfn = outDir + "/" + cssHash + ".css";

		for ( var i = 0; i < this._cssFiles.length; ++i )
		{
			var fn = this._cssFiles[i];
			var txt = sg.file.read(fn);
			
			var min = this.cssmin(txt);

			if (i == 0)
				sg.file.write(outfn, min);
			else
			{
				sg.file.append(outfn, "\n");
				sg.file.append(outfn, min);
			}
		}

		res += outfn;
	}
	else
	{
		res += "<link rel='stylesheet' type='text/css' href='" + this._cdnRoot + "/min/" + cssHash + ".css' />";
	}

	this._cssFiles = [];
	this._cssHashes = [];

	return(res);
};

vvUiTemplateResponse.prototype.defaultReplacer = function(key, request, jsEscape, urlencode) {
	var replacement = "";

	var matches = key.match(/^SCRIPTS[ \t]+(.+?)[ \t]*$/);

	if (matches)
		return(this.scriptTags(matches[1], serverConf.application_root));

	matches = key.match(/^CSS[ \t]+(.+?)[ \t]*$/);

	if (matches)
		return(this.cssTags(matches[1], serverConf.application_root));

	switch (key)
	{
		case "REPONAME":
			replacement = this.escape(request.repoName || getDefaultRepoName(request), jsEscape, urlencode);
			break;

		case "ACTFEED":
			{
				var feed = "";

				var repo = request.repoName || getDefaultRepoName(request);
				var session = vvSession.get() || {};
				var token = session['auth_token'];

				if (repo && token)
				{
					feed = "/account/activity/" + encodeURIComponent(repo) + ".xml?user_credentials=" + encodeURIComponent(token);
				}
				else if (repo)
				{
					feed = "/repos/" + repo + "/activity.xml";
				}

				replacement = this.escape(feed, jsEscape, urlencode);
			}
			break;

		case "USERACTFEED":
			{
				var feed = "";

				var repo = request.repoName || getDefaultRepoName(request) || null;
				var user = request.vvuser || null;
				var session = vvSession.get() || {};
				var token = session['auth_token'];

				if (repo && token && user)
				{	
					// Also replace periods, because that breaks onveracity
					encoded_repo = encodeURIComponent(repo).replace(/\./g,"%2D");
					feed = "/account/useractivity/" + encoded_repo + ".xml?user_credentials=" + encodeURIComponent(token);
				}
				else if (repo && user)
				{
					feed = "/repos/" + repo + "/activity/" + encodeURIComponent(user) + ".xml";
				}

				replacement = this.escape(feed, jsEscape, urlencode);
			}
			break;

		case "EMITSCRIPTS":
			replacement = this.emitScripts(this._outDir);
			break;

		case "EMITCSS":
			replacement = this.emitCss(this._outDir);
			break;

		case "ADMINID":
			var reponame = request.repoName || getDefaultRepoName(request);

			if (reponame)
			{
				var descs = sg.list_descriptors();
				var d = descs[reponame];
				if (d)
					replacement = d.admin_id;
			}
			break;

		case "UNIVERSALLINKROOT":
			replacement = this.escape(request.linkRoot, jsEscape, urlencode);
			break;

		case "LINKROOT":
			replacement = serverConf.application_root;
			break;

		case "STATICLINKROOT":
			if (serverConf.enable_diagnostics)
				replacement = "/ui";
			else
				replacement = this._cdnRoot;
			break;

		case "USERNAME":
		case "EMAIL":
			replacement = this.escape(request.vvuser, jsEscape, urlencode);
			break;

		case "LOCALREPO":
			replacement = this.escape(getLocalRepoName(request), jsEscape, urlencode);
			break;

		case "TITLE":
			replacement = this.escape(this._title, jsEscape, urlencode);
			break;

		case "MODULESTYLES":
			var mods = [];
			var css = [];

			if (typeof(vvModules) != 'undefined')
				mods = vvModules;

			var base = this._serverFiles;

			if (base)
			{
				if (! base.match(/[\\\/]$/))
					base += "/";

				for ( var i = 0; i < mods.length; ++i )
				{
					var area = mods[i].area;

					var tfn = base + "ui/modules/" + area + "/module.css";
					if (sg.fs.exists(tfn))
						css.push("modules/" + area + "/module.css");
				}

				if (css.length > 0)
					replacement = this.cssTags( css.join(","), serverConf.application_root );
			}

			break;

		case "MODULEMENUS":
			var mods = [];
			var files = [];

			if (typeof(vvModules) != 'undefined')
				mods = vvModules;

			var base = this._serverFiles;

			if (base)
			{
				if (! base.match(/[\\\/]$/))
					base += "/";

				for ( var i = 0; i < mods.length; ++i )
				{
					if (mods[i].disabled)
						continue;

					var area = mods[i].area;
					var tfn = base + "ui/modules/" + area + "/menu.js";

					if (sg.fs.exists(tfn))
						files.push("modules/" + area + "/menu.js");

					tfn = base + "ui/modules/" + area + "/module.js";

					if (sg.fs.exists(tfn))
						files.push("modules/" + area + "/module.js");
				}

				if (files.length > 0)
				{
					replacement = this.scriptTags( files.join(","), serverConf.application_root );
				}
			}

			break;

		case "CURRENTSPRINT":
		    replacement = "";
		
			if (request.repo)
			{
				var cfg = new Configuration(request.repo);
				var current = cfg.get('current_sprint');

				if (current)
					replacement = this.escape(current.value, jsEscape, urlencode);
			}
			break;

		case "SCHEME":
			if (request.scheme && request.scheme.match(/^https/))
				replacement = "https:";
			else
				replacement = "http:";
			break;

		case "OPENBRANCHES":
			if(request.repo){
				var branchesdb = new zingdb(request.repo, sg.dagnum.VC_BRANCHES);
				var branches = branchesdb.query('branch', ['*']);
				var zingClosedBranchList = branchesdb.query('closed', ['*']);
				var closed={};
				for(var i=0; i<zingClosedBranchList.length; ++i){
					closed[zingClosedBranchList[i].name] = true;
				}
				
				var openBranches = {};
				for(var iBranch = 0; iBranch<branches.length; ++iBranch){
					if(!closed[branches[iBranch].name]){
						openBranches[branches[iBranch].name] = true;
					}
				}
				
				replacement = this.escape(sg.to_json(openBranches), jsEscape, urlencode);
			}
			else{
				replacement = "{}";
			}
			break;
	}

	return(replacement);
};


vvUiTemplateResponse.prototype.cssmin = function(txt) {
	txt = txt.replace(/\}[ \t]*\/\*/g, "}\n/*");

	var lines = txt.split(/[\r\n]+/);
	var inComment = false;

	var output = "";

	for ( var j = 0; j < lines.length; ++j )
	{
		var line = lines[j];

		if (! inComment)
		{
			inComment = line.match(/^[ \t]*\/\*/);
		}

		if (inComment)
		{
			var p = line.indexOf('*/');
			if (p >= 0)
			{
				line = line.substr(p + 2);
				inComment = false;
			}
			else
				continue;
		}

		line = line.replace(/[ \t]+/g, ' ');
		line = line.replace(/^ /, '');
		line = line.replace(/^[ \r\n]+/, '');

		line = line.replace(/ *([:;\{\}]) */g, '$1');

		if (line)
		{
			output += line;
			if (! line.match(/[\{\}\;,]$/))
				output += "\n";
		}
	}

	output = output.replace(/\n\{/g, "{");

	return(output);
};
