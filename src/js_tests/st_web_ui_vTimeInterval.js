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

load("../server_files/ui/vTimeInterval.js");

function st_web_ui_vTimeInterval()
{
	this.setUp = function()
	{
	},
	
	this.verifyParseOutput = function(input, expected)
	{
		var got = vTimeInterval.parse(input);
		
		if (got !== expected)
		{
			testlib.testResult(false, "Parse Matched", expected, got);
			return;
		}
		testlib.testResult(true);
	},
	
	this.verifyFormatOutput = function(input, expected)
	{
		var got = vTimeInterval.format(input);
		
		testlib.testResult(got == expected, "Format Matched", expected, got);
	},
	
	this.testTextualInputValidation = function()
	{
		//No Units
		testlib.testResult(vTimeInterval.validateInput("1"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1.5"), "Input Valid");
		
		//Numerical
		testlib.testResult(vTimeInterval.validateInput(1), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput(1.5), "Input Valid");
		
		
		testlib.testResult(vTimeInterval.validateInput("1 minute"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 hour"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 day"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 week"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 month"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 year"), "Input Valid");
		
		// Plurals
		testlib.testResult(vTimeInterval.validateInput("2 minutes"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("2 hours"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("2 days"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("2 weeks"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("2 months"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("2 years"), "Input Valid");
		
		//Chaining
		testlib.testResult(vTimeInterval.validateInput("1 hour 1 minute"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 day 1 hour"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 week 1 day"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 month 1 week"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 year 1 month"), "Input Valid");
		
		testlib.testResult(vTimeInterval.validateInput("1 m"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 h"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 d"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 w"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 M"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("1 y"), "Input Valid");
		
		testlib.testResult(vTimeInterval.validateInput("1.5 h"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("2.5 d"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput(".5 d"), "Input Valid");
		
		// Should Fail
		testlib.testResult(!vTimeInterval.validateInput("./24 hrs"), "Input Valid");
		
		// Trailing an leading spaces
		testlib.testResult(vTimeInterval.validateInput("2 "), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput(" 2"), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput(" 2 "), "Input Valid");
		
		// Ending in period
		testlib.testResult(vTimeInterval.validateInput("2."), "Input Valid");
		testlib.testResult(vTimeInterval.validateInput("2. "), "Input Valid");
	},
	
	this.testTextualIntervals = function()
	{
		
		// No Units
		this.verifyParseOutput("1", 60);
		this.verifyParseOutput("1.5", 90);
		
		//Numerical Inputs
		this.verifyParseOutput(1, 60);
		this.verifyParseOutput(1.5, 90);
		
		/*
		 * Spelling it all the way out
		 * i.e. no abbreviations
		 */
		// Singular Values
		this.verifyParseOutput("1 minute", 1);
		this.verifyParseOutput("1 hour", 60);
		this.verifyParseOutput("1 day", 480);
		this.verifyParseOutput("1 week", 2400);
		this.verifyParseOutput("1 month", 10560);
		this.verifyParseOutput("1 year", 125280);
		
		// Plural Values
		this.verifyParseOutput("1 minutes", 1);
		this.verifyParseOutput("1 hours", 60);
		this.verifyParseOutput("1 days", 480);
		this.verifyParseOutput("1 weeks", 2400);
		this.verifyParseOutput("1 months", 10560);
		this.verifyParseOutput("1 years", 125280);
		
		this.verifyParseOutput("2 minutes", 2);
		this.verifyParseOutput("2 hours", 120);
		this.verifyParseOutput("2 days", 960);
		this.verifyParseOutput("2 weeks", 4800);
		this.verifyParseOutput("2 months", 10560 * 2);
		this.verifyParseOutput("2 years", 125280 * 2);
		
		// Stringing them together
		this.verifyParseOutput("1 hour 30 minute", 90);
		this.verifyParseOutput("1 day 1 hour 30 minute", 570);
		this.verifyParseOutput("1 week 1 day 1 hour 30 minute", 2970);
		this.verifyParseOutput("1 month 1 week 1 day 1 hour 30 minute", 13530);
		this.verifyParseOutput("1 year 1 month 1 week 1 day 1 hour 30 minute", 138810);
		
		// plurals
		this.verifyParseOutput("1 hours 30 minutes", 90);
		this.verifyParseOutput("1 days 1 hours 30 minutes", 570);
		this.verifyParseOutput("1 weeks 1 days 1 hours 30 minutes", 2970);
		this.verifyParseOutput("1 months 1 weeks 1 days 1 hours 30 minutes", 13530);
		this.verifyParseOutput("1 years 1 months 1 weeks 1 days 1 hours 30 minutes", 138810);;
		
		/*
		 * Shorthand format
		 * w/ abbreviations
		 */
		// Singular Values
		this.verifyParseOutput("1 m", 1);
		this.verifyParseOutput("1 h", 60);
		this.verifyParseOutput("1 d", 480);
		this.verifyParseOutput("1 w", 2400);
		
		// Stringing them together
		this.verifyParseOutput("1 h 30 m", 90);
		this.verifyParseOutput("1 d 1 h 30 m", 570);
		this.verifyParseOutput("1 w 1 d 1 h 30 m", 2970);

		
		// No spaces
		this.verifyParseOutput("1m", 1);
		this.verifyParseOutput("1h", 60);
		this.verifyParseOutput("1d", 480);
		this.verifyParseOutput("1w", 2400);

		
		// Stringing them together
		this.verifyParseOutput("1h 30m", 90);
		this.verifyParseOutput("1d 1h 30m", 570);
		this.verifyParseOutput("1w 1d 1h 30m", 2970);
;
		// Decimals
		this.verifyParseOutput("2.5h", 150);
		this.verifyParseOutput("1.25h", 75);
		this.verifyParseOutput("1.5d", 720);

		
	},
	
	this.testInvalidIntervals = function()
	{
		// Some jibber-jabber
		this.verifyParseOutput("asdf", null);
		this.verifyParseOutput("3c1y", null);
		this.verifyParseOutput("asdf asdf 18;.awe", null);
		this.verifyParseOutput("./24h", null);
		
	},
	
	this.testFractionalMinutes = function()
	{
		this.verifyParseOutput(".5 minutes", 0);
	},
	
	this.testFormatting = function()
	{
		this.verifyFormatOutput(1, "1 minute");
		this.verifyFormatOutput(30, "30 minutes");
		this.verifyFormatOutput(60, "1 hour");
		this.verifyFormatOutput(120, "2 hours");
		this.verifyFormatOutput(61, "1 hour 1 minute");
		this.verifyFormatOutput(121, "2 hours 1 minute");
		this.verifyFormatOutput(480, "1 day");
		this.verifyFormatOutput(481, "1 day 1 minute");
		this.verifyFormatOutput(541, "1 day 1 hour 1 minute");
		this.verifyFormatOutput(960, "2 days");
		this.verifyFormatOutput(2400, "1 week");
		this.verifyFormatOutput(4800, "2 weeks");
	}
	
	/*this.testParsingFormattedValue = function()
	{
		for (var i = 0; i < 250560; i ++) // from 0 min to 2 years
		{
			var formatted = vTimeInterval.format(i);
			var got = vTimeInterval.parse(formatted);
			testlib.testResult(i == got, "Parse matched input", i, got);
		}
	}*/
}
