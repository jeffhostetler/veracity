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

var user_authenticated = false;
function _savedEmail()
{
	var cname = vCore.getAdminCookieName();
	return vCore.getCookieValueForKey(cname, "email", null);
}

function _savedUser()
{
	var cname = vCore.getAdminCookieName();
	return vCore.getCookieValueForKey(cname, "uid", null);
}

function _saveUser(uid, email)
{
	var cookieName = vCore.getAdminCookieName();

	sgUserName = uid;
	sgUserEmail = email;

	if (uid && email)
	{
		var obj = {
			"uid": uid,
			"email": email
		}
		
		vCore.setCookieValuesFromObject(cookieName, obj, 30);
	}
	if (email)
	{
		$("#useremail").text(email);
	}
}

function _removeUserCookies()
{
    var cname = vCore.getDefaultCookieName();
    vCore.setCookie("sgfilterresults", '', -1);
    vCore.setCookie("sglastquery", '', -1);
    vCore.setCookieValueForKey(cname, "visitedFilters", [], -1);
    vCore.setCookieValueForKey(cname, "visitedItems", [], -1);
    vCore.setCookie("sgVisitedRepos", [], -1);
    var cname = vCore.getAdminCookieName();
    vCore.setCookie(cname, '', -1);
}


function _hookUserSelector() {
	var cname = vCore.getAdminCookieName();
	var cookie = vCore.getJSONCookie(cname);
	
	if (!!cookie.email)
	{
		sgUserEmail = cookie.email;
		vCore.setOption("userEmail", sgUserEmail);
	}
	
	if (!!cookie.uid)
	{
		sgUserName = cookie.uid;
		vCore.setOption("userName", sgUserName);
	}
	
	var select = $('#userselection');
	select.val(sgUserName);

	$('#useremail').text(sgUserEmail);
	
	select.change(
		function (e)
		{
		    var email = $("#userselection option:selected").text();
		    //delete cookies for this user
		    _removeUserCookies();
		    _saveUser($(this).val(), email);

			var newloc = getQueryStringVal('redir');

			if (newloc)
				window.location.href = vCore.getOption('linkRoot') + newloc;
			else
				window.location.reload();
		}
	);
}

function _findSgUser()
{
	var cname = vCore.getAdminCookieName();
	var cookie = vCore.getJSONCookie(cname);
	window.sgUserName = null;

	if (!!cookie.uid && cookie.uid != "undefined")
	    window.sgUserName = cookie.uid;

	if (!!cookie.email && cookie.email != "undefined")
	    window.sgUserEmail = cookie.email;

	if (window.sgUserName && window.sgUserEmail)
		_saveUser(window.sgUserName, window.sgUserEmail);

	var cpIsHome = !! window.CURRENT_PAGE_IS_HOME_PAGE;

	if (!user_authenticated)
	{
		var reload = function(force)
		{
			var redirect = true;
			var qs = '';
			try {
				if (! cpIsHome)
				{
					var ln = document.location.href;
					var lr = vCore.getOption('linkRoot');
					var ll = lr.length;

					if (ln.substring(0, ll) == lr)
						qs = '?redir=' + encodeURIComponent(ln.substring(ll));
				}
				redirect = force || ! cpIsHome;
			}
			catch (ex) {
				redirect = true;
			}
			if (redirect) {
				document.location.href = sgLinkRoot + "/repos/" + encodeURIComponent(startRepo) + qs;
			}
		};

		if (! window.sgUserName)
		{
			reload(false);
		}
		else
		{
			if (startRepo)
			{
				vCore.fetchUsers(
					{
						success: function(data)
						{
							var found = false;

							$.each(data, 
								   function (i, u)
								   {
									   if (u.recid == window.sgUserName)
										   {
											   found = true;
											   return(false);
										   }
								   });

							if (! found)
							{
								_removeUserCookies();
								reload(true);
							}
						}
					}
				);
			}
		}
	}
	
}

$(document).ready(_hookUserSelector);
