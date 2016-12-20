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

var vclinkFields = {
	"workitems": {
		isMultiple: true,
		isFromMe: false,
		linkType: 'vc_changeset',
		linkSource: 'item',
		linkTarget: 'commit',
		otherType: 'item'
	}
};


// getBranchesInfo() returns an object like:
// {
//     openBranchNames: ["Branch1", "Branch2", ... (branch names in lexicographical order)],
//     closedBranchNames: ["ClosedBranch1", "ClosedBranch2", ... (lexicographical order)],
//     all: {
//         Branch1: {
//             name: "Branch1",
//             presentHeads: {
//                 <csid>: <revno>
//             },
//             countPresentHeads: 1,
//             missingHeads:{
//                 <csid>: true;
//             },
//             countMissingHeads: 1
//         },
//         Branch2: { ... },
//         ...
//         ClosedBranch1: { closed: true, ... },
//         ClosedBranch2: { closed: true, ... },
//         ...
//     },
// }
var getBranchesInfo = function(repo){
	var branchesdb = new zingdb(repo, sg.dagnum.VC_BRANCHES);
	var zingClosedBranchList = branchesdb.query("closed", ["*"]);
	var closed={};
	for(var i=0; i<zingClosedBranchList.length; ++i){
		closed[zingClosedBranchList[i].name] = true;
	}
	var branchList = branchesdb.query("branch", ["*"]);
	var openBranchNames = [];
	var closedBranchNames = [];
	var all={};
	for(var i=0; i<branchList.length; ++i){
		var branchName = branchList[i].name;
		if(!all[branchName]){
			all[branchName] = {
				name: branchName,
				presentHeads: {},
				countPresentHeads: 0,
				missingHeads: {},
				countMissingHeads: 0
			};

			if(!closed[branchName]){
				openBranchNames.push(branchName);
			}
			else{
				closedBranchNames.push(branchName);
				all[branchName].closed = true;
			}
		}
		
		var revno = repo.csid_to_revno(branchList[i].csid);
		if(revno){
			++all[branchName].countPresentHeads;
			all[branchName].presentHeads[branchList[i].csid] = revno;
		}
		else{
			++all[branchName].countMissingHeads;
			all[branchName].missingHeads[branchList[i].csid] = true;
		}
	}
	openBranchNames.sort(vv.strcmpi);
	closedBranchNames.sort(vv.strcmpi);
	return {
		openBranchNames: openBranchNames,
		closedBranchNames: closedBranchNames,
		all: all
	};
};

var listOpenBranchesWithChangeset = function(csid, repo, branchesInfo, decorateForCurrentHeads) {
	branchesInfo = branchesInfo || getBranchesInfo(repo);
	var results = [];
	for(var i=0; i<branchesInfo.openBranchNames.length; ++i){
		var branchName = branchesInfo.openBranchNames[i];
		var info = branchesInfo.all[branchName];
		for(var headCsid in info.presentHeads){
			if(repo.changeset_history_contains(headCsid, csid)){
				if(!decorateForCurrentHeads){
					results.push(branchName);
				}
				else if(repo.changeset_history_contains(csid, headCsid)){
					results.push("!"+branchName);
				}
				else{
					results.push("."+branchName);
				}
				break;
			}
		}
	}
	return results;
};


var decorateHitOrMiss = function(str, hit){
	if(hit){
		return "\u2714 "+str; // HEAVY CHECK MARK prefix
		//return "\u2713 "+str; // CHECK MARK prefix
	}
	else{
		//return "\u2718 "+str; // HEAVY BALLOT X prefix
		//return "\u2757 "+str; // HEAVY EXCLAMATION MARK SYMBOL prefix
		//return "\u261b "+str; // BLACK RIGHT POINTING INDEX prefix
		return "\u2794 "+str; // HEAVY WIDE-HEADED RIGHTWARDS ARROW prefix
	}
};


