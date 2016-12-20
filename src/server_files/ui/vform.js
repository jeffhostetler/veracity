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

/**
 * TODO The enable() method should not re-enable elements that were disabled
 * 		before the form was disabled.
 */

/**
 * vForm library for client side form creation
 *
 * = Form setup
 *
 * <pre>
 * var vFormOptions = {
 * 		rectype: "value",
 * 		url: "/some/url",
 * 		method: "POST|PUT"
 * }
 *
 * var form = new vForm(vFormOptions);
 * </pre>
 *
 * == Form options
 *	url:			The URL the form will be submitted to
 *	method: 		The HTTP verb that will be used to submit the form data
 *	dataType:		The type of data expected back from the server (See JQuery.ajax docs)
 *	contentType: 	The content type that is being submitted the the server
 *	rectype: 		The rectype that this form represents
 *	prettyReader:	The form should create read only fields to be displayed when form.disable()
 *					is called instead of adding the "disabled" attribute to the form field.
 *	forceDirty: 	Force the form to always be dirty.  This will force the form to be submitted
 *					even when no fields in the form have changed
 *	multiRecord: 	This should be set to 'true' when there are multiple forms of the same rectype
 *					on the same page.  This will force vForm to create a counter so that each form's
 *					fields will have a unique id attribute.
 *
 * == Form Callbacks
 * In addition to the above form options, vForm also currently supports a series of
 * callback methods.  These can be defined during form initialization.
 *
 *  afterPopulate -	This callback will be executed after the form has finished
 *  				populating values into the form.  Once this callback had
 *  				been called the form is "ready".
 *
 *  beforeSubmit -	This callback will be executed before any form processing
 *  				has been done.  If this callback returns the boolean value
 *  				false, form submission will be aborted.
 *
 *  afterSubmit -	This call back will be executed after the form has been
 *  				submitted to the server.  It will only be executed if the
 *  				submission was successful.
 *
 *  beforeValidation - This callback will be executed before the form has been
 *  					validated.  If this callback returns the boolean value
 *  					false, the submission of the form will be aborted.
 *
 *  afterValidation - This callback will be executed after the form has been
 *  					validated.  If this callback returns the boolean value
 *  					false, the submission of the form will be aborted.  This
 *  					call back takes two arguments, the first being the vform
 *  					object, the second a boolean indicating if the form is valid.
 *
 *  beforeFilter - 	This callback takes as a argument vform object and the
 *  				new/updated record. It allows for the values in the new
 *  				record to be manipulatedbefore the save is made, but
 *  				after validation is complete.
 *
 *  onChange - 		This callback will be executed whenever a value in any form
 *  				field changes.
 * = Existing Records
 * When creating a form for an existing record you can set the record
 * the form should populate the initial values.  This record should be
 * a standard javascript object.  The object must have a variable called
 * "rectype" that matches the rectype passed in the vFormOptions at form
 * initialization.
 *
 * <pre>
 * form.setRecord(record);
 * </pre>
 *
 * = Form Creation
 * The vForm library provides a set of methods for creating form fields.
 * These methods return a jQuery extend DOM object that can be inserted directly
 * into the DOM.  The library also keeps track of these fields for various
 * purposes including:
 *
 * - Marking the form dirty
 * - Ability to reset the initial values
 * - Gathering form values at submission
 *
 * Existing field creation methods:
 *
 * - textField
 * - textArea
 * - checkbox
 * - radio
 * - repoSelect
 * - sprintSelect
 * - userSelect
 * - select
 * - dateSelect
 * - label
 * - submitButton
 * - button
 *
 * == Field Options
 *  formatter -	Function that accepts the field value as an argument and either
 *  			"get" or "set" for the second argument.  It should return the
 *  			properly formatted value for that case.
 *
 *  scale - 	An integer value that is multiplied with the field value when
 *  			retrieved from the form.  The record value is also divided by
 *  			this value before it it inserted into the field.
 *
 * == Filtering Select Fields
 * Select fields can have their options filtered by passing an method in the
 * "filter" key of the field options.  When this key exists the method will be
 * called for each select option value that will be added to the select.  The
 * select option will only be added to the select if the filter method returns
 * boolean true.  When filtering sprint, user, or repo selects, the library will
 * pass the full record to the filter method.  For all other selects a javascript
 * object will be passed with a "name" and a "value" field.
 *
 * <pre>
 * var form = new vForm(options);
 *
 * var sel = form.userSelect("user", {
 * 		filter: function (user) {
 * 			if (form.getRecordValue("recid") == user.recid)
 * 				return false;
 *
 * 			return true;
 * 		}
 * });
 * </pre>
 *
 * == Disabling a Form
 * You can disable a form by calling the disable() method on the form object.
 * This will add the disabled="true" attribute to all inputs and add the class
 * "disabled" to all buttons created by the form object.
 *
 * To re-enable a form you can call the enable() method.  This will remove the
 * disabled attribute from all elements and remove the class "disabled" from
 * all buttons.
 *
 * = Validation
 * Validations are specified on a per field basis.  This is done by adding
 * values to the validators object within the field options as such:
 *
 * form.textField("name", {
 * 		validators:
 * 		{
 * 			maxLength: 10,
 * 			required: true
 * 		}
 * });
 *
 * Here is the list of currently available built in field validators:
 *
 * - required 	[boolean] 	 Must this field have a value
 * - allowedValues [array]	 An array of allowed values
 * - maxLength	[integer] 	 Maximum length for this field
 * - minLength	[integer] 	 Minimum Length for this field
 * - length [object|integer] When an object the validator looks for values in
 * 							 in the min and max fields.  When an integer it
 * 							 check to see if the field values is that length
 * - numerical	[boolean] 	 Checks to see if the field value is a valid number
 * 							 (float of int)
 * - match		[regex]		 Match the field to the given regex
 * - custom		[function]	 Passes the value of the field to the given callback
 * 							 function.  The field value will only be considered
 * 							 valid if the function returns boolean true.
 * - liveValidation [boolean] If this flag is set, the validators for this field
 * 							  will be run each time a change event it fired
 *
 * == Custom Validation
 * vForm also supports assigning a custom validator to the entire form.  To
 * assign this validator pass it as the customValidator option during vForm
 * initialization.  This function will be called after the individual field
 * validations have been run.  The form will only be submitted if this function
 * returns boolean true.  If you need to add an error message to a particular
 * field from within this validation function, use the
 * vForm.addError(input, msg) function.
 *
 * = Form submission
 * There are currently two ways to submit the form.  The first of which is by
 * the user clicking on a submitButton() that has been appended to the content.
 * The second method is to call the submitForm class method.  The submission
 * process is identical, as the submitButton method simply calls submitForm.
 *
 *  = Clean/Dirty
 *  You can check to see if the form is in a clean (unchanged) state by calling
 *  the method form.dirty().  It will compare all fields to their
 *  original values and return false if they haven't changed.
 *
 *  If you wish the reset the form back to it's original (unchanged) values,
 *  call the reset() method.
 *
 *  Anytime the form will be redrawn, the willBeDestroyed() method should be
 *  called.  This method will check to see if there is any unsaved data in
 *  the form and display a confirmation dialog.  The code that calls this
 *  method will receive the return value from the confirmation dialog and should
 *  handle the users decision properly.
 *
 *  = SubForms
 *  A vForm object can also have a set of subforms.  The subforms will be
 *  submitted once the main form has been submitted. The main use of subforms
 *  are for the editing and creation of related items (e.g. creating a new
 *  comment for a work item or editing time logged on a work item).
 *
 *  == Adding subforms
 *  Here a code snippet demonstrating the creation of subforms:
 *
 *  <pre>
 *  voptions =
 *  {
 *  ..url: "/some/url",
 *    ...
 *  };
 *  var form = new vForm(voptions);
 *  form.setRecord(someRecord);
 *
 *  // Create form inputs
 *  soptions =
 *  {
 *    url: "/some/other/url",
 *    ...
 *  };
 *  var sform =  new vForm(soptions);
 *  sform.setRecord(someOtherRecord);
 *
 *  // create subform inputs
 *
 *  form.addSubForm(sform);
 *
 *  form.ready();
 *  </pre>
 *
 *  It is worth noting here that marking the main form ready will mark all
 *  subforms ready as well.  Also unless you have really good reason to do so,
 *  you shouldn't call the ready() method directly on the subform.  The master
 *  form will handle that task for you.
 *
 *  == SubForm Validation
 *  SubForms have their own set of validations and validation callbacks.  The
 *  subform can be validate by itself, but validating the main form will cause
 *  the subform to be validated as well.  If subform validation fail, the main
 *  form will fali to validate as well.
 *
 *  == SubForm Callback
 *  Each subform has its own set of callbacks. They are the same as the main
 *  form callbacks but are called at slightly different times.
 *
 *  beforeSubmit -		Called before the main form is submitted
 *
 *  afterSubmit -		Called at the very end of the form submission process.
 *
 *  beforeValidation - 	Called before the subform is validated
 *
 *  afterValidation - 	Called after the surboem is validated
 *
 *  onChange - 			Called when any field within the subform is changed.
 *  					This callback will not bubble up to the main form.
 *
 *  == Possible Subform Gotchas
 *
 */

var _vFormCount = 0;
var _vfLegalCharRE = /^([\s\S]*?)([\u0000-\u0008]|[\u000b-\u000c]|[\u000e-\u001F]|[\u007F-\u0084]|[\u0086-\u009F]|[\uD800-\uDFFF]|[\uFDD0-\uFDEF])/;


