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

var configFlagsAreDirty = false;

var editable =
{
	prefix : null,

	table : null,
	addButton : null,
	addLink : null,
	addControls : null,
	saveButton : null,
	textControls : null,
	focusControl : null,
	editingTr : null,
	editingRecId : null,

	loadingTr : null,
	loadingText : null,
	loadingTrCancelBtn : null,

	dataRec : null,

	populateControlsForEdit : function(tr, prefix, data) { throw("uninitialized"); },
	buildRecFromControls : function(prefix, dataRec) { throw("uninitialized"); },
	populateRowAfterEditSaved : function(tr, data) { throw("uninitialized");	},
	displayNewRow : function(editable, newRecsArray) { throw("uninitialized"); },
	resetAddControls : function() {},

	setInitialFocus : function()
	{
		if (this.focusControl != null && this.focusControl != undefined)
			this.focusControl.select();
	},

	replaceRowWithWaitText : function(row, text, allowCancel)
	{
		var newRow = this.loadingTr.clone();
		var cancelBtn = newRow.find("input[type=button]");

		newRow.find(".statusText").text(text);
		newRow.insertBefore(row);

		if (allowCancel == true)
			cancelBtn.show();
		else
			cancelBtn.hide();

		row.hide();
		newRow.show();
		newRow.data(row);

		return newRow;
	},
	replaceWaitTextWithOriginalRow : function(waitRow)
	{
		var oldRow = waitRow.data();
		waitRow.remove();
		oldRow.show();
	},
	removeWaitText : function(waitRow)
	{
		waitRow.remove();
	},

	init_before_data : function(prefix)
	{
		this.prefix = prefix;

		this.loadingTr = $("#" + this.prefix + "LoadingRow");
		this.loadingText = this.loadingTr.find(".statusText");
		this.loadingTrCancelBtn = this.loadingTr.find("input[type=button]");
		this.addLink = $("#" + this.prefix + "AddLink");
		this.table = $("#" + this.prefix + "Table");
		this.addButton = $("#" + this.prefix + "AddButton");
		this.addControls = $("#" + this.prefix + "AddControls");
		this.saveButton = $("#" + this.prefix + "SaveButton");
	},

	init_after_data : function()
	{
		this.textControls = $("#" + this.addControls[0].id + " input[type=text]");

		if (this.focusControl == null)
			this.focusControl = this.textControls[0];

		this.textControls.keyup(this,
			function(event)
			{
				switch(event.which)
				{
				case 27:
					// Escape
					event.data.cancel();
					return false;
				case 13:
					// Enter
					event.data.save();
					return false;
				}
				return true;
			});

		this.addButton.show();
	},

	add : function()
	{
		this.cancel();
		this.dataRec = {};

		this.textControls.val("");
		this.resetAddControls();

		this.addControls.appendTo(this.table);
		this.addButton.hide();
		this.addControls.show();

		this.setInitialFocus();
	},

	edit : function(recid)
	{
		this.cancel();

		this.editingTr = $("#" + recid).closest("tr");
		this.editingRecId = recid;

		this.addControls.insertBefore(this.editingTr);
		this.loadingTr.insertBefore(this.editingTr);
		this.loadingText.text("Loading...");
		this.loadingTrCancelBtn.show();
		this.editingTr.hide();
		this.addControls.hide();
		this.addLink.hide();
		this.loadingTr.show();

		var editable = this;

		vCore.ajax(
			{
				url : vCore.getOption("currentRepoUrl") + "/builds/" + editable.prefix + "/" + recid + ".json",
				dataType : "json",

				success : function(data, status, xhr)
				{
					//vCore.consoleLog(JSON.stringify(data, null, 4));

					editable.dataRec = data;
					editable.populateControlsForEdit(editable.editingTr, editable.prefix, data);
					editable.edit_ready(recid);
				},

				error : function(xhr, status, err)
				{
					editable.cancel();
				}
			});
	},

	edit_ready : function(recid)
	{
		if (this.editingTr != null && this.editingRecId == recid)
		{
			this.loadingTr.hide();
			this.addControls.show();
			this.setInitialFocus();
			this.addLink.show();
		}
	},

	del : function(recid)
	{
		var delTr = $("#" + recid).closest("tr");
		var waitRow = this.replaceRowWithWaitText(delTr, "Deleting...");
		var editable = this;

		vCore.ajax(
		{
			url : vCore.getOption("currentRepoUrl") + "/builds/" + this.prefix + "/" + recid,
			type: "DELETE",

			success : function(data, status, xhr)
			{
				editable.del_done(waitRow, recid);
			},

			error : function(xhr, status, err)
			{
				editable.replaceWaitTextWithOriginalRow(waitRow);
			}
		});
	},

	del_done : function(waitRow, recid)
	{
		if (this == seriesObj)
			$("#manualBuildSeries option[value='" + recid + "']").remove();
		else if (this == statusObj)
			$("#manualBuildStatus option[value='" + recid + "']").remove();

		this.removeWaitText(waitRow);
	},

	cancel : function()
	{
		this.dataRec = null;
		this.editingRecId = null;

		this.addButton.show();
		this.addLink.show();
		this.addControls.hide();
		this.loadingTr.hide();

		if (this.editingTr != null)
		{
			this.editingTr.show();
			this.editingTr = null;
		}

		enableRowHovers(false);
	},

	save : function()
	{
		var editable = this;
		var isNew = (this.dataRec.recid == null || this.dataRec.recid == undefined);

		var url = vCore.getOption("currentRepoUrl") + "/builds/" + this.prefix;
		if (!isNew)
			url += "/" + this.dataRec.recid;

		this.loadingText.text("Saving...");
		this.loadingTr.insertBefore(this.addControls);
		this.addControls.hide();
		this.loadingTrCancelBtn.hide();
		this.loadingTr.show();

		enableRowHovers(true);

		this.buildRecFromControls(this.prefix, this.dataRec);

		vCore.ajax(
		{
			url : url,
			type: isNew ? "POST" : "PUT",
			dataType : "json",
			contentType : "application/json",
			data : JSON.stringify(editable.dataRec),

			success : function(data, status, xhr)
			{
				//vCore.consoleLog(JSON.stringify(data, null, 4));

				editable.populateRowAfterEditSaved(editable.editingTr, data);

				editable.dataRec = data;
				editable.save_done(isNew);
			},

			error : function(xhr, status, err)
			{
				editable.cancel();
			}
		});
	},

	save_done : function(isNew, waitRow)
	{
		this.loadingTr.hide();

		this.addButton.show();

		if (!isNew)
		{
			this.editingTr.show();
			this.editingTr = null;
		}
		else
		{
			var ary = new Array(this.dataRec);
			this.displayNewRow(this, ary);
		}

		enableRowHovers(false);
		this.dataRec = null;
	}
};

