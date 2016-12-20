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

Array.isArray = Array.isArray || function (o) { return Object.prototype.toString.call(o) === '[object Array]'; };

if (!Object.keys) Object.keys = function (o)
{
    if (o !== Object(o))
        throw new TypeError('Object.keys called on non-object');
    var ret = [], p;
    for (p in o) if (Object.prototype.hasOwnProperty.call(o, p)) ret.push(p);
    return ret;
}

var _wiusers = {};
var _sprints = {};
var _comments = {};


function _disableButton(button)
{
    $(button).addClass('disabled');
    $(button).attr('disabled', true);
}

function _enableButton(button)
{
    $(button).removeClass('disabled');
    $(button).removeAttr('disabled');
}

function _enableOnInput(button, field)
{
    var b = $(button);
    var f = $(field);
    var check = function ()
    {
        var v = f.val() || '';

        if (typeof (v) == "string")
            v = v.trim();

        if (v)
            _enableButton(b);
        else
            _disableButton(b);
    };

    f.change(check);
    f.keyup(check);

    check();
}

function _niceUser(uid)
{
    if (_wiusers[uid])
        return (_wiusers[uid]);
    return (uid);
}
function _niceSprint(sid)
{
    if (_sprints[sid])
        return (_sprints[sid]);
    return (sid);
}

function _addAdder(addpage)
{
    var adder = $('#newwi');
    var addurl = sgCurrentRepoUrl + "/" + addpage;

    adder.attr('href', addurl);
}

function _findCookie(cname, defval)
{
    var un = cname + "=";
    var v = defval || false;

    if (document.cookie.length > 0)
    {
        var cookies = document.cookie.split(';');

        for (var i = 0; i < cookies.length; ++i)
        {
            var cstr = jQuery.trim(cookies[i]);

            if (cstr.substring(0, un.length) == un)
            {
                // don't just return; in corner cases, multiple versions
                // of the cookie may exist. we want the last one.
                v = decodeURIComponent(cstr.substring(un.length));
            }
        }
    }

    return (v);
}

function _setCookie(cname, cval, exp)
{
    var exdate = new Date();
    exdate.setDate(exdate.getDate() + exp);

    var cstr = cname + "=" + encodeURIComponent(cval) +
	"; path=/" +
	"; expires=" + exdate.toUTCString();

    document.cookie = cstr;
}

var utils_wimd = null;
function utils_savingNow(msg)
{
    utils_wimd = new modalPopup('none', true, { position: "fixed", css_class: "savingpopup", id: "savingpopup" });
    var outer = $(document.createElement('div')).
		addClass('spinning');
    var saving = $(document.createElement('p')).
		    addClass('saving').
		    text(msg).
		    appendTo(outer);
    var p = utils_wimd.create("", outer, null).addClass('info');
    $('#maincontent').prepend(p);

    utils_wimd.show();
}

function utils_doneSaving()
{
    if (utils_wimd)
        utils_wimd.close();

    utils_wimd = false;
}


// JQuery's innerHeight method is broken when you call is
// for the window i.e. $(window).innerHeight
// This is a workaround.
function windowHeight()
{
    var w = $(window);

    var h = w.innerHeight();

    if (isNaN(h))
        h = w.height();

    // JQuery return a height of zero in safari for innerHeight
    // the window.innerHeight value is correct
    if (!h)
        if (typeof window.innerHeight != 'undefined')
            h = window.innerHeight;

    return (h);
}

function stretch(t)
{
    if (t < 10)
    {
        t = "0" + t;
    }

    return (t);
}


function shortDateTime(d, showSeconds)
{
    var result = shortDate(d) + " at " + shortTime(d, showSeconds);

    return (result);
}

function shortDate(d, showWeekday)
{
    var curYear = new Date().getFullYear();
    var date = null;
    if (curYear == d.getFullYear())
    {
        date = d.strftime("%b %e");
    }
    else
    {
        date = d.strftime("%Y %b %e");
    }

    return (date);
}

function shortTime(d, showSeconds)
{
    if (showSeconds)
        return d.strftime("%i:%M:%S%P");
    else
        return d.strftime("%i:%M%P");

}

