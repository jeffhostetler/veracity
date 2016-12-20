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

function mdparser()
{
	this.blocks = [];
	this.fn = "";
}

mdparser.prototype.parse = function(filename)
{
	this.fn = filename;
	var text = sg.file.read(filename); // throws if the read fails, let it bubble up

	text = text.replace(/\r\n/g, "\n");
	text = text.replace(/\n+$/, "");
	var lines = text.split("\n");
	var lastblock = null;
	var lastInner = null;
	var inDl = false;
	var inUl = false;
	var skipped = true;
	var inPre = false;

	for ( var i = 0; i < lines.length; ++i )
	{
		var line = lines[i];
		line = line.replace(/[ \t]+$/, "");

		if (! line)
		{
			inUl = false;
			inDl = false;
			inPre = false;
			skipped = true;
			lastInner = null;
			continue;
		}
		
		if (line.match(/^-+$/))
		{
			if (! lastblock)
				throw this.parseError(i, line, "Header underline must follow a plain text line");

			lastblock.type = "h2";
			skipped = true;
			lastInner = null;
		}
		else if (line.match(/^=+$/))
		{
			if (! lastblock)
				throw this.parseError(i, line, "Header underline must follow a plain text line");

			lastblock.type = "h1";
			skipped = true;
			lastInner = null;
		}
		else if (!! (matches = line.match(/^:[ \t]+(.+)/)))
		{
			var defText = matches[1];
			// dd
			if (! inDl)
			{
				if ((! lastblock) || (lastblock.type != 'p'))
					throw this.parseError(i, line, "List definition must follow another list member.");

				lastblock.type = "dt";

				var newContainer = {
					"type": "dl",
					blocks : [
						lastblock
					]
				};

				this.blocks.pop();
				this.blocks.push(newContainer);
				lastblock = newContainer;
				inDl = true;
				inPre = false;
			}

			var block = {
				"type" : "dd",
				blocks : []
			};

			this.parseLine(block, defText);
			lastblock.blocks.push(block);
			skipped = true;
			lastInner = block;
		}
		else if (!! (matches = line.match(/^[\-\*][ \t]+(.+)/)))
		{
			var defText = matches[1];

			if (! inUl)
			{
				lastblock = {
					"type": "ul",
					blocks : []
				};

				this.blocks.push(lastblock);
				inUl = true;
				inPre = false;
			}

			var block = {
				"type" : "li",
				blocks : []
			};

			this.parseLine(block, defText);
			lastblock.blocks.push(block);
			skipped = true;
			lastInner = block;
		}
		else
		{
			var newBlock = {
				"type" : "p",
				"blocks" : []
			};

			var leading = false;
			var rest = "";
			var matches = line.match(/^    (.+)/);
			if (matches)
			{
				leading = true;
				rest = matches[1];
			}

			this.parseLine(newBlock, line);

			if (inDl)
			{
				newBlock.type = "dt";

				skipped = ! (line.match(/^[ \t]/));

				if (skipped || ! lastInner)
				{
					lastInner = newBlock;
					lastblock.blocks.push(newBlock);
				}
				else
					lastInner.blocks = lastInner.blocks.concat(
						[ { "type": "text", "text": " " } ],
						newBlock.blocks
					);
					
			}
			else if (inUl)
			{
				newBlock.type = "li";

				if (line.match(/^ /))
					skipped = false;

				if (skipped || ! lastInner)
				{
					lastInner = newBlock;
					lastblock.blocks.push(newBlock);
				}
				else
					lastInner.blocks = lastInner.blocks.concat(
						[ { "type": "text", "text": " " } ],
						newBlock.blocks
					);
				
			}
			else
			{
				if (skipped && leading)
				{
					inPre = true;
					newBlock = { "type": "pre",
								 "blocks": [
									{
										"type": "text",
										"text": rest
									}
								 ]
							   };
					lastblock = newBlock;
					this.blocks.push(newBlock);
				}
				else if (skipped)
				{
					lastblock = newBlock;
					this.blocks.push(newBlock);
					inPre = false;
				}
				else
				{
					if (inPre)
						lastblock.blocks = lastblock.blocks.concat(
							[ { "type": "text", "text": rest } ] );
					else
						lastblock.blocks = lastblock.blocks.concat(
							[ { "type": "text", "text": " " } ],
							newBlock.blocks
					);
				}
			}
			skipped = false;

		}

	}
};

mdparser.prototype.parseLine = function(container, line)
{
	var rest = line.replace(/^[ \t]+/, '');
	var codeRe = /^(.*?[^`])?`([^`].+?)`(.*)$/;

	var addTextBlock = function(text)
	{
		container.blocks.push
		(
			{
				"type" : "text",
				"text" : text.replace(/[ \t]+/g, ' ').replace(/``/g, "`").replace(/\\(.)/g, '$1'),
				"inline" : true
			}
		);
	};

	while (rest)
	{
		var matches = rest.match(codeRe);

		if (matches)
		{
			if (matches[1])
				addTextBlock(matches[1]);

			var block = {
				"type" : "code",
				"inline" : true,
				"text" : matches[2].replace(/[ \t]+/g, ' ')
			};
			container.blocks.push(block);

			rest = matches[3];
		}
		else
		{
			addTextBlock(rest);
			rest = "";
		}
	}
};

mdparser.prototype.parseError = function(lineno_z, line, message)
{
	var ln = lineno_z + 1;

	return(this.fn + "(" + ln + "): " + message + "\n\t" + line);
};
