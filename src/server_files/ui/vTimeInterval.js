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
if (!String.prototype.trim)
{
	String.prototype.trim = function (chars)
	{
		return this.rtrim(chars).ltrim(chars);
	};
}

if (!String.prototype.ltrim)
{
	String.prototype.ltrim = function (chars)
	{
		chars = chars || "\\s";
		return this.replace(new RegExp("^[" + chars + "]+", "g"), "");
	};
}

if (!String.prototype.rtrim)
{
	String.prototype.rtrim = function (chars)
	{
		chars = chars || "\\s";
		return this.replace(new RegExp("[" + chars + "]+$", "g"), "");
	};
}
var vTimeInterval = {
	
	HOUR: 60,
	DAY: 480,
	WEEK: 2400,
	MONTH: 10560, // Avg. # of working days per month
	YEAR: 125280, // Avg. # of working days per year

	_formatInput: function(value)
	{
		var _value = value.toLowerCase();
		_value = _value.trim();
		
		if (_value.charAt(_value.length - 1 ) == ".")
			_value = _value.substr(0, _value.length - 1);
		//_value = _value.replace(/[^a-zA-Z0-9.]/gi, " ");  // remove unnecessary chars
		_value = _value.replace(/\s/g, "");
		_value = _value.replace(/(year(s*)|yrs(s*))/, "y");
		_value = _value.replace(/(month(s*)|mth(s*))/, "M");
		_value = _value.replace(/(week(s*)|wk(s*))/, "w");
		_value = _value.replace(/day(s*)/, "d");
		_value = _value.replace(/(hour(s*)|hr(s*))/, "h");
		_value = _value.replace(/(minute(s*)|min(s*))/, "m");
		
		return _value;
	},
	
	validateInput: function(value)
	{
		if (value === null || value === undefined)
			return false;
			
		if (value === "")
			return true;
		
		var isNumber = /^[\d.]+$/;
		if (typeof value == "number" || isNumber.test(value.trim()))
			return true;
		
		var _value = this._formatInput(value);
		
		var findInvalid = /[^0-9.yMwdhm]/;
		
		if (findInvalid.test(_value))
			return false;
		
		var validFormat = /^(([0-9]*(.){1}){0,1}[0-9]+[yMwdhm]{1})+$/;
		
		return validFormat.test(_value);
	},
	
	parse: function(value)
	{
		if (!this.validateInput(value))
			return null;
		
		if (value === "")
			return null;
		
		var minutes = 0;
		var amt = 0;
		
		if (typeof value == "number")
			return Math.floor(value * this.HOUR);
		
		// Handle other date formats
		var _value = this._formatInput(value);
		
		var _arr = _value.split("");
		
		var isNumber = /^[0-9]$/;
		var isChar = /^[a-zA-Z]$/;
		var inDecimal = false;
		var decimalDepth = 0;
		
		for (var i = 0; i < _arr.length; i++)
		{
			var chr = _arr[i];
			if (chr.match(isNumber))
			{
				if (inDecimal)
				{
					amt += (chr * 1) / Math.pow(10, decimalDepth);
					decimalDepth++;
				} 
				else
				{ 
					amt = (amt * 10) + (chr * 1);
				}
			} 
			else if (chr == '.')
			{
				inDecimal = true;
				decimalDepth = 1;
			}
			else if (chr.match(isChar))
			{
				switch(chr)
				{
					case "y":
						minutes += Math.round(this.YEAR * amt);
						break;
					case "M":
						minutes += Math.round(this.MONTH * amt);
						break;
					case "w":
						minutes += Math.round(this.WEEK * amt);
						break;
					case "d":
						minutes += Math.round(this.DAY * amt);
						break;
					case "h":
						minutes +=  Math.round(this.HOUR * amt);
						break;
					case "m":
						minutes += (amt * 1);
						break;
				}
				
				amt = 0;
				inDecimal = false;
				decimalDepth = 0;
			}
		}
		
		if (amt != 0)
			minutes += Math.round(this.HOUR * amt);
		
		return Math.floor(minutes);
	},
	
	format: function(value, noValue)
	{
		noValue = noValue || "";
		
		var val = value;
		var weeks = Math.floor(val / this.WEEK);
		val = val % this.WEEK;
		var days = Math.floor(val / this.DAY);
		val = val % this.DAY;
		var hours = Math.floor(val / this.HOUR);
		val = val % this.HOUR;
		
		var out = "";
		
		function valFmt(v, str)
		{
			var _tmp = "";
			if (v > 0)
			{
				_tmp += v + " " + str;
				if (v > 1)
					_tmp += "s";
				_tmp += " ";
			}
			return _tmp;
		}
		
		out += valFmt(weeks, "week");
		out += valFmt(days, "day");
		out += valFmt(hours, "hour");
		out += valFmt(val, "minute");
		out = out.trim();
		
		if (out == "")
			out = noValue;
		
		return out.trim();
	},
	
	toHours: function(val, includeQuantifier)
	{
		if (includeQuantifier !== false)
			includeQuantifier = true;
			
		var scaled = Math.round((val / 60.0) * 100) / 100;
		
		if (includeQuantifier)
		{
			scaled += " hour" + (val > 1 ? "s" : "");
		}
		return (scaled);
	},
	
	toMinutes: function(val, includeQuantifier)
	{
		if (includeQuantifier !== false)
			includeQuantifier = true;
			
		if (val == 0)
			return 0;
			
		var ret = val + (includeQuantifier ? " minute" : "");
		if (includeQuantifier && val > 1)
			ret += "s";
			
		return ret;
	}
}