function _shortWho(longwho)
{
    var rewho = /^(.+?)[ \t]*<.+>[ \t]*$/;

    var matches = longwho.match(rewho);

    if (matches)
        return (matches[1]);

    return (longwho);
}

function setTimeStamps(ts, div, cssClass, tdwho, tdwhen)
{
    var users = [];
    var dates = [];
    $.each(ts, function (i, v)
    {

        var d = new Date(parseInt(v.timestamp));
        var username = "";

        if ((!v.username && !v.name) && v.userid)
        {
            vCore.fetchUsers({
                success: function (data)
                {
                    for (var i = 0; i < data.length; i++)
                    {
                        if (data[i].recid == v.userid)
                        {
                            username = data[i].name;
                            break;
                        }
                    }
                }

            });

        }
        var who = v.username || v.name || username || v.userid || '';
        if (div)
        {
            var csig = $('<span>').text(who + ",  ");

            csig.append(shortDateTime(d));
            div.append(csig);
            if (!cssClass)
            {
                div.addClass("sig");
            }
        }
        else if (tdwho && tdwhen)
        {
            tdwho.text(who);
            tdwhen.append(shortDateTime(d));
        }
        else
        {
            $('#who').text(who);
            $('#when').append(shortDateTime(d));
        }

    });


}


function vvComment(text, numSummaryLines)
{
    if (!text)
        return "";

    numSummaryLines = numSummaryLines || 1;
    this.details = "";
    this.rest = "";
    this.summary = "";
    this.moreText = false;

    var formattedText = vCore.createLinks(text);
    formattedText = formattedText.replace(/\r\n/gm, "<br/>");
    formattedText = formattedText.replace(/[\r|\n]/gm, "<br/>");

    this.details = formattedText;

    var splits = formattedText.split("<br/>");
    if (numSummaryLines == 1)
    {
        this.summary = splits[0];
    }
    else
    {
        this.summary = splits.slice(0, Math.min(splits.length, numSummaryLines)).join("<br/>");
    }
    this.moreText = splits.length > numSummaryLines;
    if (this.moreText)
    {
        this.rest = "<br/>" + splits.slice(numSummaryLines).join("<br/>");
    }

}

function firstHistory(rec)
{
    var oldest = (rec.history.length > 0) ? rec.history[0] : null;

    for (var i = 1; i < rec.history.length; ++i)
        if (rec.history[i].generation < oldest.generation)
            oldest = rec.history[i];

    return (oldest);
}



function formatCSComments(comments, cs)
{
    var div = $('<div>');


    comments.sort(
			function (a, b)
			{
			    return (firstHistory(a).audits[0].timestamp - firstHistory(b).audits[0].timestamp);
			});
    $.each(comments, function (index, comment)
    {

        var cp = $('<p>');
        /*  if(cssClass)
        cp.addClass(cssClass);
        else
        cp.addClass("commentlist");*/
        var h2 = $('<h2></h2>').addClass("innerheading");
        setTimeStamps(firstHistory(comment).audits, h2, true);
        cp.append(h2);

        var cdiv = $('<div>').addClass("innercontainer");
        var vc = new vvComment(comment.text);
        cdiv.html(vc.details);

        cp.append(cdiv);


        div.append(cp);
    });

    return div;
}


function formatCommentsOnly(comments, numDisplayed, extraTop, extraBottom, cssClass)
{
    var div = $('<div>');
    var num = numDisplayed || Number.MAX_VALUE;
    $.each(comments, function (index, comment)
    {
        if (index < num)
        {
            var cp = $('<p>');
            if (cssClass)
                cp.addClass(cssClass);
            else
                cp.addClass("commentlist");
            var span = $('<span>');
            setTimeStamps(firstHistory(comment).audits, span);
            cp.append(span);
            cp.append($('<br/>'));
            var cdiv = $('<div>');
            var vc = new vvComment(comment.text);
            cdiv.html(vc.details);

            cp.append(cdiv);
            if (extraTop)
            {
                cp.prepend($('<p>'));
                cp.prepend(extraTop);
            }
            if (extraBottom)
            {
                cp.append($('<p>'));
                cp.append(extraBottom);
            }
        }
        div.append(cp);
    });

    return div;

}

