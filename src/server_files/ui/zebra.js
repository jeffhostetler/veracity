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

/**
 * Javascript helper for creating tables with alternating table rows
 *
 * This library will look for all tables on the current page with class "zebra"
 * and iterate over that tables row, adding class "alternate" to the odd numbered
 * rows.
 **/
var Zebra = (function() {
	
	var refresh = function()
	{
		$(".zebra .alternate").removeClass("alternate");
		
		$(".zebra").each(function(i,table) {
			var index = 0;
			$("tr", table).each(function(j, tr) {
				if (!$(tr).is(":visible"))
					return;

				if (index % 2 == 1)
					$(tr).addClass("alternate");
				
				index++;
			});
		});
	};
	
	return {
		refresh: refresh
	};
})();

$(document).ready(function() { Zebra.refresh(); });