// JQuery's extend is used here rather than simply "subClass = Object.create(baseClass)" because IE is dumb.
var seriesObj = $.extend(true, {}, editable);
seriesObj.populateControlsForEdit = function(tr, prefix, data)
{
	var tds = $(tr).children("td");

	$("#" + prefix + "Alias").val(data.nickname);
	$("#" + prefix + "Name").val(data.name);
	$(tds[1]).text(data.nickname);
	$(tds[2]).text(data.name);
};
seriesObj.buildRecFromControls = function(prefix, rec)
{
	rec.nickname = $("#" + prefix + "Alias").val();
	rec.name = $("#" + prefix + "Name").val();
};
seriesObj.populateRowAfterEditSaved = function(tr, data)
{
	var tds = $(tr).children("td");
	$(tds[1]).text(data.nickname);
	$(tds[2]).text(data.name);

	$("#manualBuildSeries option[value='" + data.recid + "']").text(data.name);
};
seriesObj.displayNewRow = addSeriesOrEnvironments;

var envObj = $.extend(true, {}, editable);
envObj.populateControlsForEdit = seriesObj.populateControlsForEdit;
envObj.buildRecFromControls = seriesObj.buildRecFromControls;
envObj.populateRowAfterEditSaved = seriesObj.populateRowAfterEditSaved;
envObj.displayNewRow = addSeriesOrEnvironments;

