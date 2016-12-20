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


var svg;
var lastHeight = 0;
var svgGroup;
var BLUE_FILL = 'rgb(230,248,251)';
var YELLOW_FILL = 'rgb(255, 255, 234)';
var GREEN_FILL = 'rgb(228, 240, 206)';
var RED_FILL = 'rgb(230, 100, 100)';
var GREEN_STROKE = 'rgb(99, 132, 00)';
var YELLOW_STROKE = 'rgb(209, 169, 46)';
var BLUE_STROKE = 'rgb(0, 88, 104)';
var RED_STROKE = 'rgb(200, 20, 20)';

var DASHARRAY = '5,5';

function graphNode(dag, div, index, branch)
{
	this.Dag = dag || null;
	this.Div = div || null;
	this.Index = index || 0;
    
	this.Bottom = 0;
	this.Radius = 5;
	this.Left = 0;
	this.Top = 0;
	this.ColumnID = 0;
	this.Children = [];
	this.Parents = [];
	this.Lines = [];
	this.SvgParentLineFamily = [];
	this.SvgChildLineFamily = [];
	this.SvgPseudoParentLineFamily = [];
	this.SvgPseudoChildLineFamily = [];
	this.Circle = null;	
	this.positionCalculated = false;
	this.branchname = branch;
	this.fillColor = BLUE_FILL;
	this.strokeColor = BLUE_STROKE;
	this.strokeDashArray = DASHARRAY;
	
	this.reset = function ()
	{
		this.Bottom = 0;
		this.Top = 0;
		this.ColumnID = 0;
		this.Left = 0;
		this.Children = [];
		this.Parents = [];
		this.SvgParentCircleFamily = [];
		this.SvgChildCircleFamily = [];
		this.SvgParentLineFamily = [];
		this.SvgChildLineFamily = [];	
		this.SvgPseudoParentLineFamily = [];
		this.SvgPseudoChildLineFamily = [];			
		this.Circle = null;
		this.positionCalculated = false;

	};
	this.calcPosition = function ()
	{
	    if (this.positionCalculated)
	        return;

	    // TODO this is a bit of a hack.  The node needs to know
	    // its parent's parent's top position (the table)
	    var parentTop = this.Div.parent().parent().position().top;
	    var height = this.Div.height();
	    var top = this.Div.position().top;

	    this.Top = top + (height + this.Radius) / 2 - parentTop;
	    this.Bottom = this.Top + this.Radius;
	    this.positionCalculated = true;
	};

}