// This function assumes the comments have a history. Don't use for history
// items right now
// since comment history is turned off
// This function assumes the comments have a history. Don't use for history
// items right now
// since comment history is turned off
function formatComments(comments, numDisplayed, extraTop, extraBottom, cssClass)
{

    comments.sort(
			function (a, b)
			{
			    return (firstHistory(a).audits[0].timestamp - firstHistory(b).audits[0].timestamp);
			});
    return formatCommentsOnly(comments, numDisplayed, extraTop, extraBottom, cssClass);

}

function findBaseURL(lastSeg)
{

    var url = window.location.pathname;
    if (url.indexOf(lastSeg) > 0)
    {
        url = url.substring(0, url.indexOf(lastSeg));
    }

    return url.rtrim("/");
}

var branchUrl = function(branchName, repoName, suffix){
	var base;
	if(repoName){
		base = sgLinkRoot+"/repos/"+encodeURIComponent(repoName)+"/branch/"+encodeURIComponent(branchName);
	}
	else{
		base = sgCurrentRepoUrl+"/branch/"+encodeURIComponent(branchName);
	}
	if(suffix){
		return base+suffix;
	}
	else{
		return base+".html";
	}
};

function makePrettyRevIdLink(revno, id, expandable, prepend, append, urlBase)
{
    var base = urlBase || sgCurrentRepoUrl;
    var myURL = base + "/changeset.html?recid=" + id;

    var span = $('<span>');

    var csa = $('<a></a>').
		text((revno ? (revno + ":") : "") + id.slice(0, 10))
		.attr({ href: myURL }).addClass("small_link");

    if (prepend && (typeof (prepend) == "string"))
        csa.prepend(document.createTextNode(prepend));
    else if (prepend && prepend.img)
    {
        var alt = prepend.alt || ' ';
        csa.prepend(' ');
        $('<img>').attr('src', prepend.img).attr('alt', alt).prependTo(csa);
    }
    if (append && (typeof (append) == "string"))
        csa.append(document.createTextNode(append));
    else if (append && append.img)
    {
        var alt = append.alt || ' ';
        csa.append(' ');
        $('<img>').attr('src', append.img).attr('alt', alt).appendTo(csa);
    }

    span.append(csa);

    if (expandable)
    {
        if (!append) { csa.css({ "padding-right": "1px" }); }
        span.append(addFullHidDialog(id));
    }
    return span;

}

function addFullHidDialog(id)
{
    var a = $('<a>').text("...").attr({ href: "#" });

    a.click(function (e)
    {
        e.preventDefault();

        var info = $('<input>').attr({ "type": "text", "readonly": "readonly" }).addClass("hidtext").val(id);

        var pop = new modalPopup("none");

        $('#maincontent').prepend(pop.create("", info));

        var pos = a.offset();
        pos.left += a.width() + 10;

        pop.show(null, null, pos);
        info.focus();
        info.select();
    });

    return a;
}
function makeId(childurl)
{
    var idre = /[^a-zA-Z0-9]/g;

    var url = childurl.replace(/\/[^\/]*\/?$/, '');

    var theid = url.replace(idre, "_");

    return (theid);
}

function boolFromZingVal(zingVal)
{
    if (zingVal == "1")
        return true;
    if (zingVal == "0")
        return false;
    return Boolean(zingVal);
}

function lastURLSeg(url)
{

    var p = url || window.location.pathname;
    p = p.rtrim("/");

    var ar = p.split('/');

    return (ar[ar.length - 1]);

}

function getParentPath(path)
{

    var parent = path.rtrim("/");

    var index = parent.lastIndexOf("/");

    if (index < 0)
    {

        return "@";
    }

    parent = parent.slice(0, index);

    return parent;

}

function cwd()
{
    var p = window.location.pathname;
    if (p[p.length - 1] == '/')
        return p;
    else
        return p + '/';
}

function isScrollBottom()
{
    var documentHeight = $(document).height();
    var _windowHeight = windowHeight();

    var scrollP = _windowHeight + $(window).scrollTop();

    return (scrollP + 5 >= documentHeight);
}

function isVerticalScrollRequired()
{
    var docHeight = $(document).height() - 5; // Extraordinarily lame hack to deal with IE's weird height();
    return docHeight > windowHeight();
}