registerRoutes({
	"/repos/<repoName>/merge-review.json": {
		"POST": {
			availableOnReadonlyServer: true,
			onJsonReceived: function(request, data) {
				return jsonResponse(
					request.repo.merge_review(data.start, data.singleMergeReview, data.resultLimit, data.includeHistoryDetails, data.mergeBaselines)
				);
			}
		}
	},
	"/repos/<repoName>/fill-in-history-details.json": {
		"POST": {
			availableOnReadonlyServer: true,
			onJsonReceived: function(request, data) {
				return jsonResponse(
					request.repo.fill_in_history_details(data)
				);
			}
		}
	},
	"/repos/<repoName>/branch": {
		"POST": {
			requireUser: true,
			onJsonReceived: function(request, data){
				// This is for creating a new branch. Make sure it doesn't already exist.
				var branchesdb = new zingdb(request.repo, sg.dagnum.VC_BRANCHES);
				if(branchesdb.query("branch", ["*"], vv.where({name: data.name})).length>0){
					return badRequestResponse(request, "A branch with that name already exists.");
				}
				
				var audit = request.repo.create_audit(request.vvuser);
				try{
					for(var iHead=0; iHead<data.heads.length; ++iHead){
						request.repo.add_head(data.name, data.heads[iHead], audit);
					}
					
					if(data.closed){
						request.repo.close_branch(data.name, audit);
					}
					
					return OK;
				}
				finally{
					audit.fin();
				}
			}
		}
	},
	"/repos/<repoName>/branch/<branchname>/closed": {
		"PUT": {
			requireUser: true,
			onJsonReceived: function(request, data){
				var audit = request.repo.create_audit(request.vvuser);
				try{
					if(data.closed){
						request.repo.close_branch(request.branchname, audit);
					}
					else{
						request.repo.reopen_branch(request.branchname, audit);
					}
					return OK;
				}
				finally{
					audit.fin();
				}
			}
		}
	},
	"/repos/<repoName>/branch/<branchname>/heads": {
		"POST": {
			requireUser: true,
			onJsonReceived: function(request, data){
				var audit = request.repo.create_audit(request.vvuser);
				try{
					request.repo.add_head(request.branchname, data.csid, audit);
					return OK;
				}
				finally{
					audit.fin();
				}
			}
		},
		"DELETE": {
			requireUser: true,
			onDispatch: function(request){
				var audit = request.repo.create_audit(request.vvuser);
				try{
					var branchesdb = new zingdb(request.repo, sg.dagnum.VC_BRANCHES);
					var branchList = branchesdb.query("branch", ["*"], vv.where({name: request.branchname}));
					for(var i=0; i<branchList.length; ++i){
						request.repo.remove_head(request.branchname, branchList[i].csid, audit);
					}
					return OK;
				}
				finally{
					audit.fin();
				}
			}
		}
	},
	"/repos/<repoName>/branch/<branchname>/heads/<csid>": {
		"PUT": {
			requireUser: true,
			onJsonReceived: function(request, data){
				var audit = request.repo.create_audit(request.vvuser);
				try{
					request.repo.move_head(request.branchname, request.csid, data.move_to_csid, audit);
					return OK;
				}
				finally{
					audit.fin();
				}
			}
		},
		"DELETE": {
			requireUser: true,
			onDispatch: function(request){
				var audit = request.repo.create_audit(request.vvuser);
				try{
					request.repo.remove_head(request.branchname, request.csid, audit);
					return OK;
				}
				finally{
					audit.fin();
				}
			}
		}
	},
	"/repos/<repoName>/branch/<branchname>.html": {
		"GET": {
			onDispatch: function(request)
			{
				var branchName;
				var title;
				var heads = [];
				var revnos = {};
				var countPresentHeads = 0;
				var countAbsentHeads = 0;

				if(request.branchname[0]!='*'){
					branchName = request.branchname;
					title = branchName;
					
					var db = new zingdb(request.repo, sg.dagnum.VC_BRANCHES);
					var a = db.query("branch", ["*"], vv.where({name: branchName}));
					for(var i=0; i<a.length; ++i) {
						heads.push(a[i].csid);
						revnos[a[i].csid] = request.repo.csid_to_revno(a[i].csid) || null;
						if(revnos[a[i].csid]===null){
							++countAbsentHeads;
						}
						else{
							++countPresentHeads;
						}
					}
				}
				else{
					var iLastStar = request.branchname.lastIndexOf("*");
					branchName = request.branchname.slice(0, iLastStar);
					if(branchName.slice(0,5)=="*new*"){
						if(serverConf.readonly){
							throw notFoundResponse(request, "Cannot create a new branch when the server is in readonly mode.");
						}
						title = "New Branch";
					}
					else if(branchName.slice(0,14)=="*deleted*open*"){
						title = branchName.slice(14)+" (Deleted)";
					}
					else if(branchName.slice(0,16)=="*deleted*closed*"){
						title = branchName.slice(16)+" (Deleted)";
					}
					else{
						title = request.branchname.slice(iLastStar+1);
					}
					
					heads = request.branchname.slice(iLastStar+1).split(",");
					for(var i=0; i<heads.length; ++i){
						revnos[heads[i]] = request.repo.csid_to_revno(heads[i]) || null;
						if(revnos[heads[i]]===null){
							++countAbsentHeads;
						}
						else{
							++countPresentHeads;
						}
					}
					
				}
				
				var compareRevnos = function(x,y){
					if(revnos[x]!==null && revnos[y]!==null){return revnos[x]-revnos[y];}
					if(revnos[x]!==null && revnos[y]===null){return -1;}
					if(revnos[x]===null && revnos[y]!==null){return 1;}
					if(x<y){return -1;}
					return 1;
				}
				heads.sort(compareRevnos);
				
				if(heads.length>=1){
					return new vvUiTemplateResponse(request, "core", title, function(key, request){
						if(key=="BRANCHHEADS"){return ["raw:", sg.to_json(heads)];}
						else if(key=="REVNOS"){return ["raw:", sg.to_json(revnos)];}
						else if(key=="BRANCHNAME"){return branchName;}
						else if(key=="MERGEPREVIEWTOKEN"){
							if(!(countPresentHeads==2 && countAbsentHeads==0)){return "false";}
							var y = "{parents:{";
							for(var i=0; i<heads.length; ++i){
								if(i>0){y += ", ";}
								y += "\""+heads[i]+"\":"+revnos[heads[i]];
							}
							y += "}}";
							return ["raw:", y];
						}
						else if(key=="ALLHEADSTOKEN"){
							if(countPresentHeads<2){return "false";}
							var y = "[1, 0";
							for(var i=0; i<heads.length; ++i){
								if(revnos[heads[i]]!==null){
									y += ", "+revnos[heads[i]];
								}
							}
							y += "]";
							return y;
						}
						else if(key=="BRANCHHEADDETAILS"){
							var y = ''
							if(heads.length>1 && branchName[0]!="*"){
								y += '<div class="primarydetails" style="margin-bottom: 1em;">';
								y += '<div style="font-weight: bold;">This branch has multiple heads and needs to be merged.</div>';
								y += '</div>';
							}
							for(var i=0; i<heads.length; ++i){
								if(revnos[heads[i]]!==null){
									y += '<div class="outerheading">';
									y += '<a id="link'+i+'">'+revnos[heads[i]]+':'+heads[i].slice(0,10)+'</a>';
									y += '<span id="shortrev'+i+'">...</span>';
									y += '</div>';
									y += '<div class="innercontainer" id="ic'+i+'"><span class="smallProgress"></span></div>';
									y += '\n';
								}
								else{
									y += '<div class="outerheading"><a style="cursor: text">'+heads[i]+'</a> (Not Present)</div>';
									y += '<div class="innercontainer">';
									y += 'The changeset <i>'+heads[i]+'</i> has been marked as a head of this branch, but has not yet been pushed or pulled into this instance of the repository.';
									if(heads.length>1 && !serverConf.readonly && branchName.slice(0,9)!="*deleted*"){
										y += ' <input type="button" value="Remove Head" onclick="removeMissingHead(\''+heads[i]+'\')" class="branchpagewidgets"></input>';
									}
									y += '</div>';
									y += '\n';
								}
							}
							return ["raw:", y];
						}
						else if(key=="BRANCHHEADSELECTION"){
							var y = ''
							if(countPresentHeads>1){
								y += '<span id="headselectionloading" class="smallProgress"></span>';
								y += '<select id="headselection" style="margin-bottom: 1ex;" disabled="disabled" class="branchpagewidgets">';
								if(heads.length==2){y += '<option value="mergepreview">Show merge preview</option>';};
								for(var i=0; i<heads.length; ++i){
									if(revnos[heads[i]]!==null){
										y += '<option value="'+heads[i]+'">Show history from head '+revnos[heads[i]]+':'+heads[i].slice(0,10)+'</option>';
									}
								}
								if(countAbsentHeads==0){
									y += '<option value="all">Show history including all heads</option>';
								}
								else{
									y += '<option value="all">Show history including all present heads</option>';
								}
								y += '</select>';
							}
							return ["raw:", y];
						}
					}, "branch2.html");
				}
				else{
					return errorResponse(STATUS_CODE__NOT_FOUND, request, "Branch not found.");
				}
			}
		}
	},
	"/repos/<repoName>/branch/<branchName>/query-autocomplete.json": {
		"GET": {
			onDispatch: function(request){
				var results = [];
				
				var branchesInfo = getBranchesInfo(request.repo);
				var myInfo = branchesInfo.all[request.branchName];
				if(myInfo.countPresentHeads!=1 || myInfo.countAbsentHeads>0){
					// We currently only return results when there is a single, present head.
					return jsonResponse(results);
				}
				var branchHead = firstKeyIn(myInfo.presentHeads);
				var searchType=false;
				var userHash;
				
				var term = vv.getQueryParam("term", request.queryString);
				var iColon = term.indexOf(":");
				if(iColon>-1){
					var beforeColon = term.slice(0,iColon);
					var afterColon = term.slice(iColon+1);
					
					var revno = parseInt(beforeColon, 10);
					if(revno.toString()===beforeColon && revno>0){
						var csid = request.repo.revno_to_csid(revno);
						if(csid && vv.startswithi(csid, afterColon)){
							var label = decorateHitOrMiss(revno.toString()+":"+csid, request.repo.changeset_history_contains(branchHead, csid))
							results.push({label: label, value: revno+":"+csid});
							searchType = "found";
						}
					}
					else if(beforeColon in {"branch":1, "tag":1}){
						searchType = beforeColon;
						term = vv.ltrim(afterColon);
					}
				}
				else{
					var revno = parseInt(term, 10);
					if(revno.toString()===term && revno>0){
						var csid = request.repo.revno_to_csid(revno);
						if(csid){
							var label = decorateHitOrMiss(revno.toString()+":"+csid, request.repo.changeset_history_contains(branchHead, csid))
							results.push({label: label, value: revno+":"+csid});
							searchType = "found";
						}
					}
				}
				var finalSpace = (term.length>0 && vv.isBreakingWhitespace(term[term.length-1]));
				
				if(searchType=="branch" || !searchType){
					var branchesToSearch = branchesInfo.openBranchNames;
					if(term!=""){
						branchesToSearch = branchesToSearch.concat(branchesInfo.closedBranchNames);
					}
					var countBranchResults = 0;
					for(var iBranch=0; iBranch<branchesToSearch.length; ++iBranch){
						var name = branchesToSearch[iBranch];
						var info = branchesInfo.all[name];
						if(vv.startswithi(name, term) && (name!==request.branchName || name===term)){
							var label = name;
							if(info.countMissingHeads>0){label += " (missing head)";}
							else if(info.countPresentHeads>1){label += " (needs merge)";}
							if(info.closed){label += " (closed)";}
							var gotIt = true;
							if (info.countMissingHeads>0){
								gotIt = false;
							}
							else{
								for(var csid in info.presentHeads){
									if(!request.repo.changeset_history_contains(branchHead, csid)){
										gotIt = false;
										break;
									}
								}
							}
							label = decorateHitOrMiss(label, gotIt);
							
							var value = "branch: "+name+" (";
							var first = true;
							for(var branchHeadCsid in info.presentHeads){
								if(first){first=false;}else{value+=", ";}
								value += info.presentHeads[branchHeadCsid]+":"+branchHeadCsid;
							}
							for(var branchHeadCsid in info.missingHeads){
								if(first){first=false;}else{value+=", ";}
								value += "?:"+branchHeadCsid;
							}
							value += ")";
							
							results.push({label: label, value: value});
							++countBranchResults;
						}
						if(countBranchResults>=8 && info.closed){
							break;
						}
					}
				}
				
				if(searchType=="tag" || !searchType){
					var tagName = term;
					var tagsdb = new zingdb(request.repo, sg.dagnum.VC_TAGS);
					var tagResult = tagsdb.query("item", ["csid"], vv.where({tag: tagName}));
					if(tagResult.length==1){
						var tagCsid = tagResult[0].csid;
						var tagRevno = request.repo.csid_to_revno(tagCsid) || "?";
						var gotIt = false;
						if(tagRevno!="?"){
							gotIt = request.repo.changeset_history_contains(branchHead, tagCsid);
						}
						var label = decorateHitOrMiss(tagName, gotIt);
						var value = "tag: "+tagName+" ("+tagRevno+":"+tagCsid+")";
						results.push({label: label, value: value});
					}
				}
				
				if(!searchType && term.length>1){
					var a = false;
					try{
						a = request.repo.vc_lookup_hids(term);
					}
					catch(ex){
					}
					
					if(a && a.length>0){
						userHash =  userHash || getUserHash(request.repo);
						for(var i=0; i<a.length && i<8; ++i){
							var desc = request.repo.get_changeset_description(a[i]);
							var comment = "";
							if(desc.comments && desc.comments.length>0){
								comment = vv.firstLineOf(desc.comments[0].text);
							}
							var label = desc.revno+":"+a[i].slice(0, 10)+" - "+userHash[desc.audits[0].userid]+" - "+comment;
							label = decorateHitOrMiss(label, request.repo.changeset_history_contains(branchHead, a[i]));
							results.push({label: label, value: desc.revno+":"+a[i]});
						}
					}
				}
				
				if(!searchType){
					if(term.length>1){
						var zdb = new zingdb(request.repo, sg.dagnum.VC_COMMENTS);
						var a = zdb.query__fts("item", "text", term+(finalSpace?"":"*"), ['*'], 8, 0);
						if(a.length>0){
							userHash = userHash || getUserHash(request.repo);
							for(var i=0; i<a.length; ++i) try{
								var desc = request.repo.get_changeset_description(a[i].csid);
								var label = desc.revno+":"+a[i].csid.slice(0, 10)+" - "+userHash[desc.audits[0].userid]+" - "+a[i].text;
								label = decorateHitOrMiss(label, request.repo.changeset_history_contains(branchHead, a[i].csid));
								results.push({label: label, value: desc.revno+":"+a[i].csid});
							}
							catch(ex){
								// The comment that query__fts() matched was on a changeset that
								// is not present in the repository. Just ignore it.
							}
						}
					}
				}
				return jsonResponse(results);
			}
		}
	},
	"/repos/<repoName>/rev.json": {
		"GET": {
			onDispatch: function(request) {
				var q = vv.getQueryParam("q", request.queryString).trim();
				
				if(vv.startswith(q, "branch:")){
					var branchName = q.slice(4);
					if(branchName.indexOf("(")>-1){
						branchName = branchName.slice(0, branchName.indexOf("("));
					}
					branchName = branchName.trim();
					
					var branchQueryResult = new zingdb(request.repo, sg.dagnum.VC_BRANCHES).query("branch", ["csid"], vv.where({name: branchName}));
					var y=[];
					for(var iHead=0; iHead<branchQueryResult.length; ++iHead){
						var headCsid = branchQueryResult[0].csid;
						var headRevno = request.repo.csid_to_revno(headCsid) || "?";
						y.push({csid: headCsid, revno: headRevno});
					}
					return jsonResponse(y);
				}
				
				if(vv.startswith(q, "tag:")){
					var tagName = q.slice(4);
					if(tagName.indexOf("(")>-1){
						tagName = tagName.slice(0, tagName.indexOf("("));
					}
					tagName = tagName.trim();
					
					var tagQueryResult = new zingdb(request.repo, sg.dagnum.VC_TAGS).query("item", ["csid"], vv.where({tag: tagName}));
					if(tagQueryResult.length==1){
						var tagCsid = tagQueryResult[0].csid;
						var tagRevno = request.repo.csid_to_revno(tagCsid) || "?";
						return jsonResponse([{csid: tagCsid, revno: tagRevno}]);
					}
					else{
						return jsonResponse([]);
					}
				}
				
				var revno = parseInt(q, 10);
				if(revno.toString()===q && revno>0 && request.repo.revno_to_csid(revno)){
					return jsonResponse([{csid: request.repo.revno_to_csid(revno), revno: revno}]);
				}
				
				var csids = false;
				try{
					csids = request.repo.vc_lookup_hids(q);
				}
				catch(ex){
				}
				if(csids && csids.length==1){
					var revno = request.repo.csid_to_revno(csids[0]);
					if(revno){
						return jsonResponse([{csid: csids[0], revno: revno}]);
					}
				}
				
				var iColon = q.indexOf(":");
				if(iColon>-1){
					var beforeColon = q.slice(0,iColon);
					var afterColon = q.slice(iColon+1);
					
					var revno = parseInt(beforeColon, 10);
					if(revno.toString()===beforeColon && revno>0){
						var csid = request.repo.revno_to_csid(revno);
						if(csid && vv.startswithi(csid, afterColon)){
							return jsonResponse([{csid: csid, revno: revno}]);
						}
					}
				}
				
				var y=[];
				
				var branchQueryResult = new zingdb(request.repo, sg.dagnum.VC_BRANCHES).query("branch", ["csid"], vv.where({name: q}));
				for(var iHead=0; iHead<branchQueryResult.length; ++iHead){
					var headCsid = branchQueryResult[iHead].csid;
					var headRevno = request.repo.csid_to_revno(headCsid) || "?";
					y.push({csid: headCsid, revno: headRevno});
				}
				
				var tagQueryResult = new zingdb(request.repo, sg.dagnum.VC_TAGS).query("item", ["csid"], vv.where({tag: q}));
				if(tagQueryResult.length==1){
					var tagCsid = tagQueryResult[0].csid;
					var found=false;
					for(var i=0; i<branchQueryResult.length; ++i){
						if(y[i].csid == tagCsid){
							found=true;
							break;
						}
					}
					
					if(!found){
						var tagRevno = request.repo.csid_to_revno(tagCsid) || "?";
						return jsonResponse([{csid: tagCsid, revno: tagRevno}]);
					}
				}
				
				return jsonResponse(y);
			}
		}
	},
	"/repos/<repoName>/changesets/<csid>/dagquery/whats-new.json": {
		"GET": {
			onDispatch: function(request){
				var other = vv.getQueryParam("other", request.queryString);
				return jsonResponse({
					changesets: request.repo.merge_preview(request.csid, other, false),
					isDirectDescendant: request.repo.changeset_history_contains(other, request.csid)
				});
			}
		}
	},
	"/repos/<repoName>/changesets/<csid>/comments.json": {
		"GET": {
			onDispatch: function (request)
			{
				var comments = request.repo.fetch_vc_comments(request.csid);
				return jsonResponse(comments);
			}
		},
		"POST": {
			requireUser: true,
			onJsonReceived: function (request, data)
			{
				var audit = request.repo.create_audit(request.vvuser);
				try{
					request.repo.add_vc_comment(request.csid, data.text, audit);
					var comments = request.repo.fetch_vc_comments(request.csid);
					return jsonResponse(comments);
				}
				finally{
					audit.fin();
				}
			}
		}
	},
    "/repos/<repoName>/browsecs.html": {
        "GET": {
            onDispatch: function (request)
            {
                var vvt = new vvUiTemplateResponse(
							    request, "core", "Browse Changeset"
						    );

                return (vvt);
            }
        }
    },
	"/repos/<repoName>/history.html": {
		"GET": {
			onDispatch: function (request)
			{
				var vvt = new vvUiTemplateResponse(
				request, "core", "History"
			);

				return (vvt);
			}
		}
	},
	"/repos/<repoName>/tag/<tagname>.html": {
		"GET": {
			onDispatch: function(request)
			{
				var vvt = new vvUiTemplateResponse(request, "core", "Tag", 
					function( key, req ) {
						if (key == "VVTAGNAME")
							return request.tagname;
						else if (key == "VVTAGCSID")
						{
							return request.repo.lookup_vc_tag(request.tagname);
						}	
						else if (key == "TITLE")
						{
							return "Tag: " + request.tagname;
						}
					},
					"tag.html"
				);

				return vvt;
			}
		}
	},

	"/repos/<repoName>/tags.json": {
		"GET": {
			onDispatch: function (request)
			{
				var tags = request.repo.fetch_vc_tags();
				return jsonResponse(tags);
			}
		}

	},

	"/repos/<repoName>/changesets/<csid>/tags.json": {
		"GET": {
			onDispatch: function (request)
			{
				var tags = request.repo.fetch_vc_tags(request.csid);
				return jsonResponse(tags);
			}
		},

		"POST": {
			requireUser: true,
			onJsonReceived: function (request, data)
			{
				var zs = new zingdb(request.repo, sg.dagnum.VC_TAGS);
				var existingCSIDs = zs.query("item", ["csid"], vv.where({ "tag": data.text }));
				//should only be one if any
				if (existingCSIDs && existingCSIDs.length)
				{
					//if the tag is on the existing changeset return that changeset
					if (existingCSIDs[0].csid != request.csid)
					{
						var csdetails = request.repo.get_changeset_description(request.csid, true);
						return jsonResponse({ description: csdetails });
					}
				}
				else
				{
					var audit = request.repo.create_audit(request.vvuser);
					try{
						request.repo.add_vc_tag(request.csid, data.text, audit);
					}
					finally{
						audit.fin();
					}
				}

				var tags = request.repo.fetch_vc_tags(request.csid);

				return jsonResponse(tags);
			}
		}
	},
	"/repos/<repoName>/changesets/<csid>/tags/<tag>.$":
	{
		"DELETE": {
			requireUser: true,
			onDispatch: function (request)
			{
				var audit = request.repo.create_audit(request.vvuser);
				try{
					request.repo.delete_vc_tag(request.csid, request.tag, audit);
					var tags = request.repo.fetch_vc_tags(request.csid);
					return jsonResponse(tags);
				}
				finally{
					audit.fin();
				}
			}
		}

	},
	"/repos/<repoName>/stamps.json": {
		"GET": {
			onDispatch: function (request)
			{
				var stamps = request.repo.fetch_vc_stamps();
				return jsonResponse(stamps);
			}
		}

	},

	"/repos/<repoName>/changesets/<csid>/stamps/<stamp>.$":
	{
		"DELETE": {
			requireUser: true,
			onDispatch: function (request)
			{
				var audit = request.repo.create_audit(request.vvuser);
				try{
					request.repo.delete_vc_stamp(request.csid, request.stamp, audit);
					var stamps = request.repo.fetch_vc_stamps(request.csid);
					return jsonResponse(stamps);
				}
				finally{
					audit.fin();
				}
			}
		}

	},
	"/repos/<repoName>/changesets/<csid>/stamps.json": {
		"GET": {
			onDispatch: function (request)
			{
				var stamps = request.repo.fetch_vc_stamps(request.csid);
				return jsonResponse(stamps);
			}
		},
		"POST": {
			requireUser: true,
			onJsonReceived: function (request, data)
			{
				var audit = request.repo.create_audit(request.vvuser);
				try{
					request.repo.add_vc_stamp(request.csid, data.text, audit);
					var stamps = request.repo.fetch_vc_stamps(request.csid);
					return jsonResponse(stamps);
				}
				finally{
					audit.fin();
				}
			}
		}
	},
	"/repos/<repoName>/stamps.json": {
		"GET": {
			onDispatch: function (request)
			{
				var stamps = request.repo.fetch_vc_stamps();
				return jsonResponse(stamps);
			}
		}

	},
	"/repos/<repoName>/diff.json": {
		"GET":
			{
				onDispatch: function (request)
				{
					if (!vv.queryStringHas(request.queryString, "hid1") || !vv.queryStringHas(request.queryString, "hid2"))
					{
						return (badRequestResponse());
					}
					var hid1 = vv.getQueryParam("hid1", request.queryString);
					var hid2 = vv.getQueryParam("hid2", request.queryString);
					var filename = "file";
					if (vv.queryStringHas(request.queryString, "file"))
					{
						filename = vv.getQueryParam("file", request.queryString);
					}

					var diff = request.repo.diff_file(hid1, hid2, filename);
					return textResponse(diff);
				}
			}
	},
	"/repos/<repoName>/diff_local.json": {
		"GET":
			{
				onDispatch: function (request)
				{
					if (!vv.queryStringHas(request.queryString, "hid1") || !vv.queryStringHas(request.queryString, "path"))
					{
						return (badRequestResponse());
					}
					var hid1 = vv.getQueryParam("hid1", request.queryString);
					var path = vv.getQueryParam("path", request.queryString);
					path = path.rtrim("/");

					var diff = request.repo.diff_file_local(hid1, path);
					return textResponse(diff);
				}
			}
	},
	"/repos/<repoName>/zip/<csid>.zip": {
		"GET": {
			onDispatch: function (request)
			{
				var tmp = sg.fs.tmpdir();
				var path = tmp + "/" + request.csid + ".zip";
				sg.zip(request.repo, request.csid, path);
				return fileResponse(path, true, null, true, request.csid + ".zip");
			}
		}
	},
	"/repos/<repoName>/branches.json": {
		"GET": {
			onDispatch: function (request)
			{
				var branches = request.repo.named_branches();
				return jsonResponse(branches);
			}
		}

	},

	"/repos/<repoName>/branch-names.json": {
		"GET": {
			onDispatch: function (request)
			{
				var branches = get_branches(request.repo, (vv.queryStringHas(request.queryString, "all") ? true : false));

				var response = jsonResponse(branches);
				return response;
			}
		}

	},

	"/repos/<repoName>/changesets/<recid>/browsecs.json": {
		"GET": {
			onDispatch: function (request)
			{
				var gid = vv.getQueryParam("gid", request.queryString);
				var path = vv.getQueryParam("path", request.queryString);
				var tne = null;

				if (gid)
				{
					tne = request.repo.get_treenode_info_by_gid(request.recid, gid);
				}
				else
				{
					if (!path)
					{
						path = "@";
					}
					path = path.rtrim("/");
					tne = request.repo.get_treenode_info_by_path(request.recid, path);
				}
				if (tne)
				{
					return jsonResponse(tne);
				}
				return (badRequestResponse());
			}
		}

    },
    "/repos/<repoName>/changesets/<recid>.json": {
        "GET": {
            onDispatch: function (request)
            {
                var recid = request.recid;

                try
                {
                    recid = request.repo.lookup_hid(request.recid);
                    if (!recid || !recid.length)
                    {
                        return (notFoundResponse());
                    }
                }
                catch (e)
                {                   
                    return (notFoundResponse());
                }
                var desc = request.repo.get_changeset_description(recid, true);
                if (!desc || !desc.changeset_id)
                {
                    return (notFoundResponse());
                }

				var changeset = {};
				for (var i = 0; i < desc.audits.length; i++)
				{
					var un = vv.lookupUser(desc.audits[i].userid, request.repo);

					if (un)
						desc.audits[i].username = un;
				}

                changeset.description = desc;
                if (vv.queryStringHas(request.queryString, "details"))
                {
                    vv.each(desc.parents, function (i, v)
                    {
						// For WC, use new canonical STATUS
                        // var diff = request.repo.diff_changesets(i, recid);
						var diff = sg.vv2.status({"repo":request.repoName, "revs":[{"rev":i}, {"rev":recid}]});

						changeset[i] = diff;

						var baselineSpec = {};
						baselineSpec[desc.revno.toString()] = v.revno;
						changeset.description.parents[i]["mergedInCount"] = request.repo.merge_review(recid, true, 0, false, baselineSpec).length-3;
					});

					var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);

					changeset.description.associations = db.query(
						'vc_changeset',
						[
							{ 'type' : 'from_me', 'ref': 'item', 'alias': 'id', 'field': 'id' },
							{ 'type' : 'from_me', 'ref': 'item', 'alias': 'status', 'field': 'status' },
							{ 'type' : 'from_me', 'ref': 'item', 'alias': 'title', 'field': 'title' },
							{ 'type' : 'from_me', 'ref': 'item', 'alias': 'recid', 'field': 'recid' }
						],
						vv.where({ 'commit': [recid, request.repo.repo_id + ":" + recid] })
					);

					changeset.description.containingBranches = listOpenBranchesWithChangeset(recid, request.repo, null, true);
				}

				return jsonResponse(changeset);
			}
		}

	},

	"/repos/<repoName>/changeset/<recid>/<linkname>.json": {
		"GET": {
			onDispatch: function (request)
			{
				var wir = new witRequest(request, vclinkFields);

				return (wir.retrieveLink('vc_changeset'));
			}
		},
        "PUT": {
			requireUser: true,
			onJsonReceived: function (request, data)
			{
				var wir = new witRequest(request, vclinkFields);

				return (wir.updateLinks('changeset', data));

			}
		}
	},
    
	"/repos/<repoName>/changeset.html": {
		"GET": {
			onDispatch: function (request)
			{
				var vvt = new vvUiTemplateResponse(request, "core", "Changeset" );

				return (vvt);
			}
		}
	},
	"/repos/<repoName>/browsecs.html": {
		"GET": {
			onDispatch: function (request)
			{
				var vvt = new vvUiTemplateResponse(
							request, "core", "Browse Changeset"
						);

				return (vvt);
			}
		}
	}

});