var statusObj = $.extend(true, {}, editable);
statusObj.populateControlsForEdit = function(tr, prefix, data)
{
	$("#statusAlias").val(data.nickname);
	$("#statusName").val(data.name);

	setCheckboxState("statusTemporary", data.temporary);
	setCheckboxState("statusSuccessful", data.successful);
	setCheckboxState("statusETA", data.use_for_eta);
	setCheckboxState("statusActivity", data.show_in_activity);

	if (data.color == undefined)
		selectColor("colorwhite");
	else
		selectColor("color" + data.color);

	if (data.icon == undefined)
		selectIcon("iconnone");
	else
		selectIcon("icon" + data.icon);
};
statusObj.buildRecFromControls = function(prefix, rec)
{
	rec.nickname = $("#statusAlias").val();
	rec.name = $("#statusName").val();

	rec.temporary = $("#statusTemporary").is(":checked");
	rec.successful = $("#statusSuccessful").is(":checked");
	rec.use_for_eta = $("#statusETA").is(":checked");
	rec.show_in_activity = $("#statusActivity").is(":checked");
	rec.color = $("#statusColor").val();

	rec.icon = $("#statusIcon").val();
	if (rec.icon == "none")
		delete rec.icon;
};
statusObj.populateRowAfterEditSaved = function(tr, data)
{
	var tds = $(tr).children("td");
	$(tds[1]).text(data.nickname);
	$(tds[2]).text(data.name);

	checkmarkImgIfTrue(tds[3], data.temporary);
	checkmarkImgIfTrue(tds[4], data.successful);
	checkmarkImgIfTrue(tds[5], data.use_for_eta);
	checkmarkImgIfTrue(tds[6], data.show_in_activity);

	$(tds[7]).children().remove();
	$(tds[7]).append(getColorSquare(data.color));

	$(tds[8]).children().remove();
	if (data.icon != undefined)
	{
		var img = $(document.createElement("img"));
		img.attr("src", sgStaticLinkRoot + "/img/builds/" + data.icon + "16.png");
		$(tds[8]).append(img);
	}

	$("#manualBuildStatus option[value='" + data.recid + "']").text(data.name);
};
statusObj.displayNewRow = addStatuses;
statusObj.resetAddControls = function()
{
	$("#statusTemporary").removeAttr("checked");
	$("#statusSuccessful").removeAttr("checked");
	$("#statusETA").removeAttr("checked");
	$("#statusActivity").removeAttr("checked");

	selectColor("colorwhite");
	selectIcon("iconnone");
};



function menuClicked(e)
{
	var action = $(this).attr("action");
	var recid = $("#editMenu").data("recid");
	var span = $("#" + recid);
	var editable = span.data("editable");

	if (action == "edit")
	{
	    $("#editMenu").hide();
	    $("#delConfMenu").hide();
	    editable.edit(recid);
	}
	else if (action == "confirm_delete")
	    showDeleteSubMenu(recid);
	else if (action == "delete")
	{
	    $("#editMenu").hide();
	    $("#delConfMenu").hide();
	    editable.del(recid);
	}
	else
	    throw ("Unknown action: " + action);


}

function getEditLink(id, editable)
{
	var a, img;
	var linkspan = $(document.createElement("span"));
	linkspan.attr("id", id);
	linkspan.attr("class", "linkSpan");
	linkspan.attr("style", "display:none");
	linkspan.data("editable", editable);
	a = $(document.createElement("a"));
	a.attr("href", "javascript:showCtxMenu('" + id + "')");
	img = $(document.createElement("img"));
	img.attr("src", sgStaticLinkRoot + "/img/contextmenu.png");
	a.append(img);
	linkspan.append(a);

	return linkspan;
}

function checkmarkImgIfTrue(parent, zingBoolString)
{
	$(parent).children().remove();
	if (boolFromZingVal(zingBoolString))
	{
		var img = $(document.createElement("img"));
		img.attr("src", sgStaticLinkRoot + "/img/blackcheck.png");
		$(parent).append(img);
	}
}

function setCheckboxState(id, zingBoolString)
{
	// Zing gives us strings "0" and "1", both of which JS considers true.
	var actualBool = boolFromZingVal(zingBoolString);
	var cb = $("#" + id);
	if (actualBool)
		cb.attr("checked","checked");
	else
		cb.removeAttr("checked");
}

