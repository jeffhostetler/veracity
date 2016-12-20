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

var text = null;
var areas = null;
var state = {};
var witloaded = false;
var vcloaded = false;
var buildsloaded = false;
var witResults = new searchResultsObject("wit", $("#divwitResults"));
var vcResults = new searchResultsObject("vc", $("#divvcResults"));
var buildResults = new searchResultsObject("builds", $("#divbuildResults"));
var wikiResults = new searchResultsObject("wiki", $("#divwikiResults"));
var noResults = true;
var converter = null;


var _changesetHover = null;

function pageItem(items, more)
{
    this.items = items || [];
    this.more = more || false;
    this.fieldCount = {started: false};

    this.lastField = (this.items[this.items.length - 1] ? this.items[this.items.length - 1].matcharea : "");    
}


function searchResultsObject(type, displayDiv)
{
    this.pageItems = {};
    this.currentPage = 1;
    this.div = displayDiv;
    this.type = type;
    this.allItems = [];
       
    this.isNewPage = function ()
    {
        if (!this.pageItems[this.currentPage])
        {
            return true;
        }
        return false;
    };
    this.addNewPage = function (items, hasmore)
    {
        this.pageItems[this.currentPage] = new pageItem(items, hasmore);
        this.allItems = this.allItems.concat(items);

    };
    this.addToCount = function (field)
    {
        var p = this.pageItems[this.currentPage];
        if (p.fieldCount.started == false)
        {
            var pold = this.pageItems[this.currentPage - 1];
            if (pold)
            {
                p.fieldCount[field] = pold.fieldCount[field] || 0;
            }
        }
        if (!p.fieldCount[field])
        {
            p.fieldCount[field] = 0;
        }
      
        p.fieldCount.started = true;
        p.fieldCount[field]++;
    };
    this.lastSkipCount = function ()
    {
        var count = 0;
        var p = this.pageItems[this.currentPage - 1];
        if (p)
        {
            count = p.fieldCount[p.lastField];
        }
        if (this.type == "builds")
        {
            count = (this.currentPage * 20) - 20;
        }
        return count;
    };
    this.lastField = function ()
    {
        var p = this.pageItems[this.currentPage -1];
        if (!p)
        {
            return "";
        }
        return (p.lastField);
    };
    this.hasMoreItems = function ()
    {
        return this.pageItems[this.currentPage].more;
    };
    this.currentItems = function ()
    {
        return this.pageItems[this.currentPage].items;
    };
}

function lookForIngoredChars(text)
{
    var re = /[\{\|\}~\[\]\^`<=>\?@\(\)\*\+,-\.\/:;!\\#\$%&]/g;

    var ar = text.match(re);
    if (ar)
    {
        return $.unique(ar);
    }
    return null;
}

function replaceMatchText(matchtext)
{
    //words surrounded in single quotes have
    //no bearing on the results so get rid of those when highlighting
    //so highlights match results.
    if (!matchtext)
        return;
    var txt = text.trim("'");
   
    //match on quoted words and spaces     
    var regex = /[^\s"]+|"([^"]*)"/g;
    var words = txt.match(regex);
    if (!words.length)
    {
        words = txt.match(regex);
    }

    matchtext = matchtext.replace(/_/g," &#95;");

    var quoted = null;
    $.each(words, function (i, word)
    {
        quoted = word.indexOf('"') >= 0;

        var trimWord = word.replace(/"/g, '');

        if (trimWord && trimWord.length)
        {
            //split on the chars sqlite ignores              
            var splitMore = trimWord.split(/[\{\|\}~\[\]\^`<=>\?@\(\)\*\+,-\.\/:;!\\#\$%&_]/);

            for (var j = 0; j < splitMore.length; j++)
            {
                var t = splitMore[j];
                t = $.trim(t);
                if (t && t.length)
                {
                    var hasSpaces = txt.indexOf(" ") >= 0;

                    if (!quoted && (t == "NOT" || t == "AND" || t == "OR" || t == "NEAR"))
                    {
                        continue;
                    }

                    var re = "/\\b" + t + (hasSpaces || quoted ? "\\b/gi" : "/gi");
                    re = eval(re);
                    matchtext = matchtext.replace(re, "<b>$&</b>");
                }
            }

        }
    });
    matchtext = matchtext.replace(/\s&#95;/g, "_");
    return matchtext;
}