function vButton(text)
{
    var span = $('<span>').addClass("button").text(text);

    return span;
}

// message can be object (span, div etc)
// optional posObject used to position the dialog
// optional onOKfunction is called when the user clicks yes
// optional args for the onOKfunction
function confirmDialog(message, posObject, onOKfunction, okargs, onNofunction, noargs, offset, oncancelfunction, closeargs)
{
    this.ok = false;
    this.offset = offset || [0, 0];
    var position = null;
    if (posObject)
        position = posObject.offset();

   
    var div = $('<div>').append(message);
    var buttonOK = new vButton("Yes");
    var buttonNo = new vButton("No");
   

    buttonOK.click(function ()
    {
        this.ok = true;
        pop.close();
        if (onOKfunction)
            onOKfunction(okargs);

    });

    buttonNo.click(function ()
    {
        this.ok = false;
        pop.close();
        if (onNofunction)
            onNofunction(noargs);

    });
      
    var buttons = $('<div>').
		append(buttonOK).
        append(buttonNo);

    //if there is also cancel functionality add the cancel button and action
    if (oncancelfunction)
    {
        var buttonCancel = new vButton("Cancel");
        buttonCancel.click(function ()
        {
            this.ok = false;
            pop.close();
            if (oncancelfunction)
                oncancelfunction(closeargs);
        });

        buttons.append(buttonCancel);
    }	

    var pop = new modalPopup(null, null, { onX: oncancelfunction });

    $('#maincontent').prepend(pop.create("Confirm Action", div, null, buttons).addClass('info'));

    pop.show(null, null, position, this.offset[0], this.offset[1]);

}

function fileViewer()
{
    this.textPatterns = [];
    this.init();
    var self = this;

}
extend(fileViewer, {

    init: function (wait)
    {
        $(window).append(this.div);
        var w = true;
        var self = this;
        if (wait)
        {
            w = false;
        }
        vCore.ajax({
            url: sgCurrentRepoUrl + "/fileclasses.json",
            async: w,
            success: function (data, status, xhr)
            {
                if (data && data[":text"])
                {
                    self.textPatterns = data[":text"].patterns;
                }
            }
        });

    },
    getCSFileLink: function (hid, name)
    {
        var a = $('<a>').text(name);
        var s = this;
        a.attr({ href: "#", 'title': "view file" });
        a.click(function (e)
        {
            e.preventDefault();
            e.stopPropagation();
            s.displayFile(hid, null, name);
        });

        return a;
    },
    displayFile: function (hid, path, file)
    {
        utils_savingNow("Loading...");
        var isText = this.checkFileType({ "path": path, "file": file });
        this._displayFile({ "hid": hid, "path": path, "file": file }, isText);
    },
    displayLocalFile: function (path)
    {
        utils_savingNow("Loading...");
        var isText = this.checkFileType({ "path": path });
        this._displayLocalFile({ "path": path }, isText);
    },
    checkFileType: function (params)
    {
        if (!this.textPatterns.length)
        {
            this.init(true);
        }
        var file = params.file || lastURLSeg(params.path);

        var isText = false;
        var segs = file.split(".");

        var test1 = "*." + segs[segs.length - 1];
        var test2 = segs[segs.length - 1];
        isText = $.inArray(test1, this.textPatterns) >= 0 || $.inArray(test2, this.textPatterns) >= 0;

        return isText;
    },
    _displayFile: function (params, isText)
    {
        var url = sgCurrentRepoUrl + "/blobs/" + params.hid + "/" + params.file;
        this._display(url, params, isText);

    },
    _display: function (url, params, isText)
    {
        if (isText)
        {
            $('#maincontent').addClass('spinning');
            var pre = $('<pre>').addClass("prettyprint");

            vCore.ajax({
                url: url,

                success: function (data, status, xhr)
                {
                    $('#maincontent').removeClass('spinning');
                    if (xhr && xhr.responseText)
                    {
                        filePopup(xhr.responseText, params.file || params.path, url);
                    }
                },
                error: function (d, s, x)
                {
                    $('#maincontent').removeClass('spinning');
                },
                complete: function ()
                {
                    utils_doneSaving();

                }
            });
        }
        else
        {
            utils_doneSaving();
            window.open(url, params.path);
        }

    },
    _displayLocalFile: function (params, isText)
    {
        var url = sgLinkRoot + "/local/file?path=" + params.path;
        this._display(url, params, isText);
    }

});

