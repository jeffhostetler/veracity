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

var vvWiHistory = {
	toggleButton: null,
	vtoggle: null,
	container: null,
	showing: false,
	swapWith: false,
	retrieved: false,

	wrap: function (content)
	{
		this.swapWith = $(content);

		this.container = $(document.createElement('div')).
			addClass('zingform wihistory outercontainer').
			insertAfter(this.swapWith).
			hide();
	},

	getToggle: function ()
	{
		if (!this.toggleButton)
		{
			this.toggleButton = $('<input type="button" id="togglehistory" class="primary headerbutton" value="Show Change History" />');

			this.toggleButton.click(
				function (e)
				{
					e.stopPropagation();
					e.preventDefault();

					vvWiHistory.showing = !vvWiHistory.showing;

					if (vvWiHistory.showing)
					{
						$('#wihToggle').show();
						vvWiHistory.show();
					}
					else
					{
						$('#wihToggle').hide();
						vvWiHistory.hide();
					}

					vCore.resizeMainContent();
				}
			);
		}

		return (this.toggleButton);
	},

	hide: function ()
	{
		vvWiHistory.showing = false;
		vvWiHistory.container.hide();
		$('#wihToggle').hide();
		vvWiHistory.swapWith.show();
		vvWiHistory.toggleButton.val('Show Change History');
	},

	show: function ()
	{
		vvWiHistory.showing = true;
		vvWiHistory.swapWith.hide();
		vvWiHistory.container.show();
		vvWiHistory.toggleButton.val('Hide Change History');

		vvWiHistory.getWiHistory();
	},

	enable: function (enabled)
	{
		if (enabled == undefined)
			enabled = true;

		if (this.toggleButton)
			if (enabled)
				this.toggleButton.show();
			else
				this.toggleButton.hide();
	},

	disable: function ()
	{
		this.hide();

		return (this.enable(false));
	},

	reset: function ()
	{
		this.container.empty();
		this.retrieved = false;
	},

	getWiHistory: function ()
	{
		if (this.retrieved)
			return;

		utils_savingNow('Retrieving history...');
		_spinUp(this.container);

		var bugId = getQueryStringVal('recid');

		vCore.ajax(
			{
				url: sgCurrentRepoUrl + "/workitem/" + bugId + "/history.json",
				dataType: 'json',

				complete: function ()
				{
					utils_doneSaving();
				},

				success: function (data)
				{
					vvWiHistory.displayHistory(data);
				}
			}
		);
	},

	showField: function (record, fn, tbl, verbose)
	{
		if (record[fn] === undefined)
			return (false);

		var val = vCore.sanitize(record[fn]);

		var row = $(document.createElement('tr')).
			appendTo(tbl);

		if (verbose)
			row.addClass('wihVerbose');

		$(document.createElement('th')).
			attr('scope', 'row').
			text(fn + ":").
			appendTo(row);

		var parts = null;

		if (fn == "Estimate")
			val = vTimeInterval.format(val, "Unestimated");
		if (fn == "amount")
			val = vTimeInterval.format(val);
		else if (fn == 'milestone')
			val = val || "Product Backlog";
		else if ($.inArray(fn, ["Related To", "Blocks", "Depends On", "Duplicate Of", "Duplicated By", "Associated Commit", "commit"]) >= 0)
			parts = record[fn] || [];

		var mainText = val;
		var remainder = null;

		if ((mainText.length > 250) && !parts)
		{
			remainder = mainText.substring(250);
			mainText = mainText.substring(0, 250);
		}

		var td = $(document.createElement('td')).appendTo(row);

		if (parts)
		{
			for ( var i = 0; i < parts.length; ++i )
			{
				if (i > 0)
					td.append($('<br />'));

				td.append(document.createTextNode(parts[i]));
			}
		}
		else
			td.html(vCore.createLinks(mainText));

		if (remainder)
		{
			var therest = $(document.createElement('span')).
				html(vCore.createLinks(remainder)).
				hide().
				appendTo(td);

			var more = $(document.createElement('a')).
				text('…').
				attr('href', '#').
				attr('title', 'Show all ' + val.length + ' characters').
				appendTo(td).
				click(
					function (e)
					{
						e.stopPropagation();
						e.preventDefault();

						more.hide();
						therest.show();
						vCore.resizeMainContent();
					}
				);
		}

		return (true);
	},

	displayHistory: function (data)
	{
		this.retrieved = true;
		var scrollTo = null;
		var o = this;

		var csid = getQueryStringVal('csid');
		var anyVerbose = false;

		$.each
		(
			data,
			function (i, record)
			{
				var d = new Date(parseInt(record.when));
				var dateStr = shortDateTime(d); // d.strftime("%Y/%m/%d %i:%M%P");

				var title = record.action;

				if (record.who)
					title += " by " + record.who;

				title += " on " + dateStr;

				delete record.action;
				delete record.who;
				delete record.when;

				var header = $(document.createElement('h3')).
					text(title).
					appendTo(vvWiHistory.container);

				if (csid && (record.csid == csid))
					scrollTo = header;

				var tbl = $(document.createElement('table')).
					appendTo(vvWiHistory.container);

				var fields = [
					"Name",
					"description",
					"status",
					"Assigned To",
					"verifier",
					"priority",
					"Estimate",
					"commit",
					"comment",
					"attachment",
					"milestone",
					"amount",
					"stamp",
					"Duplicate Of",
					"Duplicated By",
					"Blocks",
					"Depends On",
					"Related To"
				];

				var verboseFields = [
					"listorder",
					"id",
					"reporter"
				];

				var verboseOnly = true;

				$.each
				(
					fields,
					function (i, fn)
					{
						if (o.showField(record, fn, tbl, false))
							verboseOnly = false;
					}
				);

				$.each
				(
					verboseFields,
					function (i, fn)
					{
						if (o.showField(record, fn, tbl, true))
							anyVerbose = true;
					}
				);

				if (verboseOnly)
				{
					header.addClass('wihVerbose');
					anyVerbose = true;
				}
			}
		);

		if (anyVerbose)
		{
			$('.wihVerbose').hide();
			var showPrompt = "Show Verbose History";
			var hidePrompt = "Hide Verbose History";

			var showExtra = vvWiHistory.vtoggle;

			if (! showExtra)
			{
				showExtra = $('<input type="button" id="wihToggle" class="primary headerbutton" />').insertAfter(vvWiHistory.toggleButton);
				vvWiHistory.vtoggle = showExtra;
			}

			var showing = false;

			showExtra.
				val(showPrompt).
				show();

			showExtra.click(
				function (e)
				{
					e.stopPropagation();
					e.preventDefault();

					showing = !showing;

					if (showing)
					{
						showExtra.val(hidePrompt);
						$('.wihVerbose').show();
					}
					else
					{
						showExtra.val(showPrompt);
						$('.wihVerbose').hide();
					}

					return (false);
				}
			);
		}

		vCore.resizeMainContent();
		if (scrollTo)
			scrollTo[0].scrollIntoView(true);
	}
};