function vForm(options)
{
	this.options = jQuery.extend(
	{
		url: null,
		method: "POST",
		dataType: "text",
		contentType: "application/json",
		rectype: null,
		newRecord: true,
        forceValueSet: false,
		prettyReader: false,
		forceDirty: false,
		multiRecord: false,  // When set to true, field ids have a counter appended to them
		queuedSave: true, // Save the record and subforms using mulitple ajax requests
		/**
		 * Key this form will use when submitting a serialized save.  This will
		 * only be used by sub forms.  Example: A subform with key "work" will be
		 * set to the key "work" in the master forms object.
		 **/
		formKey: null,
		submitParentOnly: false, //don't submit just the child form by iteself, parent should submit any child forms.

		customValidator: null,
		errorFormatter: vFormErrorFormatter,

		// Callbacks
		afterPopulate: null,
		beforeSubmit: null,
		afterSubmit: null,
		beforeValidation: null,
		afterValidation: null,
		onChange: null,
        handleUnload: false
        

	}, (options || {}));
	var vfself = this;

	this.ajaxQueue = new ajaxQueue({
		onSuccess: function() { vfself._submitComplete(); },
		onError: function() {
			 vfself._submitError();
		}
	});


	this.errorFormatter = new this.options.errorFormatter(this);

	this._formId = _vFormCount++;

	this._ready = false;
	this.submitting = false;
	this.disabled = false;

	this.record = null;

	this.fields = [];
	this.buttons = [];
	this.wrappers = {};

	this.multiNames = [];

	this.master = null;
	this.subForms = [];
	
	this.userSelects = [];
	this.sprintSelects = [];
	this.repoSelects = [];

	this.repos = null;
	this.users = null;
	this.inactiveUsers = null;
	this.sprints = null;

	this.fieldsHash = {};

	this.reposDispatched = false;
	this.usersDispatched = false;
	this.sprintsDispatched = false;
	this._init();
};

/**
 * Initialization Methods
 */
extend(vForm, {

    _init: function ()
    {
        var self = this;
        if (this.options.handleUnload)
        {
            window.onbeforeunload = function ()
            {
                var dirty = false;
                if (checkUnload)
                {
                    if (self.options.dirtyHandler)
                        dirty = self.options.dirtyHandler();
                    else
                        dirty = self.isDirty(true);

                }
                if (dirty)
                    return "You have unsaved changes.";
            }
        }
    },
    /**
    * Set the record this form is representing
    *
    * The record that is being passed will be used to populate the forms fields
    * The record's rectype must match the value passed in a form init
    */
    setRecord: function (record)
    {
        if (!record)
            return;

        if (record.rectype != this.options.rectype)
        {
            vCore.consoleLog("Invalid Rectype");
            return false;
        }

        this.options.newRecord = !record.recid;

        this.record = record;

        if (this._deferredSprints)
            this._sprintsNeeded();
        if (this._deferredUsers)
            this._usersNeeded();
    },

    /**
    * Private: Helper method to get the record value for a specific key
    *
    * @private
    */
    getRecordValue: function (key)
    {
        if (!this.record)
            return;

        return this.record[key];
    },

    /**
    * Private: Triggers the retrieval of the repos list from the server
    *
    * @private
    */
    _reposNeeded: function ()
    {
        if (this.reposDispatched)
            return;

        // Let the master form handle the fetching of the repos
        if (this.master)
        {
            this.master._reposNeeded();
            return;
        }

        var vfself = this;

        vCore.fetchRepos({
            success: function (repos)
            {
                vfself.repos = repos;
                // pass repos to subforms
                $.each(vfself.subForms, function (i, v) { v.repos = repos; });
                vfself._populate();

            }
        });

        this.reposDispatched = true;
    },

    /**
    * Private: Triggers the retrieval of the user list from the server
    *
    * @private
    */
    _usersNeeded: function ()
    {
        if (this.usersDispatched)
            return;

        // Let the master form handle the fetching of the user
        if (this.master)
        {
            this.master._usersNeeded();
            return;
        }

        var vfself = this;

        vCore.fetchUsers({
            success: function (users, inactiveUsers)
            {
                vfself.users = users;
                vfself.inactiveUsers = inactiveUsers;
                // pass users to subforms
                $.each(vfself.subForms, function (i, v) { v.users = users; v.inactiveUsers = inactiveUsers; });
                vfself._populate();

            }
        });

        this.usersDispatched = true;
    },

    /**
    * Private: Triggers the retrieval of the sprints lit from the server
    */
    _sprintsNeeded: function ()
    {
        if (this.sprintsDispatched)
            return;

        // Let the master form handle the fetching of the sprints
        if (this.master)
        {
            this.master._sprintsNeeded();
            return;
        }

        var vfself = this;

        vCore.fetchSprints({
            success: function (sprints)
            {
                vfself.sprints = sprints;
                // pass sprints to subforms
                $.each(vfself.subForms, function (i, v) { v.sprints = sprints; });
                vfself._populate();
            },
            releaseOnly: vfself.releasedSprintsOnly
        });

        this.sprintsDispatched = true;
    },

    /**
    * Mark the form ready
    *
    * This method should be called after all of the form elements have been
    * created.  Once this method has been called the form will populate
    * the form values and populate any user, repo, and sprint selects that have
    * been created
    */
    ready: function (isReady)
    {
        if ((isReady !== undefined) && !isReady)
        {
            this._ready = false;
            return;
        }

        this._ready = true;

        this._markLoading();

        this._populate();

        // Mark subforms ready
        $.each(this.subForms, function (i, f) { f.ready(); });
    },

    /**
    * Private: mark a field as loading
    *
    * This will display a progress indicator next the field
    */
    _markLoading: function ()
    {
        var l = this.userSelects.concat(this.repoSelects).concat(this.sprintSelects);

        for (var i = 0; i < l.length; i++)
        {
            this._setLoading(l[i]);
        }
    },

    _setLoading: function (input)
    {
        var el = $("<div>").addClass("smallProgress").addClass("inputLoading").css({ 'float': "left" });

        input.after(el);
    },

    _clearLoading: function (input)
    {


        var sibs = $(input).siblings(".inputLoading");
        if (sibs)
            sibs.first().remove();
    },

    /**
    * Populate the form
    *
    * This method will populate all of the helper selects (user, repos, etc...)
    * with the options they should have.  Next it will set all of the fields of
    * form to the values specified by the record.
    *
    * @private
    */
    _populate: function ()
    {
        // Populate is also called when the ajax fetches of users, repos, and
        // sprints return.  If the form hasn't been marked ready yet, do not
        // populate
        // TODO should populate also exit if one of the other dispatched ajax
        // TODO requests hasn't returned?  Does it matter?
        if (!this._ready)
            return;

        var vfself = this;
        this._populateUserSelects();
        this._populateSprintSelects();
        this._populateRepoSelects();

        // TODO this is probably way too simplistic for some of the more complex
        // TODO field types.
        $.each(this.fields, function (i, f)
        {
            var options = this.data("options");
            var name = '';
            if (options)
                name = options.name;
            else
                name = this.attr("name");
            var v = vfself.getRecordValue(name);

            var hf = f;

            if (f.isVFormField)
                hf = f.field || f;

            var preEditable = hf.attr && hf[0] &&
					   ((hf[0].tagName == 'textarea') ||
						((hf[0].tagName == 'input') && (hf.attr('type') == 'text')));

            if ((!vfself.getFieldValue(f)) || (!vfself.options.newRecord) || (!preEditable) || (vfself.options.forceValueSet))
                vfself.setFieldValue(this, v);
        });

        // Populate the subforms
        $.each(this.subForms, function (i, v) { v._populate(); });

        if (this.options.afterPopulate)
            this.options.afterPopulate(this);
    },

    setFieldValue: function (f, v, updateLastVal)
    {
        if (updateLastVal !== false)
            updateLastVal = true;

        if (f.isVFormField)
        {
            f.setVal(v, updateLastVal);
            return;
        }

        var options = f.data("options") || {};

        if (options)
            var name = options.name;
        else
            var name = f.attr("name");

        if (options && options.scale)
        {
            if (!v)
                v = 0;
            else
                v = Math.floor(v / options.scale);
        }

        var lastVal = v;
        if (options && options.formatter)
        {
            v = options.formatter(v, "set");
        }

        if (f.data('isdate'))
        {
            var val = new Date(parseInt(v));
            f.datepicker('setDate', val);

            // Get the value back out of the form
            // Datepicker does some massaging
            var d = f.datepicker('getDate');
        }
        else if ($.inArray(name, this.multiNames) >= 0)
        {
            if (typeof v == "string")
            {
                var val = v.split(",");
            }
            else
                val = v;

            $("input[name=" + name + "]").val(val);
        }
        else
        {
            if (f.attr)
            {
                switch (f.attr("type"))
                {
                    case "radio":
                        $("input[name=" + name + "]").val([v]);
                        break;
                    case "checkbox":
                        //If the record value is undefined, massage the value
                        if (v == undefined)
                            v = "0";

                        if (f.val() == v)
                        {
                            f.attr("checked", true);
                        }

                        break;
                    default:
                        f.val((v === undefined) ? '' : v);
                }
            }
            else
                f.val(v);
        }

        if (updateLastVal)
            f.data("lastval", lastVal);

        if (options.onchange)
            options.onchange(f);
    },

    /**
    * Private: Populate user selects
    *
    * @private
    */
    _populateUserSelects: function ()
    {
        if (this.users == null)
            return;

        var vfself = this;

        $.each(this.userSelects, function (i, v)
        {
            var ele = $("#" + v.attr("id")); // Refetch node
            var options = ele.data("options") || {};

            var uservals = [];
            $.each(vfself.users, function (i, user)
            {
                if (user.recid != NOBODY)
                {
                    var val = {
                        name: user.name,
                        value: user.recid
                    };
                    if (options.filter)
                    {
                        if (options.filter(user))
                            uservals.push(val);
                    }
                    else
                        uservals.push(val);
                }
            });

            uservals.sort(
					function (a, b)
					{
					    if (a.name == b.name)
					        return (0);
					    else if (a.name < b.name)
					        return (-1);
					    else
					        return (1);
					}
				);

            vfself._clearLoading(ele);
            vfself._populateOptions(ele, uservals);
        });
    },

    /**
    * Private: Populate repo selects
    *
    * @private
    */
    _populateRepoSelects: function ()
    {
        if (this.repos == null)
            return;

        var vfself = this;

        $.each(this.repoSelects, function (i, v)
        {
            var ele = $("#" + v.attr("id")); // Refetch node
            var options = ele.data("options");

            var repovals = [];
            $.each(vfself.repos, function (i, repo)
            {
                var val = {
                    name: repo.name,
                    value: repo.recid
                };
                if (options.filter)
                {
                    if (options.filter(repo))
                        repovals.push(val);
                }
                else
                    repovals.push(val);
            });
            vfself._clearLoading(ele);
            vfself._populateOptions(ele, repovals, false);
        });
    },

    /**
    * Private: Populate Sprint Selects
    *
    * @private
    */
    _populateSprintSelects: function ()
    {
        // TODO How should this handle nested sprints
        // TODO Should this hide released sprints

        if (this.sprints == null)
            return;

        var vfself = this;
        var milestoneTree = new vCore.milestoneTree(vfself.sprints);
        
        $.each(this.sprintSelects, function (i, v)
        {
            if (v.isVFormField)
            {
                var options = v.options;
                var ele = v.field;
            }
            else
            {
                var ele = $("#" + v.attr("id")); // Refetch node
                var options = ele.data("options");
            }

            if (ele.length == 0)
                return;

            var sprintvals = [];
            if (options && options.nested)
            {
                sprintvals = vfself._generateNestedSprintValues(milestoneTree.root, 0, options);
            }
            else
            {
                $.each(vfself.sprints, function (i, sprint)
                {
                    var val = {
                        name: sprint.name,
                        value: sprint.recid,
                        data: { "released": (sprint.releasedate != undefined) }
                    };
                    if (options && options.filter)
                    {
                        if (options.filter(sprint))
                            sprintvals.push(val);
                    }
                    else
                        sprintvals.push(val);
                });
            }

            vfself._clearLoading(ele);
            vfself._populateOptions(ele, sprintvals, false);
        });
    },

    _generateNestedSprintValues: function (node, depth, options)
    {
        var vals = [];
        var prefix = "";
        var released = false;

        if (node.data)
            released = (node.data.releasedate != undefined);
        for (var i = 0; i < depth; i++) { prefix += "-"; }

        var val = {
            name: prefix != "" ? prefix + " " + node.title : node.title,
            value: node.key,
            data: { "released": released }
        };

        if (options.filter)
        {
            if (options.filter(node.data))
            {
                vals.push(val);
            }
        }
        else
            vals.push(val);

        if (node.hasChildren())
        {
            for (i = 0; i < node.children.length; i++)
            {
                var child = node.children[i];

                var childVals = this._generateNestedSprintValues(child, depth + 1, options);

                vals = vals.concat(childVals);
            }
        }

        return vals;
    },

    setUsers: function (users)
    {
        users = users || [];
        this.users = users;
    }
});