function historyGraph(tableId, colSpacer)
{
    tableId = tableId || "tablehistory";
    graphDivId = tableId+"_graph";

    var FLAG_PARENT_HIGHLIGHT = 1;
    var FLAG_CHILD_HIGHLIGHT = 2;

    var currentLeaves = [];
    var graphNodes = {};
	var csids = {}; // revno-to-csid map
    var indexedNodes = [];
  
    var maxCols = 0;
    var columns;
    var hItems = [];
    var columnNum = 0;
    var lastNode = {};
    colSpacer = colSpacer || 20;
    var filtered = false;
    var branchNodes = [];
    var visitedParentHighlights = {};
    var visitedChildHighlights = {};
    var lineMap = {};

    var allRotateButtons = [];
    var _theta = false;
    var _r1, _r1x1, _r1y1, _r1x2, _r1y2;
    var _r2,               _r2x2, _r2y2;
    var _r3, _r3x1, _r3y1, _r3x2, _r3y2;
    var _arrowRightDX1, _arrowRightDY1, _arrowRightDX2, _arrowRightDY2;

    var currentHighlight = null;
    //Todo this is probably overkill now that
    //we don't sort
    function graphColumn(id)
    {
        this.Id = id;
        this.NodeCount = 0;
        this.LeftOffset = (this.Id * colSpacer) + (colSpacer/2);
    }

    function getColumn(id)
    {
        for (var i = 0; i < columns.length; i++)
        {
            if (columns[i].Id == id)
            {
                return columns[i];
            }
        }
    }
    function getBestLine(node, pnode, checkval, nodeLeft, pnodeLeft)
    {
        var column = getColumn(checkval);
        var hasCollision = false;
        var usedLines = [];
      
        for (var i = node.Index; i < pnode.Index; i++)
        {
            var cgn = indexedNodes[i];
            var val = getColumn(cgn.ColumnID).LeftOffset;

            //check for nodes on same line       
            if (i > node.Index)
            {
                if ($.inArray(val, usedLines) < 0)
                    usedLines.push(val);
                if (cgn.ColumnID == checkval)
                {
                    hasCollision = true;
                }
            }
            //check for lines on same line
            if (cgn.Lines)
            {
                $.each(cgn.Lines, function (i, v)
                {
                    if ($.inArray(v, usedLines) < 0)
                        usedLines.push(v);
                });
            }

            if ($.inArray(column.LeftOffset, cgn.Lines) >= 0)
            {
                hasCollision = true;
            }

        }
       
        if (hasCollision)
        {
            usedLines.sort(function (a, b) { return a - b; });
            var firstTry = column.LeftOffset + 10;
            while ($.inArray(firstTry, usedLines) >= 0)
                firstTry += 10;
          
            return { change: true, line: Math.abs(nodeLeft - firstTry) };
        }

        return { change: false, line: Math.abs(nodeLeft - pnodeLeft) };

    }

    function getBestColumn(node, pnode)
    {
        var maxColumn = -1;
        var column = -1;
        var clear = true;
        var columnsInUse = [];

      
        for (var i = node.Index + 1; i < pnode.Index; i++)
        {
            var cgn = indexedNodes[i];
            columnsInUse.push(cgn.ColumnID);
            maxColumn = Math.max(maxColumn, cgn.ColumnID);
        }

        $.each(currentLeaves, function (index, pid)
        {
            var cgn = graphNodes[pid];
            columnsInUse.push(cgn.ColumnID);
            maxColumn = Math.max(maxColumn, cgn.ColumnID);
        });

        for (var j = pnode.ColumnID; j < maxColumn; j++)
        {
            if ($.inArray(j, columnsInUse) < 0)
            {
                column = j;
                break;
            }
        }

        if (column < 0)
        {
            column = maxColumn + 1;
        }

        return column;
    }

    function positionNode(cgn)
    {
		if(cgn.Dag.indent!==undefined){
			cgn.ColumnID = cgn.Dag.indent;
			maxCols = Math.max(maxCols, cgn.ColumnID);
			if(!getColumn(cgn.ColumnID)){
				columns.push(new graphColumn(cgn.ColumnID));
			}
			
			for (pid in cgn.Dag.parents){
				var pnode = graphNodes[pid];
				if(pnode){
					cgn.Parents.push(pnode);
					pnode.Children.push(cgn);
				}
			}
			return;
		}

        var minLeafColumn = maxCols;
        var minAvailParent = 100;
        var parentsInArray = [];

        var countParentsFetched = 0;

		var parentsArray = cgn.Dag.parents;
		if (cgn.Dag.pseudo_parents != null)
		{
			parentsArray = cgn.Dag.pseudo_parents;
		}
        for (pid in parentsArray)
        {
            var pnode = graphNodes[pid];
            if (pnode)
            {
                cgn.Parents.push(pnode);
                pnode.Children.push(cgn);
                countParentsFetched++;
                if (currentLeaves.length > 0)
                {
                    var idx = $.inArray(pid, currentLeaves);
                    if (idx >= 0)
                    {
                        currentLeaves.splice(idx, 1);
                        var curColumn = pnode.ColumnID;
                        minLeafColumn = Math.min(minLeafColumn, curColumn);
                        parentsInArray.push(pnode);
                    }
                    else
                    {
                        var c = getBestColumn(cgn, pnode);
                        columnNum = c;
                    }
                }
            }
            else
            {
                //if filtered by user or stamp or path don't
                //change the column or the graph could get crazy huge
                if (!(filterflags & FLAG_NAME_FILTER) && !(filterflags & FLAG_STAMP_FILTER) && !(filterflags & FLAG_PATH_FILTER))
                {
                    var c = getBestColumn(cgn, lastNode);
                    columnNum = c;
                }
            }

        }

        if (parentsInArray.length > 0)
        {
            if (parentsInArray.length == parentsArray.length || countParentsFetched == 1)
            {
                columnNum = cgn.ColumnID = minLeafColumn;
            }
            else
            {
                columnNum = cgn.ColumnID = Math.min(columnNum, minLeafColumn);
            }
        }
        else
        {
            cgn.ColumnID = columnNum;
        }
        maxCols = Math.max(maxCols, cgn.ColumnID);

        var column = getColumn(cgn.ColumnID);
        if (!column)
        {
            column = new graphColumn(cgn.ColumnID);
            columns.push(column);
        }

        //TODO we don't sort now, so this probably isn't needed
        //column.NodeCount++;
        currentLeaves.push(cgn.Dag.changeset_id);
    }

    function calculatePositions()
    {
        columns = [];
        columns.length = 0;
        columnNum = 0;
        currentLeaves = [];

        for (var i = indexedNodes.length - 1; i >= 0; i--)
        {
            var cgn = indexedNodes[i];

            if (i == indexedNodes.length - 1)
            {
                lastNode = cgn;
            }

            if (cgn)
            {
                cgn.reset();

                positionNode(cgn);
            }
        }
    }

    function formatFamilyBlockFromServer(node, div, childrenOnly)
    {
		if(node.formattingFamilyBlockFromServer){
			return;
		}
		node.formattingFamilyBlockFromServer = true;

		var forceMergePreviewChildNode = (node.Children && node.Children.length==1 && node.Children[0].Dag.changeset_id=="(merge-preview)");

        var url = sgCurrentRepoUrl + "/changesets/" + node.Dag.changeset_id + ".json";
        div.addClass('smallProgress').css({ "height": 200, "width": 200 });

        vCore.ajax(
			{
			    url: url,
			    dataType: 'json',

			    error: function (xhr, textStatus, errorThrown)
			    {
			        div.removeClass('spinning');
			    },
			    success: function (data)
			    {
			        div.removeClass('smallProgress').css({ "height": "", "width": "" });
			        if (data)
			        {
			            if (data.description.children.length > 0 || forceMergePreviewChildNode)
			            {
			                div.append("<div class='innerheading'>Children</div>");
			                var plist = $("<div class='innercontainer'></div>").appendTo(div);

							if(forceMergePreviewChildNode){
								var p = $('<p>').addClass("commentlist");
								p.append("(merge-preview)").append($('<br/>'));
								plist.append(p);
							}

			                $.each(data.description.children, function (i, child)
			                {
			                    var pChild = $('<p>').addClass("commentlist");
			                    pChild.append("<b>" + child.revno + ":" + child.changeset_id.slice(0, 10) + "</b>").append($('<br/>'));
			                    setTimeStamps(child.audits, pChild);
			                    pChild.append($('<br/>'));
			                    if (child.comments.length > 0)
			                    {
			                        var vc = new vvComment(child.comments[0].text);
			                        pChild.append(vc.summary);
			                    }
			                    plist.append(pChild);
			                });
			            }
			            if (!childrenOnly)
			            {
			                div.append($("<div class='innerheading'>Parents</div>"));

							var plist = $("<div class='innercontainer'></div>").
								appendTo(div);

			                $.each(data.description.parents, function (i, parent)
			                {
			                    var pParent = $('<p>').addClass("commentlist");
			                    pParent.append("<b>" + parent.revno + ":" + i.slice(0, 10) + "</b>").append($('<br/>'));

			                    setTimeStamps(parent.audits, pParent);
			                    pParent.append($('<br/>'));
			                    if (parent.comments.length > 0)
			                    {
			                        var vc = new vvComment(parent.comments[0].text);
			                        pParent.append(vc.summary);
			                    }
			                    plist.append(pParent);
			                });
			            }
			        }

			    }
			});
    }

    function addFamilyPopupInfo(nodes, div, title)
    {
		$("<div class='innerheading'></div>").
			text(title).
			appendTo(div);

		var list = $("<div class='innercontainer'></div>").
			appendTo(div);

        $.each(nodes, function (i, v)
        {
			var p = $('<p>').addClass("commentlist");
			if(v.Dag.changeset_id == "(merge-preview)"){
				p.append("(merge-preview)").append($('<br/>'));
			}
			else{
				var c = graphNodes[v.Dag.changeset_id];
				if (c && c.Dag.audits)
				{
					p.append("<b>" + c.Dag.revno + ":" + c.Dag.changeset_id.slice(0, 10) + "</b>").append($('<br/>'));
					setTimeStamps(c.Dag.audits, p);
					p.append($('<br/>'));
					if (c.Dag.comments.length > 0)
					{
						var vc = new vvComment(c.Dag.comments[0].text);
						p.append(vc.summary);
					}
				}
			}
			list.append(p);
        });
    }


    function addMouseEvents(circle, node)
    {
		// Add the node rotate button, if needed.
		var dp = null;
		if(node.Dag.displayParent!==undefined && node.Dag.displayParent!=1){
			dp = graphNodes[csids[node.Dag.displayParent]];
		}
		if(dp && node.Dag.indent!=dp.Dag.indent){
			var theta;
			var r1, r1x1, r1y1, r1x2, r1y2;
			var r2,             r2x2, r2y2;
			var r3, r3x1, r3y1, r3x2, r3y2;
			if(_theta===false || node.Dag.Index!=dp.Dag.Index+1){
				var hyp = Math.sqrt( (node.Left-dp.Left)*(node.Left-dp.Left) + (node.Top-dp.Top)*(node.Top-dp.Top) );
				var adj = (node.Top-dp.Top);
				theta = Math.acos(adj/hyp) - 0.05;
				
				r1 = 5;
				r1x1 = r1*Math.sin(      - 0.2);
				r1y1 = r1*Math.cos(      - 0.2);
				r1x2 = r1*Math.sin(theta + 0.2);
				r1y2 = r1*Math.cos(theta + 0.2);
				
				r2 = hyp - 11;
				r2x2 = r2*Math.sin(theta);
				r2y2 = r2*Math.cos(theta);
				
				r3 = hyp - 6;
				r3x1 = r3*Math.sin(      - 0.2);
				r3y1 = r3*Math.cos(      - 0.2);
				r3x2 = r3*Math.sin(theta + 0.2);
				r3y2 = r3*Math.cos(theta + 0.2);
				
				arrowRightDX1 = 4.0 * Math.cos( Math.PI/6-theta);
				arrowRightDY1 = 4.0 * Math.sin( Math.PI/6-theta);
				arrowRightDX2 = 4.0 * Math.cos(-Math.PI/6-theta);
				arrowRightDY2 = 4.0 * Math.sin(-Math.PI/6-theta);
				
				if(node.Dag.Index==dp.Dag.Index+1){
					_theta = theta;
					_r1=r1; _r1x1=r1x1; _r1y1=r1y1; _r1x2=r1x2; _r1y2=r1y2;
					_r2=r2;                         _r2x2=r2x2; _r2y2=r2y2;
					_r3=r3; _r3x1=r3x1; _r3y1=r3y1; _r3x2=r3x2; _r3y2=r3y2;
					_arrowRightDX1 = arrowRightDX1;
					_arrowRightDY1 = arrowRightDY1;
					_arrowRightDX2 = arrowRightDX2;
					_arrowRightDY2 = arrowRightDY2;
				}
			}
			else{
				theta = _theta;
				r1=_r1; r1x1=_r1x1; r1y1=_r1y1; r1x2=_r1x2; r1y2=_r1y2;
				r2=_r2;                         r2x2=_r2x2; r2y2=_r2y2;
				r3=_r3; r3x1=_r3x1; r3y1=_r3y1; r3x2=_r3x2; r3y2=_r3y2;
				arrowRightDX1 = _arrowRightDX1;
				arrowRightDY1 = _arrowRightDY1;
				arrowRightDX2 = _arrowRightDX2;
				arrowRightDY2 = _arrowRightDY2;
			}
			
			if(dp.Dag.indent<node.Dag.indent){
				var rb = svg.group(svgGroup);
				var clickeroo = svg.path(rb,
					svg.createPath().move(dp.Left+r3x2, dp.Top+r3y2)
					                .arc(r3, r3, 0, false, true,  dp.Left+r3x1, dp.Top+r3y1)
					                .line(dp.Left+r1x1, dp.Top+r1y1)
					                .arc(r1, r1, 0, false, false, dp.Left+r1x2, dp.Top+r1y2)
					                .close()
					                .path(),
					{strokeWidth: 1, stroke: "none", "stroke-opacity": 0, fill: RED_FILL, "fill-opacity": 0});
				var arrow = svg.path(rb,
					svg.createPath().move(dp.Left+r2x2, dp.Top+r2y2)
					                .line(dp.Left+r2x2-arrowRightDX1, dp.Top+r2y2-arrowRightDY1)
					                .move(dp.Left+r2x2, dp.Top+r2y2)
					                .line(dp.Left+r2x2-arrowRightDX2, dp.Top+r2y2-arrowRightDY2)
					                .move(dp.Left+r2x2, dp.Top+r2y2)
					                .arc(r2, r2, 0, false, true, dp.Left, dp.Top+r2)
					                .line(dp.Left+3.4, dp.Top+r2-2)
					                .move(dp.Left, dp.Top+r2)
					                .line(dp.Left+3.4, dp.Top+r2+2)
					                .path(),
					{strokeWidth: 1, stroke: RED_STROKE, "stroke-opacity": 0.7, fill: "none"});
				//rb.parentNode.insertBefore(rb, rb.parentNode.firstChild);
				rb.onmouseover = function (e){
					svg.change(clickeroo, {stroke: RED_STROKE, "fill-opacity": 0.2});
					svg.change(arrow, {stroke: RED_STROKE, "stroke-opacity": 1});
				};
				rb.onmouseout = function (e){
					svg.change(clickeroo, {stroke: "none", "fill-opacity": 0});
					svg.change(arrow, {stroke: RED_STROKE, "stroke-opacity": 0.7});
				};
				rb.onclick = function (e){
					for(var iRb=0; iRb<allRotateButtons.length; ++iRb){
						allRotateButtons[iRb].rb.onmouseover = null;
						allRotateButtons[iRb].rb.onmouseout = null;
						allRotateButtons[iRb].rb.onclick = null;
					}
					nodeRotateFunctions[tableId](dp.Dag.Index, node.Dag.revno);
				};
				allRotateButtons.push({rb: rb, Index: node.Index});
			}
		}

        circle.onmouseover = function (e)
        {
			svg.change(circle, { r: node.Radius + 2 });
			highlightCloseFamilyNodes(node, YELLOW_FILL, YELLOW_STROKE, false, FLAG_CHILD_HIGHLIGHT | FLAG_PARENT_HIGHLIGHT);

			if (! node.pop)
			{
				var gotKids = false;

				var divParents = $('<div>');
				var divChildren = $('<div>');
				var div = $('<div />');
				node.pop = div;

				if (node.branchname && node.branchname.length)
				{
					$("<div class='innerheading'>Branch</div>").
						appendTo(div);

					var bl = $('<div class="innercontainer"></div>').
						appendTo(div);
					
					bl.append(document.createTextNode(node.branchname[0]));
					
					for ( var i = 1; i < node.branchname.length; ++i )
						bl.append(", ").append(document.createTextNode(node.branchname[i]));
				}
				var parents = [];
				$.each(node.Dag.parents, function (i, p)
					   {
						   parents.push(p);
					   });
				//alert("Parents length: " + parents.length);
				
				function arraysAreEqual(arr1, arr2)
				{
					if (arr1 == null && arr2 == null)
						return true;
					else if (arr1 == null || arr2 == null)
						return false;
					else if (arr1.length != arr2.length)
						return false;
					else //Both are not null.  They are the same length.
					{
						for (var indx in arr1)
						{
							if (arr2[indx] == null || arr1[indx] != arr2[indx]) //an item in arr1 is not in arr2
								return false;
						}
					}
					
					return true;
				}
				
				if (parents.length > 0)
				{
					if (arraysAreEqual(node.Parents, parents) || node.Dag.changeset_id=="(merge-preview)")
					{
						addFamilyPopupInfo(node.Parents, divParents, "Parents");
					}
					else
					{
						formatFamilyBlockFromServer(node, div, false);
						//if we have to get any of the parents from the server, we will have
						//the child info so set this so we don't hit the server twice
						gotKids = true;
					}
				}

				if (!gotKids)
				{
					if ((filterflags & FLAG_NAME_FILTER) || (filterflags & FLAG_STAMP_FILTER) || (filterflags & FLAG_TO_FILTER))
					{
						//if the history results have been filtered by name, stamp or date to
						//we need to get the changeset info to make sure all the kids are accounted for
						formatFamilyBlockFromServer(node, divChildren, true);
					}
					//We will always have all the kids that exist if the query is not filtered by name, stamp or date to
					else if (node.Children.length > 0)
					{
						addFamilyPopupInfo(node.Children, divChildren, "Children");
					}
				}
				div.append(divChildren);
				div.append(divParents);

				var t= $(circle);
				var n = t.next();

				if (n.length && n[0].localName && n[0].localName == 'text')
					t = n;

				var width = Math.min(t.width(), 15) + 20;
				var offset = t.offset();
				var left = width + offset.left;
				var top = offset.top - $(window).scrollTop();

				node.pop.vvdialog(
					{
						"title": "",
						"dialogClass": "hoverdialog primary",
						"resizable": false,
						"width": "auto",
						"minHeight": 0,
						"maxWidth": 400,
						"width": 400,
						"position": [left, top],
						"autoOpen": false
					}
				);
			}
				
			node.pop.vvdialog("open");
		};
        circle.onmouseout = function (e)
        {
            highlightCloseFamilyNodes(node);
            if (currentHighlight == null || node.Dag.revno != currentHighlight.Dag.revno)
            {
                svg.change(circle, { r: node.Radius });
            }

			if (node.pop)
				node.pop.vvdialog("close");
        };
        circle.onclick = function (e)
        {
            visitedParentHighlights = {};
            visitedChildHighlights = {};
            if (currentHighlight)
            {
                svg.change(currentHighlight.Circle, { r: node.Radius });
                highlightAllParentNodes(currentHighlight, BLUE_FILL, BLUE_STROKE, true);
                highlightAllChildNodes(currentHighlight, BLUE_FILL, BLUE_STROKE, true);
                visitedParentHighlights = {};
                visitedChildHighlights = {};

                if (currentHighlight.Dag.revno == node.Dag.revno)
                {
                    currentHighlight = null;
                    return;
                }
            }

            currentHighlight = node;
            svg.change(circle, { r: node.Radius + 2 });
            highlightAllParentNodes(node, GREEN_FILL, GREEN_STROKE, true);
            highlightAllChildNodes(node, GREEN_FILL, GREEN_STROKE, true);
        };
    }

    this.addToGraph = function (dag, div, index, branchname)
    {
        var gn = new graphNode(dag, div, index, branchname);
        graphNodes[dag.changeset_id] = gn;
        csids[dag.revno] = dag.changeset_id;
        indexedNodes[index] = gn;
        if (branchname && branchname.length)
        {
            branchNodes.push(gn);
        }
    };

	this.connectPseudoChildren = function (new_parent_changeset_id, arrayOfPseudoChildren)
    {
		for (var child_changeset_id in arrayOfPseudoChildren)
		{
			childNode = graphNodes[child_changeset_id];
			//Make sure that the new child doesn't already have this entry as a real parent.
			if (childNode.Dag.parents != null && childNode.Dag.parents[new_parent_changeset_id] == null)
			{
				if (childNode.Dag.pseudo_parents == null)
					childNode.Dag.pseudo_parents = {};
				if (childNode.Dag.pseudo_parents[new_parent_changeset_id] == null)
					childNode.Dag.pseudo_parents[new_parent_changeset_id] = 0;
			}
		}
    };
	
    function highlightAllParentNodes(node, fillColor, strokeColor, stick)
    {
        highlightCloseFamilyNodes(node, fillColor, strokeColor, stick, FLAG_PARENT_HIGHLIGHT);
        visitedParentHighlights[node.Dag.revno] = node;
        $.each(node.Parents, function (i, n)
        {
            if (!visitedParentHighlights[n.Dag.revno])
            {
                highlightAllParentNodes(n, fillColor, strokeColor, stick);
            }
        });
    };

    function highlightAllChildNodes(node, fillColor, strokeColor, stick)
    {
        highlightCloseFamilyNodes(node, fillColor, strokeColor, stick, FLAG_CHILD_HIGHLIGHT);
        visitedChildHighlights[node.Dag.revno] = node;

        $.each(node.Children, function (i, n)
        {
            if (!visitedChildHighlights[n.Dag.revno])
            {
                highlightAllChildNodes(n, fillColor, strokeColor, stick);
            }
        });
    };


    function highlightCloseFamilyNodes(node, fillColor, strokeColor, stick, flgs)
    {
        var circle = node.Circle;

        var strWidth = 1;
        var flags = flgs || (FLAG_CHILD_HIGHLIGHT | FLAG_PARENT_HIGHLIGHT);


        if (node)
        {
            if (flags & FLAG_PARENT_HIGHLIGHT)
            {
                $.each(node.SvgParentCircleFamily, function (i, n)
                {
                    var cnode = graphNodes[n];
                    if (cnode)
                    {
                        if (stick)
                        {
                            cnode.fillColor = fillColor;
                            cnode.strokeColor = strokeColor;
                        }
                        svg.change(cnode.Circle, { fill: fillColor || cnode.fillColor, stroke: strokeColor || cnode.strokeColor });
                    }
                });

                $.each(node.SvgParentLineFamily, function (i, n)
                {
                    var line = lineMap[n];
                    if (line)
                    {
                        if (stick)
                        {
                            line.strokeColor = strokeColor;
                        }
                        svg.change(line.line, { fill: 'none', stroke: strokeColor || line.strokeColor, strokeWidth: strWidth, 'stroke-dasharray': 'None' });
                    }
                });
				
				$.each(node.SvgPseudoParentLineFamily, function (i, n)
                {
                    var line = lineMap[n];
                    if (line)
                    {
                        if (stick)
                        {
                            line.strokeColor = strokeColor;
                        }
                        svg.change(line.line, { fill: 'none', stroke: strokeColor || line.strokeColor, strokeWidth: strWidth, 'stroke-dasharray': node.strokeDashArray });
                    }
                });
            }
            if (flags & FLAG_CHILD_HIGHLIGHT)
            {
                $.each(node.SvgChildCircleFamily, function (i, n)
                {
                    var cnode = graphNodes[n];
                    if (cnode)
                    {
                        if (stick)
                        {
                            cnode.fillColor = fillColor;
                            cnode.strokeColor = strokeColor;
                        }
                        svg.change(cnode.Circle, { fill: fillColor || cnode.fillColor, stroke: strokeColor || cnode.strokeColor });
                    }
                });

                $.each(node.SvgChildLineFamily, function (i, n)
                {
                    var line = lineMap[n];
                    if (line)
                    {
                        if (stick)
                        {
                            line.strokeColor = strokeColor;
                        }
                        svg.change(line.line, { fill: 'none', stroke: strokeColor || line.strokeColor, strokeWidth: strWidth, 'stroke-dasharray': 'None' });
                    }
                });
				
				$.each(node.SvgPseudoChildLineFamily, function (i, n)
                {
                    var line = lineMap[n];
                    if (line)
                    {
                        if (stick)
                        {
                            line.strokeColor = strokeColor;
                        }
                        svg.change(line.line, { fill: 'none', stroke: strokeColor || line.strokeColor, strokeWidth: strWidth, 'stroke-dasharray': node.strokeDashArray });
                    }
                });
            }
        }

    };

	this.disconnectFromTo = function(firstRowIndex, lastRowIndex){
		// Remove graph edges.
		$.each(graphNodes, function (index, node){
			if((firstRowIndex <= node.Index) && (node.Index<=lastRowIndex)){
				var hitlist = [];
				$.each(node.SvgParentLineFamily, function (i, n){
					var line = lineMap[n];
					if (line){
						svg.remove(line.line);
						hitlist.push(i);
					}
				});
				for(var i=0; i<hitlist.length; ++i){
					delete lineMap[hitlist[i]];
				}
				
			}
		});
		
		// Remove rotate buttons.
		var i=0;
		while(i<allRotateButtons.length){
			if((firstRowIndex < allRotateButtons[i].Index) && (allRotateButtons[i].Index<=lastRowIndex)){
				svg.remove(allRotateButtons[i].rb);
				allRotateButtons.splice(i, 1);
			}
			else{
				++i;
			}
		}
	};

	this.deleteFromTo = function(firstRowIndex, lastRowIndex){
		// Remove graph nodes.
		var hitlist = [];
		$.each(graphNodes, function (index, node){
			if((firstRowIndex <= node.Index) && (node.Index<=lastRowIndex)){
				if(node.Circle){
					svg.remove(node.Circle);
					delete node.Circle;
				}
				hitlist.push(index);
			}
		});
		for(var i=0; i<hitlist.length; ++i){
			if (graphNodes[hitlist[i]].pop){
				graphNodes[hitlist[i]].pop.vvdialog("close");
				graphNodes[hitlist[i]].pop.remove();
			}
			delete graphNodes[hitlist[i]];
		}
	};

    this.clearGraph = function ()
    {
        if (svgGroup)
        {
            try
            {
                svg.remove(svgGroup);
            }
            catch (e)
            {

            }
        }
        indexedNodes = [];
      
        currentLeaves = [];
        branchNodes = [];

		for(var graphNodeId in graphNodes){
			if (graphNodes[graphNodeId].pop){
				graphNodes[graphNodeId].pop.vvdialog("close");
				graphNodes[graphNodeId].pop.remove();
			}
		}
        graphNodes = {};

        lineMap = {};
        maxCols = -1;
        columns = [];
        hItems = [];
        columnNum = 0;
        visitedParentHighlights = {};
        visitedChildHighlights = {};

        allRotateButtons = [];
    };

    this.update = function (s)
    {
        var self = this;
        _theta = false; // This causes us to redo the calculations for the node-rotate button graphics.

        for (var graphNodeId in graphNodes)
        {
            if (graphNodes[graphNodeId].pop)
            {
                graphNodes[graphNodeId].pop.vvdialog("close");
            }
        }
        //reset the node lines
        $.each(graphNodes, function (index, node)
        {
            node.Lines = [];
        });
        visitedParentHighlights = {};
        visitedChildHighlights = {};
        lineMap = {};
        if (!svg)
            svg = s;

        if (!svg)
            return;
        $('#' + graphDivId).show();
        maxCols = -1;
        if ($('.commentcell').html() == null)
            return;

        calculatePositions();

        var left = $('td.commentcell').first().position().left;
        var top = $('td.commentcell').first().position().top;

        var canvasWidth = (maxCols * colSpacer) + (2 * colSpacer) + 20;
        //make room on canvas for branch names
        $.each(branchNodes, function (b, bnode)
        {
            for (var ib = 0; ib < bnode.branchname.length; ib++)
            {
                var span = $("<span>").css("font-size", "10px").text("(" + bnode.branchname[ib] + ")"); //take into account branch close parens
                //when figuring name width

                $("#maincontent").append(span);
                var len = span.width();
                span.remove();

                var off = len + getColumn(bnode.ColumnID).LeftOffset + bnode.Radius;
                canvasWidth = Math.max(canvasWidth, off);
                $('#' + tableId + ' tr.rowIndex' + bnode.Index + ' td.graphdetails').css({ "padding-left": off + 40 });
            }

        });

        // Note: Resize column widths before calculating heights since the resizing can affect table and row heights.
        $('#' + tableId + ' .graphspace').css({ "width": canvasWidth });
        svg.change(svg.root(), { width: canvasWidth });

        var canvasHeight = $('#' + tableId).height() - $('#' + tableId + ' thead').height() + 2; // "+ 2" because of graphDiv's -2px top margin.
        svg.change(svg.root(), { height: canvasHeight });
        $(".graphHead").css("width", canvasWidth);

        if (svgGroup)
        {
            try
            {
                svg.remove(svgGroup);
            }
            catch (e)
            {

            }
        }

        svgGroup = svg.group();

        var gLines = svg.group(svgGroup, { fill: 'none', stroke: BLUE_STROKE, strokeWidth: 1 });
        var c;

        $.each(graphNodes, function (index, node)
        {
            node.calcPosition();
            var leftCol = getColumn(node.ColumnID);

            if (leftCol)
                left = leftCol.LeftOffset;

            c = svg.circle(svgGroup, left, node.Top, node.Radius, { id: "c" + node.Index, fill: BLUE_FILL, stroke: BLUE_STROKE, strokeWidth: 1 });

            node.Circle = c;
            node.Left = left;
            node.SvgParentCircleFamily.push(node.Dag.changeset_id);
            node.SvgChildCircleFamily.push(node.Dag.changeset_id);
            $.each(node.Children, function (i, v)
            {
                v.SvgParentCircleFamily.push(node.Dag.changeset_id);
            });

            if (node.branchname && node.branchname.length)
            {
                var topMult = 3;
                var startTop = (node.branchname.length > 1 ? (node.Top - (node.branchname.length / 2) * 10) : node.Top);
                $.each(node.branchname, function (b, bname)
                {
                    var realname = bname;
                    if (bname[0] == "(" && bname[bname.length - 1] == ")" && !(bname in sgOpenBranches))
                    {
                        realname = bname.slice(1, bname.length - 1);
                    }
                    var branchLink = svg.link(svgGroup, branchUrl(realname));
                    var btext = svg.text(branchLink, node.Left + 8, startTop + topMult, bname, { fill: BLUE_STROKE, fontSize: '10' });
                    topMult += 10;
                });
            }
            addMouseEvents(c, node);

            var pGraphNodes = [];
			if (node.Dag.pseudo_parents != null)
			{
				for (var pid in node.Dag.pseudo_parents)
				{
					var pnode = graphNodes[pid];
					if (pnode)
					{
						pGraphNodes.push(pnode);
					}
				}
			}
			else
			{
				for (var pid in node.Dag.parents)
				{
					var pnode = graphNodes[pid];
					if (pnode)
					{
						pGraphNodes.push(pnode);
					}
				}
			}
            pGraphNodes.sort(function (a, b)
			{
                return (b.Index - a.Index);
            });
            var countParentLinesDrawn = 0;
            $.each(pGraphNodes, function (i, pnode)
            {

                if (pnode)
                {
                    pnode.calcPosition();
                    pnode.SvgChildCircleFamily.push(node.Dag.changeset_id);
                    if (!node.Dag.displayChildren || node.Dag.displayChildren.indexOf(pnode.Dag.revno) > -1)
                    {
                        var l;
                        var pleft = colSpacer;
                        var pCol = getColumn(pnode.ColumnID);
                        if (pCol)
                            pleft = pCol.LeftOffset;
                        var finalLinePos = left;

                        if (Math.abs(pnode.Index - node.Index) == 1)
                        {
                            l = svg.polyline(gLines, [[left, node.Bottom - 5], [pleft, pnode.Top]]);
                        }
                        else
                        {
                            if (left < pleft)
                            {
                                var lineLength = getBestLine(node, pnode, pnode.ColumnID, left, pleft);
                                finalLinePos = left + lineLength.line;
                                if (lineLength.change)
                                {
                                    l = svg.polyline(gLines, [[left, node.Bottom - 5], [finalLinePos, node.Top + 15], [left + lineLength.line, pnode.Top - 15], [pleft, pnode.Top]]);
                                }
                                else
                                {
                                    l = svg.polyline(gLines, [[left, node.Bottom - 5], [finalLinePos, node.Bottom + 15], [pleft, pnode.Top]]);
                                }
                            }
                            else
                            {
                                var lineLength = getBestLine(node, pnode, node.ColumnID, left, pleft);

                                if (lineLength.change)
                                {
                                    finalLinePos = left + lineLength.line;
                                    l = svg.polyline(gLines, [[left, node.Bottom - 5], [finalLinePos, node.Top + 15], [finalLinePos, pnode.Top - 15], [pleft, pnode.Top]]);
                                }
                                else
                                {
                                    l = svg.polyline(gLines, [[left, node.Bottom - 5], [left, pnode.Top - 15], [pleft, pnode.Top]]);

                                }
                            }
                        }
                        var lineKey = node.Dag.revno.toString() + pnode.Dag.revno.toString();
                        lineMap[lineKey] = { line: l, strokeColor: node.strokeColor };
                        node.SvgParentLineFamily.push(lineKey);
                        pnode.SvgChildLineFamily.push(lineKey);

						//This check is saying "The parent node is in pseudo_parents, and not also in parents".
						if (node.Dag.parents != null && node.Dag.parents[pnode.Dag.changeset_id] == null
						    && node.Dag.pseudo_parents != null && node.Dag.pseudo_parents[pnode.Dag.changeset_id] != null)
						{
							node.SvgPseudoParentLineFamily.push(lineKey);
							pnode.SvgPseudoChildLineFamily.push(lineKey);
							svg.change(l, { fill: 'none', stroke: node.strokeColor, 'stroke-dasharray': node.strokeDashArray });
						}
                        //add the line to all the nodes it passes through
                        //so draw doesn't draw over any existing lines
                        for (var line = node.Index; line < pnode.Index; line++)
                        {
                            if (!indexedNodes[line].Lines)
                                indexedNodes[line].Lines = [];

                            indexedNodes[line].Lines.push(finalLinePos);
                        }
                        ++countParentLinesDrawn;
                    }
                }
            });
            if (node.Dag.displayChildren && (countParentLinesDrawn < node.Dag.displayChildren.length))
            {
                var l = svg.polyline(gLines, [[left, node.Bottom - 5], [left, canvasHeight]]);
                var lineKey = node.Dag.revno.toString() + "...";
                lineMap[lineKey] = { line: l, strokeColor: node.strokeColor };
                node.SvgParentLineFamily.push(lineKey);
            }
        });

        if (currentHighlight)
        {
            svg.change(currentHighlight.Circle, { r: currentHighlight.Radius + 2 });
            highlightAllParentNodes(currentHighlight, GREEN_FILL, GREEN_STROKE, true);
            highlightAllChildNodes(currentHighlight, GREEN_FILL, GREEN_STROKE, true);
        }
    };

	var clearHighlight = function(){
		if (currentHighlight){
			svg.change(currentHighlight.Circle, { r: currentHighlight.Radius });
	
			visitedParentHighlights = {};
			visitedChildHighlights = {};
			highlightAllParentNodes(currentHighlight, BLUE_FILL, BLUE_STROKE, true);
			highlightAllChildNodes(currentHighlight, BLUE_FILL, BLUE_STROKE, true);
	
			currentHighlight = null;
		}
	}

    this.draw = function ()
    {
        if($('#'+graphDivId).length==0){
            var graphHead = $('#'+tableId+' th.graphhead');
            var graphDiv = $('<div id="'+graphDivId+'"></div>');
            var downshift = graphHead.height() + (graphHead.outerHeight(true)-graphHead.height())/2;
            graphDiv.css({"font-weight": "normal", "float":"left", height:0, width:0, margin:0, border:0, padding:0, "margin-top":downshift, overflow:"visible"});
            graphHead.prepend(graphDiv);
            graphHead.css({overflow: "visible"});
            svg = false;
        }

		if($.browser.msie){
			$('#'+tableId+' td').css({"border-bottom-color": "rgba(0,0,0,0.1)"});
		}

        try
        {
            if (!svg)
            {
                $('#'+graphDivId).svg(
					{
					    onLoad: this.update,
					    settings: { overflow: "visible" },
					    initPath: sgStaticLinkRoot + "/lib/"
					});
                if ($.browser.msie)
                {
                    $('embed').attr("wmode", "transparent");
                }
            }
            else
                this.update();
        }
        catch (e)
        {
            $('#'+graphDivId).hide();
        }

        $('#'+graphDivId).mouseup($(this), function (e)
        {
            e.stopPropagation();
            if (currentHighlight)
            {
                if (!$(e.target).is("circle") && !$(e.target).is("path"))
                {
                    clearHighlight();
                }

            }

        });
    };

	var resizeEvents = multipleEventsWaiterOuter(2000, this.draw, this);
	this.onWindowResize = function(e){
		resizeEvents.fire();
	}

	this.setHighlight = function(csid){
		clearHighlight();
		currentHighlight = graphNodes[csid];
	}
}


