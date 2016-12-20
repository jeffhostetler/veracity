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

function st_ssjs_templates()
{
	this.setUp = function()
	{
		var serverFiles = sg.local_settings().machine.server.files;
		var core = serverFiles + "/core/";
		load(serverFiles + "/dispatch.js");
		load(core + "vv.js");
		load(core + "textCache.js");
		load(core + "templates.js");
	};

	this.cleanUp = function(vvt)
	{
		var st = vvt.onChunk();
		if (st)
			st.fin();
	};

	this.testTestAngles = function()
	{
		var template = 'repo name: {{{REPONAME}}}';
		var request = {
			'repoName': '<>',
			'headers': {}
		};

		var tfn = sg.fs.tmpdir() + "/" + sg.gid() + ".html";
		sg.file.write(tfn, template);

		var vvt = new vvUiTemplateResponse(request, "core", "test", null, tfn);

		var expanded = vvt.fillTemplate(template);

		testlib.equal('repo name: &lt;&gt;', expanded, "expansion of " + request.repoName);

		this.cleanUp(vvt);
	};

	this.testAmpersand = function()
	{
		var template = 'repo name: {{{REPONAME}}}';
		var request = {
			'repoName': '<&>',
			'headers': {}
		};

		var tfn = sg.fs.tmpdir() + "/" + sg.gid() + ".html";
		sg.file.write(tfn, template);

		var vvt = new vvUiTemplateResponse(request, "core", "test", null, tfn);

		var expanded = vvt.fillTemplate(template);

		testlib.equal('repo name: &lt;&amp;&gt;', expanded, "expansion of " + request.repoName);

		this.cleanUp(vvt);
	};

	this.testQuotes = function()
	{
		var template = 'repo name: {{{REPONAME}}}';
		var request = {
			'repoName': "' \"",
			'headers': {}
		};

		var tfn = sg.fs.tmpdir() + "/" + sg.gid() + ".html";
		sg.file.write(tfn, template);

		var vvt = new vvUiTemplateResponse(request, "core", "test", null, tfn);

		var expanded = vvt.fillTemplate(template);

		testlib.equal('repo name: &#39; &quot;', expanded, "expansion of " + request.repoName);

		this.cleanUp(vvt);
	};

	this.testEscapeExtension = function()
	{
		var template = 'my var: {{{MYVAR}}}';
		var request = {
			'repoName': 'fred',
			'headers': {}
		};

		var tfn = sg.fs.tmpdir() + "/" + sg.gid() + ".html";
		sg.file.write(tfn, template);

		var replacer = function(key, request) {
			return( "< yo! >" );
		};

		var vvt = new vvUiTemplateResponse(request, "core", "test", replacer, tfn);

		var expanded = vvt.fillTemplate(template);

		testlib.equal('my var: &lt; yo! &gt;', expanded, "expansion of " + template);

		this.cleanUp(vvt);
	};


	this.testTestJSAngles = function()
	{
		var template = 'repo name: {{{js:REPONAME}}}';
		var request = {
			'repoName': '<>',
			'headers': {}
		};

		var tfn = sg.fs.tmpdir() + "/" + sg.gid() + ".html";
		sg.file.write(tfn, template);

		var vvt = new vvUiTemplateResponse(request, "core", "test", null, tfn);

		var expanded = vvt.fillTemplate(template);

		testlib.equal('repo name: <>', expanded, "expansion of " + request.repoName);

		this.cleanUp(vvt);
	};

	this.testJSAmpersand = function()
	{
		var template = 'repo name: {{{js:REPONAME}}}';
		var request = {
			'repoName': '<&>',
			'headers': {}
		};

		var tfn = sg.fs.tmpdir() + "/" + sg.gid() + ".html";
		sg.file.write(tfn, template);

		var vvt = new vvUiTemplateResponse(request, "core", "test", null, tfn);

		var expanded = vvt.fillTemplate(template);

		testlib.equal('repo name: <&>', expanded, "expansion of " + request.repoName);

		this.cleanUp(vvt);
	};

	this.testJSQuotes = function()
	{
		var template = 'repo name: {{{js:REPONAME}}}';
		var request = {
			'repoName': "' \"",
			'headers': {}
		};

		var tfn = sg.fs.tmpdir() + "/" + sg.gid() + ".html";
		sg.file.write(tfn, template);

		var vvt = new vvUiTemplateResponse(request, "core", "test", null, tfn);

		var expanded = vvt.fillTemplate(template);

		testlib.equal('repo name: \\\' \\"', expanded, "expansion of " + request.repoName);

		this.cleanUp(vvt);
	};

	this.testJSEscapes = function()
	{
		var template = 'repo name: {{{js:REPONAME}}}';
		var request = {
			'repoName': "\n\r\t\v\\",
			'headers': {}
		};

		var tfn = sg.fs.tmpdir() + "/" + sg.gid() + ".html";
		sg.file.write(tfn, template);

		var vvt = new vvUiTemplateResponse(request, "core", "test", null, tfn);

		var expanded = vvt.fillTemplate(template);

		testlib.equal('repo name: \\n\\r\\t\\v\\\\', expanded, "expansion of " + request.repoName);

		this.cleanUp(vvt);
	};

}