function formatMatchText(result, li, donttruncate)
{
    var matchtext = result.matchtext;
    if (result.category == "wiki" || result.category == "work items")
    {
        matchtext = converter.makeHtml(matchtext);       
    }
    else
    {
        //get the truncated text length, truncate at 3 lines
        var cmt = new vvComment(matchtext, 3, (result.category == "wiki" ? true : false));

        var remainder = null;
        matchtext = cmt.summary;       
       
    }
    if (result.category == "changesets" && result.matcharea != "comment")
    {
        matchtext += " - " + result.description;
    }

    var div = $("<div>").addClass("commentlist");
    div.resizable({ handles: "n, s" });
    var ftext = replaceMatchText(matchtext);   

    if (result.category == "wiki" || result.category == "work items")
    {
        div.addClass("wiki");
        div.addClass("cropped");
    }
    else
    {
        ftext = vCore.sanitize(ftext);
    }
	
	// replaceMatchText() isn't very smart about how it relpaces the 
	// found text with it's bold equivalent.  Here we fix any links
	// that might have had matched results in them 
	var _div = $("<div>");
	_div.append(ftext);
	links = $(_div).find("a");
	if (links.length > 0)
	{
	 	$.each(links, function(i, link) {
			
	 		var __div = $("<div>");
	 		__div.html($(link).attr("href"));
	 		$(link).attr("href",__div.text());
	 	});
		ftext = _div.html();
	}
	
    div.append(ftext);
   
    li.append(div);
   
   
    return li;
}

function formatBuildDate(val)
{
    var d = new Date(parseInt(val));
    return shortDateTime(d); // d.strftime("%Y/%m/%d %H:%M%P");
}

function createSearchResultItem(result)
{
    var li = $("<li>").addClass("searchResult");
    var span = $("<span>");
    var div = $("<div>").addClass("searchTitle");
    var a = $("<a>");
    var spanName = $("<span>").text(result.name);
   
    div.append(a);
    
    li.append(div);  
    var title = $("<span>");
    if (result.category != "wiki" && result.category != "work items")
    {
        title.append(" - " + (result.category == "builds" ? formatBuildDate(result.extra) : vCore.htmlEntityEncode(result.extra)));
    }
    if (result.category == "changesets")
    {
        url = sgCurrentRepoUrl + "/changeset.html?recid=" + result.id;

        _changesetHover.addHover(spanName, result.id);
    }
    else if (result.category == "builds")
    {
        title.append(" - " + vCore.htmlEntityEncode(result.description));
        url = sgCurrentRepoUrl + "/build.html?recid=" + result.id;
    }
    else if(result.category == "work items")
    {
        url = sgCurrentRepoUrl + "/workitem.html?recid=" + result.id;
         
         title.append(" - " + replaceMatchText(result.description, true));
        _menuWorkItemHover.addHover(spanName, result.id);
    }
    else if (result.category == "wiki")
    {
        url = sgCurrentRepoUrl + "/wiki.html?page=" + encodeURIComponent(result.id);       
    }

    a.attr({ "href": url });
    a.html(spanName);
    a.append(title);
 
    if ((result.catetory == "changesets" && result.matcharea == "comment"))
    {
        return li;
    }
    return formatMatchText(result, li); 
}

function showPageLinks(resultsObj)
{
    if (resultsObj.currentPage > 1)
    {
        $("#a" + resultsObj.type + "_prev").show();
    }
    else
    {
        $("#a" + resultsObj.type + "_prev").hide();
    }
    if (resultsObj.hasMoreItems())
    {
        $("#a" + resultsObj.type + "_next").show();
    }
    else
    {
        $("#a" + resultsObj.type + "_next").hide();
    }
}

function _doNav(resultObj, direction)
{
    resultObj.currentPage += direction;
    state[resultObj.type + "fld"] = resultObj.lastField();
    state[resultObj.type + "pg"] = resultObj.currentPage;
    state[resultObj.type + "skip"] = resultObj.lastSkipCount();
    $.bbq.pushState(state);
    if (resultObj.isNewPage())
    {
        doSearch(resultObj);
    }
    else
    {
        displaySpecificResults({ items: resultObj.currentItems(), more: resultObj.hasMoreItems() }, resultObj);
    }

}