/**
 * Clean/Dirty and form reset method
 */
extend(vForm, {

    _compareValues: function (one, two)
    {
        if (!one && !two)
            return true;

        if (one === "" || one === undefined)
            one = null;
        if (two === "" || two === undefined)
            two = null;

        return (one == two);
    },

    /**
    * Check to see if any of the form's field values have changed
    */
    isDirty: function (includeSubs)
    {
        if (this.options.forceDirty)
            return true;

        if (includeSubs !== false)
            includeSubs = true;

        var dirty = this._formIsDirty();

        // Check to see if the subforms are dirty
        if (includeSubs)
            dirty = this._subFormsAreDirty() || dirty;

        return dirty;
    },

    /**
    * Checks to see if the form is dirty.  Ignores subforms
    */
    _formIsDirty: function ()
    {
        var vfself = this;
        var dirty = false;
        $.each(this.fields, function (i, v)
        {
            if (v.isVFormField)
            {
                dirty = dirty || v.isDirty();              
            }
            else if (v.isDirty)
            {
                dirty = dirty || v.isDirty();           
            }
            else
            {
                var name = v.attr("name");
                var val = vfself.getFieldValue(v);
                var opts = v.data('options') || {};

                if (!opts.dontSubmit)
                    if (!vfself._compareValues(val, v.data("lastval")))
                    {
                        dirty = true;
                    }
            }
        });

        return dirty;
    },

    /**
    * Reset a dirty form back to it's original values
    */
    reset: function ()
    {
        var vfself = this;
        $.each(this.fields, function (i, v)
        {
            if (v.isVFormField)
                v.reset();
            else
                vfself.setFieldValue(v, v.data("lastval"), false);
        });

        this.clearErrors();
        // Reset the subforms
        this._resetSubForms();
    },

    /**
    * Disable a form
    *
    * This will disable all inputs in the form
    */
    disable: function ()
    {
        this.disabled = true;

        $.each(this.fields, function (i, f)
        {
            if (f.isVFormField)
            {
                f.disable();
            }
            else
            {
                var options = $(f).data("options");

                if (options && options.disableable)
                    $(f).attr("disabled", "true");
            }
        });

        $.each(this.buttons, function (i, b)
        {
            var options = $(b).data("options");

            if (options && options.disableable)
                $(b).addClass("disabled");
        });
    },

    /**
    * Enable a form
    *
    * Enable a previously disabled form
    */
    enable: function ()
    {
        this.disabled = false;

        $.each(this.fields, function (i, f)
        {
            if (f.isVFormField)
                f.enable();
            else
                $(f).removeAttr("disabled");
        });

        $.each(this.buttons, function (i, b)
        {
            $(b).removeClass("disabled");
        });
    },

    willBeDestroyed: function ()
    {
        if (this.isDirty())
            return confirm("This form has been edited.  Are you sure you want to lose your changes?");
    },

    _inputDidChange: function (input, event)
    {

        if (this.options.onChange)
            this.options.onChange(this);
    }
});

/**
 * Form creation methods
 */