function browseFolderControl()
{
    hideIndex = 0;
    var fv = new fileViewer();

    this.creatFolderDivLink = function (container, name, cid, url, browseFunction, fromFS)
    {
        var a = $('<a>').text(name);
        a.attr({ href: "#", 'title': "view folder" });

        a.click(function (e)
        {
            container.find('li').removeClass("selected");
            $(this).parent().addClass("selected");

            e.preventDefault();
            e.stopPropagation();
            var windowWidth = $(window).width() || window.innerWidth;
            var newCtr = $(document.getElementById(cid));

            if (newCtr.length == 0)
            {
                newCtr = $('<div>');
                newCtr.attr({ id: cid });
                newCtr.addClass('dir');

                if ((container.position().left + container.width() * 3) > windowWidth)
                {
                    $('div.dir').eq(hideIndex).hide();
                    hideIndex++;
                }
                container.after(newCtr);

                if (!fromFS)
                {
                    browseFunction(url, newCtr);
                }
            }

            if (fromFS)
            {
                browseFunction(url, newCtr);
            }

            newCtr.nextAll().remove();

            if (hideIndex > 0)
            {

                if ((newCtr.position().left + newCtr.width() * 3) < windowWidth)
                {
                    $('div.dir').eq(hideIndex - 1).show();
                    hideIndex--;
                }

            }

        });

        return a;
    };

    this.creatFileDivLink = function (container, hid, name)
    {
        return fv.getCSFileLink(hid, name);
    };


}

function getQueryStringVals(url)
{
    var vars = ""
    if (url)
        vars = $.deparam.querystring(url);
    else
        vars = $.deparam.querystring();

    return vars;
}

function getQueryStringVal(name, url)
{
    var queryhash = getQueryStringVals(url);
    return queryhash[name];
}

String.prototype.trim = function (chars)
{
    return this.rtrim(chars).ltrim(chars);
};
String.prototype.ltrim = function (chars)
{
    chars = chars || "\\s";
    return this.replace(new RegExp("^[" + chars + "]+", "g"), "");
};

String.prototype.rtrim = function (chars)
{
    chars = chars || "\\s";
    return this.replace(new RegExp("[" + chars + "]+$", "g"), "");
};

String.prototype.compare = function (b)
{
    var a = this + '';
    return (a === b) ? 0 : (a > b) ? 1 : -1;
};


function getViolations(text)
{
    var v = false;

    try
    {
        eval('v = ' + text);

        if (v && !v.constraint_violations)
        {
            v = false;
        }
    }
    catch (e)
    {
        v = false;
    }

    if (v)
    {
        var errs = [];
        var viols = v.constraint_violations;

        for (var i = 0; i < viols.length; ++i)
        {
            var viol = viols[i];
            var desc = "violates a template constraint";
            var val = viol.field_value;

            if (viol.type == "unique")
                desc = "is not unique";
            else if (viol.type == "maxlength")
            {
                if (val.length > 1024)
                {
                    val = val.substring(0, 1024) + "...";
                }
                desc = "is too long";
            }

            else if (viol.type == "minlength")
                desc = "is too short";
            else if (viol.type == "prohibited")
                desc = "is prohibited";
            else if (viol.type == "allowed")
                desc = "is not an allowed value";
            else if (viol.type == "min")
                desc = "is too low";
            else if (viol.type == "max")
                desc = "is too high";

            errs.push("field '" + viol.field_name + "' " + desc + ":\r\n" + val + "\r\n");
        }

        return (errs.join("\r\n"));
    }
    else
        return (text);
}


function reportError(message, xhr, textStatus, errorThrown, options)
{
    var msg = message || "";

    msg += "\n";

    if (errorThrown)
        msg += "\n" + errorThrown + "\n";

    if (xhr && xhr.responseText)
        msg += "\n" + getViolations(xhr.responseText);

    msg = msg.replace(/^[\r\n]+/, '');
    msg = msg.replace(/[\r\n]+$/, '');

    var lines = msg.split(/\r?\n/);

    var div = $('<div />');

    for (var i = 0; i < lines.length; ++i)
    {
        if (i > 0)
            div.append($('<br />'));
        div.append(document.createTextNode(lines[i]));
    }

    var pop = new modalPopup(null, false, options);

    $('body').prepend(pop.create(textStatus, div).addClass('info'));
    pop.show();

}