function doNavigation(target)
{    
    switch ($(target).attr("id"))
    {
        case "awit_prev":
            {
                _doNav(witResults, -1);  
            }
            break;
        case "awit_next":
            {
                _doNav(witResults, 1);                
            }
            break;
        case "avc_prev":
            {
                _doNav(vcResults, -1);                
            }
            break;
        case "avc_next":
            {
                _doNav(vcResults, 1); 
            }
            break;
        case "abuilds_prev":
            {
                _doNav(buildResults, -1);
            }
            break;
        case "abuilds_next":
            {               
                _doNav(buildResults, 1);
            }
        case "awiki_prev":
            {
                _doNav(wikiResults, -1);
            }
            break;
        case "awiki_next":
            {
                _doNav(wikiResults, 1);
            }
            break;
    }

}

function fillInPreviousPages (resultObj, items, hasMore)
{
    var splits = Math.floor(items.length / 20);
    var left = items.length % 20;
    resultObj.currentPage = 0;
    
    var pitems = [];
    for (var i = 0; i < splits; i++)
    {  
        resultObj.currentPage++;
        pitems = items.slice(i * 20, (i * 20) + 20);

        resultObj.addNewPage(pitems, true);
        $.each(pitems, function (d, p)
        {
            resultObj.addToCount(p.matcharea);
        });   
    }
    if (left)
    {
        resultObj.currentPage++;
        pitems = items.slice(items.length - left, items.length);
        resultObj.addNewPage(pitems, hasMore);
        $.each(pitems, function (d, p)
        {
            resultObj.addToCount(p.matcharea);
        });
    }
    else
    {
        resultObj.pageItems[splits].more = hasMore;
    }
    return pitems;
}

function displaySpecificResults(data, resultsObj, rs)
{
    var itemsOnPage = {};
    if (data && data.items && data.items.length)
    {
        var restart = rs || false;
        noResults = false;
        var items = data.items;

        if (restart)
        {
            items = fillInPreviousPages(resultsObj, items, data.more);
        }
        if (data.divider)
        {
            resultsObj.divider = data.divider;
        }

        resultsObj.div.find("ul").remove();
        var ul = $("<ul class='innercontainer'></ul>");

        resultsObj.div.find("h3").after(ul);

        var isnew = resultsObj.isNewPage();
      
        if (isnew)
        {
            resultsObj.addNewPage(items, data.more);
        }
        $.each(items, function (i, v)
        {
            if (isnew)
            {
                resultsObj.addToCount(v.matcharea);
            }
            if (!itemsOnPage[v.id])
            {
                var li = createSearchResultItem(v);

                ul.append(li);
                itemsOnPage[v.id] = v;
            }
        });

        resultsObj.div.show();

        showPageLinks(resultsObj);
    }
    $('#divSearchResults').show();
}


function displayResults(data)
{
    $("#ingoredchars").hide();
    $("#divwitResults").find("ul").remove();
    $("#divvcResults").find("ul").remove();
    $("#divbuildsResults").find("ul").remove();
    $("#divwikiResults").find("ul").remove();
    $("#divwitResults").hide();
    $("#divvcResults").hide();
    $("#divbuildsResults").hide();
    $("#divwikiResults").hide();
    witResults = new searchResultsObject("wit", $("#divwitResults"));
    vcResults = new searchResultsObject("vc", $("#divvcResults"));
    buildResults = new searchResultsObject("builds", $("#divbuildsResults"));
    wikiResults = new searchResultsObject("wiki", $("#divwikiResults"));
    
    displaySpecificResults(data["wit"], witResults);
    displaySpecificResults(data["vc"], vcResults);
    displaySpecificResults(data["builds"], buildResults);
    displaySpecificResults(data["wiki"], wikiResults);
    $('#divSearchResults').show();
    if (noResults)
    {
        $("#divnoresults").show();
    }
    var ignoredChars = lookForIngoredChars(text);
    if (ignoredChars)
    {
        $("#ignoredchars").text("the following were not used as literal characters in the search results: " + ignoredChars.join(" "));
        $("#ignoredchars").show();
    }
}

