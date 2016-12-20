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
var fv = null;

function compute_diff_link_for_file__hid_wc(item_j, ref_r)
{
    if (item_j[ref_r] == undefined)
	return "";

    // The hid of the working copy version may or may not have
    // been computed.  If it has, we can avoid adding a trivial
    // diff button/link on the page. But if not, let it go and
    // set up for the live diff.

    if ((item_j["WC"] != undefined) && (item_j["WC"].hid != undefined))
	if (item_j[ref_r].hid == item_j["WC"].hid)
	    return;

    // TODO 2012/02/21 this knows that B stores the baseline HID
    // TODO            and the normal repo-path.  Should we let
    // TODO            this use a @g domain repo-path since we know it.

    var diffURL = (sgCurrentRepoUrl
		   + "/diff_local.json?hid1="
		   + item_j[ref_r].hid
		   + "&path="
		   + item_j.path);
    var imgd = $('<img>').attr("src", sgStaticLinkRoot + "/img/diff.png");

    var a_diff = $('<a>').html(imgd);
    a_diff.attr(
	{
	    href: "#",
	    'title': "diff " + ref_r + " and working copy"
	});
    a_diff.addClass("download_link");
    a_diff.click(function (e)
		 {
		     e.preventDefault();
		     e.stopPropagation();
		     showDiff(diffURL, $('<div>'), true);
		 });

    return a_diff;
}

function compute_diff_link_for_file__hid_hid(item_j, ref_a, ref_b)
{
    if ((item_j[ref_a] == undefined) || (item_j[ref_b] == undefined))
	return "";

    if (item_j[ref_a].hid == item_j[ref_b].hid)
	return "";

    var diffURL = (sgCurrentRepoUrl
		   + "/diff.json?hid1="
		   + item_j[ref_a].hid
		   + "&hid2="
		   + item_j[ref_b].hid);
    var imgd = $('<img>').attr("src", sgStaticLinkRoot + "/img/diff.png");

    var a_diff = $('<a>').html(imgd);
    a_diff.attr(
	{
	    href: "#",
	    'title': "diff " + ref_a + " and " + ref_b
	});
    a_diff.addClass("download_link");
    a_diff.click(function (e)
		 {
		     e.preventDefault();
		     e.stopPropagation();
		     showDiff(diffURL, $('<div>'), true);
		 });

    return a_diff;
}

