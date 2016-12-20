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

(function (window)
{
    var setup = function ()
    {
        var follower = $('#buildSection');

        if (follower.length != 1)
            return;

        var sec = $('<h3 class="innerheading">Associated Work Items</h3>')
			.insertBefore(follower);

        $('<input type="text" id="inputwit" class="searchinput innercontainer" />')
			.insertBefore(follower);

        var d = $('<div class="containerdetails" id="wits"></div>')
			.insertBefore(follower)
            .hide();

        vCore.bind('dataloaded', initWits);
    };

    var initWits = function (data)
    {
        setWits(data.description.associations);
        initWitComplete(data.description.associations);
    };

    function deleteWit(data)
    {
        var wits = data[0];
        var recid = data[1];
        var csid = getQueryStringVal("recid");

        vCore.ajax(
			{
			    type: "DELETE",
			    url: sgCurrentRepoUrl + "/workitem/" + recid + "/commit/" + csid + ".json",
			    success: function ()
			    {

			        for (var i = wits.length - 1; i >= 0; --i)
			            if (wits[i].recid == recid)
			                wits.splice(i, 1);
			        setWits(wits);
			    }
			}
		);
    }

    function setWits(wits)
    {
        $('#wits table').remove();
        if (wits && (wits.length > 0))
        {
            var div = $('#wits');
            var ul = $('<table />').appendTo(div);

            $.each(wits, function (i, v)
            {
                var tr = $('<tr>');

                var tdLink = $('<td>');
                var link = $('<a />').text(v.id).addClass("wit").
						   attr('href', sgCurrentRepoUrl + "/workitem.html?recid=" + v.recid);
                _wiHover.addHover(link, v.recid);

                tdLink.html(link);
                tdDel = $('<td>');

                var x = $(document.createElement('img')).
		            attr('src', sgStaticLinkRoot + "/img/blue_delete_short.png").
		            attr('alt', '[-]').
		            css('vertical-align', 'text-bottom');

                var del = $('<a />').html(x).attr(
						   {
						       "title": "delete this association",
						       href: "#"
						   });


                del.click(function (e)
                {
                    e.preventDefault();
                    e.stopPropagation();

                    confirmDialog("Delete link with " + v.id + "?", del, deleteWit, [wits, v.recid]);
                });
                tdDel.append(del);

                tr.append(tdDel).
			      append(tdLink).
				  append("<td>" + vCore.htmlEntityEncode(v.title) + "</td>").
                  appendTo(ul);
            });
            $('#wits').show();
        }
    }


    function addWit(id, wits)
    {
        var posturl = sgCurrentRepoUrl + "/workitem/" + id + "/commit/" + getQueryStringVal("recid") + ".json";

        vCore.ajax(
			{
			    url: posturl,
			    type: "POST",
			    success: function ()
			    {
			        var u = sgCurrentRepoUrl + "/workitem/" + id + ".json?_fields=status,title,id,recid";

			        vCore.ajax(
						{
						    url: u,
						    dataType: 'json',
						    success: function (data)
						    {
						        wits.push(data);
						        setWits(wits);
						    }
						}
					);
			    }
			}
		);

    }


    function initWitComplete(wits)
    {
        var searchfield = $('#inputwit');

        searchfield.keypress(
			function (e)
			{
			    var key = (e.which || e.keyCode);

			    if (key == 13)
			    {
			        e.preventDefault();
			        e.stopPropagation();
			        return (false);
			    }

			    return (true);
			}
		);

        var loadZero = function ()
        {
            if (searchfield.data('fired') || !searchfield.data('waiting'))
                return;

            if (searchfield.val())
                return;

            searchfield.autocomplete('search', '');
        };

        searchfield.blur(
			function ()
			{
			    searchfield.data('waiting', false);
			}
		);

        searchfield.focus(
			function (e)
			{
			    searchfield.data('fired', false);
			    var v = searchfield.val();

			    if (!v)
			    {
			        searchfield.data('waiting', true);
			        window.setTimeout(loadZero, 3000);
			    }
			    else
			    {
			        window.setTimeout(
						function ()
						{
						    searchfield.autocomplete('search', v);
						}, 100);
			    }
			}
		);

        searchfield.autocomplete(
			{
			    minLength: 0,
			    delay: 400,
			    'position': {
			        'my': "right top", 'at': "right bottom"
			    },

			    source: function (req, callback)
			    {
			        searchfield.data('fired', true);
			        var term = req.term;
			        var results = [];

			        // todo: already-check

			        var u = sgCurrentRepoUrl + "/workitem-fts.json?term=" + encodeURIComponent(term || '');

			        if (!term)
			        {
			            var vlist = _visitedItems() || [];

			            var ids = [];
			            $.each
						(
							vlist,
							function (i, v)
							{
							    var nv = v.split(":");
							    ids.push(nv[0]);
							}
						);

			            u = sgCurrentRepoUrl + "/workitem-details.json?_ids=" + encodeURIComponent(ids.join(','));
			        }
			        else if (term.length < 3)
			        {
			            callback([]);
			            return;
			        }

			        vCore.ajax(
						{
						    url: u,
						    dataType: 'json',
						    errorMessage: 'Unable to retrieve work items at this time.',

						    success: function (data)
						    {
						        var already = {};
						        results = [];

						        for (var i = 0; i < wits.length; ++i)
						            already[wits[i].recid] = true;

						        for (i = 0; i < data.length; ++i)
						            if (!already[data[i].value])
						            {
						                results.push(data[i]);
						                already[data[i].value] = true;
						            }
						    },

						    complete: function ()
						    {
						        callback(results);
						    }
						}
					);

			    },

			    select: function (event, ui)
			    {
			        event.stopPropagation();
			        event.preventDefault();

			        addWit(ui.item.value, wits);
			        searchfield.val('');
			        searchfield.blur();

			        return (false);
			    }
			}
		);

        var el = searchfield.data('autocomplete').element;
        el.oldVal = el.val;

        el.val = function (v)
        {
            return (el.oldVal());
        };
    }

    vCore.bind('postkickoff', setup);
})(window);
