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

var NOBODY = "g3141592653589793238462643383279502884197169399375105820974944592";

var checkUnload = true;

(function (window, undefined)
{

    var vCore = function () { };

    /*
    * This set of functions allows you to set global configuration variables.
    *
    * This method for storing application configuration should be
    * used in place of global variables
    */
    jQuery.extend(vCore, {
        options: {
            debug: true // Hardcode debug for now
        },

		sanitizeRE: /([\u0000-\u0008]|[\u000b-\u000c]|[\u000e-\u001F]|[\u007F-\u0084]|[\u0086-\u009F]|[\uD800-\uDFFF]|[\uFDD0-\uFDEF])/g,

        setOption: function (key, value)
        {
            this.options[key] = value;
        },

        getOption: function (key)
        {
            return this.options[key];
        }
    });

    /*
    * Debugging Functions
    */

    jQuery.extend(vCore, {

        // Log a message to the javascript console
        consoleLog: function (msg)
        {
            if (!this.getOption("debug"))
                return;

            try
            {
                if (window.console && console.log)
                {
                    console.log(msg);
                }
                else if (window.opera && opera.postError)
                {
                    opera.postError(msg);
                }
            }
            catch (e)
            {
            }
        }
    });

    /**
    * Cookie Management Functions
    */
    jQuery.extend(vCore, {

        setCookie: function (name, value, expiration)
        {
            var exdate = new Date();
            exdate.setDate(exdate.getDate() + expiration);

            var cstr = name + "=" + encodeURIComponent(value) +
			"; path=/" +
			"; expires=" + exdate.toUTCString();

            document.cookie = cstr;
        },

        getCookie: function (name, defaultVal)
        {
            var un = name + "=";
            var v = defaultVal || false;

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
        },

        getJSONCookie: function (name, defaultVal)
        {

            var _cookie = this.getCookie(name, defaultVal);
            if (_cookie == defaultVal)
                return _cookie;
            if (!_cookie)
                return "";
            return JSON.parse(_cookie);
        },

        getCookieValueForKey: function (name, key, defaultVal)
        {
            var obj = this.getJSONCookie(name, defaultVal);

            if (!obj)
                return null;

            return obj[key] || defaultVal;
        },

        setCookieValueForKey: function (name, key, value, expiration)
        {
            var obj = this.getJSONCookie(name);

            if (!obj)
                obj = {};

            obj[key] = value;

            _cookie = JSON.stringify(obj);

            this.setCookie(name, _cookie, expiration);
        },

        deleteCookieKey: function (name, key)
        {
            var obj = this.getJSONCookie(name);
            if (obj)
                delete obj[key];

            _cookie = JSON.stringify(obj);
            this.setCookie(name, _cookie, 30);
        },

        setCookieValuesFromObject: function (name, obj, expiration)
        {
            var cobj = this.getJSONCookie(name);

            if (!cobj)
                cobj = {};

            $.each(obj, function (key, value)
            {
                cobj[key] = value;
            });

            var _cookie = JSON.stringify(cobj);

            this.setCookie(name, _cookie, expiration);
        },

		getGlobalDefaultCookieName: function()
		{
			return "vvdefaults";
		},

        getDefaultCookieName: function ()
        {
            var prefix = startRepo || "";
			prefix = encodeURIComponent(prefix).replace(/\%/g, '_');
            return prefix + "defaults";
        },

		getAdminCookieName: function()
	    {
			var prefix = vCore.getOption("adminId") || '';
			return prefix + "admindefaults";
		}
    });

    /**
    * URL Generation Methods
    */

    jQuery.extend(vCore, {

        linkRoot: function ()
        {
            return this.getOption("linkRoot");
        },

        staticRoot: function ()
        {
            return this.getOption("staticLinkRoot");
        },

        repoRoot: function ()
        {
            return this.getOption("currentRepoUrl");
        },

        /**
        * Create the requested URL for a record
        *
        * This method will create a URL for the given options.  It accepts
        * either two or three arguments.
        *
        *  vCore.urlForRecord(record, urlType);
        *
        *  or
        *
        *  vCore.urlForRecord(rectype, <recid || record>, urlType);
        */
        urlForRecord: function ()
        {
            if (arguments.length <= 2)
            {
                if (typeof arguments[0] != "object")
                    return null;

                var recid = arguments[0].recid;
                var rectype = arguments[0].rectype;
                var _urlType = arguments[1] || "view";
            }
            else
            {
                var rectype = arguments[0];
                if (arguments[1] == null)
                {
                    var _urlType = "create";
                }
                else
                {
                    if (typeof arguments[1] == "object")
                        var recid = arguments[1].recid;
                    else
                        var recid = arguments[1];

                    var _urlType = arguments[2];
                }
            }

            //TODO HACK map rectype to url
            if (rectype == "item")
                rectype = "workitem";
            if (rectype == "sprint")
                rectype = "milestone";

            if (_urlType == "create")
                return this.repoRoot() + "/" + rectype;
            else if (_urlType == "update")
                return this.repoRoot() + "/" + rectype + "/" + recid;
            else
                return this.repoRoot() + "/" + rectype + "/" + recid + ".json";

        }
    });

    /*
    * This set of functions handles the fetching of sets of records
    */
    jQuery.extend(vCore, {

        ajaxCallCount: 0,
        ajaxWriteCount: 0,

        defaultAjaxOptions:
		{
		    reportErrors: true,
		    redirectOnAuthFail: true,
            abortOK: false
		},

        /**
        * Wraps all Ajax calls.
        *
        * - Uses a single error-reporting stream for all ajax errors
        * - Inserts a user-identifying From: header in all ajax calls
        * - Attempts to cause server-side caching of any GET requests
        *   (unless you insist otherwise)
        *
        * the options object is the same as that used by jQuery.ajax(),
        * with the following additional members
        *
        * .errorMessage - if seen, this is used as the title/header for
        *                 the error message popup
        * .noErrorReport - if true, we don't display a message on error
        *
        * To avoid server-side cache attempts, and kick in jQuery's cache-prevention
        * logic, set options.cache = false.
        */
        ajax: function (opts)
        {
            var options = $.extend(true, {}, this.defaultAjaxOptions, opts);

			if(opts.isWriteCall===undefined){
				var cacheable = (!options.type) || (options.type.toLowerCase() == "get") || (options.type.toLowerCase() == "head");
				options.isWriteCall = !cacheable;
			}

            options.cache = false;

            var olderror = options.error;
            var oldsuccess = options.success;
            var oldbeforesend = options.beforeSend;
            var abortOK = options.abortOK;

            options.beforeSend = function (xhr)
            {
                if (window.sgUserName && ! options.noFromHeader)
                    xhr.setRequestHeader('From', sgUserName);

                if (oldbeforesend)
                    oldbeforesend(xhr);
            },

			options.error = function (xhr, status, errorthrown)
			{
			    if (options.isWriteCall)
			        vCore.ajaxWriteCount--;

			    vCore.ajaxLog(options.url, false, false);
			
				//don't redirect for things that auto reload in the background 
				//like activity stream. 
			    if (xhr.status == 401 && options.redirectOnAuthFail)
			    {	
			    	var redir_base = vCore.options.redirUrlBase;
			    	if (redir_base)
			    	{
			    		var qs = "";					
	
						var pathAndQuery = window.location.pathname + window.location.search;
						if (pathAndQuery)
							qs = '?redir=' + pathAndQuery;
							
						window.location.href = redir_base + qs;
				    	return;
			    	}			    		
			    }
			    
			    if (olderror)
			        olderror(xhr, status, errorthrown);
			        
			    var isGet = (!options.type || options.type.toLowerCase() == "get" || abortOK);
			    var isAbortedGet = isGet && xhr.status == 0;

			    if (options.reportErrors && !isAbortedGet)
			    {
					var ctype = xhr.getResponseHeader('Content-Type') || '';

					if (ctype.match(/^text\/x?html/i))
						{
							xhr.responseText = 'An error occurred processing your request';
							errorthrown = null;
							status = 'Error';
						}

					reportError(options.errorMessage || options.url, xhr, vvCapitalize(status), errorthrown);
				}
			};

            options.success = function (data, status, xhr)
            {
           
                if (options.isWriteCall)
                    vCore.ajaxWriteCount--;
			
                vCore.ajaxLog(options.url, false, true);

                // if the server doesn't respond, under some browsers we end up here
                // with a 0 status, instead of in the error handler
                if (xhr.status == 0)
                {
                    options.error(xhr, status);
                    return;
                }

                if (oldsuccess)
                    oldsuccess(data, status, xhr);
            };

            var oldcomplete = options.complete;

            options.complete = function (xhr, status)
            {
                vCore.ajaxCallCount--;

                if (oldcomplete)
                    oldcomplete(xhr, status);
            };

            vCore.ajaxCallCount++;
            if (options.isWriteCall)
                vCore.ajaxWriteCount++;

            vCore.ajaxLog(options.url, true);
            $.ajax(options);
        },

        ajaxLogLevel: 0,

        ajaxLog: function (url, starting)
        {
            var msg = (new Date()).getTime() + " ";
            for (var i = 0; i < vCore.ajaxLogLevel; ++i)
                msg += "+ ";

            msg += "ajax " + (starting ? "start" : "end") + ": " + url;

            if (starting)
                ++vCore.ajaxLogLevel;
            else
                if (vCore.ajaxLogLevel > 0)
                    --vCore.ajaxLogLevel;

            //            vCore.consoleLog(msg);
        },

        fetchOptions: {
            dataType: 'json',
            includeLinked: false,
            success: null,
            error: null,
            justNames: false //don't add inactive to the name, just return the raw name
        },

        _users: {},
		_inactiveUsers: {},
        _sprints: null,

        setUsers: function (users)
        {
			if (this._users[''])
				return;

			var inactives = [];
			var actives = [];

			for ( var i = users.length - 1; i >= 0; --i )
			{
				users[i].value = users[i].value || users[i].recid;

				if (users[i].inactive)
				{
					users[i].name += " (inactive)";
					inactives.push(users[i]);
				}
				else
					actives.push(users[i]);
			}

			this._users[''] = actives;
			this._inactiveUsers[''] = inactives;
		},

        fetchUsers: function (options)
        {
            var s = $.extend(true, {}, vCore.fetchSettings, options);
          
            var repo = options.repo || '';
			var val = options.val || '';

            if (vCore._users[repo] && !options.force)
            {
                if (s.success)
                    s.success(vCore._users[repo], vCore._inactiveUsers[repo] || [], val);
                if (s.complete)
                    s.complete();
                return;
            }
			
            var success = function (data)
            {
                var realUsers = [];
			 var inactiveUsers = [];
                $.each(data, function (i, u)
                {
                    if (u.recid != NOBODY)
                    {
						if (u.inactive)
						{
							u.value = u.recid;
                            if (!s.justNames)
                            {
						        u.name += " (inactive)";
                            }
							inactiveUsers.push(u);
						}
						else
							realUsers.push(u);
                    }
                });
                vCore._users[repo] = realUsers;
				vCore._inactiveUsers[repo] = inactiveUsers;
                if (s.success)
                    s.success(realUsers, inactiveUsers, val);
            };

            var uurl = this.getOption("currentRepoUrl") + "/users.json";

            if (repo)
                uurl = uurl.replace('/repos/' + encodeURIComponent(this.getOption('repo')), '/repos/' + encodeURIComponent(repo));

            vCore.ajax({
				reportErrors: s.reportErrors,
                url: uurl,
                dataType: 'json',
                noFromHeader: true,
                success: success,
                error: s.error,
                complete: s.complete
            });
        },

        setSprints: function (sprints)
        {
            if (vCore._sprints)
                return;

            vCore._sprints = sprints;
        },

        fetchSprints: function (options)
        {
            var s = $.extend(true, {}, vCore.fetchSettings, options);

            if (vCore._sprints && !options.force)
            {
                if (s.success)
                    s.success(vCore._sprints);
                if (s.complete)
                    s.complete();
                return;
            }

            var success = function (data, status, xhr)
            {
                vCore._sprints = data;
                if (s.success)
                    s.success(data, status, xhr);
            };


            var uurl = this.getOption("currentRepoUrl") + "/milestones.json?_sort=enddate&_desc=1";
            if (s.releaseOnly)
                uurl += '&_unreleased=true';

            if (s.includeLinked)
                uurl += "&_includelinked=true";

            vCore.ajax({
                url: uurl,
				reportErrors: s.reportErrors,
                dataType: 'json',
                success: success,
                error: s.error,
                complete: s.complete
            });
        },

        fetchRepos: function (options)
        {
            var s = $.extend(true, {}, vCore.fetchSettings, options);

            var url = this.getOption("linkRoot") + "/repos.json";

			vCore.ajax({
				reportErrors: s.reportErrors,
                url: url,
                dataType: 'json',
				noFromHeader: true,
                success: function(data) {
					data = $.map(data, function(v) { return v["descriptor"]; });
					s.success(data);
				},
                error: s.error,
                complete: s.complete
            });
        },
		
        fetchFilters: function (options)
        {
            var s = $.extend(true, {}, vCore.fetchSettings, options);

	    if (! this.getOption("currentRepoUrl"))
		return;

            var url = this.getOption("currentRepoUrl") + "/filters.json";

            if (s.max)
            {
                url += "?max=" + s.max;
            }
            if (s.details)
            {
                url += "?details";
            }

            vCore.ajax({
                url: url,
                dataType: 'json',
                success: s.success,
                error: s.error,
                complete: s.complete
            });
        }
    });

    /**
    * This set of methods handles the fetching of singular records
    */
    jQuery.extend(vCore, {

        singleFetchOptions: {
            success: null,
            error: null,
            includeLinked: false,
            dataType: "json"
        },

        fetchItem: function (recid, options)
        {
            if (!recid)
            {
                vCore.consoleLog("fetchRecor Error: null recid");
                return; // TODO this should probably display an error
            }

            var s = $.extend(true, {}, vCore.singleFetchOptions, options);

            url = this.getOption("currentRepoUrl") + "/workitem/" + recid + ".json";

            if (s.includeLinked)
            {
                url += "?_includelinked=1";
            }

            vCore.ajax({
                url: url,
                dataType: s.dataType,
                success: s.success,
                error: s.error,
                complete: s.complete
            });
        },

        fetchSprint: function (recid, options)
        {
            if (!recid)
            {
                vCore.consoleLog("fetchRecor Error: null recid");
                return; // TODO this should probably display an error
            }

            var s = $.extend(true, {}, vCore.singleFetchOptions, options);

            url = this.getOption("currentRepoUrl") + "/milestone/" + recid + ".json";

            if (s.includeLinked)
            {
                url += "?_includelinked=1";
            }

            vCore.ajax({
                url: url,
                dataType: s.dataType,
                success: s.success,
                error: s.error,
                complete: s.complete
            });
        }
    });

    jQuery.extend(vCore, {
        setConfig: function (key, value, options)
        {
            var s = $.extend(true, {}, options);


            var rec = {
                key: key,
                value: value
            }

            var url = this.getOption("currentRepoUrl") + "/config/" + key + ".json";
            vCore.ajax({
                url: url,
                type: "PUT",
                data: JSON.stringify(rec),
                success: s.success,
                error: s.error,
                complete: s.complete
            })
        },

        getConfig: function (key, options)
        {
            var s = $.extend(true, {}, options);

            var url = this.getOption("currentRepoUrl") + "/config/" + key + ".json";

            vCore.ajax({
                url: url,
                dataType: "json",
                success: s.success,
                error: s.error,
                complete: s.complete
            })
        }
    });
    /*
    * Layout methods
    */
    (function ()
    {
        // Constructor
        function VVLayout()
        {
            var self = this;
            // Bind events
            $(window).resize(function ()
            {
                self.resizeMainContent();
                self.setMenuHeight();
            });

            $(document).ready(function ()
            {
                self.resizeMainContent();

                $("#activity").bind("resize", self.resizeMainContent);
                $("#rightSidebar").bind("resize", self.resizeMainContent);

                $("#maincontent").ajaxComplete(function(e, xhr, settings) 
                {
                    //don't trigger the resize for things like heartbeat.json since that
                   // makes the page jumpy                 
                    if (!settings.no_ui)
                    {
                        self.resizeMainContent();
                    }
                });

               

                self.setMenuHeight();
            });
        }

        extend(VVLayout, {

            // Display the left hand sidebar
            showSidebar: function ()
            {
                var width = $(window).width() - 300;
                $("#rightSidebar").show();
                $("#rightSidebar").trigger("resize");
            },

            // Hide the left hand sidebar
            hideSidebar: function ()
            {
                $("#pagebody").css("background", "none");
                $("#rightSidebar").hide();
                $("#rightSidebar").trigger("resize");
            },

            /*
            * Resize the maincontent div
            *
            * This method will set the width and margin of the
            * maincontent div taking into account the other items
            * visible on the page.
            */
            resizeMainContent: function ()
            {
                var main = $("#maincontent");
                var wwidth = $(window).width();
                var actWidth = $("#activity").width();

                var sidebarWidth = 0;

                if ($("#rightSidebar").css("display") != "none")
                {
                    sidebarWidth = $("#rightSidebar").width();
                }

                var padding = main.outerWidth() - main.width();
                var newWidth = Math.ceil(wwidth - actWidth - sidebarWidth - padding);
				actWidth = Math.round(actWidth);

				var diff = Math.abs(newWidth - main.width());

                // Make sure the size actually changes so we aren't
                // sending an unnecessary resize event (ahem... IE)
                if (diff > 1)
                {
                    $("#contentWrapper").css({ "margin-left": actWidth, "width": wwidth - actWidth });
                    main.css({ width: newWidth });
                    main.trigger("resize");
                }

                // Hack for when user uses text zoom.
                if ($("#rightSidebar").offset())
                {
                    if ($("#rightSidebar").offset().top > 100)
                    {
                        main.css({ width: newWidth - 8 });
                    }
                }

				if (this.afterResizeMainContent)
					this.afterResizeMainContent();
            },

            // Set the minimum width for the #top div
            // This prevents the menu from wrapping to multiple lines if the
            // window is too small
            setMenuHeight: function ()
            {
                var self = this;
                if (!$("#topmenu").is(":visible") || !$("#identity li").is(":visible"))
                {
                    setTimeout(function () { self.setMenuHeight(); }, 1000);
                    return;
                }

				var tm = $('#topmenu');
				var id = $('#identity');
				var slack = 20;

				if (! self.needed)
				{
					id.find('.widest').removeClass('hidden');
					self.needed = tm.width() + id.width();
				}
				
				var avail = ($(window).width() - tm.offset().left) - slack;

				if (self.needed <= avail)
					id.find('.widest').removeClass('hidden');
				else
					id.find('.widest').addClass('hidden');

				return;

                var minWidth = $("#logo").outerWidth(true) + $("#topmenu").outerWidth(true) + $("#identity li").outerWidth(true) + 5;
                if ($(window).width() < minWidth)
                {
                    $("#top").height(34 * 2 + 2);
                    $("#logo a").height(36);
                    $("#maincontent").css("margin-top", "94px");
                    $("#rightSidebar").css("margin-top", "97px");
                    $("#activity").css("margin-top", "36px");
                    if ($(window).width() < $("#topmenu").outerWidth(true))
                        $("#identity").hide();
                    else
                        $("#identity").show();
                }
                else
                {
                    $("#top").height(34);
                    $("#logo a").height(34);
                    $("#maincontent").css("margin-top", "60px");
                    $("#rightSidebar").css("margin-top", "63px");
                    $("#activity").css("margin-top", "0px");
                }

                $("body").css("min-width", $("#topmenu").outerWidth(true));
            }
        });

        vCore.layout = new VVLayout();

    })();

    /*
    * Shortcut methods to access layout functions from the global vCore object
    */
    jQuery.extend(vCore, {
        showSidebar: function ()
        {
            vCore.layout.showSidebar();
        },

        hideSidebar: function ()
        {
            vCore.layout.hideSidebar();
        },

        setMenuHeight: function ()
        {
            vCore.layout.setMenuHeight();
        },

        resizeMainContent: function ()
        {
            vCore.layout.resizeMainContent();
        },

        refreshLayout: function ()
        {
            vCore.layout.setMenuHeight();
            vCore.layout.resizeMainContent();
        }
    });

    /**
    * blur / page exit handling 
    */
    jQuery.extend(vCore,
	{
	    vvIsBusy: function (e)
	    {
	        return (vCore.ajaxWriteCount > 0);
	    },

	    vvBusyMessage: "Veracity is busy right now. Leaving this page might leave your work in an unpredictable state.\r\n" +
						"Are you sure you want to leave this page right now?",

	    catchBeforeUnload: function (e)
	    {
	        if (vCore.vvIsBusy())
	        {
	            var ev = e || window.event;

	            if (ev)
	                ev.returnValue = vCore.vvBusyMessage;

	            return (vCore.vvBusyMessage);
	        }
	    }
	});

    $(window).bind('beforeunload', vCore.catchBeforeUnload);

    /**
    * Date and Time Methods
    */
    jQuery.extend(vCore, {
        dayMs: (60 * 60 * 24 * 1000),

        workingDays: function (start, end)
        {
            if (!start || !end)
                return null;

            var _date = this.dateToDayStart(start);
            var enddate = this.dateToDayEnd(end);
            var days = 0;

            while (_date.getTime() <= enddate.getTime())
            {
                var d = _date.getDay();
                if (d != 0 && d != 6)
                    days++;

                _date.setDate(_date.getDate() + 1);
            }

            return days;
        },

        nextWorkingDay: function (date)
        {
            var _d = new Date(date.getTime());
            _d.setDate(_d.getDate() + 1);
            while ((_d.getDay() == 0 || _d.getDay() == 6))
            {
                _d.setDate(_d.getDate() + 1);
            }

            return _d
        },

        /*
        * Returns a new Date object with the time set to 23:59:59
        */
        dateToDayEnd: function (date)
        {
            if (!date)
                return null;

            var d = new Date(date.getTime());
            d.setHours(23);
            d.setMinutes(59);
            d.setSeconds(59);
            d.setMilliseconds(999);
            return d;
        },

        /*
        * Returns a new Date object with the time set to 00:00:00
        */
        dateToDayStart: function (date)
        {
            if (!date)
                return null;

            var d = new Date(date.getTime());
            d.setHours(0);
            d.setMinutes(0);
            d.setSeconds(0);
            d.setMilliseconds(0);
            return d;
        },

        /*
        * Helper function to get a given dates timestamp at 00:00:00
        */
        timestampAtDayStart: function (d)
        {
            d = this.dateToDayStart(d);
            if (d)
                return (d.getTime());

            return null;
        },

        /*
        * Helper function to get a given dates timestamp at 23:59:59
        */
        timestampAtDayEnd: function (d)
        {
            d = this.dateToDayEnd(d);

            if (d)
                return (d.getTime());

            return null;
        },

        /*
        * Helper function to determine if two dates represent the same day
        */
        areSameDay: function (a, b)
        {
            var same = true;

            same = same && (a.getYear() == b.getYear());
            same = same && (a.getMonth() == b.getMonth());
            same = same && (a.getDate() == b.getDate());

            return same;
        },

        /**
        * Formats a date using the format configured in vCore
        *
        * If no format exists it will default to mm/dd/YY
        *
        */
        formatDate: function (date, fmt)
        {
            var format = fmt || vCore.getOption("dateFormat");
            if (format)
            {
                return date.strftime(format);
            }
            else
            {
                return date.strftime("%D");
            }
        }

    });

    /**
    * Link scraping
    **/
    jQuery.extend(vCore, {
        htmlEntityEncode: function (text)
        {
            html = $(document.createElement("pre")).text(text).html();
            var id = "#vCoreTestDiv" + Math.floor(Math.random() * 10000001)
            var testDiv = $(document.createElement("pre")).attr("id", id);
            try
            {
                $("body").append(testDiv);
                testDiv.html(html);
                testDiv.remove();
                return html;
            }
            catch (e)
            {
                testDiv.remove();
                return this.stripInvalidChars(html);
            }
        },

        // URL encode that goes further that Javascript's built in encodeURI()
        urlEncode: function (text)
        {
            var _out = encodeURI(text);
            _out = _out.replace(/#/g, "%23");
            _out = _out.replace(/\$/g, "%24");
            _out = _out.replace(/&/g, "%26");
            _out = _out.replace(/=/g, "%3D");

            return _out;
        },

        stripInvalidChars: function (text)
        {
            exp = /[^\u0009\u000A\u000D\u0020-\uD7FF\uE000-\uFFFD\u10000-\u10FFFF]/g;
            text = text.replace(exp, "")
            return text;
        },

        createLinks: function (text)
        {
            if (!text)
                return "";

            text = this.htmlEntityEncode(text);
            
            var exp = /(\b(https?|ftp|file|cal|dav|tel|fax|callto|mailto|rtsp|webcal|cvs|git|svn|svn(\+ssh)?|bzr|feed|smb|afp|nfs|rtmp|mms|gopher):\/\/[-A-Z0-9+&@#\/%?=~_|!:,.;()\[\]{}^']*[-A-Z0-9+&@#\/%=~_|;])/ig;
            linked = text.replace(exp, "<a href=\"$1\">$1</a>");
            
            return linked;
        }
    });
    jQuery.extend(vCore, {
    	    wikiLinks: function(text, wikiUrl) {
		    text = text || '';
		    return(
			    text.replace(/\[\[([^\[\]]+)\]\]/g,
						     function(str, p1) {
							     var p = encodeURIComponent(p1);
							     var res = "[" + p1 + "](" + wikiUrl + "?page=" + p + ")";
							     return( res );
						     }
						    )
		    );
	    },

	    bugLinks: function(text) {
		    text = text || '';
		    return(
			    text.replace(
					    /(^|[^\\a-zA-Z0-9])\^([a-zA-Z]+[0-9]+)/g,
				    function(str, preamble, bugid) {
					    var bid = encodeURIComponent(bugid);
					    return( preamble + "[" + bugid + "](" + sgCurrentRepoUrl + "/wiki/workitem/" + bid + ".json)" );
				    }
			    )
		    );
	    },

        filterLinks: function(text) {
            text = text || '';
            return(
                text.replace(
                        /\bfilter:\/\/([a-zA-Z0-9]+)\b/g,
                    function(str, filterid) {
                        return sgCurrentRepoUrl + "/workitems.html?filter=" + filterid;
                    }
                )
            );
        },

        branchLinks: function(text) {
            text = text || '';

            text = text.replace(
                    /\bbranch:\/\/"([^"]+)"/g,
                    function(str, branchname) {
                        return branchUrl(branchname);
                    }
            );
            text = text.replace(
                    /\bbranch:\/\/([^\]\)\t \r\n]+)/g,
                    function(str, branchname) {
                        return branchUrl(branchname);
                    }
            );

            return text;
        },

        tagLinks: function(text) {
            text = text || '';

            text = text.replace(
                    /\btag:\/\/"([^"]+)"/g,
                    function(str, tagname) {
                        return sgCurrentRepoUrl + "/tag/" + encodeURIComponent(tagname) + ".html";
                    }
            );
            text = text.replace(
                    /\btag:\/\/([^\]\)\t \r\n]+)/g,
                    function(str, tagname) {
                        return sgCurrentRepoUrl + "/tag/" + encodeURIComponent(tagname) + ".html";
                    }
            );

            return text;
        },

	    csLinks: function(text) {
		    var remainder = text || '';
		    text = "";
		    var linkRe = /^([\s\S]*?)(\\?@)((?:[a-zA-Z_\-0-9%']+:)?)([a-f0-9]+)([\s\S]*)$/;

		    while (remainder)
		    {
			    var matches = remainder.match(linkRe);

			    if (! matches)
			    {
				    text += remainder;
				    remainder = "";
				    break;
			    }

			    var before = matches[1];
			    var preamble = matches[2];
			    var repo = matches[3];
			    var csid = matches[4];
			    var after = matches[5];

			    text += before;

			    if (preamble == "\\@")
			    {
				    text += "@";
				    remainder = repo + csid + after;
			    }
			    else
			    {
				    var repUrl = sgCurrentRepoUrl;

				    if (repo)
					    repUrl = repUrl.replace("repos/" + encodeURIComponent(startRepo), "repos/" + encodeURIComponent(repo.replace(/:$/, '')));
				    else
					    repo = '';

				    var bid = csid.slice(0,8);

				    text += preamble + "[" + repo + bid + "](" + repUrl + "/changeset.html?recid=" + csid + ")";
				    remainder = after;
			    }
		    }

		    return(text);
	    }

    });
    /**
    * Utilities
    */
    jQuery.extend(vCore, {
        /**
        * Normalize event.which, turning IE values into non-IE values:
        * 1: left
        * 2: middle
        * 3: right
        */
        whichClicked: function (which)
        {
            if ($.browser.msie)
            {
                switch (which)
                {
                    case 0: return 1;
                }
            }
            return which;
        },
        //set page title where
        //in the format: <optional page name> value (repoName)
        
        setPageTitle: function(value, pageName)
        {
            document.title = (pageName ? pageName + " " : "") + value + " (" + startRepo + ")";
        },

		sanitize: function(txt)
		{
			txt = txt || '';

			if (typeof(txt) == 'string')
				return( txt.replace(vCore.sanitizeRE, "?") );
			else
				return(txt);
		}
    });

	/**
	 * routines for modules to extend initialized pages
	 */
	jQuery.extend(vCore, {
		bind: function(event, fn) {
			if (! vCore.bindings)
				vCore.bindings = {};

			if (! vCore.bindings[event])
				vCore.bindings[event] = [];
			
			vCore.bindings[event].push(fn);
		},

		trigger: function(event, data) {
			if (! vCore.bindings)
				vCore.bindings = {};

			var ko = vCore.bindings[event] || [];

			for ( var i = 0; i < ko.length; ++i )
				ko[i](data);
		}

	});

    (function ()
    {

        var milestoneTree = function (milestones)
        {
            this.root = new milestoneTreeNode();
            this.root.title = "Product Backlog";
            this.length = 0;
            this.unattachedNodes = [];

            if (milestones && milestones.length)
            {
                for (var i = 0; i < milestones.length; i++)
                {
                    this.insertMilestone(milestones[i]);
                }
            }
        }

        extend(milestoneTree, {
            reset: function ()
            {
                this.root = new milestoneTreeNode();
                this.root.title = "Product Backlog";
                this.unattachedNodes = [];
            },

            insertMilestone: function (milestone)
            {
                var recid = milestone.recid;
                var parent_recid = milestone.parent;

                var node = new milestoneTreeNode(milestone.recid, milestone);
                this.length++;
                if (!parent_recid)
                {
                    this.root.addChild(node);
                    this._retryUnattached();
                    return;
                }

                if (!this.inTree(parent_recid))
                {
                    this.unattachedNodes.push(node);
                    return;
                }

                var parentNode = this.root.getNode(parent_recid)
                parentNode.addChild(node);
                this._retryUnattached();


            },

            _retryUnattached: function ()
            {
                var unparentedNodes = [];
                for (var i = 0; i < this.unattachedNodes.length; i++)
                {
                    var node = this.unattachedNodes[i];
                    if (this.inTree(node.data.parent))
                    {
                        var parentNode = this.root.getNode(node.data.parent)
                        parentNode.addChild(node);
                    }
                    else
                    {
                        unparentedNodes.push(node);
                    }
                }

                this.unattachedNodes = unparentedNodes;
            },

            getMilestones: function ()
            {
                return this.root.getData();
            },

            getSubTree: function (recid)
            {
                var tree = new milestoneTree();
                tree.root = this.root.getNode(recid);
                var nodes = this.root.getData();
                tree.length = nodes.length;

                return tree;
            },

            inTree: function (recid)
            {
                return this.root.inTree(recid);
            },

            getMilestone: function (recid)
            {
                return this.root.getNode(recid);
            },

            removeMilestone: function (recid)
            {
                this.root.removeNode(recid);
            }
        });

        function milestoneTreeNode(key, data)
        {
            this.key = key || null;
            this.data = data || null;

            if (this.data)
                this.title = this.data.name;

            this.children = [];
            this.parent = null;
        }

        extend(milestoneTreeNode, {
            addChild: function (child)
            {
                child.parent = this;
                this.children.push(child);
                this.children.sort(function (a, b)
                {
                    if (a.data.startdate == b.data.startdate)
                        return 0;
                    else if (a.data.startdate < b.data.startdate)
                        return -1;
                    else
                        return 1;
                });
            },

            removeChild: function (child)
            {
                for (var i = 0; i < this.children.length; i++)
                {
                    if (child.key == this.children[i].key)
                    {
                        this.children.splice(i, 1);
                        return;
                    }
                }
            },
            numParents: function ()
            {
                var i = 0;
                var p = this.parent;
                while (p != null)
                {
                    i++;
                    p = p.parent;
                }
                return i;
            },
            getAllChildren: function (children)
            {
                node = this;
                children = children.concat(node.children);
                if (node.hasChildren())
                {                    
                    for (var i = 0; i < node.children.length; i++)
                    {
                        node.children[i].getAllChildren(children);
                    }
                }  
                                       
                return children;
            },
            setParent: function (newparent)
            {
                this.parent.removeChild(this);
                newparent.addChild(this);
            },

            hasChildren: function ()
            {
                return this.children.length > 0;
            },

            getNode: function (key)
            {
                if (this.key == key)
                    return this;

                for (var i = 0; i < this.children.length; i++)
                {
                    var child = this.children[i];
                    var node = child.getNode(key);
                    if (node)
                        return node;

                }

                return false;
            },

            inTree: function (key)
            {
                if (this.key == key)
                    return true;

                for (var i = 0; i < this.children.length; i++)
                {
                    var child = this.children[i];

                    if (child.inTree(key))
                        return true;

                }

                return false;
            },

            removeNode: function (key)
            {
                for (var i = 0; i < this.children.length; i++)
                {
                    var child = this.children[i];

                    if (child.key == key)
                        this.removeChild(child);
                    else
                        child.removeNode(key);
                }
            },

            getData: function ()
            {
                var data = this.data ? [this.data] : [];
                for (var i = 0; i < this.children.length; i++)
                {
                    data = data.concat(this.children[i].getData());
                }

                return data;
            }
        });

        vCore.milestoneTree = milestoneTree;
    })();
    // Expose vCore to the global object
    window.vCore = vCore;
})(window);