function doSearch(resultsObj, restart)
{
    noResults = true;   
    
    $("#divnoresults").hide();
    $("#ignoredchars").hide();
    var params = {};
    var url = sgCurrentRepoUrl + "/searchadvanced.json";
    var loadingDiv = $("#loading");

    params["text"] = $.deparam.fragment()["text"] || $.deparam.querystring()["text"];
    params["areas"] = $.deparam.fragment()["areas"] || $.deparam.querystring()["areas"];
    
    if (!params["areas"] || !params["text"])
    {
        $("#divnoresults").show();
        return;
    }

    if (resultsObj)
    {
        var skip = resultsObj.lastSkipCount() || $.deparam.fragment()[resultsObj.type + "skip"] || 0;
        
        skip = parseInt(skip);
        var lf = "";
        if (restart)
        {
            params["restart"] = true;

            params["max"] = (parseInt($.deparam.fragment()[resultsObj.type + "pg"]) * 20) || 20;
            skip = 0;
        }
        else
        {
            lf = resultsObj.lastField() || $.deparam.fragment()[resultsObj.type + "fld"] || "";
            params["lf"] = lf;
        }
        params["areas"] = resultsObj.type;
        params["skip"] = skip;
           
        loadingDiv = $("#" + resultsObj.type + "Loading");
    }
    url = $.param.querystring(url, params);   

    if (params["text"])
    {
        loadingDiv.show();
      
        vCore.ajax({
            url: url,
            success: function (data)
            {                
                if (data.error)
                {
                    reportError("invalid input - " + data.error);

                    $(".loading").hide();
                    return;
                }
                if (resultsObj)
                {
                    if (resultsObj.type == "wit")
                        displaySpecificResults(data["wit"], witResults, restart);
                    else if (resultsObj.type == "vc")
                        displaySpecificResults(data["vc"], vcResults, restart);
                    else if (resultsObj.type == "builds")
                        displaySpecificResults(data["builds"], buildResults, restart);
                    else if (resultsObj.type == "wiki")
                        displaySpecificResults(data["wiki"], wikiResults, restart);
                }
                else
                {
                    if (getQueryStringVal("redirect"))
                    {
                        var possibleSingles = [];
                        var singleurl = "";

                        displayResults(data);
                        for (var i in data)
                        {
                            if (data[i].items.length > 1)
                            {
                                displayResults(data);
                                break;
                            }
                            else
                            {
                                if (data[i].items && data[i].items.length)
                                {
                                    var item = data[i].items[0];
                                    if (i == "vc")
                                    {
                                        singleurl = sgCurrentRepoUrl + "/changeset.html?recid=" + item.id;
                                    }
                                    else if (i == "builds")
                                    {
                                        singleurl = sgCurrentRepoUrl + "/build.html?recid=" + item.id;
                                    }
                                    else if (i == "wit")
                                    {
                                        singleurl = sgCurrentRepoUrl + "/workitem.html?recid=" + item.id;
                                    }
                                    else if (i == "wiki")
                                    {
                                        singleurl = sgCurrentRepoUrl + "/wiki.html?page=" + encodeURIComponent(item.title);
                                    }
                                    possibleSingles.push(singleurl);
                                }
                            }

                        }

                        if (possibleSingles.length == 1)
                        {
                            window.location.href = singleurl;
                        }
                        else
                        {
                            displayResults(data);
                        }
                    }
                    else
                    {
                        displayResults(data);
                    }
                }
                loaded = true;

                $(".loading").hide();
            }
        });
    }

}

