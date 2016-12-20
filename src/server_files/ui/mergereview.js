// onclick event code can only reference members of the global object...
var nodeRotateFunctions = {};

// Dummy variables to keep historygraph.js happy.
var FLAG_NAME_FILTER = 1; 
var FLAG_STAMP_FILTER = 2; 
var FLAG_FROM_FILTER = 4;
var FLAG_TO_FILTER = 8;
var FLAG_BRANCH_FILTER = 16;
var FLAG_PATH_FILTER = 32; 
var filterflags = 0;

// Global constants.
var CHUNK_SIZE = 100;
var HG_COLUMN_WIDTH = 16;

var newMergeReviewPane = function(params){
	var tableId = params.tableId;

	var historyMode = (params.mode==="history");
	var changesetDetailsMode = (params.mode==="changeset details");

	var boldChangesets = params.boldChangesets || {};
	var markers = {};

	var hg = new historyGraph(tableId, HG_COLUMN_WIDTH);

	var head;
	var mergePreviewMode = false;
	var results = [];
	var rowCount = 0;
	var continuationToken;

	var mergeBaselines = {};

	var contextMenusPresent = (params.branchName && !vCore.getOption("readonlyServer"));
	var contextMenuIndex = false;

	var generateTokenForMergePreview = function(forcedNewMergePreviewBaseline){
		if(forcedNewMergePreviewBaseline){
			mergeBaselines["0"] = forcedNewMergePreviewBaseline;
		}
		var token = [1];
		var mergePreviewBaseline=false;
		if(mergeBaselines["0"]){
			mergePreviewBaseline = mergeBaselines["0"];
			token.push(mergePreviewBaseline);
		}
		for(var csid in head.parents){
			if(head.parents[csid]!=mergePreviewBaseline){
				token.push(head.parents[csid]);
			}
		}
		return token;
	}
	var reset = function(newHead, forcedNewMergePreviewBaseline){
		if(newHead!=head){
			results = [];
			head = newHead;
			if(head.parents){
				mergePreviewMode = true;
				continuationToken = generateTokenForMergePreview(forcedNewMergePreviewBaseline);
			}
			else{
				mergePreviewMode = false;
				continuationToken = head;
			}
			fetchNewRows();
			return true;
		}
		else{
			return false;
		}
	};


	// All changesets we've fetched, keyed by changeset_id.
	var changesetDetailsCache = {};
	var noncacheableProps = {
		// Properties that change on rotation
		Index: true,
		displayChildren: true,
		continuationToken: true,
		indent: true,
		displayParent: true,
		indexInParent: true,
		// Properties that don't need to be cached, despite being unchanging.
		revno: true,
		changeset_id: true,
		parents:true
	};
	var addToChangesetDetailsCache = function(node, copyToResults){
		var cacheEntry = {};
		for(var prop in node){
			if(!(prop in noncacheableProps)){
				cacheEntry[prop] = node[prop];
				if(copyToResults){
					results[node.Index][prop] = node[prop];
				}
			}
		}
		changesetDetailsCache[node.changeset_id] = cacheEntry;
	};
	var getCachedChangesetDetails = function(node){
		var cacheEntry = changesetDetailsCache[node.changeset_id];
		if(cacheEntry){
			for(var prop in cacheEntry){
				node[prop] = cacheEntry[prop];
			}
			return true;
		}
		return false;
	};

	var populateDetailsDiv = function(divdetails, node){
		if (node.tags != null && node.tags.length>0){
			var tags = node.tags.sort();
			var i=0;
			var ch=0;
			while(i<tags.length && ch+tags[i].length<=25){
				var span = $('<span>').text(tags[i]).addClass("vvtag");
				divdetails.append(' ');
				divdetails.append(span);
				
				ch += tags[i].length;
				++i;
			}
			if(i == tags.length-1 && tags[i].length<=25){
				var span = $('<span>').text(tags[i]).addClass("vvtag");
				divdetails.append(' ');
				divdetails.append(span);
				++i;
			}
			if(i < tags.length){
				var moreTagsShowing = false;
				var revelationButton = $('<img alt="\u2192">').attr({src: sgStaticLinkRoot+"/img/tags_open1.png"});
				revelationButton.css({cursor: "default", "padding-top": "1px"});
				revelationButton.hover(
					function(){
						if(moreTagsShowing){
							revelationButton.attr({src: sgStaticLinkRoot+"/img/tags_close2.png"});
						}
						else{
							revelationButton.attr({src: sgStaticLinkRoot+"/img/tags_open2.png"});
						}
					},
					function(){
						if(moreTagsShowing){
							revelationButton.attr({src: sgStaticLinkRoot+"/img/tags_close1.png"});
						}
						else{
							revelationButton.attr({src: sgStaticLinkRoot+"/img/tags_open1.png"});
						}
					}
				);
				revelationButton.click(function(){
					moreTagsShowing = !moreTagsShowing;
					$('#'+tableId+'_r'+node.Index+' .moretags').css({display: ((moreTagsShowing)?("inline"):("none"))});
					revelationButton.attr({alt: ((moreTagsShowing)?("\u2190"):("\u2192"))});
					revelationButton.attr({src: sgStaticLinkRoot+"/img/tags_"+((moreTagsShowing)?("close"):("open"))+"2.png"});
				});
				
				var moreTags = $('<span class="moretags">');
				while(i < tags.length){
					var span = $('<span>').text(tags[i]).addClass("vvtag");
					moreTags.append(' ');
					moreTags.append(span);
					++i
				};
				moreTags.hide();
				
				divdetails.append(' ');
				divdetails.append(revelationButton);
				divdetails.append(moreTags);
			}
		}
		
		if(node.audits && node.audits.length>=1){
			var spanwho = $('<span>').css({color: "#666666", "font-size": "x-small", "font-style": "italic", "font-weight": "lighter"});
			var who = node.audits[0].username || node.audits[0].userid;
			spanwho.text(who);
			
			divdetails.append(' ');
			divdetails.append(spanwho);
		}
		if (node.comments && node.comments.length > 0){
			var spancomment = $('<span>');
			// history is turned off right now which messes this up
			// so check to see if we have history
			if (node.comments[0].history){
				node.comments.sort(function (a, b){
					return (firstHistory(a).audits[0].timestamp - firstHistory(b).audits[0].timestamp);
				});
			}
			// var spancomment =
			// $('<span>').text(node.comments[0].text).addClass("comment");
			var vc = new vvComment(node.comments[0].text);
			spancomment.html(vc.summary);
			
			divdetails.append(' ');
			divdetails.append(spancomment);
		}
		if(node.audits && node.audits.length>=1){
			var spanwhen = $('<span>').css({color: "#666666", "font-size": "x-small", "font-style": "italic", "font-weight": "lighter"});
			var d = new Date(parseInt(node.audits[0].timestamp));
			spanwhen.text(shortDateTime(d));
			
			divdetails.append(' ');
			divdetails.append(spanwhen);
		}
		
		if (node.stamps != null){
			var stamps = node.stamps.sort();
			for(var i=0; i<stamps.length; ++i){
				var stampName = stamps[i];
				var a = $('<a>').attr({
					"title": "view changesets with this stamp",
					href: sgCurrentRepoUrl + "/history.html?stamp=" + encodeURIComponent(stampName)
				});
				a.text(stampName);
				var span = $('<span>').append(a).addClass("stamp");
				divdetails.append(' ');
				divdetails.append(span);
			};
		}
	};

	var showContextMenu = function(Index){
		contextMenuIndex = Index;
		var csid = results[Index].changeset_id;
		$('#csmenuremovehead').hide();
		$('#csmenuaddhead').hide();
		$('#csmenufastforward').hide();
		$('#csmenurollback').hide();
		if(params.branchName[0]!="*"){
			if(csid in boldChangesets){
				$('#csmenuremovehead').show();
			}
			else if(params.branchHeadCount===1){
				if(markers[csid]=="\u2794" || markers[csid]=="\u2192"){
					if(results[0].changeset_id=="(merge-preview)"){
						$('#csmenuaddhead').show();
					}
					else{
						$('#csmenufastforward').show();
					}
				}
				else{
					$('#csmenurollback').show();
				}
			}
		}
		$('#csmenunewbranch a').attr({href: branchUrl("*new*"+csid)});
		showCtxMenu("context"+Index, $("#cscontextmenu"));
	};

	var displayNode = function(node){
		var tr;
		if(node.Index<rowCount){
			tr = $('#'+tableId+"_r"+node.Index);
			tr.empty();
			tr.show();
		}
		else{
			tr = $('<tr class="trReader"></tr>').attr('id', tableId+"_r"+node.Index);
			enableRowHovers(false, tr);
		}

		if(contextMenusPresent){
			var tdmenu = $('<td style="min-width: 22px; width: 22px">');
			if(node.changeset_id!="(merge-preview)"){
				var span = $('<span>').addClass('linkSpan');
				if(!isTouchDevice()){span.hide();}
				
				var amenu = $('<a class="small_link">');
				amenu.append($('<img>').attr({src: sgStaticLinkRoot+"/img/contextmenu.png"}));
				amenu.attr({title: "view context menu", id: "context"+node.Index});
				amenu.click(function(e){
					e.preventDefault();
					e.stopPropagation();
					showContextMenu(node.Index);
				});
				
				span.append(amenu);
				tdmenu.append(span);
			}
			tr.append(tdmenu);
		}


		var tdlink = $('<td>').css('padding-left','5px');
		if(node.revno){
			if(node.changeset_id in boldChangesets){
				var b = $('<b>');
				b.append(makePrettyRevIdLink(node.revno,node.changeset_id, true));
				tdlink.append(b);
			}
			else{
				tdlink.append(makePrettyRevIdLink(node.revno,node.changeset_id, true));
			}
			
			var spanprefix = $('<span class="highlightprefixtext">');
			if(markers[node.changeset_id]){
				spanprefix.text(markers[node.changeset_id]);
			}
			else{
				spanprefix.css({display: "none"});
			}
			tdlink.prepend(spanprefix);
		}
		else{
			tdlink.append("(merge-preview)");
		}
		tr.append(tdlink);

		var tddetails = $('<td>').addClass("commentcell graphdetails");
		tddetails.css({"padding-left": HG_COLUMN_WIDTH*(node.indent+2)});
		var divdetails = $('<div>').css({height: "1.5em", "white-space": "nowrap", "text-overflow": "ellipsis", "-o-text-overflow": "ellipsis", "-ms-text-overflow": "ellipsis", "overflow": "hidden"});
		if(node.audits){
			populateDetailsDiv(divdetails, node);
		}
		tddetails.append(divdetails);
		tr.append(tddetails);

		if(node.Index==rowCount){
			$('#'+tableId).append(tr);
			++rowCount
		}
		hg.addToGraph(node, tddetails, node.Index, []);
	};

	var fetchNewRows = function(){
		if(!continuationToken){
			return;
		}
		vCore.ajax({
			url: sgCurrentRepoUrl+"/merge-review.json",
			type: "POST",
			isWriteCall: false,
			data: JSON.stringify({
				start: continuationToken,
				resultLimit: CHUNK_SIZE,
				singleMergeReview: changesetDetailsMode,
				includeHistoryDetails: true,
				mergeBaselines: mergeBaselines
			}),
			dataType: 'json',
			success: function(data) {
				if(results.length==0){
					$('#'+tableId+' tbody td').hide();
					hg.clearGraph();
					
					if(mergePreviewMode){
						// Insert the dummy node client-side.
						var node = {
							Index: 0,
							displayChildren: continuationToken.slice(1),
							indent: 0,
							revno: 0,
							changeset_id: "(merge-preview)",
							parents: head.parents
						};
						results.push(node);
						displayNode(node);
					}
				}
				
				var countNewRows = data.length-1;
				for(var i=0; i<countNewRows; ++i){
					var node = data[i];
					node.Index = results.length;
					if(mergePreviewMode && (node.changeset_id in head.parents)){
						node.displayParent = 0;
					}
					results.push(node);
					addToChangesetDetailsCache(node);
					displayNode(node);
				}
				
				if(changesetDetailsMode && results[results.length-1].indent==0){
					results[results.length-1].displayChildren = [];
					continuationToken = false;
				}
				else{
					continuationToken = data[data.length-1];
				}
				
				hg.draw();
				
				if(params.onResultsFetched){
					params.onResultsFetched(results, !continuationToken);
				}
			}
		});
	};

	var fetchHistoryDetails = function(csids, skip){
		vCore.ajax({
			url: sgCurrentRepoUrl+"/fill-in-history-details.json",
			type: "POST",
			isWriteCall: false,
			data: JSON.stringify(csids.slice(skip, skip+CHUNK_SIZE)),
			dataType: 'json',
			success: function(data) {
				for(var i=0; i<data.length; ++i){
					var details=data[i];
					addToChangesetDetailsCache(details, true);
					populateDetailsDiv($('#'+tableId+"_r"+details.Index+" .commentcell div"), details);
				}
				if(skip+data.length<csids.length){
					fetchHistoryDetails(csids, skip+data.length);
				}
			}
		});
	};

	var refetchFromTo = function(firstRowIndex, lastRowIndex){
		var changesetsToFetchHistoryFor = [];
		vCore.ajax({
			url: sgCurrentRepoUrl+"/merge-review.json",
			type: "POST",
			data: JSON.stringify({
				start: results[firstRowIndex].continuationToken || results[firstRowIndex].changeset_id,
				resultLimit: lastRowIndex-firstRowIndex+1,
				singleMergeReview: changesetDetailsMode,
				includeHistoryDetails: false,
				mergeBaselines: mergeBaselines
			}),
			dataType: 'json',
			success: function(data) {
				hg.deleteFromTo(firstRowIndex, lastRowIndex);
				
				var countNewRows = data.length-1;
				
				for(var i=0; i<countNewRows; ++i){
					var node = data[i];
					node.Index = firstRowIndex+i;
					if(mergePreviewMode && (node.changeset_id in head.parents)){
						node.displayParent = 0;
					}
					if(!getCachedChangesetDetails(node)){
						changesetsToFetchHistoryFor.push({changeset_id: node.changeset_id, Index: node.Index});
					}
					results[node.Index] = node;
					displayNode(node);
				}
				
				if(firstRowIndex+countNewRows==results.length){
					if(changesetDetailsMode && results[results.length-1].indent==0){
						results[results.length-1].displayChildren = [];
						continuationToken = false;
					}
					else{
						continuationToken = data[data.length-1];
					}
				}
				
				hg.draw();
				
				if(changesetsToFetchHistoryFor.length>0){
					fetchHistoryDetails(changesetsToFetchHistoryFor, 0);
				}
			}
		});
	};

	var nodeRotate = function(mergeNodeIndex, baselineNodeRevno){
		var mergeNode = results[mergeNodeIndex];
		baselineNodeRevno = baselineNodeRevno || mergeNode.displayChildren[1];
		mergeBaselines[mergeNode.revno.toString()] = baselineNodeRevno;
		
		if(params.onBeginRotateNode){
			params.onBeginRotateNode(mergeNode, baselineNodeRevno);
		}
		
		if(changesetDetailsMode && mergeNode.Index===0){
			// Rotating the top node on the changeset details page.
			// This is more like an initial fetch than a rotation.
			results = [];
			continuationToken = head;
			fetchNewRows();
		}
		else if(mergePreviewMode && mergeNode.Index===0){
			// Rotating the merge-preview node.......
			results[1].continuationToken = generateTokenForMergePreview();
			hg.disconnectFromTo(0, results.length-1);
			refetchFromTo(1, results.length-1);
		}
		else{
			var firstRowIndex = mergeNode.Index;
			var lastRowIndex = firstRowIndex;
			if(mergeNode.indent===0){
				lastRowIndex = results.length-1;
			}
			else {
				while((lastRowIndex < results.length-1) && !(lastRowIndex+1 <= results.length-1 && results[lastRowIndex+1].indent < mergeNode.indent)){
					++lastRowIndex;
				}
			}

			hg.disconnectFromTo(firstRowIndex, lastRowIndex);
			refetchFromTo(firstRowIndex, lastRowIndex);
		}
	};
	nodeRotateFunctions[tableId] = nodeRotate;

	var redraw = function(){
		hg.draw();
	};

	var setMarkers = function(newMarkers){
		markers = newMarkers;
		$('.highlightprefixtext').css({display: "none"});
		for(var i=0; i<results.length; ++i){
			var marker = markers[results[i].changeset_id];
			if(marker){
				$('#'+tableId+"_r"+i+" .highlightprefixtext").text(marker);
				$('#'+tableId+"_r"+i+" .highlightprefixtext").css({display: "inline"});
			}
		}
	};

	var hide = function(){
		hg.clearGraph();
		$('#'+tableId+' tr.trReader').hide();
	};

	var addHead = function(){
		if(contextMenuIndex>=0 && contextMenuIndex<results.length){
			vCore.ajax({
				url: branchUrl(params.branchName, null, "/heads"),
				type: "POST",
				data: JSON.stringify({csid: results[contextMenuIndex].changeset_id}),
				dataType: 'json',
				success: function(){
					window.location.reload();
				}
			});
		}
	};
	var moveHead = function(){
		if(contextMenuIndex>=0 && contextMenuIndex<results.length && params.head===params.head.toString() && results[contextMenuIndex].changeset_id!=params.head){
			vCore.ajax({
				url: branchUrl(params.branchName, null, "/heads/"+params.head),
				type: "PUT",
				data: JSON.stringify({move_to_csid: results[contextMenuIndex].changeset_id}),
				dataType: 'json',
				success: function(){
					window.location.reload();
				}
			});
		}
	};
	var removeHead = function(){
		if(contextMenuIndex>=0 && contextMenuIndex<results.length){
			vCore.ajax({
				url: branchUrl(params.branchName, null, "/heads/"+results[contextMenuIndex].changeset_id),
				type: "DELETE",
				success: function(){
					if(params.branchHeadCount==1){
						window.location.href = branchUrl("*deleted*"+params.branchName+"*"+params.head);
					}
					else{
						window.location.reload();
					}
				}
			});
		}
	}

	if(contextMenusPresent){
		$('#'+tableId+" thead tr").prepend($('<th>').css({"min-width": "22px", "width": "22px", "padding": "0"}));
	}

	reset(params.head);

	return {
		reset: reset,
		setMarkers: setMarkers,
		fetchMoreResults: fetchNewRows,
		redraw: redraw,
		hide: hide,
		hg: hg,
		addHead: addHead,
		moveHead: moveHead,
		removeHead: removeHead
	};
};
