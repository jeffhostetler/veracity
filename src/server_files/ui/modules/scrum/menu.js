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

(function(window) 
{
	var before = $('#topmenusource');

	if (before.length < 1)
		return;

	var wimenu = $('<li id="topmenuworkitems" class="menudropdown needrepo"></li>');

	var drop = $('<span class="menudropdownsub">Work Items</span>').
		appendTo(wimenu);
	var submenu = $('<ul class="menucontainer submenu workitems" id="ulworkitemsmenu"></ul>').
		appendTo(wimenu);
	var subctr = $('<li class="menucontainer"></li>').
		css('padding', 0).
		appendTo(submenu);

	$('<ul id="visitedmenu"></ul>').
		appendTo(subctr);
	var current = $('<ul id="current"></ul>').
		appendTo(subctr);
	$('<li class="separator" id="Li2"></li>').
		appendTo(current);

	var deffilter = $('<li class="submenuli" id="workitems.html?filter=default"></li>').
		appendTo(current);

	$('<a class="menulink">My Current Items</a>').
		attr('href', vCore.getOption('currentRepoUrl') + "/workitems.html?filter=default").
		appendTo(deffilter);

	var newfilter = $('<li class="submenuli" id="menunewfilter"></li>').
		appendTo(current);
	$('<a class="menulink">New Filter</a>').
		attr('href', vCore.getOption('currentRepoUrl') + "/workitems.html").
		appendTo(newfilter);

	$('<li class="separator" id="Li5"></li>').
		appendTo(current);

	$('<ul id="filtersmenu"></ul>').
		appendTo(subctr);
	
	var manage = $('<ul></ul>').
		appendTo(subctr);

	$('<li class="separator" id="Li5"></li>').
		appendTo(manage);
	var manageitem = $('<li class="submenuli" id="menumanagefilters"></li>').
		appendTo(manage);
	$('<a class="menulink" title="view and manage all filters">Filters...</a>').
		attr('href', vCore.getOption('currentRepoUrl') + "/managefilters.html").
		appendTo(manageitem);

	$('<li class="separator" id="Li5"></li>').
		appendTo(manage);

	var bd = $('<li class="submenuli" id="burndown.html"></li>').
		appendTo(manage);
	$('<a class="menulink">Burndown</a>').
		attr('href', vCore.getOption('currentRepoUrl') + "/burndown.html").
		appendTo(bd);

	wimenu.insertAfter(before);

	if (! vCore.getOption('readonlyServer'))
	{
		var newwi = $('<li id="topmenuworkitem"></li>').
			insertAfter(wimenu);
	
		$('<a class="menulink needrepo">New Work Item</a>').
			attr('href', vCore.getOption('currentRepoUrl') + "/workitem.html").
			appendTo(newwi);
	}

})(window);
