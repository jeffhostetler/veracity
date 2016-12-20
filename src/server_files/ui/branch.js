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
 * This is the JavaScript file for the branch2.html page.
 */

var _changesetHover = null;
var makeRevLinkWithHover = function(revno, csid){
	var a = makePrettyRevIdLink(revno, csid);
	_changesetHover.addHover(a, csid);
	return a;
}

var loadingHistory = false;
var allRetrieved = false;

var onResultsFetched = function(results, lastChunk){
	loadingHistory = false;
	allRetrieved = lastChunk;
	$('#tbl_footer .statusText').text("");
	$('#tbl_footer').hide();
	$('.branchpagewidgets').removeAttr('disabled');
	$('#headselectionloading').css({visibility: "hidden"});
};

var mergeReviewPane;

var loadBranchDetails = function(idx){
	vCore.ajax({
		url: sgCurrentRepoUrl + "/changesets/" + branchHeads[idx] + ".json",
		success: function(data){
			var ic = $('#ic'+idx);
			ic.empty();
			
			var sigspan = $('<span>');
			setTimeStamps(data.description.audits, sigspan);
			ic.append(sigspan);
			
			var cmts = data.description.comments || [];
			if (cmts.length > 0){
				formatComments(cmts).appendTo(ic);
			}
		}
	});
};

var currentQuery = "";
var runQuery = function(label, value){
	if(value===currentQuery || value===""){
		return;
	}
	currentQuery = value;

	$('#revlookupresultcontainer').hide();
	$('#queryresultcontainer').hide();

	var iColon = value.indexOf(":");
	if(iColon>-1 && label){
		// Something was selected from the autocomplete dropdown.
		// In this case we already have a well-defined changeset(s) and we know if it's in the branch.
		
		var precolon = value.slice(0, iColon);
		var postcolon = value.slice(iColon+1);
		
		var revno = false;
		var csid = false;
		
		revno = parseInt(precolon, 10);
		if(revno.toString()===precolon && revno>0){
			csid = postcolon;
		}
		else if(precolon=="branch" || precolon=="tag"){
			postcolon = postcolon.slice(postcolon.lastIndexOf("(")+1, -1);
			if(postcolon.indexOf(",")>-1){
				$('#queryresultdetails').text("The selection does not define a unique changeset in the repository. Please pick a single changeset to view its results individually.");
				$('#queryresultcontainer').show();
				return;
			}
			else if(postcolon.indexOf("?")>-1){
				$('#queryresultdetails').text("The selected changeset is missing from this instance of the repository. It is not in this branch.");
				$('#queryresultcontainer').show();
				return;
			}
			var iColon2 = postcolon.indexOf(":");
			revno = parseInt(postcolon.slice(0, iColon2), 10);
			csid = postcolon.slice(iColon2+1);
		}
		
		if(label[0]=="\u2714"){
			reportChangesetInBranch(revno, csid);
		}
		else{
			fetchMergePreview(revno, csid);
		}
	}
	else{
		// Arbitrary user text. We need to lookup the changeset and then do the dagquery.
		$('#querytext').autocomplete("close");
		$('.branchpagewidgets').attr('disabled', 'disabled');
		$('#revlookupspinner').css({display: "inline"});
		$('#revlookupresultdetails').text("Looking up revision '"+value+"'.");
		$('#revlookupresultcontainer').show();
		vCore.ajax({
			url: sgCurrentRepoUrl+"/rev.json?q="+encodeURIComponent(value),
			success: function(data){
				if(data.length==1 && data[0].revno!='?'){
					$('#revlookupresultdetails').text("Results for '"+value+"':");
					fetchMergePreview(data[0].revno, data[0].csid);
				}
				else{
					if(data.length==0){
						$('#revlookupresultdetails').text("No revision matching '"+value+"' was found.");
					}
					else{
						$('#revlookupresultdetails').text("'"+value+"' does not define a unique changeset in the repository. Matches include: ");
						for(var i=0; i<data.length; ++i){
							if(i>0){
								if(i==data.length-1){
									$('#revlookupresultdetails').append(" and ");
								}
								else{
									$('#revlookupresultdetails').append(", ");
								}
							}
							if(data[i].revno!='?'){
								$('#revlookupresultdetails').append(makeRevLinkWithHover(data[i].revno, data[i].csid));
							}
							else{
								$('#revlookupresultdetails').append($('<span style="font-style: italic">').text(data[i].csid));
								$('#revlookupresultdetails').append(" (not present in repository)");
							}
						}
						$('#revlookupresultdetails').append(".");
					}
					$('.branchpagewidgets').removeAttr("disabled");
				}
			},
			error: function(){
				$('.branchpagewidgets').removeAttr("disabled");
			},
			complete: function(){
				
				$('#revlookupspinner').css({display: "none"});
			}
		});
	}
};