function showTopSection()
{
	if ( $("#manualBuildStatus option").length > 1 || ( $("#manualBuildStatus option").length > 0 && $("#manualBuildStatus option")[0].text != "(None)") )
	{
		if ( $("#manualBuildSeries option").length > 1 || ( $("#manualBuildSeries option").length > 0 && $("#manualBuildSeries option")[0].text != "(None)") )
			$("#flags").show();
	}
}

function addSeriesOrEnvironments(editable, recs)
{
	var tr, td, i, a, div, rec, tf;

	for (i = 0; i < recs.length; i++)
	{
		rec = recs[i];

		tr = $(document.createElement("tr"));

		td = $(document.createElement("td"));
		var spacer = $('<div>').addClass("spacer");
		td.append(spacer);
		td.append(getEditLink(rec.recid, editable));
		tr.append(td);

		td = $(document.createElement("td"));
		td.text(rec.nickname);
		tr.append(td);

		td = $(document.createElement("td"));
		td.text(rec.name);
		tr.append(td);

		td = $(document.createElement("td"));
		tr.append(td);

		editable.table.append(tr);

		if (editable == seriesObj)
		{
			var opt = $(document.createElement("option"));
			opt.text(rec.name);
			opt.attr("value", rec.recid);
			 $("#manualBuildSeries").append(opt);
		}
	}

	/* If there's at least one status and at least one series, show the manual build drop-downs at the top. */
	if (editable == seriesObj)
		showTopSection();
}

function getColorSquare(colorString)
{
	var square = $("#colorSquare").clone();
	square.removeAttr("id");
	square.addClass("buildstatus" + colorString);
	square.show();
	return square;
}

function addStatuses(editable, recs)
{
	var tr, td, i, a, rec;

	for (i = 0; i < recs.length; i++)
	{
		rec = recs[i];

		tr = $(document.createElement("tr"));

		td = $(document.createElement("td"));
		var spacer = $('<div>').addClass("spacer");
		td.append(spacer);
		td.append(getEditLink(rec.recid, editable));
		tr.append(td);

		td = $(document.createElement("td"));
		td.text(rec.nickname);
		tr.append(td);

		td = $(document.createElement("td"));
		td.text(rec.name);
		tr.append(td);

		td = $(document.createElement("td"));
		td.attr("align", "center");
		checkmarkImgIfTrue(td, rec.temporary);
		tr.append(td);

		td = $(document.createElement("td"));
		td.attr("align", "center");
		checkmarkImgIfTrue(td, rec.successful);
		tr.append(td);

		td = $(document.createElement("td"));
		td.attr("align", "center");
		checkmarkImgIfTrue(td, rec.use_for_eta);
		tr.append(td);

		td = $(document.createElement("td"));
		td.attr("align", "center");
		checkmarkImgIfTrue(td, rec.show_in_activity);
		tr.append(td);

		td = $(document.createElement("td"));
		td.attr("align", "center");
		td.append(getColorSquare(rec.color));
		tr.append(td);

		td = $(document.createElement("td"));
		td.attr("align", "center");
		if (rec.icon != undefined)
		{
			var img = $(document.createElement("img"));
			img.attr("src", sgStaticLinkRoot + "/img/builds/" + rec.icon + "16.png");
			td.append(img);
		}
		tr.append(td);

		td = $(document.createElement("td"));
		tr.append(td);

		editable.table.append(tr);

		/* Add to the manual build drop-downs at the top */
		var opt = $(document.createElement("option"));
		opt.text(rec.name);
		opt.attr("value", rec.recid);
		$("#manualBuildStatus").append(opt);
	}

	showTopSection();
}

function selectIcon(id)
{
	$("div#iconContainer img").css("border", "none");
	$("#" + id).css("border", "2px solid black");
	$("#statusIcon").val(id.slice(4)); // remove leading "icon"
}

function selectColor(id)
{
	$("div#colorContainer div").css("border", "2px solid white");
	$("#" + id).css("border", "2px solid black");
	$("#statusColor").val(id.slice(5)); // remove leading "color"
}