extend(vForm, {

    defaultOptions:
	{
	    addClass: null,
	    disableable: true,
	    readOnly: false
	},

    fieldOptions:
	{
	    label: null,
	    formatter: null,
	    dontSubmit: false,
	    defaultValue: null,
	    validators:
		{
		    liveValidation: false,
		    required: false,
		    allowedValues: null,
		    maxLength: null,
		    minLength: null,
		    minVal: null,
		    length: null,
		    numerical: false,
		    matchRegex: null,
		    custom: null,
		    betweenDates: null
		},
	    error:
		{
		    tag: "div",
		    addClass: null
		},
	    reader:
		{
		    hasReader: true,
		    tag: "p",
		    addClass: null,
		    hideLabel: false,
		    formatter: null,
		    postformatter: null

		}
	},

    selectOptions:
	{
	    allowEmptySelection: false,
	    emptySelectionText: null
	},

    multiLineOptions:
	{
	    multiLine: true,
	    rows: 10,
	    reader:
		{
		    tag: "pre"
		}
	},

    markdownOptions:
    {
        id_prefix: "wmd-input"
    },
    markdowneditor: null,
    markdownlinks: null,

    field: function (container, type, name, options)
    {
        var vfself = this;

        if (!type)
            return null;

        if (type == "textarea")
            options = $.extend(true, { _append: false }, this.defaultOptions, this.fieldOptions, this.multiLineOptions, (options || {}));
        else if (type == "markdowntextarea")
            options = $.extend(true, { _append: false }, this.defaultOptions, this.fieldOptions, this.multiLineOptions, this.markdownOptions, (options || {}));
        else
            options = $.extend(true, { _append: false }, this.defaultOptions, this.fieldOptions, (options || {}));

        var field = null;
        var realName = name;
        switch (type)
        {
            case "hidden":
                //options.reader.hasReader = false;
                field = this.hiddenField(name, options);
                break;
            case "text":
                field = this.textField(name, options);
                break;
            case "textarea":
                field = this.textArea(name, options);
                break;
            case "markdowntextarea":
                field = this.markdownTextArea(name, options)
                name = "wmd-input" + name;
                break;
            case "checkbox":
                field = this.checkbox(name, options.value, options);
                break;
            case "booleanCheckbox":
                field = this.booleanCheckbox(name, options);
                break;
            case "radio":
                field = this.radio(name, options.value, options);
                break;
            case "select":
                field = this.select(name, null, options);
                break;
            case "dateSelect":
                field = this.dateSelect(name, options);
                break;
            case "userSelect":
                field = this.userSelect(name, options);
                break;
            case "repoSelect":
                field = this.repoSelect(name, options);
                break;
            case "sprintSelect":
                field = this.sprintSelect(name, options);
                break;

            default:
                throw ("Unknown Field Type");
                break;
        }

        // Some of the input options may have been manipulated during
        // field creation.
        options = field.data("options");

        if (options.addClass)
            field.addClass(options.addClass);

        var err = this.errorFormatter.createContainer(field, options.error);

        if (options.error.addClass)
            err.addClass(options.error.addClass);

        var label = null;
        if (options.label)
        {
            label = this.label(name, options.label);
        }

        var reader = null;
        if (this.options.prettyReader && options.reader.hasReader)
        {
            reader = $(document.createElement(options.reader.tag));
            reader.addClass("reader");

            reader.addClass(options.reader.addClass || 'innercontainer');

            reader.css({ "display": "none" });
            if (options.reader.addClass)
                reader.addClass(options.reader.addClass);

            var blankVal = options.emptySelectionText || '';
            if (type == "markdowntextarea")
            {
                options.reader.formatter = function (val) { return vfself.markdowneditor.converter.makeHtml(val) };
                options.reader.markedDown = true;
            }
            if (type == "sprintSelect")
            {
                options.reader.formatter = function (val) { return vfself._sprintFormatter(val, blankVal); };
            }

            if (type == "repoSelect")
                options.reader.formatter = function (val) { return vfself._repoFormatter(val); };

            if (type == "userSelect")
                options.reader.formatter = function (val) { return vfself._userFormatter(val); };

            if (type == "select")
                options.reader.formatter = function (val)
                {
                    var opt = field.find("option[value=" + val + "]");
                    if (opt)
                    {
                        _val = opt.text();
                        return _val == "" ? blankVal : _val;
                    }

                    return val;
                };
        }

        var vffield = new vFormField(this, label, field, reader, err, options);

        if (options.readOnly)
            vffield.readOnly();

        this.fields.push(vffield);
        this.fieldsHash[name] = vffield.field;

        if (container)
        {
            container.append(label);
            container.
		 		append(reader).
		 		append(err);


            if (options.wrapField)
            {
                var ctr = $('<div class="inputwrapper"></div>').appendTo(container).append(field);
                vffield.fieldWrapper = ctr;
                field.css('display', 'block').css('float', 'left');

                if (options.isBlock)
                    ctr.addClass('wrapblock');
            }
            else
                container.append(field);

            if (type == "markdowntextarea")
            {
                if (label)
                {
                    label.addClass("labeldescreader");
                    label.hide();
                }
                field.addClass("markdown");
                this.markdowneditor = new vvMarkdownEditor(field, this.options.rectype + "_" + realName, container, true);
                this.markdowneditor.draw();
            }
        }

        return vffield;
    },

    _sprintFormatter: function (val, blankVal)
    {
        var ret = val || '';

        if (this.sprints)
        {
            $.each(this.sprints, function (i, v)
            {
                if (v.recid == val)
                {
                    ret = v.name;
                    return false;
                }
            });
        }

        if (ret == '')
            ret = blankVal || ret;

        return ret;
    },

    _repoFormatter: function (val)
    {
        var ret = val;
        if (this.repos)
        {
            $.each(this.repos, function (i, v)
            {
                if (v.recid == val)
                {
                    ret = v.name;
                    return false;
                }
            });
        }
        return ret;
    },

    _userFormatter: function (val)
    {
        var ret = val || "";
        if (this.users)
        {
            $.each(this.users, function (i, v)
            {
                if (v.recid == val)
                {
                    ret = v.name;
                    return false;
                }
            });

            if (ret == val)
                $.each(this.inactiveUsers || [], function (i, v)
                {
                    if (v.recid == val)
                    {
                        ret = v.name;
                        return false;
                    }
                });
        }
        return ret.replace(/@/, "@\u200B");
    },

    addCustomField: function (name, field, options)
    {
        var vfself = this;
        var opt = $.extend({}, options);

        opt.name = name;

        field.data("options", opt);

        $(field).change(function (event)
        {
            vfself._inputDidChange(this, event);
        });

        this.fields.push(field);
    },

    /**
    * Create a label element
    *
    * This method will return a label element for the
    * given field name with the given label text.
    * For example, the following snippet:
    *
    * form.label("name", "Name:");
    *
    * would return a DOM node representing
    *
    * <lable for="<rectype>_name">Name:</label>
    *
    * @param name Name of the field the label is for
    * @param text Text for the label element
    */
    label: function (name, text)
    {
        var id = this._generateId(name);
        var label = $(document.createElement("label"))
			.attr("for", id)
			.attr("id", id + "_label")
			.text(text);

        if (!this.options.smallLabels)
            label.addClass('innerheading');

        return label;
    },


    /**
    * Create a submit button for the form
    */
    submitButton: function (name, options)
    {
        var vfself = this;
        var button = this.button(name, options);
        button.attr(
		{
		    id: this.options.rectype + "_submit"
		});
        button.click(function (event) { vfself.submitForm(); });

        return button;
    },

    /**
    * Create a button
    */
    button: function (name, options)
    {
        var s = $.extend(true, {}, this.defaultOptions, (options || {}));
        var button = $(document.createElement("input"));
        button.attr(
		{
		    type: "button",
		    value: name
		}).data("options", s);

        this.buttons.push(button);

        return button;
    },

    /**
    * Create a hidden field element
    */
    hiddenField: function (name, options)
    {
        var field = $(document.createElement("input"));
        field.attr({ "type": "hidden" });
        var s = $.extend(true, {}, this.defaultOptions, this.fieldOptions, (options || {}));
        this._setupInput(field, name, s);

        return field;
    },

    /**
    * Create a text field element
    */
    textField: function (name, options)
    {
        var field = $(document.createElement("input"));
        field.attr({ "type": "text" });
        var s = $.extend(true, {}, this.defaultOptions, this.fieldOptions, (options || {}));
        this._setupInput(field, name, s);

        return field;
    },

    /**
    * Create an image button element
    */
    imageButton: function (name, imgsrc, options)
    {
        var s = $.extend(true, {}, this.defaultOptions, (options || {}));
        var button = $(document.createElement("input"));
        button.attr(
		{
		    id: name,
		    type: "image",
		    value: name,
		    src: imgsrc
		}).data("options", s);

        this.buttons.push(button);

        return button;
    },

    /**
    * Create a textarea element
    */
    textArea: function (name, options)
    {
        var field = $(document.createElement("textarea"));
        var s = $.extend(true, {},
			this.defaultOptions,
			this.fieldOptions,
			this.multiLineOptions,
			(options || {}));

        field.attr("rows", s.rows);

        this._setupInput(field, name, s);

        return field;
    },
    /**
    * Create a textarea element
    */
    markdownTextArea: function (name, options)
    {
        var field = $(document.createElement("textarea"));
        var s = $.extend(true, {},
			this.defaultOptions,
			this.fieldOptions,
			this.multiLineOptions,
            this.markdownOptions,
			(options || {}));

        field.attr("rows", s.rows);
        field.attr("placeholder", name + " text");

        this._setupInput(field, name, s);

        return field;
    },


    /**
    * Create a checkbox
    */
    checkbox: function (name, value, options)
    {
        var field = $(document.createElement("input"));
        field.attr({
            type: "checkbox",
            value: value
        });

        var s = $.extend(true, {}, this.defaultOptions, this.fieldOptions, (options || {}));
        this._setupInput(field, name, s);

        if ($.inArray(name, this.multiNames) == -1)
            this.multiNames.push(name);

        return field;
    },

    booleanCheckbox: function (name, options)
    {
        var opts = $.extend(true,
			{
			    unchecked: "0",
			    'boolean': true
			},
			this.defaultOptions,
			this.fieldOptions,
			(options || {})
		);

        var field = $(document.createElement("input"));
        field.attr({
            type: "checkbox",
            value: "1"
        });

        this._setupInput(field, name, opts);

        return field;
    },

    /**
    * Create a radio button
    */
    radio: function (name, value, options)
    {
        var field = $(document.createElement("input"));
        field.attr({
            type: "radio",
            value: value
        });

        var s = $.extend(true, {}, this.defaultOptions, this.fieldOptions, (options || {}));
        this._setupInput(field, name, s);

        return field;
    },

    /**
    * Create a calendar based date select
    */
    dateSelect: function (name, options)
    {
        var field = $(document.createElement("input"));
        field.attr("placeholder", "YYYY/MM/DD");
        var s = $.extend(true, { "isdate": true }, this.defaultOptions, this.fieldOptions, (options || {}));

        this._setupInput(field, name, s);

        var pickeroptions = { dateFormat: 'yy/mm/dd', changeYear: true, changeMonth: true };
        if (s.validators.betweenDates)
        {
            pickeroptions.minDate = s.validators.betweenDates.start;
            pickeroptions.maxDate = s.validators.betweenDates.end;
        }

        $(field).datepicker(pickeroptions);
        $(field).data("isdate", true);

        return field;
    },

    /**
    * Create a select populated with available repos
    *
    * The created select will be populated with the retrieved list of repos
    * when the form is marked ready.
    */
    repoSelect: function (name, options)
    {
        this._reposNeeded();
        var select = this.select(name, null, options);

        this.repoSelects.push(select);

        return select;
    },

    /**
    * Create a select to be populated with the list of sprints
    *
    * The created select will be populate with the retrieved list of sprints
    * when the form is marked ready.
    */
    sprintSelect: function (name, options)
    {
        if (options.deferLoading)
            this._deferredSprints = true;
        else
            this._sprintsNeeded();
        var select = this.select(name, null, options);

        this.sprintSelects.push(select);

        return select;
    },

    /**
    * Create a select to be populate with the list or users
    *
    * The created select will be populates with the retrieved list of sprints
    * when the form is marked ready.
    */
    userSelect: function (name, options)
    {
        if (options.deferLoading)
            this._deferredUsers = true;
        else
            this._usersNeeded();

        options = jQuery.extend({ 'userSelect': true }, (options || {}));

        var select = this.select(name, null, options);

        this.userSelects.push(select);

        return select;
    },


    /**
    * Create a generic select element
    */
    select: function (name, optionValues, options)
    {
        var select = $(document.createElement("select"));
        var s = $.extend(true, {}, this.defaultOptions, this.fieldOptions, this.selectOptions, (options || {}));
        this._setupInput(select, name, s);

        // Populate the select options if there have been provided
        this._populateOptions(select, optionValues);

        return select;
    },

    /**
    * Private: create an option element
    */
    _createOption: function (name, value)
    {
        name = name || '';
        if (value === undefined)
            value = name;

        return ($(document.createElement('option')).
				val(value || "").
				text(name || "")
			  );
    },

    /**
    * Private: Populate the given select with options
    *
    * @private
    * @param {HTMLSelect} select select to populate
    * @param {Array} values An array of objects with name and value keys
    */
    _populateOptions: function (select, values, filter)
    {
        if (filter == null)
            filter = true;
        var options = select.data("options") || {};
        var vfself = this;
        var oldVal = select.val();

        select.empty();

        if (options.allowEmptySelection)
        {
            var opt;

            if (options.emptySelectionText)
                opt = this._createOption(options.emptySelectionText, "");
            else
                opt = this._createOption();

            select.append(opt);
        }

        if (!values)
            return;

        if (options.filter && filter)
        {
            var _newvals = [];
            $.each(values, function (i, v)
            {
                if (options.filter(v.value))
                    _newvals.push(v);
            });
            values = _newvals;
        }

        for (var i = 0; i < values.length; i++)
        {
            var val = values[i];
            var opt = this._createOption(val.name, val.value);
            if (val.data)
            {
                opt.data(val.data);
            }
            if (val.disabled)
                opt.attr("disabled", true);

            select.append(opt);
        }

        if (vfself.options.newRecord && options.defaultValue)
            select.val(options.defaultValue);
        else if ((oldVal == '') || oldVal)
            select.val(oldVal);
    },

    /**
    * Private: Generate the id of a field for a given name
    */
    _generateId: function (name)
    {
        var base = this.options.rectype + "_" + name;
        if (this.options.multiRecord)
            return base + "_" + this._formId;
        else
            return base;
    },

    /**
    * Private: Performs common setup tasks on a form field
    */
    _setupInput: function (input, name, options)
    {
        var vfself = this;
        options.name = name;

        input.attr({
            "id": (options.id_prefix ? options.id_prefix : "") + this._generateId(name),
            "name": name
        });

        input.data("options", options);

        input.change(function (event)
        {
            if (options.onchange)
                options.onchange($(input));

            vfself._inputDidChange(this, event);
        });

        if (options.validators.liveValidation)
        {
            var validator = function (e)
            {
                if (!vfself.ready)
                    return;

                if (options.name)
                    input = vfself.getField(options.name);

                if (!vfself.validateInput(input, true))
                {
                    if (input.isVFormField)
                        var errors = input.errors;
                    else
                        var errors = input.data("errors");

                    vfself.errorFormatter.displayInputError(input, errors);
                }
                else
                {
                    vfself.errorFormatter.hideInputError(input);
                }

            };

            input.keyup(validator);
            input.blur(validator);
        }

        // Record field
        if (options._append !== false)
            this.fields.push(input);
    }
});