function sleep(delay)
{
    var start = new Date().getTime();
    while (new Date().getTime() < start + delay);
}

function multipleEventsWaiterOuter(waitTimeInMilliseconds, onAfterLastEvent, _this, _arguments)
{
    var recentEventCount = 0;
    var expireOneAndCheckWaiting = function ()
    {
        --recentEventCount;
        if (recentEventCount === 0)
        {
            onAfterLastEvent.apply(_this || {}, _arguments || []);
        }
    }
    return {
        fire: function ()
        {
            ++recentEventCount;
            window.setTimeout(expireOneAndCheckWaiting, waitTimeInMilliseconds);
        }
    }
}


function fitToContent(text, maxHeight)
{
    var pn = $(text.parent());
    var l = text.position().left;
    var pw = pn.innerWidth();

    text.css('width', (pw - l - 50) + "px");

    var adjustedHeight = text.height();
    var relative_error = parseInt(text.attr('relative_error'));
    if (!maxHeight || maxHeight > adjustedHeight)
    {
        adjustedHeight = Math.max(text[0].scrollHeight, adjustedHeight);
        if (maxHeight)
            adjustedHeight = Math.min(maxHeight, adjustedHeight);
        if ((adjustedHeight - relative_error) > text.height())
        {
            text.css('height', (adjustedHeight - relative_error) + "px");
            // chrome fix
            if (text[0].scrollHeight != adjustedHeight)
            {
                var relative = text[0].scrollHeight - adjustedHeight;
                if (relative_error != relative)
                {
                    text.attr('relative_error', relative + relative_error);
                }
            }
        }
    }
}

function _findElement(idOrEl)
{
    if (typeof (idOrEl) == 'string')
    {
        if (idOrEl.substring(0, 1) == '#')
            return ($(idOrEl));
        else
            return ($('#' + idOrEl));
    }

    return ($(idOrEl));
}

function _spinUp(id, fillContainer)
{
    var el = _findElement(id);

    if (fillContainer)
    {
        var windowHeight = windowHeight();
        var elTop = el.offset().top;
        var elHeight = el.height();
        var fillHeight = windowHeight - elTop;

        if (elHeight < fillHeight)
            el.height(fillHeight);
    }

    $(el).addClass('spinning');
}

function _spinDown(id, fillContainer)
{
    var el = _findElement(id);

    if (fillContainer)
        el.height('auto');

    $(el).removeClass('spinning');
}

function _inputLoading(input)
{
    var el = $("<div>").addClass("smallProgress").addClass("inputLoading")
    el.css({ "float": "left" });

    input.after(el);
}

function _stopInputLoading(input)
{
    $(input).siblings(".inputLoading").first().remove();
}

function autoResizeText(ta, maxHeight)
{
    var resize = function ()
    {
        fitToContent($(this), maxHeight);
    };
    $(ta).attr('relative_error', 0);
    $(ta).each(resize);
    $(ta).keyup(resize).keydown(resize);
}