var fetchMergePreview = function(revno, csid){
	$('.branchpagewidgets').attr('disabled', 'disabled');
	$('#queryspinner').css({display: "inline"});
	$('#queryresultdetails').text("Querying history regarding changeset ");
	$('#queryresultdetails').append(makeRevLinkWithHover(revno, csid));
	$('#queryresultcontainer').show();

	vCore.ajax({
		url: sgCurrentRepoUrl+"/changesets/"+branchHeads[0]+"/dagquery/whats-new.json?other="+csid,
		success: function(data){
			if(data.changesets.length>0){
				reportMergePreview(revno, csid, data.changesets, data.isDirectDescendant);
			}
			else{
				reportChangesetInBranch(revno, csid);
			}
		},
		complete: function(){
			$('.branchpagewidgets').removeAttr("disabled");
			$('#queryspinner').css({display: "none"});
		}
	});
}

var reportChangesetInBranch = function(revno, csid){
	$('#querytext').autocomplete("close"); // In case autocomplete results came in while we were doing the query.
	
	$('#queryresultdetails').text("\u2714");
	$('#queryresultdetails').append(makeRevLinkWithHover(revno, csid));
	$('#queryresultdetails').append("is in this branch.");
	$('#queryresultcontainer').show();
	var markers={};
	markers[csid] = "\u2714";
	mergeReviewPane.reset(branchHeads[0]);
	mergeReviewPane.setMarkers(markers);
}

var reportMergePreview = function(revno, csid, newChangesets, isDirectDescendant){
	$('#querytext').autocomplete("close"); // In case autocomplete results came in while we were doing the query.
	
	$('#queryspinner').css({display: "none"});
	$('#queryresultcontainer').show();
	
	var revlink = makeRevLinkWithHover(revno, csid);
	
	var markers = {};
	for(var i=0; i<newChangesets.length; ++i){
		markers[newChangesets[i]] = "\u2192";
	}
	markers[csid] = "\u2794";
	
	if(!isDirectDescendant){
		$('#queryresultdetails').text("Merging \u2794");
		$('#queryresultdetails').append(revlink);
		$('#queryresultdetails').append("into this branch would bring in a total of ");
		if(newChangesets.length>1){
			$('#queryresultdetails').append(newChangesets.length.toString()+" changesets");
		}
		else{
			$('#queryresultdetails').append("1 changeset");
		}
		$('#queryresultdetails').append(" (plus the merge). See preview below.");
		
		var branchHead = branchHeads[0];
		var branchHeadRevno = revnos[branchHead];
		
		var mergeReviewToken = {parents:{}};
		mergeReviewToken.parents[branchHead] = branchHeadRevno;
		mergeReviewToken.parents[csid] = revno;
		
		mergeReviewPane.hide();
		$('#tbl_footer').show();
		mergeReviewPane.reset(mergeReviewToken, branchHeadRevno);
		mergeReviewPane.setMarkers(markers);
	}
	else{
		$('#queryresultdetails').text("A fast-forward merge to \u2794");
		$('#queryresultdetails').append(revlink);
		$('#queryresultdetails').append("would bring a total of ");
		if(newChangesets.length>1){
			$('#queryresultdetails').append(newChangesets.length.toString()+" changesets");
		}
		else{
			$('#queryresultdetails').append("1 changeset");
		}
		$('#queryresultdetails').append(" into this branch.");
		
		mergeReviewPane.hide();
		$('#tbl_footer').show();
		mergeReviewPane.reset(csid);
		mergeReviewPane.setMarkers(markers);
	}
};

var bubbleError = false;
var postNewBranch = function() {
	var name = $('#titletext').val().trim();
	var error = validate_object_name(name, get_object_name_constraints());
	if(error){
		if(!bubbleError){
			bubbleError = new bubbleErrorFormatter();
			$('#titletext').after(bubbleError.createContainer($('#titletext')));
		}
		bubbleError.displayInputError($('#titletext'), ["Branch names "+error]);
	}
	else{
		vCore.ajax({
			type: "POST",
			url: sgCurrentRepoUrl+"/branch",
			data: JSON.stringify({name: name, heads: branchHeads, closed: false}),
			success: function(data){
				window.location.href = branchUrl(name);
			}
		});
	}
};

