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

// the extra (text) file parameter is for debugging only; it
// creates a plain-text version of the encoded C text
//
if ((arguments.length < 2) || (arguments.length > 3))
{
	print("usage: vscript md2c.js infile.md outfile.c [outfile.txt]");
	quit(1);
}

var inf = arguments[0];
var outf = arguments[1];
var outtext = (arguments.length == 3) ? arguments[2] : null;
var lastWasNl = true;
var charsOut = 0;

var loadpath = inf.replace(/[^\\\/]+$/, '');
var lib = loadpath + "/mdparser.js";
load(lib);

var stringName = inf.replace(/^.+[\/\\]/, '').
	replace(/\..*$/, '').
	replace(/[^a-z0-9]/g, '_') .
	replace(/__+/g, '_');

if (sg.fs.exists(outf))
	sg.fs.remove(outf);

if (outtext && sg.fs.exists(outtext))
	sg.fs.remove(outtext);

var md = new mdparser();
md.parse(inf);

// var debugFn = stringName + ".log";
// sg.file.append(debugFn, sg.to_json__pretty_print(md));

startBlock(stringName);

for ( var bn = 0; bn < md.blocks.length; ++bn )
{
	printBlock(md.blocks[bn]);
}

endBlock();


function printBlock(block, indent)
{
	var totalChars = 0;

	indent = indent || 0;

	if ((block.type == "dt") || (block.type == "li"))
		indent += 1;
	else if (block.type == "dd")
		indent += 2;

	// special treatment for pre
	if (block.type == "pre")
	{
		printPreBlock(block, indent);
		return(0);
	}

	if (block.text)
	{
		var txt = block.text;

		if (block.type == "code")
		{
			// don't wrap code blocks. replace spaces (temporarily) with non-breaking ones
			txt = "'" + txt.replace(/ /g, "\u00A0") + "'";
		}
		totalChars = output(txt, indent);
	}
	else
	{
		if (block.type == "h1")
		{
			endBlock();
			startBlock(stringName, getBlockText(block));
			outputRepeat("=", totalChars, indent);
		}
		else
		{
			for ( var ii = 0; ii < block.blocks.length; ++ii )
			{
				totalChars += printBlock(block.blocks[ii], indent);
			}

			if (! block.inline)
			{
				output("\n", indent);
				
				if (block.type == "h2")
				{
					outputRepeat("-", totalChars, indent);
				}
				if (! block.type.match(/^(li|dl|dt|dd|ul)$/))
					output("\n", indent);
			}
		}
	}

	return(totalChars);
}

function printPreBlock(block, indent)
{
	indent += 1;

	for ( var i = 0; i < block.blocks.length; ++i )
	{
		var line = block.blocks[i].text.
			replace(/ /g, "\u00A0") + "\n";
		output(line, indent);
	}
	
	output("\n", 0);
}

function chunk(text, indent, soFar)
{
	var avail = 79;

	if (soFar)
		avail -= soFar;
	else if (indent)
		avail -= (indent * 4);	

	if (text.length <= avail)
		return(
			{
				"now" : text,
				"later" : ""
			}
		);
	
	var endChar = avail;
	var breakAt = text.length;

	// try for the nearest earlier space
	while ((endChar > 0) && (text.charAt(endChar) != ' '))
		--endChar;

	if ((endChar > 0) || (soFar > 0))
		breakAt = endChar;
	else
	{
		endChar = avail;

		while ((endChar < text.length) && (text.charAt(endChar) != ' '))
			++endChar;

		if (endChar < text.length)
			breakAt = endChar;
	}

	var results = 
		{
			"now": text.substring(0, breakAt).replace(/ +$/, ''),
			"later" : text.substring(breakAt).replace(/^ +/, '')
		};

	return( results );
}

function rawOut(text)
{
	if (outtext)
		sg.file.append(outtext, text);
}

function cOut(escaped)
{
	sg.file.append(outf, "\t\"" + escaped + "\"\n");
}

function output(text, indent)
{
	var parts = chunk(text, indent, charsOut);

	do
	{
		if (lastWasNl)
		{
			charsOut = 0;
			for ( var ii = 0; ii < (indent * 4); ++ii )
				parts.now = " " + parts.now;
		}

		parts.now = parts.now.replace(/\u00A0/g, " ");

		var escaped = parts.now;

		escaped = escaped.replace(/\\/g, "\\\\").
			replace(/\n/g, "\\n").
			replace(/"/g, "\\\"");

		rawOut(parts.now);
		cOut(escaped);

		charsOut += parts.now.length;

		parts = chunk(parts.later, indent, 0);

		if (parts.now)
		{
			rawOut("\n");
			cOut("\\n");
			charsOut = 0;
			lastWasNl = true;
		}
	} while (parts.now);

	lastWasNl = !! text.match(/\n$/);
	if (lastWasNl)
	{
		charsOut = 0;
	}

	return(text.length);
}

function outputRepeat(txt, repeats, indent)
{
	var st = "";

	for ( var i = 0; i < repeats; ++i )
		st += txt;

	output(st, indent);
}


function getBlockText(block)
{
	if (block.text)
		return(block.text);

	var result = "";

	for ( var i = 0; i < block.blocks.length; ++i )
		result += getBlockText(block.blocks[i]);

	return(result);
}


function startBlock(varname, sub)
{
	var st = varname;

	if (sub)
		st += "_" + sub.replace(/[^0-9a-zA-Z]+/g, '_');

	sg.file.append(	outf, "const char *" + st + " = \"\"\n" );
}


function endBlock()
{
	sg.file.append(outf, "\n;\n");
}


