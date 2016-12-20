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


var vv = {
    /**
    * called like jQuery.each (a.k.a. $.each)
    * 
    * vv.each(arrayOrObject, function(index,element) { } );
    */
    each: function (obj, fn)
    {
        if ((!obj) || (!fn))
            return;

        var isObj = (obj.length === undefined) ||
			(toString.call(obj) === "[object Function]");

        if (isObj)
        {
            for (var i in obj)
            {
                if (fn(i, obj[i]) === false)
                    break;
            }
        }
        else
        {
            var len = obj.length;

            for (var i = 0; i < len; ++i)
                if (fn(i, obj[i]) === false)
                    break;
        }
    },

    /**
    * build a zing 'where' query out of a hash of criteria, escapes values as needed
    * 
    * recognizes null as a special case, and leaves numeric values unquoted and unescaped
    * (based on runtime types, so parseInt() beforehand if needed)
    * 
    * to match multiple values on a single field, give an array of values instead of a single value
    * 
    * examples:
    *   match status 'open' and priority 'High'
    *   wherestr = vv.where( { 'status' : 'open', 'priority': 'High' } )
    * 
    *   match priority 'High'; status 'verified', 'fixed', or 'duplicate'
    *   wherestr = vv.where(
    *     {
    *       'priority': 'High',
    *       'status' : [ 'verified', 'fixed', 'duplicate' ]
    *     }
    * 
    *   match missing or blank value
    *   wherestr = vv.where(
    *     {
    *       'sprint' : null,
    *       'description': [ '', null ]
    *     }
    */
    where: function (criteria)
    {
        var wherestr = '';

        for (var field in criteria)
        {
            var value = criteria[field];

            if (value && (typeof (value) == 'object') && (value.length !== undefined))
            {
                var orStr = '';

                for (var i in value)
                {
                    orStr = this._addToWhere(orStr, field, value[i], '||');
                }

                var needParen = (wherestr.length > 0);

                if (needParen)
                {
                    wherestr = "(" + wherestr + ") && (";
                }
                wherestr += orStr;
                if (needParen)
                    wherestr += ")";
            }
            else
            {
                wherestr = this._addToWhere(wherestr, field, value, "&&", false);
            }
        }

        return (wherestr);
    },

    //same functionality as vv.where() but formats for the zing_db.q() function
    where_q: function (criteria)
    {
        var whereArray = [];

        for (var field in criteria)
        {
            var value = criteria[field];

            if (value && (typeof (value) == 'object') && (value.length !== undefined))
            {
                var orArray = [];

                for (var i in value)
                {
                    orArray = this._addToWhere_q(orArray, field, value[i], '||');

                }
                if (orArray)
                {
                    if (whereArray.length > 0)
                    {
                        whereArray = [whereArray, "&&", orArray];

                    }
                    else
                    {
                        whereArray = orArray;
                    }
                }
            }
            else
            {
                whereArray = this._addToWhere_q(whereArray, field, value, "&&", false);
            }
        }

        return (whereArray);
    },
    /**
    * return all fields, no links, of the versions of a record over time
    * 
    * each field has an _audit member added, which is an object with .who and .when members
    * 
    * parameters:
    *   db: the zingdb holding our record
    *   recid: the record's recid field
    * 
    * example:
    * [
    *	{
    *		"assignee" : "g1058ee013a9b4db892280707d6a3bd774df0a87afd9211dfb682c8bcc8e13b9a",
    *		"id" : "12345",
    *		"listorder" : "1",
    *		"recid" : "gc3e1dd28a1d541ea956c3a4bd36954a74e5313e8fd9211df8677c8bcc8e13b9a",
    *		"rectype" : "item",
    *		"reporter" : "g1058ee013a9b4db892280707d6a3bd774df0a87afd9211dfb682c8bcc8e13b9a",
    *		"status" : "open",
    *		"title" : "first work item",
    *		"_audit" : 
    *		{
    *			"who" : "g79ecb6261b384a4eaef8db97b7871d354df798b0fd9211dfbcbec8bcc8e13b9a",
    *			"when" : 1291239066564.000000
    *		}
    *	},
    *	{
    *		"assignee" : "g1058ee013a9b4db892280707d6a3bd774df0a87afd9211dfb682c8bcc8e13b9a",
    *		"description" : "",
    *		"id" : "12345",
    *		"listorder" : "1",
    *		"estimate" : "0",
    *		"priority" : "Medium",
    *		"recid" : "gc3e1dd28a1d541ea956c3a4bd36954a74e5313e8fd9211df8677c8bcc8e13b9a",
    *		"rectype" : "item",
    *		"reporter" : "g1058ee013a9b4db892280707d6a3bd774df0a87afd9211dfb682c8bcc8e13b9a",
    *		"status" : "open",
    *		"title" : "first work item",
    *		"verifier" : "",
    *		"_audit" : 
    *		{
    *			"who" : "g79ecb6261b384a4eaef8db97b7871d354df798b0fd9211dfbcbec8bcc8e13b9a",
    *			"when" : 1291315892218.000000
    *		}
    *	}
    * ]
    *
    */
    recordHistory: function (db, recid)
    {
        var results = [];
        var hidHistory = {};
        var data = db.get_record_all_versions(recid);
        var rec = db.get_record(recid, ['*', 'history']);

        if (rec.history)
        {
            for (var j = 0; j < rec.history.length; ++j)
            {
                var hist = rec.history[j];
                var hid = hist.hidrec;

                if ((!hist.audits) || (hist.audits.length < 1))
                    continue;

                hidHistory[hid] = hist.audits[0];
            }
        }

        for (i = 0; i < data.length; ++i)
        {
            rec = data[i];
            var hid = rec.hidrec;

            if (!hidHistory[hid])
                continue;

            rec._audit = hidHistory[hid];
            delete rec.hidrec;
            delete rec.history;

            results.push(rec);
        }

        results.sort(
			function (a, b)
			{
			    return (a._audit.when - b._audit.when);
			}
		);

        return (results);
    },

    /**
    * build a lookup table based on one field of a list of records (default: recid)
    * 
    * each entry in the table associates that field value with the whole record
    * 
    * example:
    *   vv.buildLookup( 
    *      [
    *        { recid: "1", val: "one" },
    *        { recid: "a", val: "dos" }
    *      ])
    *   ==
    *   {
    *      "1" : {recid: "1", val: "one" },
    *      "a" : {recid: "a", val: "dos" } 
    *   }
    * 
    *  the optional second parameter names a field to use instead of 'recid'
    */
    buildLookup: function (recarray, field)
    {
        var results = {};
        var fn = field || "recid";

		for ( var i = 0; i < recarray.length; ++i )
        {
            var rec = recarray[i];

            results[rec[fn]] = rec;
        }

        return (results);
    },

    _users: {},
    _userLeafHids: {},

    lookupUser: function (userid, repo)
    {
        var repokey = repo.repo_id + repo.name;

        var udb = new zingdb(repo, sg.dagnum.USERS);
        var hids = udb.get_leaves();
        var userLeafHid = hids[0];

        if ( !this._users[repokey] || (this._userLeafHids[repokey] != userLeafHid) )
        {
            var userrecs = udb.query('user', ['*']);

            for (var i = 0; i < userrecs.length; ++i)
                if (userrecs[i].inactive)
                    userrecs[i].name += ' (inactive)';

            this._users[repokey] = this.buildLookup(userrecs);
            this._userLeafHids[repokey] = userLeafHid;
        }

        if (this._users[repokey][userid])
            return (this._users[repokey][userid].name);

		vv.log(userid, "- not found in", this._users[repokey], [repo.name, repo.repo_id]);

        return (false);
    },

	// given a record with rec.history[], will find the latest-gen history record (or NULL)
	getLastHistory: function(record)
	{
		var newest = null;
		
		var hist = record.history || [];

        // TODO these history arrays are already sorted most-recent-first.
        // TODO can't we just grab the entry at index 0?
		for ( var i = 0; i < hist.length; ++i )
		{
			if (! newest)
			{
				newest = hist[i];
				continue;
			}

			if (hist[i].generation > newest.generation)
				newest = hist[i];
		}

		return(newest);
	},

    // TODO is the following function an identical copy of the one just above?
	getLastHistory: function(record)
	{
		var newest = null;
		
		var hist = record.history || [];

		for ( var i = 0; i < hist.length; ++i )
		{
			if (! newest)
			{
				newest = hist[i];
				continue;
			}

			if (hist[i].generation > newest.generation)
				newest = hist[i];
		}

		return(newest);
	
	},

	getLastAudit: function(record)
	{
		var newest = this.getLastHistory(record);
		var lastAudit = null;
		
		if (newest)
		{
			var audits = newest.audits || [];
			for ( var i = 0; i < audits.length; ++i )
			{
				if (! lastAudit)
				{
					lastAudit = audits[i];
					continue;
				}

				if (audits[i].timestamp > lastAudit.timestamp)
					lastAudit = audits[i];
			}
		}

		return(lastAudit);
	},

	// given a record with rec.history[], will find the oldest-gen history record (or NULL)
	getFirstHistory: function(record)
	{
		var oldest = null;
		
		var hist = record.history || [];

		for ( var i = 0; i < hist.length; ++i )
		{
			if (! oldest)
			{
				oldest = hist[i];
				continue;
			}

			if (hist[i].generation < oldest.generation)
				oldest = hist[i];
		}

		return(oldest);
	},

	// given a record with rec.history[].audits[], will find the oldest audit on the first-gen history record (or NULL)
	getFirstAudit: function(record)
	{
		var oldest = this.getFirstHistory(record);
		var firstAudit = null;
		
		if (oldest)
		{
			var audits = oldest.audits || [];
			for ( var i = 0; i < audits.length; ++i )
			{
				if (! firstAudit)
				{
					firstAudit = audits[i];
					continue;
				}

				if (audits[i].timestamp < firstAudit.timestamp)
					firstAudit = audits[i];
			}
		}

		return(firstAudit);
	},

    parseQueryString: function (qs)
    {
        qs = qs.split("+").join(" ");
        var valArray = qs.split("&");
        var retObj = {};
        var i;
        var nameAndValue;

        for (i = 0; i < valArray.length; i++)
        {
            if (valArray[i].length > 0)
            {
                nameAndValue = valArray[i].split("=");
                retObj[unescape(nameAndValue[0])] = (unescape(nameAndValue[1]) || '');
            }
        }

        delete retObj['_'];

        return retObj;
    },
    getQueryParams: function (qs)
    {
        if (qs)
        {
            qs = qs.split("+").join(" ");
            var params = {};
            var tokens;

            var queryParamsRegEx = new RegExp("[?&]?([^=]+)=([^&]*)", "g");
            while (tokens = queryParamsRegEx.exec(qs))
            {
                params[decodeURIComponent(tokens[1])]
			    = decodeURIComponent(tokens[2]);
            }

            delete params['_'];

            return params;
        }
        return null;
    },
    getQueryParam: function (key, querystring)
    {
        if (querystring)
        {
            var vals = this.getQueryParams(querystring);
            return vals[key];
        }
        return null;
    },

    queryStringHas: function (qs, part)
    {
        var parts = this.parseQueryString(qs);

        return (parts[part] !== undefined);
    },

    txcommit: function (ztx)
    {
        var ct = ztx.commit();

        if (ct.errors)
        {
            var msg = sg.to_json(ct);

            throw (msg);
        }

        return (ct);
    },

    _addToWhere: function (wherestr, field, value, op, compareOperator)
    {
        var needParen = wherestr.length > 0;
        var result = '';

        var compareOp = compareOperator || "==";

		var matches = field.match(/^!\s*(\S+)/);
		if (matches)
		{
			if (compareOp != '==')
				throw "Negation can only be used with equality operations";

			field = matches[1];
			compareOp = '!=';
		}

        if (needParen)
        {
            result += "(";
            result += wherestr;
            result += ") " + op + " (";
        }

        result += field + " " + compareOp + " " + this._sqlEscape(value) + "";
        if (needParen)
            result += ")";

        return (result);
    },

    //same functionality as _addToWhere except formats for the zing_db.q() function syntax
    _addToWhere_q: function (whereArray, field, value, op, compareOperator)
    {
        var needWrap = whereArray.length > 0;
        var result = null;
        var compareOp = compareOperator || "==";
        var where = [field, compareOp, value];
        if (needWrap)
        {
            result = [whereArray, op, where];
        }
        else
        {
            result = where;
        }

        return (result);

    },
    _sqlEscape: function (st, forFts)
    {
        if (st === null)
            return ('#NULL');
		else if (st === 0)
			return('0');
		else if (st === false)
			return('#FALSE');
		else if (st === true)
			return('#TRUE');
        else if (!st)
            return ("''");
        else if (typeof (st) != 'string')
            return (st);

        st = st.replace(/'/g, "''");

		if (forFts)
		{
			st = st.replace(/\(/g, "\\(");
			st = st.replace(/\)/g, "\\)");
		}
		
        return ("'" + st + "'");
    },

    // Borrowed from jQuery
    inArray: function (elem, array)
    {
        if (array)
        {
            if (array.indexOf)
            {
                return array.indexOf(elem);
            }

            for (var i = 0, length = array.length; i < length; i++)
            {
                if (array[i] === elem)
                {
                    return i;
                }
            } 
        }

        return -1;
    },


    extend: function (result /*  , ... */)
    {
        for (var i = 1; i < arguments.length; ++i)
        {
            var o = arguments[i] || {};

            for (var f in o)
                if (typeof (result[f]) == 'undefined')
                    result[f] = o[f];
        }

        return (result);
    },

    /**
    * Log to testlib.log (if defined), or sg.log otherwise.
    * 
    * Takes a variable number of parameters, outputting all in one space-separated string.
    * Object parameters are JSONified on the way, others are logged as their toString() equivalents.
    * Null, undefined, other 'false' values evaluate to ''.
    */
    log: function ()
    {
        var msg = '';

        for (var i = 0; i < arguments.length; ++i)
        {
            var txt = arguments[i] || '';

            if (typeof (txt) == 'object')
                txt = sg.to_json(txt);
            else if (typeof (txt) != 'string')
                txt = txt.toString();

            msg += txt + " ";
        }

        if (typeof (testlib) != 'undefined')
            testlib.log(msg);
        else
            sg.log(msg);
    },

    array_intersect: function (arr1, arr2)
    {
        var res = [];

        var pArr = (arr1.length >= arr2.length) ? arr2 : arr1;
        var pArr2 = (arr1.length >= arr2.length) ? arr1 : arr2;

        for (var i = 0; i < pArr.length; i++)
        {
            var val = pArr[i];
            if (pArr2.indexOf(val) != -1)
                res.push(val)
        }

        return val;
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
	
	firstLineOf: function(x){
		var i = 0;
		while(i<x.length && x[i]!="\r" && x[i]!="\n"){++i;}
		return x.slice(0,i);
	},
	
	ltrim: function(x){
		var i=0;
		while(i<x.length && vv.isBreakingWhitespace(x[i])){++i;}
		return x.slice(i);
	},
	
	isBreakingWhitespace: function(x){
		return x==" " || x=="\u0009" || x=="\u000A" || x=="\u000B" || x=="\u000C" || x=="\u000D";
	},
	
	startswith: function(str, prefix){
		if(str.length < prefix.length){return false;}
		for(var i=0; i<prefix.length; ++i){if(str[i]!=prefix[i]){return false;}}
		return true;
	},
	
	strcmpi: function(x,y){
		return x.toLocaleLowerCase().localeCompare(y.toLocaleLowerCase());
	},
	
	startswithi: function(str, prefix){
		if(prefix===""){return true;}
		str = str.toLocaleLowerCase();
		prefix = prefix.toLocaleLowerCase();
		if(str.length < prefix.length){return false;}
		for(var i=0; i<prefix.length; ++i){if(str[i]!=prefix[i]){return false;}}
		return true;
	}
	
};

if (!Object.keys) {
  Object.keys = (function () {
    var hasOwnProperty = Object.prototype.hasOwnProperty,
        hasDontEnumBug = !({toString: null}).propertyIsEnumerable('toString'),
        dontEnums = [
          'toString',
          'toLocaleString',
          'valueOf',
          'hasOwnProperty',
          'isPrototypeOf',
          'propertyIsEnumerable',
          'constructor'
        ],
        dontEnumsLength = dontEnums.length

    return function (obj) {
      if (typeof obj !== 'object' && typeof obj !== 'function' || obj === null) throw new TypeError('Object.keys called on non-object')

      var result = []

      for (var prop in obj) {
        if (hasOwnProperty.call(obj, prop)) result.push(prop)
      }

      if (hasDontEnumBug) {
        for (var i=0; i < dontEnumsLength; i++) {
          if (hasOwnProperty.call(obj, dontEnums[i])) result.push(dontEnums[i])
        }
      }
      return result
    }
  })()
};