var toggleBranchClosed = function(){
	$('.branchpagewidgets').attr({disabled: "disabled"});
	if(branchName in sgOpenBranches){
		vCore.ajax({
			type: "PUT",
			url: branchUrl(branchName, null, "/closed"),
			data: JSON.stringify({closed: true}),
			success: function(data){
				$('#maintitle').append(" (Closed)");
				$('#closebutton').attr({value: "Reopen"});
				delete sgOpenBranches[branchName];
			},
			complete: function(){
				$('.branchpagewidgets').removeAttr("disabled");
			}
		});
	}
	else{
		vCore.ajax({
			type: "PUT",
			url: branchUrl(branchName, null, "/closed"),
			data: JSON.stringify({closed: false}),
			success: function(data){
				$('#maintitle').text(branchName);
				$('#closebutton').attr({value: "Close"});
				sgOpenBranches[branchName] = true;
			},
			complete: function(){
				$('.branchpagewidgets').removeAttr("disabled");
			}
		});
	}
};
var deleteBranch = function(){
	var closedOrOpen = ( (branchName in sgOpenBranches) ? "open" : "closed" )
	$('.branchpagewidgets').attr({disabled: "disabled"});
	vCore.ajax({
		type: "DELETE",
		url: branchUrl(branchName, null, "/heads"),
		success: function(data){
			window.location.href = branchUrl("*deleted*"+closedOrOpen+"*"+branchName+"*"+branchHeads.join(","));
		},
		error: function(){
			$('.branchpagewidgets').removeAttr("disabled");
		}
	});
};
var undoDelete = function(name, branchWasClosed){
	$('.branchpagewidgets').attr({disabled: "disabled"});
	vCore.ajax({
		type: "POST",
		url: sgCurrentRepoUrl+"/branch",
		data: JSON.stringify({name: name, heads: branchHeads, closed: branchWasClosed}),
		success: function(data){
			window.location.href = branchUrl(name);
		},
		error: function(){
			$('.branchpagewidgets').removeAttr("disabled");
		}
	});
}

var removeMissingHead = function(csid){
	$('.branchpagewidgets').attr({disabled: "disabled"});
	vCore.ajax({
		type: "DELETE",
		url: branchUrl(branchName, null, "/heads/"+csid),
		success: function(data){
			window.location.reload();
		},
		error: function(){
			$('.branchpagewidgets').removeAttr("disabled");
		}
	});
};

