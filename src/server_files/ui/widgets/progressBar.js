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

function progressBar(baseName)
{

	this.baseName = baseName;
	this.rendered = false;
	
	this.progress = null
	this.summaryDiv = null;
	this.valueDiv = null;
	this.textDiv = null;
}

extend(progressBar, {
	
	render: function()
	{
		this.progress = $(document.createElement("div")).attr("id", this.baseName + "Progress");
		this.summaryDiv = $(document.createElement("span")).attr("id", this.baseName + "ProgressSummary");
		this.progress.append(this.summaryDiv);

		var progBarWrap = $(document.createElement("div")).attr("id", this.baseName + "ProgressBarWrap").addClass("progressBar");
		this.valueDiv = $(document.createElement("div")).attr("id", this.baseName + "ProgressBarValue").addClass("progressValue");
		this.textDiv = $(document.createElement("div")).attr("id", this.baseName + "ProgressBarText").addClass("progressText");
		this.valueDiv.append(this.textDiv);
		progBarWrap.append(this.valueDiv);
		this.progress.append(progBarWrap);
		
		this.rendered = true;
		
		return this.progress;
	},
	
	setSummary: function(summary)
	{
		if (!this.rendered)
			return;
		
		if (typeof summary == "string")
			this.summaryDiv.text(summary);
		else
		{
			this.summaryDiv.empty();
			this.summaryDiv.append(summary);
		}
	},
	
	setPercentage: function(percent)
	{
		if (!this.rendered)
			return;
		
		if (!percent)
			percent = 0;
			
		var wPercent = percent > 100 ? 100 : percent;
		
		this.valueDiv.css("width", wPercent + "%");
		this.textDiv.text(percent + "%");
	},
	
	hide: function()
	{
		this.progress.hide();
	},
	
	show: function()
	{
		this.progress.show();
	}
});



function smallProgressIndicator(id, text)
{
	this.id = id;
	this.div = null;
	this.text = text;
}

extend(smallProgressIndicator, {
	
	draw: function()
	{
		this.div = $("<div>").attr("id", this.id).addClass("clearfix");
		
		this.div.append($("<div>").addClass("smallProgress"));
		this.div.append($("<div>").addClass("statusText").text(this.text));
		
		return this.div;
	},
	
	setText: function(text) 
	{
		this.text = text;
		
		if (this.div)
			$(".statusText", this.div).text(this.text);
	},
	
	hide: function()
	{
		this.div.hide();
	},
	
	show: function()
	{
		this.div.show();
	}
});
