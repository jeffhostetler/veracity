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

function modalPopup(backID, hidemenu, options)
{
	this.popupStatus = 0;
	this.backID = backID || "#backgroundPopup";

	if (this.backID.indexOf("#") != 0)
		this.backID = "#" + this.backID;

	this.options = $.extend(true, {
		id: "popup",
		onClose: null
	}, (options || {}));

	var div;

	this.create = function (title, content, menu, buttons)
	{
	    var self = this;
	    var clss = this.options.css_class || "popup primary";
	    $('div#' + this.options.id).remove();

	    div = $('<div>').attr({ id: this.options.id }).addClass(clss);
	    if (!hidemenu)
	    {
	        var x = $('<a>').attr({ href: "#", id: "popupClose" }).text("x").click(
                function (e)
                {
                    e.preventDefault();
                    self.close();
                    if (self.options.onX)
                        self.options.onX();
                });

	        var h1 = $('<h1>').attr({ id: "popupTitle" }).text(title).addClass('clearfix');

	        if (menu)
	            h1.append(menu);

	        div.append(h1);
	        h1.append(x);

	    }

	    var maxWidth = $(window).width() - 100;
	    var content_div = $('<div>').attr({ id: "popupContent" }).addClass('innercontainer');
	    content_div.html(content);
	    div.append(content_div);

	    if (buttons)
	        div.append(buttons.addClass('buttons'));
	    div.css({ "max-width": maxWidth });
	    // Click out event
	    $(this.backID).click(function (e)
	    {
	        e.preventDefault();
	        self.close();
	    });

	    // Press Escape event
	    $(document).keyup(function (e)
	    {
	        if (e.keyCode == '27')
	        {
	            e.preventDefault();
	            self.close();
	        }
	    });

	    return div;

	};

	this.close = function ()
	{
		if (this.popupStatus == 1)
		{
			$(this.backID).fadeOut("slow");
			$("#" + this.options.id).fadeOut("slow");
			this.popupStatus = 0;
		}

		if (this.options.onClose)
			this.options.onClose();
	};

	this.show = function (height, width, position, yOffset, xOffset, forceLeft)
	{
		yoff = yOffset || 0;
		xoff = xOffset || 0;

		if (height)
		{
			$("#" + this.options.id).css({
				"height": height
			});
		}
		if (width)
		{
			$("#" + this.options.id).css({
				"width": width
			});
		}

		if (!position)
		{
			this.center();
		}
		else
		{
			var wHeight = windowHeight();
			var wWidth = $(window).width() || window.innerWidth;
			var top = position.top + yoff;
			var left = position.left + xoff;

			var docScrollTop = $(document).scrollTop();
			var docScrollLeft = $(document).scrollLeft();

			if (top + div.height() > docScrollTop + wHeight)
			{
				top = docScrollTop + 20;
			}

			if (left + div.width() > docScrollLeft + wWidth)
			{
				left -= $("#" + this.options.id).width();

				if (left < 0)
				{
					left = position.left + xoff;
				}
			}

			if (this.options.position)
			{
				$("#" + this.options.id).css({
					"position": this.options.position
				});
			}
			else
			{
				$("#" + this.options.id).css({
					"position": "absolute"
				});
			}
			$("#" + this.options.id).css({
				"top": top,
				"left": left
			});
		}

		// loads popup only if it is disabled
		if (this.popupStatus == 0)
		{
			if (this.backID)
			{
				$(this.backID).css({
					"opacity": "0.7"
				});
				$(this.backID).fadeIn("slow");
			}
			$("#" + this.options.id).fadeIn("slow");
			this.popupStatus = 1;
		}
	};

	// centering popup
	this.center = function ()
	{
		// request data for centering
		var wHeight = windowHeight();
		var windowWidth = $(window).width() || window.innerWidth;

		var popupHeight = $("#" + this.options.id).height();
		var popupWidth = $("#" + this.options.id).width();

		// if the popup is larger than the screen set the
		// top to a constant else center it on the screen
		var left = 5;
		var top = 30;

		var docScrollTop = $(document).scrollTop();
		var docScrollLeft = $(document).scrollLeft();

		if (popupWidth < windowWidth)
		{
			left = (windowWidth / 2) - (popupWidth / 2);
		}
		if (popupHeight < windowHeight)
		{
			top = (wHeight / 2) - (popupHeight / 2);
		}
		if (this.options.position == "fixed")
		{
			$("#" + this.options.id).css({
				"position": "fixed",
				"top": top,
				"left": left
			});
		}
		else
		{
			$("#" + this.options.id).css({
				"position": "absolute",
				"top": top + docScrollTop,
				"left": left + docScrollLeft
			});
		}
	};
}