function sgKickoff()
{
	if(branchName[0]!="*"){
		_saveVisitedBranch(branchName);
		if(!(branchName in sgOpenBranches)){
			$('#maintitle').append(" (Closed)");
			$('#closebutton').attr({value: "Reopen"});
		}
		
		if(!vCore.getOption("readonlyServer")){
			$('#buttonsdiv').show();
			$('#titlecontainerbar').css({"padding-top": "2px"});
			$('#titlecontainerbar').height($('#buttonsdiv').outerHeight()+4);
		}
	}
	else if(branchName=="*new"){
		var titletext = $('<input type="text" size="30" id="titletext" style="display: inline; font-size: large;">');
		var titlesubmit = $('<input type="button" value="Create" style="display: inline">');
		titlesubmit.click(postNewBranch);
		titletext.keyup(function(e){if(e.which==13){
			e.preventDefault();
			e.stopPropagation();
			postNewBranch();
		}});
		$('#maintitle').html(titletext).append(" ").append(titlesubmit);
		
		titletext.focus();
		_addInputPlaceholder(titletext, "Enter new branch name.");
		
		$('#titlecontainerbar').css({"padding-top": "2px"});
		$('#titlecontainerbar').height(titletext.outerHeight()+4);
		
	}
	else if(branchName.slice(0,9)=="*deleted*"){
		var branchWasClosed = false;
		if(branchName.slice(0,16)=="*deleted*closed*"){
			branchWasClosed = true;
		}
		
		var iLastStar = branchName.lastIndexOf("*");
		var name = branchName.slice(iLastStar+1);
		
		$('#maintitle').text("Branch '"+name+"' has been deleted. ");
		
		var undoDeleteButton = $('<input type="button" value="Undo">');
		undoDeleteButton.click(function(){
			undoDelete(name, branchWasClosed);
		});
		
		$('#maintitle').append(undoDeleteButton);
		$('#titlecontainerbar').css({"padding-top": "2px"});
		$('#titlecontainerbar').height(undoDeleteButton.outerHeight()+4);
	}

	$('#backgroundPopup').remove();
	$(window).scrollTop(0);

	$(window).scroll(function(){
		if(!loadingHistory && isScrollBottom() && !allRetrieved){
			loadingHistory = true;
			$('#tbl_footer .statusText').text("Loading History...");
			$('#tbl_footer').show();
			mergeReviewPane.fetchMoreResults();
		}
	});

	_changesetHover = new hoverControl(fetchChangesetHover);

	var anyPresentHeads = false;
	for(var i=0; i<branchHeads.length; ++i){
		if(revnos[branchHeads[i]]!==null){
			anyPresentHeads = true;
			
			$('#link'+i).attr({href: sgCurrentRepoUrl + "/changeset.html?recid=" + branchHeads[i]});
			var shortrev = $('#shortrev'+i);
			shortrev.empty();
			shortrev.append(addFullHidDialog(branchHeads[i]));
			
			var ziplink = $('<a>');
			ziplink.css({"margin-left": "1ex"});
			ziplink.attr({
				title: "download zip of repo at this version",
				href: sgCurrentRepoUrl + "/zip/" + branchHeads[i] + ".zip"
			});
			ziplink.append($('<img>').attr({src: sgStaticLinkRoot+"/img/green_download.png"}));
			shortrev.after(ziplink);
			
			var browselink = $('<a>');
			browselink.css({"margin-left": "0.5ex"});
			browselink.attr({
				title: "browse this changeset",
				href: sgCurrentRepoUrl + "/browsecs.html?recid=" + branchHeads[i]+ "&revno=" + revnos[branchHeads[i]]});
			browselink.append($('<img>').attr({src: sgStaticLinkRoot+"/img/green_goto_browse.png"}));
			ziplink.after(browselink);
			
			loadBranchDetails(i);
		}
	}

	if(anyPresentHeads){
		loadingHistory = true;
		mergeReviewPane = newMergeReviewPane({
			tableId: "tablemergereviewhistory",
			head: mergePreviewToken || branchHeads[0],
			mode: "history",
			branchName: branchName,
			branchHeadCount: branchHeads.length,
			boldChangesets: revnos, // (What's significant here is not the revnos, but that the keys are the csids.)
			onResultsFetched: onResultsFetched
		});
		
		$('#csmenuremovehead').click(function(){mergeReviewPane.removeHead();});
		
		if(branchHeads.length==1){
			$('.forsinglepresenthead').show();
			
			$('#csmenuremovehead').append(" (Delete Branch)");
			
			$('#csmenufastforward').click(function(){mergeReviewPane.moveHead();});
			$('#csmenuaddhead').click(function(){mergeReviewPane.addHead();});
			$('#csmenurollback').click(function(){mergeReviewPane.moveHead();});
			
			$('#historyqueryclearbutton').click(function(){
				$('#querytext').val("");
				$('#querytext').blur(); // To trigger placeholder text to reappear.
				$('#revlookupresultcontainer').hide();
				$('#queryresultcontainer').hide();
				if(currentQuery!==""){
					currentQuery = "";
					if(mergeReviewPane.reset(branchHeads[0])){
						mergeReviewPane.hide();
						$('#tbl_footer').show();
					}
					mergeReviewPane.setMarkers({});
				}
			});
			
			_addInputPlaceholder($('#querytext'), "Enter a branch or a changeset to see its relationship to this branch.");
			
			$("#querytext").autocomplete({
				autoFocus: true,
				source: sgCurrentRepoUrl + "/branch/" + encodeURIComponent(branchName) + "/query-autocomplete.json",
				select: function(event, ui){
					runQuery(ui.item.label, ui.item.value);
					event.preventDefault();
					event.stopPropagation();
				}
			});
			
			$("#querytext").keyup(function(e){if(e.which==13){
				e.preventDefault();
				e.stopPropagation();
				runQuery(null, $("#querytext").val());
			}});
		}
	}
	else{
		$('#tablemergereviewhistory').hide();
		$('#tbl_footer').hide();
	}
	
	$('#headselection').change(function(){
		var head = $(this).val();
		if(head=="mergepreview"){head=mergePreviewToken;}
		if(head=="all"){head=allHeadsToken;}
		$('.branchpagewidgets').attr('disabled', 'disabled');
		$('#headselectionloading').css({visibility: "visible"});
		mergeReviewPane.reset(head);
	});
}