function updateSearchParams(performSearch)
{
    var checked = $("input[name=searcharea]:checked");
    var areaArray = [];
    var currentAreas = state["areas"];

    $.each(checked, function (i, v)
    {
        areaArray.push($(v).val());
    });
    text = $('#searchSearchText').val();
    var unchecked = $("input[name=searcharea]:not(:checked)");
    $.each(unchecked, function (i, v)
    {
        var val = $(v).val();
        $("#div" + val + "Results").hide();
    });
    areas = [];
    if (areaArray.length)
    {
        areas = areaArray.join(",");
        currentAreas = currentAreas || "";

        if (text && (areas.length > currentAreas.length))
        {
            $("#searchhint").show();
        }

        state["areas"] = areas;
    }
    else
    {
        $("#searchhint").hide();
        delete state.areas;
    }
    if (performSearch)
    {
        delete state.witfld;
        delete state.witpg;
        delete state.witskip;
        delete state.vcfld;
        delete state.vcpg;
        delete state.vcskip;
        delete state.buildsfld;
        delete state.buildspg;
        delete state.buildsskip;

        $.bbq.removeState("witfld");
        $.bbq.removeState("witskip");
        $.bbq.removeState("witpg");

        $.bbq.removeState("vcfld");
        $.bbq.removeState("vcskip");
        $.bbq.removeState("vcpg");

        $.bbq.removeState("buildsfld");
        $.bbq.removeState("buildsskip");
        $.bbq.removeState("buildspg");

        $.bbq.removeState("wikifld");
        $.bbq.removeState("wikiskip");
        $.bbq.removeState("wikipg");

        if (text && text.length && areas.length)
        {
            state["text"] = text;

            $.bbq.pushState(state);

            doSearch();
        }
        else
        {
            reportError("Search text and at least one search area must be selected to perform a search.");
        }

    }
}

function _setUpAdvancedSearch()
{
    $('#searchSearchText').keyup(function (e1)
    {
        $("#searchhint").hide();
        if (e1.which == 13)
        {
            e1.preventDefault();
            e1.stopPropagation();
            $('#divSearchResults').hide();
            $('#divSearchResults').find("ul").remove();
            updateSearchParams(true);
        }       
    });
    if (areas && areas.indexOf("wit") >= 0)
    {
        $("#cbxWitSearch").attr("checked", "checked");
    }
    if (areas && areas.indexOf("vc") >= 0)
    {
        $("#cbxVCSearch").attr("checked", "checked");
    }
    if (areas && areas.indexOf("builds") >= 0)
    {
        $("#cbxBuildSearch").attr("checked", "checked");
    }
    if (areas && areas.indexOf("wiki") >= 0)
    {
        $("#cbxWikiSearch").attr("checked", "checked");
    }
    $("#advancedButtonSearch").click(function ()
    {
        $('#divSearchResults').hide();
        $('#divSearchResults').find("ul").remove();
        $("#searchhint").hide();
        updateSearchParams(true);
    });

    $("input[name=searcharea]").click(function ()
    {
        updateSearchParams(false);   
    });
   
    $("#clearSearch").click(function ()
    {
        $("#searchhint").hide();
        $('#searchSearchText').val("");
        $('#divSearchResults').find("ul").remove();
        $('#divSearchResults').hide();
        delete state.text;
        $.bbq.removeState("text");
    });

    $(".searchnav a").click(function (e)
    {
        e.stopPropagation();
        e.preventDefault();

		var target = $(e.target).closest('a');

        doNavigation(target);
    });
}
function sgKickoff()
{
    var md = new vvMarkdownEditor();
    converter = md.converter;
  
    text = $.deparam.fragment()["text"] || getQueryStringVal("text");

    areas = $.deparam.fragment()["areas"] || getQueryStringVal("areas") || "wit,vc,builds,wiki";
    _changesetHover = new hoverControl(fetchChangesetHover);
    state["areas"] = areas;
    _setUpAdvancedSearch();

    if (text)
    {
        state["text"] = text;
        $("#searchSearchText").val(text);
        var witPg = $.deparam.fragment()["witpg"];
        var vcPg = $.deparam.fragment()["vcpg"];
        var buildPg = $.deparam.fragment()["buildspg"];
        var wikiPg = $.deparam.fragment()["wikipg"];
        if (witPg || vcPg || buildPg)
        {
            if (areas.indexOf("wit") >= 0)
            {
                witResults.currentPage = parseInt(witPg) || 1;
                doSearch(witResults, true);
            }
            if (areas.indexOf("vc") >= 0)
            {
                vcResults.currentPage = parseInt(vcPg) || 1;
                doSearch(vcResults, true);
            }
            if (areas.indexOf("builds") >= 0)
            {
                buildResults.currentPage = parseInt(buildPg) || 1;
                doSearch(buildResults, true);
            }
            if (areas.indexOf("wiki") >= 0)
            {
                wikiResults.currentPage = parseInt(wikiPg) || 1;
                doSearch(wikiResults, true);
            }
        }
        if (!buildPg && !vcPg && !witPg && !wikiPg)
        {
            doSearch();
        }
    }
}