window.onbeforeunload = function ()
{
	if (configFlagsAreDirty || seriesObj.editingTr != null || envObj.editingTr != null || statusObj.editingTr != null)
		return "You have unsaved changes.";
};

var manualBuildSeriesRecId = "null";
var manualBuildStatusRecId = "null";
function ConfigChanged()
{
	configFlagsAreDirty	= true;
	$("#saveBtn, #cancelBtn").removeAttr("disabled").removeClass('disabled');
}

function CancelConfig()
{
	configFlagsAreDirty = false;
	$("#manualBuildSeries").val(manualBuildSeriesRecId);
	$("#manualBuildStatus").val(manualBuildStatusRecId);
	$("#saveBtn, #cancelBtn").attr("disabled", "1").addClass('disabled');
}

function SaveConfig()
{
	if ($("#manualBuildSeries").val() == "null" || $("#manualBuildStatus").val() == "null")
	{
		alert("Both a series and a status must be chosen.");
		return;
	}

	vCore.ajax(
	{
		url : vCore.getOption("currentRepoUrl") + "/builds/config.json",
		type: "PUT",
		dataType : "json",
		contentType : "application/json",
		data : JSON.stringify(
		{
			series: $("#manualBuildSeries").val(),
			status: $("#manualBuildStatus").val()
		}),

		success : function(data, status, xhr)
		{
			manualBuildSeriesRecId = $("#manualBuildSeries").val();
			manualBuildStatusRecId = $("#manualBuildStatus").val();
			configFlagsAreDirty = false;
			$("#saveBtn").attr("disabled", "1");
			$("#cancelBtn").attr("disabled", "1");
		}
	});

}

function getBuildConfigData()
{
	vCore.ajax(
	{
		url : vCore.getOption("currentRepoUrl") + "/builds/config.json",
		dataType : "json",

		success : function(data, status, xhr)
		{
			//vCore.consoleLog(JSON.stringify(data, null, 4));

			if (data.series != null)
			{

				addSeriesOrEnvironments(seriesObj, data.series);
				seriesObj.init_after_data();

				if (data.config != null)
				{
					manualBuildSeriesRecId = data.config.manual_build_series;
					 $("#manualBuildSeries").val(manualBuildSeriesRecId);
				}
				else
				{
					var opt = $(document.createElement("option"));
					opt.attr("value", "null");
					opt.text("(None)");
					$("#manualBuildSeries").append(opt);
					$("#manualBuildSeries").val("null");
				}

			}

			if (data.environments != null)
			{
				addSeriesOrEnvironments(envObj, data.environments);
				envObj.init_after_data();
			}

			if (data.statuses != null)
			{
				addStatuses(statusObj, data.statuses);
				statusObj.init_after_data();

				if (data.config != null)
				{
					manualBuildStatusRecId = data.config.manual_build_status;
					$("#manualBuildStatus").val(manualBuildStatusRecId);
				}
				else
				{
					opt = $(document.createElement("option"));
					opt.attr("value", "null");
					opt.text("(None)");
					$("#manualBuildStatus").append(opt);
					$("#manualBuildStatus").val("null");
				}
			}

			$("div#iconContainer img").click(function() { selectIcon($(this).attr("id")); });

			var buildColors = ["green", "red", "gray", "yellow", "white"];
			var colorContainer = $("#colorContainer");
			for (var i = 0; i < buildColors.length; i++)
			{
				var color = buildColors[i];
				var square = getColorSquare(color);
				square.attr("id", "color" + color);
				square.css("float", "left");
				colorContainer.append(square);
			}
			$("div#colorContainer div").click(function() { selectColor($(this).attr("id")); });

			enableRowHovers();

			// Hook up click events for context menu.
			$("#editMenu li").click(menuClicked);
			$("#delConfMenu li").click(menuClicked);
		},

		complete : function(xhr, status)
		{
			$("#seriesLoadingRow").hide();
			$("#envLoadingRow").hide();
			$("#statusLoadingRow").hide();
		}
	});
}

function sgKickoff()
{
	seriesObj.init_before_data("series");
	envObj.init_before_data("env");
	statusObj.init_before_data("status");
	getBuildConfigData();
}
