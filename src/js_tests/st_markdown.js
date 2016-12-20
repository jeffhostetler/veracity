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
load("../docs/mdparser.js");

function st_zing_join()
{
	this.no_setup = true;

	this.setUp = function()
	{
		if (! sg.fs.exists(tempDir))
			sg.fs.mkdir_recursive(tempDir);
		sg.fs.cd(tempDir);
	};

	this.parseMd = function(mdtext) 
	{
		var fn = sg.gid();
		sg.file.write(fn, mdtext);
		var mp = new mdparser();
		mp.parse(fn);
		
		return(mp);
	}

	this.testTrivial = function()
	{
		var mp = this.parseMd("");

		testlib.equal(0, mp.blocks.length, "block length on empty file");
	};

	this.testHeader = function()
	{
		var mp = this.parseMd("One\n---");

		if (! testlib.equal(1, mp.blocks.length, "block count"))
			return;

		testlib.equal("h2", mp.blocks[0].type, "block type");

		if (! testlib.equal(1, mp.blocks[0].blocks.length, "inner block count"))
			return;

		testlib.equal("text", mp.blocks[0].blocks[0].type, "inner type");
		testlib.equal("One", mp.blocks[0].blocks[0].text);

		mp = this.parseMd("Two\n===");

		if (! testlib.equal(1, mp.blocks.length, "block count"))
			return;

		testlib.equal("h1", mp.blocks[0].type, "block type");
	};

	this.testCode = function()
	{
		var mp = this.parseMd("There is `code` in here.");

		if (! testlib.equal(1, mp.blocks.length, "block count"))
			return;
		testlib.testResult(! mp.blocks[0].inline, "outer block should not be inline");

		testlib.equal("p", mp.blocks[0].type, "outer block type");

		var inner = mp.blocks[0].blocks;
		if (! testlib.equal(3, inner.length, "inner block count"))
			return;


		testlib.equal("There is ", inner[0].text, "inner 0 text");
		testlib.equal("text", inner[0].type, "inner 0 type");
		testlib.testResult(inner[0].inline, "inner 0 should be inline");


		testlib.equal("code", inner[1].text, "inner 1 text");
		testlib.equal("code", inner[1].type, "inner 1 type");
		testlib.testResult(inner[1].inline, "inner 1 should be inline");


		testlib.equal(" in here.", inner[2].text, "inner 2 text");
		testlib.equal("text", inner[2].type, "inner 2 type");
		testlib.testResult(inner[2].inline, "inner 2 should be inline");
	};

	this.testNormalize = function()
	{
		var mp = this.parseMd("line   with   double  spaces");
		testlib.equal("line with double spaces", mp.blocks[0].blocks[0].text, "parsed text");
	};

	this.testDL = function()
	{
		var mp  = this.parseMd(
			"entry 1\n" +
			": def 1\r\n" +
			"entry 2\n" +
			": \t def 2"
		);

		if (! testlib.equal(1, mp.blocks.length, "outer block count"))
			return;

		testlib.equal("dl", mp.blocks[0].type, "outer block type");

		var inner = mp.blocks[0].blocks;

		if (! testlib.equal(4, inner.length, "inner block count"))
			return;

		testlib.equal("dt", inner[0].type, "inner 0 type");
		testlib.testResult(! inner[0].inline, "inner 0 not inline");
		testlib.equal("entry 1", inner[0].blocks[0].text, "inner 0 text");


		testlib.equal("dd", inner[1].type, "inner 1 type");
		testlib.testResult(! inner[1].inline, "inner 1 not inline");
		testlib.equal("def 1", inner[1].blocks[0].text, "inner 1 text");


		testlib.equal("dt", inner[2].type, "inner 2 type");
		testlib.testResult(! inner[2].inline, "inner 2 not inline");
		testlib.equal("entry 2", inner[2].blocks[0].text, "inner 2 text");


		testlib.equal("dd", inner[3].type, "inner 3 type");
		testlib.testResult(! inner[3].inline, "inner 3 not inline");
		testlib.equal("def 2", inner[3].blocks[0].text, "inner 3 text");
	};

	this.testQuoteBacktick = function()
	{
		var mp = this.parseMd("There is no ``code`` in here.");

		if (! testlib.equal(1, mp.blocks.length, "block count"))
			return;
		testlib.testResult(! mp.blocks[0].inline, "outer block should not be inline");

		testlib.equal("p", mp.blocks[0].type, "outer block type");

		var inner = mp.blocks[0].blocks;
		if (! testlib.equal(1, inner.length, "inner block count"))
			return;


		testlib.equal("There is no `code` in here.", inner[0].text, "inner 0 text");
		testlib.equal("text", inner[0].type, "inner 0 type");
		testlib.testResult(inner[0].inline, "inner 0 should be inline");
	};

	this.testLineBreaksInParagraphs = function()
	{
		// double-line-break == paragraph
		var mp = this.parseMd("First graph.\n\nSecond graph.");
		testlib.equal(2, mp.blocks.length);

		// single == just whitespace
		mp = this.parseMd("First sentence.\nSecond sentence.");

		if (! testlib.equal(1, mp.blocks.length))
			return;
	};
	this.testLineBreaksInLists = function()
	{
		// double-line-break == paragraph
		var mp = this.parseMd("First graph.\nSecond line of graph.\n: First def.\n  Second line of def.");
		testlib.equal(1, mp.blocks.length);

		var dl = mp.blocks[0];

		testlib.equal(2, dl.blocks.length);
	};

	this.testUnorderedList = function()
	{
		var mp = this.parseMd("Intro\n- First\n- Second\n More of second\n- third");
		testlib.equal(2, mp.blocks.length);

		testlib.equal("ul", mp.blocks[1].type);

		testlib.equal(3, mp.blocks[1].blocks.length, "blocks in list");
	};

	this.testUnendingUnordered = function()
	{
		var mdIn = "* Line 1\n* Line 2\n\nRegular old text.";

		var mp = this.parseMd(mdIn);

		if (! testlib.equal(2, mp.blocks.length, "block count"))
			return;

		testlib.equal("ul", mp.blocks[0].type);
		testlib.equal("p", mp.blocks[1].type);
	};

	this.testFoldInSpaces = function() 
	{
		var plain = this.parseMd("First line \n  second line");

		var block = plain.blocks[0];

		testlib.equal("p", block.type);
		testlib.log(sg.to_json__pretty_print(block));
		testlib.equal("First line", block.blocks[0].text);
		testlib.equal(" ", block.blocks[1].text);
		testlib.equal("second line", block.blocks[2].text);

		var dl = this.parseMd("An item\n: its definition contains a line break right\n  here.");

		block = dl.blocks[0];
		testlib.equal("dl", block.type);
		var inner = block.blocks[1];
		testlib.equal("dd", inner.type);	
		testlib.equal(3, inner.blocks.length, "inner block count");
		testlib.equal("text", inner.blocks[0].type);

		testlib.equal("its definition contains a line break right", inner.blocks[0].text);
		testlib.equal(" ", inner.blocks[1].text);
		testlib.equal("here.", inner.blocks[2].text);
	};

	this.testPreBlock = function()
	{
		var parsed = this.parseMd(
			"para\n" +
			"\n" +
			"    pre\n" +
			"      pre plus 2 spaces\n" +
			"\n" +
			"para\n" 
		);

		testlib.equal("p", parsed.blocks[0].type);
		testlib.equal("pre", parsed.blocks[1].type);
		testlib.equal("p", parsed.blocks[2].type);
	};
}

