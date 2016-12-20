(function(window) 
{
	var tm = $('#topmenu');
	
	var mi = $("<li id='topmenuwiki' class='menudropdown needrepo'></li>");

	var sp = $('<span class="menudropdownsub">Wiki</span>').
		appendTo(mi);

	var wmenu = $('<ul class="submenu" id="ulwikimenu"></ul>');

	var addMi = function(title, link) {
		var li = $('<li class="submenuli" />').
			appendTo(wmenu);

		var ln = $("<a class='menulink'></a>").
			text(title).
			attr('href', vCore.getOption('currentRepoUrl') + link).
			appendTo(li);

		return(li);
	};

	addMi('Home', "/wiki.html");
	addMi('New Page', "/wiki.html?page=New%20Page");
	 
	wmenu.appendTo(mi);
	tm.append(mi);
 })(window);