// AJAX common calls
function showDiff(url, container, showAsPopup, title)
{
    container.find('pre').remove();

    vCore.ajax({
        url: url,
        'dataType': 'text',
        success: function (data, status, xhr)
        {

            $('#maincontent').removeClass('spinning');
            var pre = $('<pre>').addClass("prettyprint");

            var lines = data.split(/\r?\n/);
            var diffText = "";

            $.each(lines, function (line, val)
            {
                if (line > 1)
                {
                    var span = $('<span>');

                    if (val.indexOf("@@") == 0)
                    {
                        span.addClass("diff_header");
                    }
                    else if (val.indexOf("+") == 0)
                    {
                        span.addClass("diff_added");
                    }
                    else if (val.indexOf("-") == 0)
                    {
                        span.addClass("diff_removed");
                    }

                    val = prettyPrintOne((val + "\n").replace(/[<]/g, "&lt;")
								.replace(/[>]/g, "&gt;"));
                    span.html(val);
                    pre.append(span);
                }
                else
                    pre.append($('<span>').addClass("diff_header").html(val).append("\n"));
            });

            container.append(pre);

            if (showAsPopup)
            {
                title = title || getQueryStringVal("file", url) || "file";

                container.vvdialog(
					{
					    "title": title,
					    "resizable": true,
					    "minWidth": 200,
					    "maxWidth": Math.floor($(window).width() * .9),
					    "width": Math.floor(Math.max(200, $(window).width() * .75)),
					    "height": Math.floor($(window).height() * .9),
					    "maxHeight": Math.floor($(window).height() * .9),
					    "autoOpen": false,

					    "buttons": {
					        "OK": function () { container.vvdialog("vvclose"); }
					    },
					    close: function (event, ui)
					    {
					        $("#backgroundPopup").hide();
					    }
					}
				);
                container.vvdialog("vvopen");
            }
        },
        error: function (d, s, x)
        {
            $('#maincontent').removeClass('spinning');
        }
    });

}

function filePopup(data, path, url)
{
    var pre = $('<pre>').addClass("prettyprint");
    var txt = data.replace(/[<]/g, "&lt;")
			.replace(/[>]/g, "&gt;");
    var content = prettyPrintOne(txt);
    $("#divFileViewerPopup").remove();
    var div = $('<div>').attr("id", "divFileViewerPopup");
    try
    {
        pre.html(content);
    }
    catch (e)
    {
        pre.text(txt);
    }

    var span = $('<span>').attr({ id: "newwindow" }).css({ "float": "right", "margin-right": "20px" });
    var rv = $('<a>').text("raw view").attr({ href: url, target: "new" });
    span.append(rv);
    //div.append(span);
    div.append(pre);

    var title = path || file;

    div.vvdialog(
		{
		    "title": title,
		    "resizable": true,
		    "minWidth": 200,
		    "maxWidth": Math.floor($(window).width() * .9),
		    "width": Math.floor(Math.max(200, $(window).width() * .75)),
		    "height": Math.floor($(window).height() * .9),
		    "maxHeight": Math.floor($(window).height() * .9),
		    "autoOpen": false,

		    "buttons": {
		        "OK": function () { div.vvdialog("vvclose"); }
		    },
		    close: function (event, ui)
		    {
		        $("#backgroundPopup").hide();
		    }
		}
	);
    div.vvdialog("vvopen");
    var titlediv = $(".ui-dialog-title");
    span.insertAfter(titlediv);
}

function isTouchDevice()
{
    var types = ["iphone", "ipod", "ipad", "android", "s60", "windows ce", "blackberry", "palm"];
    var found = false;
    var ua = navigator.userAgent.toLowerCase();

    for (var i = 0; i < types.length; i++)
    {
        found |= (ua.search(types[i]) > -1);
    }

    return found;
}

/*
* Compares two values
*/
function strcmp(a, b)
{
    return ((a == b) ? 0 : ((a > b) ? 1 : -1));
}

/*
* Test to see if a javascript object is empty (i.e. {})
*/
function isEmpty(obj)
{
    for (var prop in obj)
    {
        if (obj.hasOwnProperty(prop))
            return false;
    }

    return true;
}


/*
* Avoid jQuery behavior which forces display: block; add/remove .hidden instead (for items previously display:none, 
* we *do* set display:block if no other mode was passed
*/
function hideEl(e)
{
    $(e).addClass('hidden');
}

function showEl(e, disp)
{
    var el = $(e);
    el.removeClass('hidden');

    if ("none" == el.css('display'))
        el.css('display', disp || 'block');
}