/**
 * Validation Methods
 */
extend(vForm, {

	/**
	* Validate the form
	*/
	validate: function ()
	{
		var vfself = this;
		if (this.options.beforeValidation)
		{
			if (this.options.beforeValidation(vfself) === false)
				return false;
		}

		var valid = true;

		var inp = this.fields;

		// Validate all fields
		for (var i = 0; i < inp.length; i++)
		{
			var f = inp[i];
			var _valid = this.validateInput(f);
			valid = valid && _valid;
		}

		if (this.options.customValidator)
			valid = this.options.customValidator(vfself) && valid;

		// Validate the subforms
		var subFormsValid = this._validateSubForms();
		valid = valid && subFormsValid;

		// Callback
		if (this.options.afterValidation)
		{
			if (this.options.afterValidation(vfself, valid) === false)
				return false;
		}

		return valid;
	},

	/**
	* Validate the value of the given input
	*/
	validateInput: function (input, live)
	{
		if (live !== true)
			live = false;

		var _valid = true;

		if (input.isVFormField)
		{
			var options = input.options;
			input.errors = [];
		}
		else
		{
			var options = input.data("options");
			input.data("errors", []);
		}

		if (!options)
			options = {};

		var validators = options.validators;

		if (!validators)
			validators = {};

		if (validators.legalChars !== false)
			validators.legalChars = true;

		if (validators.legalChars)
			_valid = this.validateLegalChars(input) && _valid;

		if (validators.allowedValues)
			_valid = this.validateAllowedValues(input, validators.allowedValues) && _valid;

		if (validators.betweenDates)
			_valid = this.validateBetweenDates(input, validators.betweenDates) && _valid;

		if (validators.maxLength != null)
			_valid = this.validateMaxLength(input, validators.maxLength) && _valid;

		if (validators.minLength != null)
			_valid = this.validateMinLength(input, validators.minLength) && _valid;

		if (validators.minVal != null)
			_valid = this.validateMinVal(input, validators.minVal) && _valid;

		if (validators.maxVal != null)
			_valid = this.validateMaxVal(input, validators.maxVal) && _valid;

		if (validators.length != null)
		{
			var val = validators.length;

			if (typeof val == "object")
			{
				var _max = val.max;
				var _min = val.min;

				if (max && min)
				{
					_valid = this.validateLength(input, val.min, val.max) && _valid;
				}
				else if (max)
					_valid = this.validateMaxLength(input, val.max) && _valid;
				else if (min)
					_valid = this.validateMinLength(input, val.max) && _valid;

			}
			else if (typeof val == "number")
				_valid = this.validateLength(input, val, val) && _valid;
		}

		if (validators.numerical)
			_valid = this.validateNumerical(input) && _valid;

		if (validators.match)
		{
			_valid = this.validateMatch(input, validators.match) && _valid;
		}

		if (validators.custom)
			_valid = this.validateCustom(input, validators.custom) && _valid;

		if (!live && validators.required)
			_valid = this.validateRequired(input) && _valid;

		if (!_valid)
			vCore.consoleLog("Field '" + options.name + "' invalid");

		return _valid;
	},

	validateRequired: function (field)
	{
		var val = this.getFieldValue(field);
		var ret = (val != "" && val != null && val.length != 0);

		if (!ret)
		{
			this.addError(field, "This field is required");
		}

		return ret;
	},

	validateAllowedValued: function (field, values)
	{
		var val = this.getFieldValue(field);
		var ret = ($.inArray(val, values) > -1);

		if (!ret)
			this.addError(field, "Invalid value");

		return ret;
	},

	validateBetweenDates: function (input, betweenDates)
	{
		var val = this.getFieldValue(input);
		var date = new Date(parseInt(val));
		val = vCore.dateToDayStart(date);
		var start = vCore.dateToDayStart(betweenDates.start);
		var end = vCore.dateToDayEnd(betweenDates.end);

		var ret = (val.getTime() >= start.getTime()
					&& val.getTime() <= end.getTime());

		if (!ret)
			this.addError(input, "Must be between " + betweenDates.start + " and " + betweenDates.end);

		return ret;
	},

	validateMaxLength: function (input, length)
	{
		var curlen = this.getFieldValue(input).length;
		var ret = (curlen <= length);

		if (!ret)
			this.addError(input, "Must be shorter than " + (length + 1) + " characters (currently " + curlen + ")");

		return ret;
	},

	validateMinLength: function (input, length)
	{
		var ret = (this.getFieldValue(input).length >= length);

		if (!ret)
			this.addError(input, "Must be at least " + length + " characters");

		return ret;
	},

	validateLength: function (input, min, max)
	{
		var ret = this.validateMinLength(input, min) && this.validateMaxLength(input, max);

		if (!ret)
			this.addError(input, "Length must be between " + min + " and " + max);

		return ret;
	},

	validateMinVal: function (input, minVal)
	{
		var v = this.getFieldValue(input);

		var ret = (v === undefined) || (v == "") || (parseInt(v) >= minVal);

		if (!ret)
			this.addError(input, "Must be at least " + minVal);

		return ret;
	},

	validateMaxVal: function (input, maxVal)
	{
		var v = this.getFieldValue(input);

		var ret = (v === undefined) || (v == "") || (parseInt(v) <= maxVal);

		if (!ret)
			this.addError(input, "Value cannot be larger than " + maxVal);

		return ret;
	},

	validateNumerical: function (input)
	{
		var regex = /^[0-9]*\.?[0-9]*$/;
		var val = this.getFieldValue(input);
		if (!val)
			this.addError(input, "Must be a number");

		var ret = regex.test(val);

		if (!ret)
			this.addError(input, "Must be a number");

		return ret;
	},

	validateMatch: function (input, regex)
	{
		var val = this.getFieldValue(input);
		var ret = regex.match(val);

		if (!ret)
			this.addError(input, "Invalid value");
	},

	validateCustom: function (input, validator)
	{
		var val = this.getFieldValue(input, true);

		var ret = validator(val);

		if (ret !== true)
		{
			if (typeof ret == "string")
				this.addError(input, ret);
			else
				this.addError(input, "Validation failed");
		}

		return (ret === true);
	},

	validateLegalChars: function (input)
	{
		var regex = _vfLegalCharRE;

		var val = this.getFieldValue(input);

		var ret = true;

		if (typeof(val) == "string")
		{
			var matches = val.match(regex);

			ret = ! matches;

			if (! ret)
				this.addError(input, "Contains control character(s) at position " + (matches[1].length + 1));
		}

		return ret;
	},

	addError: function (input, msg)
	{
		if (input.isVFormField)
		{
			input.errors.push(msg);
		}
		else
		{
			var _err = input.data("errors");

			if (!_err)
				_err = [];

			_err.push(msg);

			input.data("errors", _err);
		}
	},

	/**
	* Display validation errors
	*
	*
	*/
	displayErrors: function ()
	{
		var inp = this.fields;

		for (var i = 0; i < inp.length; i++)
		{
			var f = inp[i];
			if (f.isVFormField)
				var _err = f.errors;
			else
				var _err = f.data("errors");

			// This can happen if the field is not a vFormField and it
			// id not currently inserted in the DOM
			if (!_err || _err.length == 0)
				continue;

			if ((f.isVFormField && f.field.is(":visible")) || !f.isVFormField && f.is(":visible"))
				this.errorFormatter.displayInputError(f, _err);
		}

		// Subform support
		this._displaySubFormErrors();
	},

	hideErrors: function ()
	{
		var inp = this.fields;

		for (var i = 0; i < inp.length; i++)
		{
			var f = inp[i];

			this.errorFormatter.hideInputError(f);
		}

		// Subform support
		//this._displaySubFormErrors();
	},

	clearErrors: function ()
	{
		var inp = this.fields;

		for (var i = 0; i < inp.length; i++)
		{
			var f = inp[i];
			f.data("errors", []);
			this.errorFormatter.hideInputError(f);
		}

		this._clearSubFormErrors();
	}

});