function display_canonical_status(tbl, canonical_status)
{
    // This table needs to match the one in sg_wc__status__classic_format.c

    // TODO 2012/01/06 Get 'bVerbose' from URL.
    // TODO 2012/01/06 Consider a --no-ignores and any other options.
    // TODO 2012/01/06 Use some CSS to make it pretty.

    var aSections = [ { "prefix" : "Added",      "bReportWas" : false, "bVerboseOnly" : false },
		      { "prefix" : "Modified",   "bReportWas" : false, "bVerboseOnly" : false },
		      { "prefix" : "Auto-Merged","bReportWas" : false, "bVerboseOnly" : false },
		      { "prefix" : "Attributes", "bReportWas" : false, "bVerboseOnly" : false },
		      { "prefix" : "Removed",    "bReportWas" : false, "bVerboseOnly" : false },
		      { "prefix" : "Existence",  "bReportWas" : false, "bVerboseOnly" : false },
		      { "prefix" : "Renamed",    "bReportWas" : true,  "bVerboseOnly" : false },
		      { "prefix" : "Moved",      "bReportWas" : true,  "bVerboseOnly" : false },
		      { "prefix" : "Lost",       "bReportWas" : false, "bVerboseOnly" : false },
		      { "prefix" : "Found",      "bReportWas" : false, "bVerboseOnly" : false },
		      { "prefix" : "Ignored",    "bReportWas" : false, "bVerboseOnly" : true  },
		      { "prefix" : "Sparse",     "bReportWas" : false, "bVerboseOnly" : false },
		      { "prefix" : "Resolved",   "bReportWas" : false, "bVerboseOnly" : false },
		      { "prefix" : "Unresolved", "bReportWas" : false, "bVerboseOnly" : false },
		      { "prefix" : "Choice Resolved",   "bReportWas" : false, "bVerboseOnly" : true },
		      { "prefix" : "Choice Unresolved", "bReportWas" : false, "bVerboseOnly" : true },
		      { "prefix" : "Unchanged",  "bReportWas" : false, "bVerboseOnly" : false },
		    ];

    var aRef = [ "B", "C", "A" ];    // See SG_WC__STATUS_SUBSECTION_*

    var bVerbose = false;	// TODO get this from the URL/control

    $.each(aSections,
	   function(k, section_k)
	   {
	       if (section_k.bVerboseOnly && !bVerbose)	// section only displayed when verbose
		   return;

	       var prefix = section_k.prefix;
	       var prefix_re = new RegExp( "^" + prefix + ".*");
	       var bReportWas = section_k.bReportWas;
	       var bReportExtra = (bReportWas || bVerbose);

	       $.each(canonical_status,
		      function(j, item_j)
		      {
			  $.each(item_j.headings,
				 function(h, heading_h)
				 {
				     if (prefix_re.test(heading_h))
				     {
					 var tr = $("<tr>");
					 var td0 = $("<td>");	// col0 is label
					 var td1 = $("<td>");	// col1 is * for multiple changes
					 var td2_A_B = $("<td>");	// col2 is file-diff anchor
					 var td2_A_C = $("<td>");	// col2 is file-diff anchor
					 var td2_B_C = $("<td>");	// col2 is file-diff anchor
					 var td2_B_WC = $("<td>");	// col2 is file-diff anchor
					 var td2_C_WC = $("<td>");	// col2 is file-diff anchor
					 var td2_A_WC = $("<td>");	// col2 is file-diff anchor
					 var td3 = $("<td>");	// col3 is repo-path
					 var bExtra = false;

					 td0.css("text-align", "right");
					 td0.text(heading_h);

					 // If the item has a conflict or multiple changes, 
					 // we flag it.
					 //
					 // TOOD 2011/11/11 Think about making this a
					 // TODO            small image and/or adding
					 // TODO            a tool-tip or legend for it.

					 if (item_j.status.isUnresolved)
					     td1.append("!");
					 else if (item_j.status.isResolved)
					     td1.append(".");
					 else
					     td1.append(" ");	// TODO &nbsp; ?

					 if (item_j.status.isMultiple)
					     td1.append("+");

					 // For modified files, we have a link to
					 // show either the diffs or the whole file.
					 // Everything else gets just a plain/undecorated
					 // repo-path.
					 //
					 // TODO 2011/11/11 Think about using a fixed-width
					 // TODO            font for the repo-paths.

					 if (item_j.status.isFile && (prefix == "Modified"))
					 {
					     td2_A_B.append(  compute_diff_link_for_file__hid_hid(item_j, "A", "B") );
					     td2_A_C.append(  compute_diff_link_for_file__hid_hid(item_j, "A", "C") );
					     td2_B_C.append(  compute_diff_link_for_file__hid_hid(item_j, "B", "C") );

					     td2_B_WC.append( compute_diff_link_for_file__hid_wc(item_j,  "B", "WC") );
					     td2_C_WC.append( compute_diff_link_for_file__hid_wc(item_j,  "C", "WC") );

					     td2_A_WC.append( compute_diff_link_for_file__hid_wc(item_j,  "A", "WC") );
					     
					     var a_view = $('<a>').text(item_j.path);
					     a_view.attr(
						 {
						     href: "#",
						     'title': "view current working copy"
						 });
					     a_view.click(function (e)
							  {
							      var tmpPath = item_j.path.ltrim("@/");
							      fv.displayLocalFile(tmpPath);
							  });

					     td3.append(a_view);
					 }
					 else if (item_j.status.isRemoved)
					 {
					     td3.append( "(" + item_j.path + ")" );
					 }
					 else
					 {
					     td3.append(item_j.path);
					 }

					 // Display any secondary data (such as a renamed 
					 // item's oldname) and verbose data in the second row.
					 //
					 // TODO 2011/11/11 think about dropping the font.

					 if (bReportExtra)
					 {
					     var tdextra = $("<td>");

					     $.each(aRef,
						    function(r, ref_r)
						    {
							if (item_j[ref_r] != undefined)
							{
							    tdextra.append("# " + ref_r + " was " + item_j[ref_r].path );
							    tdextra.append($("<br>"));
							}
						    });

					     if (bVerbose)
					     {
						 tdextra.append("# Id @" + item_j.gid);
						 tdextra.append($("<br>"));
					     }
					 }

					 tr.append(td0);
					 tr.append(td1);
					 tr.append(td2_A_B);
					 tr.append(td2_A_C);
					 tr.append(td2_B_C);
					 tr.append(td2_B_WC);
					 tr.append(td2_C_WC);
					 tr.append(td2_A_WC);
					 tr.append(td3);

					 tbl.append(tr);

					 if (bReportExtra)
					 {
					     var tr2 = $("<tr>");
					     var tdfiller0 = $("<td>");
					     var tdfiller1 = $("<td>");
					     var tdfiller2_A_B = $("<td>");
					     var tdfiller2_A_C = $("<td>");
					     var tdfiller2_B_C = $("<td>");
					     var tdfiller2_B_WC = $("<td>");
					     var tdfiller2_C_WC = $("<td>");
					     var tdfiller2_A_WC = $("<td>");

					     tr2.append(tdfiller0);
					     tr2.append(tdfiller1);
					     tr2.append(tdfiller2_A_B);
					     tr2.append(tdfiller2_A_C);
					     tr2.append(tdfiller2_B_C);
					     tr2.append(tdfiller2_B_WC);
					     tr2.append(tdfiller2_C_WC);
					     tr2.append(tdfiller2_A_WC);
					     tr2.append(tdextra);

					     tbl.append(tr2);
					 }

				     }
				 });
		      });
	   });
}

function getStatus()
{

	var ctr = $('#statusbox');
	$('#fsLoading .statusText').text("Geting Working Copy Status...");
	$('#fsLoading').show();

	vCore.ajax(
		{
		    url: "/local/mstatus.json",
		    dataType: 'json',

		    error: function (xhr, textStatus, errorThrown)
		    {
		        $('#fsLoading').hide();
		    },

		    success: function (data)
		    {

		        $('#fsLoading').hide();

		        if (isEmpty(data.status))
		        {
		            var div = $('<p>').width(300).height(30);
		            div.text("no changes in your working copy");
		            div.addClass("highlight");

		            ctr.append(div);
		            return;
		        }

			var tbl = $("<table>").addClass("cslist innercontainer empty");
			display_canonical_status(tbl, data.status);
		        ctr.append(tbl);
		    }
		});
}

function sgKickoff()
{
    fv = new fileViewer();
	getStatus();
}
