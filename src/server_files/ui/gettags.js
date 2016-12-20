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

var currentNumber = 0;
var dataChunk = 50;
var tagItems = [];
var scrollPosition = 0;
var _changesetHover = null;

function clearFilter()
{
	currentNumber = 0;
	$("#tbl_footer .statusText").text("Clearing Tag Filter...");
	$("#tbl_footer").show();

	$('#tag').val("");
	$('#tabletags tbody tr').remove();
	showTags();
	$("#tbl_footer").hide();
}

function filterTags(text)
{
	currentNumber = 0;
	$("#tbl_footer .statusText").text("Filtering Tags...");
	$("#tbl_footer").show();
	var filtered = [];
	$('#tabletags tbody tr').remove();

	$.each(tagItems, function(i, v)
	{
		if (v.tag.indexOf(text) >= 0)
			filtered.push(v);

	});

	showTags(filtered);
	$("#tbl_footer").hide();

}

function showTags(items)
{
	var show = items || tagItems;

	if (show.length > currentNumber)
	{
		var showItems = show.slice(currentNumber, currentNumber += dataChunk);

		$.each(showItems, function (index, value)
		{
			var tr = $('<tr class="trReader"></tr>');

			var td1 = $('<td>');
			var td2 = $('<td>');

			td1.text(value.tag);

			var urlBase = window.location.pathname.rtrim("/").rtrim("tags")
					.rtrim("/");
			var url = urlBase + "/changeset.html?recid=" + value.csid;
			var csa = $('<a>').text(value.csid).attr(
			{
				href: url
			});
			_changesetHover.addHover(csa, value.csid);
			td2.append(csa);

			tr.append(td1);
			tr.append(td2);

			$('#tabletags tbody').first().append(tr);

		});

		$("#tbl_footer").hide();;

		$(".datatable tbody tr").hover(function()
		{
			$(this).css("background-color", "#E4F0CE");
		}, function()
		{
			$(this).css("background-color", "white");
		});

	}

}

function getTags()
{
	$(window).scrollTop(scrollPosition);

	$("#tbl_footer .statusText").text("Loading Tags...");
	$("#tbl_footer").show();

	var str = "";

	var url = sgCurrentRepoUrl + "/tags.json";

	vCore.ajax(
	{
		url : url,
		dataType : 'json',

		error : function(data, textStatus, errorThrown)
		{
			$("#tbl_footer").hide();
		},
		success : function(data, status, xhr)
		{
			$("#tbl_footer").hide();
			if ((status != "success") || (!data))
			{
				return;
			}
			tagItems = data.sort(function(a, b)
			{
				return (a.tag.toLowerCase().compare(b.tag.toLowerCase()));
			});
			showTags();
			$(window).scroll(scrollPosition);
		}
	});

}

function sgKickoff()
{
	_changesetHover = new hoverControl(fetchChangesetHover);
	getTags();
	$('#tag').keyup(function()
	{
		var text = $('#tag').val();
		if (text.length > 0)
		{
			filterTags(text);
		}
		else
		{
			clearFilter();
		}

	});

	$(window).scroll(function()
	{

		scrollPosition = $(window).scrollTop();
		if (isScrollBottom())
		{
			showTags();
		}
	});

}