/**
 * Form Submission methods
 */
extend(vForm, {
	/**
	 * Submit the
	 */
	submitForm: function()
	{
		if (this.submitting)
			return;

		if (this.options.submitParentOnly)
			return;

		if (this.options.beforeSubmit)
		{
			if (this.options.beforeSubmit(this) === false)
				return;
		}

		if (this._callSubFormBeforeSubmit() === false)
			return;

		this.clearErrors();
		var valid = this.validate();

		if (!valid)
		{
			vCore.consoleLog("Form Validation Failed");
			this.displayErrors();
			this.ajaxQueue.clearQueue();
			return;
		}

		this._submit();
	},

	/**
	 * Private: submit the form
	 *
	 * DO NOT CALL THIS FROM OUTSIDE THE LIB, that is all...
	 */
	_submit: function()
	{
		this.submitting = true;
		var vfself = this;

		if (this.options.queuedSave)
			var newRec = this.getFieldValues();
		else
			var newRec = this.getSerializedFieldValues();

		// Force the rectype
		newRec.rectype = this.options.rectype;

		if (this.options.beforeFilter)
		{
			newRec = this.options.beforeFilter(vfself, newRec);
			if (newRec === false)
				return;
		}


		if (this.isDirty(!this.options.queuedSave))
		{
			if (this.options.duringSubmit)
				this.options.duringSubmit.start();

			var request = {
				url: this.options.url,
				dataType: this.options.dataType,
				contentType: this.options.contentType,
				processData: false,
				type: this.options.method,

				success: function(data, statusText, xhr) {

					// If this is a new record, set our record
					if (!vfself.record)
					{
						newRec.rectype = vfself.options.rectype;
						vfself.record = newRec; //TODO should this call setRecord?
					}
					// set the recid for new records
					if (data)
						vfself.record.recid = data.replace(/["]/g, "");
				},

                error: function ()
                {
                    vfself.submitting = false;
                }
            };

			if (this.options.method == "GET")
			{
				request.url = request.url + this.getQueryString();
			}
			else
			{
				request.data = JSON.stringify(newRec);
			}

			this.ajaxQueue.addQueueRequest(request);
		}

		if (this.options.queuedSave)
			this._queueSubForms();

		if (!this.master)
		{
			this.ajaxQueue.execute();
		}
	},

    getQueryString: function ()
    {
        var ret = "?";
        $.each(this.fields, function (i, field)
        {
            if (field.isVFormField)
                var options = field.options;
            else
                var options = field.data("options");

			var name = options.name;

			if (options.dontSubmit)
				return;

			var val = vfself.getFieldValue(field);

			ret += name + "=" + encodeURIComponent(val);

		});

		return ret;
	},

    /**
    * Collect all of the forms values
    *
    * @private
    */
    getFieldValues: function ()
    {
        var values = {};
        var vfself = this;
        $.each(this.fields, function (i, field)
        {
            if (field.isVFormField)
                var options = field.options;
            else
                var options = field.data("options");

			var name = options.name;

			if (options.dontSubmit)
				return;

			var val = vfself.getFieldValue(field);

			if ($.inArray(name, vfself.multiNames) >= 0)
			{
				if (!values[name])
					values[name] = [];

				values[name].push(val);
			}
			else
			{
				values[name] = val;
			}

		});

		values.rectype = this.options.rectype;

		return values;
	},

	getFieldValue: function(field, raw)
	{
		if (field.isVFormField)
			return field.getVal(raw);

		/**
		 * This code is deprecated and will be removed
		 **/
		if (raw == null || raw == undefined)
			raw = false;

		var options = field.data("options") || {};
		var name = options.name;
		var vfself = this;
		var val;

		if (!! field.data('isdate'))
		{
			var d = field.datepicker('getDate');
			val = d.getTime();
		}
		else if ($.inArray(name, vfself.multiNames) >= 0)
		{
			if (field.attr)
			{
				switch (field.attr("type"))
				{

                    case "checkbox":
                        if (field.is(":checked"))
                        {
                            val = field.val();
                        }
                        else
                        {
                            if (options.unchecked)
                                val = options.unchecked;
                        }
                        break;
                }
            }
            else
                val = field.val();
        }
        else
        {
            if (field.attr)
            {
                switch (field.attr("type"))
                {
                    case "radio":
                        val = $('input:radio[name=' + name + ']:checked').val();
                        break;
                    case "checkbox":
                        if (field.is(":checked"))
                        {
                            val = field.val();
                        }
                        else
                        {
                            if (options.unchecked)
                                val = options.unchecked;
                        }
                        break;
                    default:
                        val = field.val();
                        break;
                }
            }
            else
                val = field.val();

		}
		// IE HACK when the value of a select is an empty string
		// IE returns the value as an empty object
		// This causes a type mismatch in the backend
		if ((typeof val == "object") && ((val == null) || (val.length == 0)))
		{
			val = "";
		}

		if (raw)
			return val;

		if (options && options.formatter)
		{
			val = options.formatter(val, "get");
		}

		if (options && options.scale)
		{
			val = (val || 0) * options.scale;
		}

		return val;
	},

	getSerializedFieldValues: function()
	{
		var rec = this.getFieldValues();

		$.each(this.subForms, function(i, form) {
			var key = form.options.formKey;
			if (!key)
				return;

			var subRec = form.getSerializedFieldValues();


			if (form.options.multiRecord)
			{
				if (!rec[key])
					rec[key] = [];

				if (!Array.isArray(rec[key]))
				{
					var existing = rec[key];
					rec[key] = [ existing ];
				}

				rec[key].push(subRec);
			}
			else
			{
				rec[key] = subRec;
			}
		});

		return rec;
	},

	/**
	 * Get a field for a specified name for the form
	 *
	 * This method will search through all of the fields created for this form
	 * attempting to find one that matches the given name.  If such a field is
	 * not found, it return null.
	 **/
	getField: function(name)
	{
		var ret = null;

		var toSearch = this.fields;

		$.each(toSearch, function(i, field) {
			if (field.isVFormField)
			{
				var fieldName = field.options.name;
			}
			else
			{
				var options = field.data("options");
				if (options.name)
					var fieldName = options.name;
				else
					var fieldName = field.attr("name");
			}

			if (name == fieldName)
			{
				ret = field;
				return false;
			}
		});

		return ret;
	},

	_submitError: function()
	{
		if (this.options.duringSubmit)
			this.options.duringSubmit.finish();
	},

    _submitComplete: function ()
    {


		if (this.options.duringSubmit)
			this.options.duringSubmit.finish();

		var recid = this.record.recid;
		this.record = this.getFieldValues();
		this.record.recid = recid;

		if (this.options.afterSubmit)
			this.options.afterSubmit(this, this.record);

		this._callSubFormSubmitComplete();

        // re-populate so lastval gets set properly
        this._populate();
        this.submitting = false;
    }
});

/**
 * SubFrom Methods
 */
extend(vForm, {

	/**
	 * Add a vForm object as a subform of the current form
	 *
	 */
	addSubForm: function(form)
	{
		if (!form)
			return;

		form.master = this;
		form.ajaxQueue = this.ajaxQueue;

		// Don't double add
		if ($.inArray(form, this.subForms) == -1)
			this.subForms.push(form);
	},

	removeSubForm: function(form)
	{
		if (!form)
			return;

		var index = $.inArray(form, this.subForms);
		if (index == -1)
			return;

		this.subForms.splice(index, 1);
		form.master = null;
		form.resetAjaxQueue();
	},

	/**
	  *
	  */
	unbindSubForms: function()
	{
		var vfself = this;

		$.each(this.subForms, function(i, form) {
			form.master = null;
			form.resetAjaxQueue();
		});

		this.subForms = [];
	},

	resetAjaxQueue: function()
	{
		var vfself = this;
		this.ajaxQueue = new ajaxQueue({
			onSuccess: function() { vfself._submitComplete(); },
			onError: function() {
				 vfself._submitError();
			}
		});
	},

	/**
	 * Private: Validate all subform
	 *
	 * Validates all of the subforms and aggregates their validation statuses
	 */
	_validateSubForms: function()
	{
		var valid = true;
		for (var i = 0; i < this.subForms.length; i++)
		{
			var form = this.subForms[i];
			valid &= form.validate();
		}

		return valid;
	},

	/**
	 * Private: Display any errors on subforms that exists after validation
	 */
	_displaySubFormErrors: function()
	{
		for (var i = 0; i < this.subForms.length; i++)
		{
			this.subForms[i].displayErrors();
		}
	},

	/**
	 * Private: Clear subform errors
	 */
	_clearSubFormErrors: function()
	{
		for (var i = 0; i < this.subForms.length; i++)
		{
			this.subForms[i].clearErrors();
		}
	},

	/**
	 * Private: Check to see if any of the subforms are dirty
	 */
	_subFormsAreDirty: function()
	{
		var dirty = false;
		for (var i = 0; i < this.subForms.length; i++)
		{
			dirty = dirty || this.subForms[i].isDirty(true);
		}
		return dirty;
	},

	/**
	 * Private: Reset SubForm values
	 */
	_resetSubForms: function()
	{
		for (var i = 0; i < this.subForms.length; i++)
		{
			this.subForms[i].reset();
		}
	},

	/**
	 * Private: Submit all subforms
	 */
	_queueSubForms: function()
	{
		for (var i = 0; i < this.subForms.length; i++)
		{
			this.subForms[i]._submit();
		}
	},

	_callSubFormBeforeSubmit: function()
	{
		for (var i = 0; i < this.subForms.length; i++)
		{
			var sub = this.subForms[i];
			var subOptions = sub.options;
			if (subOptions.beforeSubmit)
			{
				if (subOptions.beforeSubmit(sub) === false)
					return false;
			}
		}

		return true;
	},

	_callSubFormSubmitComplete: function()
	{
		for (var i = 0; i < this.subForms.length; i++)
		{
			this.subForms[i]._submitComplete();
		}
	}

});

/**
 * Default vForm Error Formatter
 *
 * By default the vForm library uses this simple error formatter.  It simply
 * appends the error messages as html to the error container.  This class also
 * defines the interface that can be used to create custom error formatters.
 *
 * = Error Formatter Interface
 * To inplement a custom error formatter it is necessary to implement three
 * methods:
 *
 * createContainer(input options):
 * 		This method takes two arguments the input to create the container for
 *		and the options.error section of the inputs options.  It should create a
 *		container that will be used to display an error for an input.  The
 *		container should be a dom element ready to be inserted into the DOM.
 *
 * displayInputError(input, msgs):
 * 		This method should display the error messages for a given input. It
 * 		takes two arguments.  The first is the input the errors should be
 * 		displayed for.  This arguemnt may be a vFormFielf or simply an HTML
 * 		input.  The second argument is an array of in the validation error
 * 		messages for the input.  As is evident from name of the method, it
 * 		should take the error messages and display them on screen.
 *
 * hideInputError(input):
 * 		This method should hide the error messages for the give input.  It takes
 * 		as an argument the input that should have it's errors hidden.
 *
 **/
function vFormErrorFormatter(form) {
	this.vForm = form;
}

extend(vFormErrorFormatter, {

	defaultOptions:
	{
		tag: "div",
		addClass: null
	},

	createContainer: function(input, opts)
	{
		var options = $.extend(true, {}, this.defaultOptions, opts);
		var err = $(document.createElement(options.tag));
		err.attr("id", input.attr("id") + "_err");
		err.addClass("validationerror").hide();
		return err;
	},

	displayInputError: function(input, errors)
	{
		if (errors.length == 0)
			return;

		var err = this._findErrorContainer(input);

		var html = $("<p>");
		for (var i = 0; i < errors.length; i++)
		{
			var e = errors[i].replace(/(\r\n|\n)/g, "<br />");
			var span = $("<span>").text(e);
			html.append(span);
		}

		//var html = errors.join("<br/>");
		err.empty();

		err.html(html);
		err.fadeIn();
	},

	hideInputError: function(input)
	{
		var err = this._findErrorContainer(input);

		err.empty();
		err.fadeOut();
	},

	_findErrorContainer: function(input)
	{
		if (input.isVFormField)
		{
			if (input.errorContainer)
				return $(input.errorContainer);
		}

		var eid = input.attr("id") + "_err";

		var err = document.getElementById(eid);

		if (! err)
		{
			err = this.createContainer(input);

			$(err).insertAfter(input);
		}

		return($(err));
	}
});

/**
 * Bubble Error Formatter
 *
 * This Error formatter display error messages in transparent "thought bubble"
 * next to the field with the error.  Currently this formatter assumes you are
 * using only vFormFields created using the vForm.field() method.  It will not
 * work with directly created field (i.e. by calling vForm.textField())
 **/
function bubbleErrorFormatter(form)
{
	this.vForm = form;
	this.fallback = new vFormErrorFormatter(form);
	this.options = {};
}

extend(bubbleErrorFormatter, {

    createContainer: function (input, options)
    {
        if (options)
        {
            this.options = options;
        }
        var wrapper = $("<div>").attr("id", input.attr("id") + "_err");
        wrapper.addClass("bubbleError").hide();

        var pointer = $("<div>").addClass("errorPointer clearfix");
        var body = $("<div>").addClass("errorBody clearfix shadow");

        var text = $("<div>").addClass("errorText");
        var close = $("<a>").text("x").addClass("errorClose");
        close.attr("title", "Close");
        close.click(function (event)
        {
            wrapper.fadeOut();
        });
        body.append(text).append(close);

        wrapper.append(pointer).append(body);

        return wrapper;
    },

    displayInputError: function (input, errors, container)
    {
        if (errors.length == 0)
            return;

        var errorContainer = input.errorContainer || $("#" + $(input).attr("id") + "_err");
        // If this isn't a vFormField, fallback to the default error formatter
        if (!input.isVFormField && !errorContainer)
        {
            this.fallback.displayInputError(input, errors);
            return;
        }

        var html = $("<p>");
        for (var i = 0; i < errors.length; i++)
        {
            var e = errors[i].replace(/(\r\n|\n)/g, "<br />");
            var span = $("<span>").text(e);
            html.append(span);
        }

        $(".errorText", errorContainer).empty();
        $(".errorText", errorContainer).html(html);

        var field = input.field || input;
        var offset = field.position();
        var scrollTop = field.offsetParent().scrollTop();
        var height = field.height();
        var errorHeight = $(".bubbleError").height();
        //if the field is off the screen center the error message     
        if (!this.options.trustposition)
        {
            if (offset.top + height <= $(window).scrollTop() + $("#top").height() ||
            offset.top + height + 36 >= $(window).height() + $(window).scrollTop())
            {
                errorContainer.css({
                    "position": "fixed",
                    "top": "50%",
                    "left": "50%",
                    "margin-top": "-" + errorHeight / 2 + "px",
                    "margin-left": "-200px"

                });
                $(".errorPointer").hide();
            }
            else
            {
                // TODO need to take into account the fields position in the scrollable
                // TODO area in order to ensure we don't render the error message off screen
                errorContainer.css({
                    "top": (offset.top + height + 10) + "px",
                    "left": (offset.left + 10) + "px",
                    "margin-top": "0px",
                    "margin-left": "0px"
                });
                $(".errorPointer").show();
            }
        }

        errorContainer.fadeIn();
    },

    hideInputError: function (input)
    {
        var errorContainer = input.errorContainer || $("#" + $(input).attr("id") + "_err");
        if (errorContainer)
            errorContainer.fadeOut();
        else
            this.fallback.hideInputError(input);
    }
});

/**
 * vFormField input wrapper
 *
 * This object that is desinged to act as a wrapper for at set of elements that
 * are associated with a vForm form field.  This allows the library to easily
 * access other elements that are associated with the field wether they are
 * currently inserted in the DOM or not.
 *
 * = Constructor
 * The constructor for a vFormField takes 6 arguments:
 *
 * form:
 * 		The vForm this vFormField is associated with.
 * label (optional):
 * 		The label element for this field.
 * field:
 * 		The field for this vForm field.  It is not safe to simply assume this
 * 		will be an html element.  It might be a javascript object that is
 * 		representing a more complex input.
 * reader (optional):
 *		The html element that will be used as the read-only version of this
 *		field.  The vFormField object will observe any change in the value of
 * 		the field and populate the reader with the new value.  When the field
 *		is to be marked disabled, the html field will be hidden and this element
 * 		will be displayed.  If this argument is null the field will simply be
 *		disabled.
 * error:
 *		The error container for this field.  This element is created and used by
 *		the vFormErrorFormatter.
 * options:
 *		The input options for this vFormField.  These options are also stored in
 * 		this.field.data("options") but they are stored in the vFormField object
 *		as well in case the field has been removed from or is not yet inserted
 *		in the DOM.
 **/
function vFormField(form, label, field, reader, error, options)
{
	var vffself = this;

	this.isVFormField = true;
	this.vform = form || null;
	this.label = label || null;
	this.field = field || null;
	this.reader = reader || null;
	this.errorContainer = error || null;
	this.options = options || {};
	this.errors = [];
	this.enabled = true;

	this.field.change( function(event) {
		if (vffself.reader)
		{
			var val = vffself.getVal(true);

			vffself.setReaderVal(val);
		}
	});

	if (this.field.is("select"))
	{
		var self = this;
		this.addOption = function(val, text) {
			var opt = $(document.createElement('option')).
				val(val).
				text(text);

			self.field.append(opt);
		};
	}
}

extend(vFormField, {

    val: function ()
    {
        if (arguments.length == 0)
            return this.getVal();
        else
            this.setVal(arguments[0]);
    },

    getVal: function (raw)
    {
        if (raw == null || raw == undefined)
            raw = false;

        if (this.options)
        {
            var options = this.options;
        }
        else
        {
            var options = this.field.data("options") || {};
        }

        var name = options.name;
        var objectAllowed = options.objectAllowed || false;

        if ((this.options.isdate || (!!this.field.data('isdate'))) && this.field.hasClass("hasDatepicker"))
        {
            if (raw)
            {
                val = this.field.val();
            }
            else
            {
                var d = this.field.datepicker('getDate');
                if (!d)
                    val = null;
                else
                    val = d.getTime();
            }
        }
        else if ($.inArray(name, this.vform.multiNames) >= 0)
        {
            if (this.field.attr)
            {
                switch (this.field.attr("type"))
                {

                    case "checkbox":
                        if (this.field.is(":checked"))
                        {
                            val = field.val();
                        }
                        else
                        {
                            if (options.unchecked)
                                val = options.unchecked;
                        }
                        break;
                }
            }
            else
                val = field.val();
        }
        else
        {
            if (this.field.attr)
            {
                switch (this.field.attr("type"))
                {
                    case "radio":
                        val = $('input:radio[name=' + name + ']:checked').val();
                        break;
                    case "checkbox":
                        if (this.field.is(":checked"))
                        {
                            val = this.field.val();
                        }
                        else
                        {
                            if (options.unchecked)
                                val = options.unchecked;
                        }
                        break;
                    default:
                        val = this.field.val();
                        break;
                }
            }
            else
                val = this.field.val();

        }
        // IE HACK when the value of a select is an empty string
        // IE returns the value as an empty object
        // This causes a type mismatch in the backend
        if ((typeof val == "object") && ((val == null) || (val.length == 0)) && !objectAllowed)
        {
            val = "";
        }

        if (raw)
            return val;

        if (options && options.formatter)
        {
            val = options.formatter(val, "get");
        }

        if (options && options.scale)
        {
            val = (val || 0) * options.scale;
        }

        return val;
    },

    setVal: function (val, updateLastVal, setReader)
    {
        // If updateLastVal is set to false the field will remember it's original
        // value for last val instead of the new value.  This allows to you change
        // the field value through the library using the fields formatting
        // while still keeping the field dirty
        if (updateLastVal !== false)
            updateLastVal = true;
        if (setReader !== false)
            setReader = true;

        var options = this.field.data("options");
        if (options)
            var name = options.name;
        else
            var name = this.field.attr("name");

        if (options && options.scale)
        {
            if (!val)
                val = 0;
            else
                val = Math.floor(val / options.scale);
        }

        var lastVal = val;  //save the raw value for the lastval
        if (options && options.formatter)
        {
            val = options.formatter(val, "set");
        }

        if (options && options.userSelect)
        {
            var found = false;

            var opts = this.field.find("option");

            for (var i = 0; i < opts.length; ++i)
                if (opts[i].value == val)
                {
                    found = true;
                    break;
                }

            if (!found)
            {
                var iu = (this.vform && this.vform.inactiveUsers) ? this.vform.inactiveUsers : [];
                for (i = 0; i < iu.length; ++i)
                {
                    var u = iu[i];

                    if (u.value == val)
                    {
                        this.field.append(this.vform._createOption(u.name, u.value));
                        break;
                    }
                }
            }

            if (found)
            {
                this.field.val(val);
            }
        }


        if ((this.options.isdate || this.field.data('isdate')) && this.field.hasClass("hasDatepicker"))
        {
            if (val == "" || val == undefined || val == null)
                return;

            var date = new Date(parseInt(val));
            this.field.datepicker('setDate', date);

            // Get the correct value to set the reader with
            val = this.field.val();
            lastVal = this.getVal();
        }
        else if (this.vform && ($.inArray(name, this.vform.multiNames) >= 0))
        {
            if (typeof val == "string")
            {
                val = val.split(",");
            }

            $("input[name=" + name + "]").val(val);
        }
        else
        {
            if (this.field.attr)
            {
                switch (this.field.attr("type"))
                {
                    case "radio":
                        $("input[name=" + name + "]").val([val]);
                        break;
                    case "checkbox":
                        //If the record value is undefined, massage the value
                        if (val == undefined)
                            val = "0";

                        if (this.field.val() == val)
                        {
                            this.field.attr("checked", true);
                        }

                        break;
                    default:
                        this.field.val((val == undefined || val == null) ? "" : val);
                }
            }
            else
                this.field.val((val == undefined || val == null) ? "" : val);
        }


        if (updateLastVal)
            this.field.data("lastval", lastVal);

        if (setReader)
            this.setReaderVal(val);

        // Fields that are hidden don't have their change events fired
        if (!this.field.is(":visible"))
        {
            if (this.options.onchange)
                this.options.onchange(this.field);
        }

    },

    sanitizeText: function (txt)
    {
        return (vCore.sanitize(txt));
    },

    setReaderVal: function (val)
    {
        if (!this.vform.options.prettyReader || !this.reader)
            return;

        if (typeof val == "string")
            val = this.sanitizeText(val);

        if (this.options.reader.formatter)
        {
            val = this.options.reader.formatter(val);
        }
        if (typeof val == "string")
        {
            var linked = val;
            if (this.options.reader.markedDown)
            {
                this.reader.addClass("wiki");
                if (!this.markdownlinks)
                {
                    this.markdownlinks = new vvMarkdownLinks();
                }
            }
            else
            {
                linked = vCore.createLinks(val);
                linked = linked.replace(/(\r|\n)/g, "<br/>");
            }
            this.reader.html(linked);
            if (this.options.reader.markedDown)
            {
                this.markdownlinks.validateLinks(this.reader);
            }
        }
        else
        {
            this.reader.empty();
            this.reader.append(val);
        }
    },

    disable: function ()
    {
        if (!this.options.disableable || this.options.readOnly)
            return;

        this.enabled = false;
        if (this.reader)
        {
            this.reader.show();
            this.field.hide();
            if (this.fieldWrapper)
                this.fieldWrapper.hide();
        }
        else
            this.field.attr("disabled", true);

        if (this.options.reader && this.options.reader.hideLabel)
            this.label.hide();
    },

    enable: function ()
    {
        if (!this.options.disableable || this.options.readOnly)
            return;

        this.enabled = true;
        if (this.reader)
        {
            this.reader.hide();
            this.field.show();
            if (this.fieldWrapper)
                this.fieldWrapper.show();
        }
        else
        {
            if (this.field.removeAttr)
                this.field.removeAttr("disabled");
            else
                this.field.attr("disabled", false);
        }

        if (this.options.reader && this.options.reader.hideLabel)
            this.label.show();
    },

    hide: function ()
    {
        if (this.label)
            this.label.hide();
        if (this.reader)
            this.reader.hide();
        this.field.hide();
        this.errorContainer.hide();

    },

    show: function ()
    {
        if (this.label && !(!this.enabled && this.options.reader.hideLabel))
            this.label.show();
        if (this.reader && !this.enabled)
            this.reader.show();
        this.field.show();
        this.enabled ? this.enable() : this.disable();
    },

    attr: function ()
    {
        return this.field.attr.apply(this, arguments);
    },

    readOnly: function ()
    {
        if (!this.options.readOnly)
            return;

        if (this.reader)
        {
            this.reader.show();
            this.field.hide();
        }
        else
            this.field.attr("disabled", true);

        if (this.options.reader.hideLabel)
            this.label.hide();

    },

    data: function (key)
    {
        if (arguments.length == 1)
            return this.field.data(key);
        else
            this.field.data(key, arguments[1]);
    },

    _compareValues: function (one, two)
    {
        if (!one && !two)
            return true;
        if (one === "" || one === undefined)
            one = null;
        if (two === "" || two === undefined)
            two = null;

        return (one == two);
    },

    isDirty: function ()
    {
        var last = this.data("lastval");
        var val = this.getVal();

        if (this.field._compareValues)
            return (!this.field._compareValues(last, val));

        return !this._compareValues(last, val);
    },

    refresh: function ()
    {
        this.setVal(this.getVal());
    },

    reset: function ()
    {
        var last = this.data("lastval");
        this.setVal(last, false);
    }
});