/**
* Creates a clickable image that hides/shows other elements
*
* @param openSrc URL of the image used to show the elements
* @param closeSrc URL of the image used to hide the elements
* @param elements array of elements to be hidden/shown
* @param showAtStart if true, show the elements initially. otherwise hide.
*/
function _toggler(openSrc, closeSrc, elements, showAtStart)
{
    var i = $(document.createElement('img'));
    i.attr('src', openSrc);

    var doToggle = function (show)
    {
        if (show)
            i.attr('src', closeSrc);
        else
            i.attr('src', openSrc);

        $.each(elements,
			function (j, el)
			{
			    if (show)
			        $(el).show();
			    else
			        $(el).hide();
			}
		);

        vCore.resizeMainContent();
    };

    doToggle(showAtStart);

    i.css('cursor', 'pointer');

    i.data('open', showAtStart);

    i.click(
		function (e)
		{
		    i.data('open', !i.data('open'));
		    doToggle(i.data('open'));
		}
	);

    return (i);
}

function _addInputPlaceholder(input, msg)
{
    if ('placeholder' in input[0])
    {
        input.attr('placeholder', msg);
        return;
    }

    input.data('placeholderval', msg);

    var hidePlaceholder = function (e)
    {
        if (input.val() == msg)
            input.val('');

        input.removeClass('placeholder');
    };

    var showPlaceholder = function (e)
    {
        if (input.val() == '')
            input.val(msg);

        if (input.val() == msg)
        {
            input.addClass('placeholder');
        }
    };

    showPlaceholder();

    input.click(hidePlaceholder);
    input.blur(showPlaceholder);
    input.focus(hidePlaceholder);
}

//over ride the "modal" option and just do our own
//background modalization since ui.dialog modal and ie have problems
$.widget("custom.vvdialog", $.ui.dialog,
{
    vvopen: function ()
    {
        $("#backgroundPopup").css({
            "opacity": "0.7"
        });
        $("#backgroundPopup").show();

        this.open();

    },
    vvclose: function ()
    {
        $("#backgroundPopup").hide();

        this.close();
    }

});

/*
Quick way to define prototypes that take up less space and result in
smaller file size; much less verbose than standard
foobar.prototype.someFunc = function() lists.

@param f Function object/constructor to add to.
@param addMe Object literal that contains the properties/methods to
add to f's prototype.
*/
function extend(f, addMe)
{
    for (var i in addMe)
    {
        f.prototype[i] = addMe[i];
    }
}

function vvCapitalize(st)
{
    st = st || '';
    return (st.replace(/(?:^|\s)\S/g, function (a) { return a.toUpperCase(); }));
}

/*
* Resize text to fill a container.
* From http://stackoverflow.com/questions/687998/auto-size-dynamic-text-to-fill-fixed-size-container
*
* Set up your container like this:
* <div class="jtextfill" style="width:150px;height:40px;">
*   <span id="textInHere"></span>
* </div>
*
* Then do something like this:
* $(".jtextfill").textfill({ maxFontPixels: 36 });
*/
; (function ($)
{
    $.fn.textfill = function (options)
    {
        var fontSize = options.maxFontPixels;
        var ourText = $('span:visible:first', this);
        var maxHeight = $(this).height();
        var maxWidth = $(this).width();
        var textHeight;
        var textWidth;
        do
        {
            ourText.css('font-size', fontSize);
            textHeight = ourText.height();
            textWidth = ourText.width();
            fontSize = fontSize - 1;
        }
        while (textHeight > maxHeight || textWidth > maxWidth && fontSize > 3);
        return this;
    };
})(jQuery);

$(document).ready(
	function ()
	{
	    /* there's a lovely Firefox bug where box-sizing: border-box doesn't play nicely with minHeight, 
	    * so we fake minHeight here */
	    var fixEm = function ()
	    {
	        fixels(".innerheading, .formlabel", 23);
	        fixels(".dataheaderrow th", 18);
	        fixels(".outerheading", 28);
	    };

	    $(window).resize(fixEm);

	    fixEm();

	    // Hack to fix min-height box sizing firefox and ie bug
	    function fixels(element, height)
	    {
	        var el = $(element);

	        $.each(el, function (i, v)
	        {
	            var e = $(v);

	            var skip = false;

	            if (e.parent())
	            {
	                var na = e.parent().closest('.noadjust');

	                if (na && (na.length > 0))
	                    skip = true;
	            }

	            if ((e.height() != height) && !skip)
	                e.css('height', height);
	        }
				  );
	    }

	    var ttl = document.title || 'Veracity';
	    ttl = ttl.replace(/[ \t]*\([ \t]*\)[ \t]*/, '');
	    document.title = ttl;
	});