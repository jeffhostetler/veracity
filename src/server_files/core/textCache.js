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

function textCache(request, dags, contentType){
sg.server.request_profiler_start(sg.server.CACHE_CHECK);
try{
	this.etag = "";
	this.cachefn = "";
	this.oldetag = "";
	this.urlhash = "";
	this.contentType = contentType || CONTENT_TYPE__JSON;

	this.oldetag = request.headers["If-None-Match"] || "";

	var urlHashStrs = 
		[
			request.requestMethod,
			request.uri,
			this._normalizeQs(request.queryString)
		];
	var dagHashStrs = [];

	dags = dags || [ sg.dagnum.VERSION_CONTROL ];

	for ( i = 0; i < dags.length; ++i )
	{
		var passdag = "" + dags[i];
		
		var leaves = request.repo.fetch_dag_leaves(passdag);

		for ( var j in leaves )
			dagHashStrs.push(leaves[j]);
	}

	this.urlHash = sg.hash(urlHashStrs);

	this.etag = this.urlHash + "." + sg.hash(dagHashStrs);

	this.cachedir = this._getCacheDir();
	this.cachefn = this.cachedir + this.etag;
}
finally{
	sg.server.request_profiler_stop();	
}
}

textCache.prototype._normalizeQs = function(qs)
{
	var result = '';
	var parts = vv.parseQueryString(qs);

	delete parts['_'];

	var keys = [];

	for ( var vari in parts )
		keys.push(vari);

	keys.sort();

	for ( var i = 0; i < keys.length; ++i )
	{
		if (result)
			result += "&";
		result += keys[i] + "=" + parts[keys[i]];
	}

	return(result);
};

/* Get the cachedir once per context */
var textCache_cachedir = null;
textCache.prototype._getCacheDir = function()
{
	if (textCache_cachedir === null)
	{
		var cachedir = serverConf.cache;

		if (cachedir)
		{
			if (! cachedir.match(/[\\\/]$/))
				cachedir += "/";
		}
		else
		{
			cachedir = sg.server_state_dir();

			if (! cachedir.match(/[\\\/]$/))
				cachedir += "/";

			cachedir += "vvcache/";
		}

		sg.mutex.lock(cachedir);
		try
		{
			if (! sg.fs.exists(cachedir))
			{
				sg.fs.mkdir(cachedir);
				sg.fs.chmod(cachedir, 0755);
			}
		}
		finally
		{
			sg.mutex.unlock(cachedir);
		}

		textCache_cachedir = cachedir;
		//sg.log("debug", "set textcache dir: " + textCache_cachedir);
	}

	return textCache_cachedir;
};

textCache.prototype._cleanOldCache = function(cachedir, prefix, currentfn)
{
	if (! sg.fs.exists(cachedir))
		return;

	//sg.log("debug", "textCache cleanup - triggered by write of: " + this.etag);

	try
	{
		prefix += ".";
		var files = sg.fs.ls(cachedir, prefix);

		for ( var i = 0; i < files.length; ++i )
		{
			var fn = files[i];
			
			if (fn != currentfn)
			{
				// The filename is the etag, so this is the same mutex name used in cache lookups.
				// This prevents us from deleting a file while somebody else expects it to exist, like
				// between the isCached check and the read from the file.
				sg.mutex.lock(fn);

				var rm_path = cachedir + fn;
				try
				{
					//sg.log("debug", "textCache cleanup - removing: " + rm_path);
					sg.fs.remove(rm_path);
				}
				catch (ex)
				{
					// The mutex we're holding does NOT prevent us from attempting to delete a file that another
					// thread got to first, which happens with some frequency. So we check for the file's 
					// existence here, before logging an error, to make sure it's not simply that. By doing the 
					// existence check here (rather than before the delete) we don't incur the perf hit of 
					// another stat in the common case; the file usually does exist when we try to delete it.
					if (sg.fs.exists(rm_path))
						sg.log("normal", "textCache cleanup failed to remove [" + rm_path + "] with " + ex);
				}
				finally
				{
					sg.mutex.unlock(fn);
				}
			}
		}
	}
	catch(ex)
	{
		sg.log("urgent", "textCache cleanup failed with: " + ex);
	}
	//finally
	//{
	//	sg.log("debug", "textCache cleanup done");
	//}
};

textCache.prototype._isCached = function() 
{
	sg.server.request_profiler_start(sg.server.CACHE_CHECK);
	try{
		return( this.etag && sg.fs.exists( this.cachefn ) );
	}
	finally{
		sg.server.request_profiler_stop();
	}
};

textCache.prototype._cacheWrite = function(textOrObj)
{
	sg.server.request_profiler_start(sg.server.CACHE_WRITE);
	try{
		
		//sg.log("debug", "textCache write: " + this.etag);

		var write = textOrObj;
		if ( typeof(textOrObj) == "object")
			write = sg.to_json(textOrObj);
	
		sg.file.write(this.cachefn, write, 0600);
		//sg.log("debug", "textCache write: " + this.etag + " - written");

		this.oldetag = this.etag;
	
		if (! sg.fs.exists(this.cachefn))
			throw "Unable to create cache file";

		return write;
	}
	catch(ex)
	{
		//sg.log("urgent", "textCache write for [" + this.etag + "] FAILED with " + ex);
	}
	finally{
		sg.server.request_profiler_stop();
	}
};

textCache.prototype.get_obj = function(fetchFunc, thisCtx)
{
	var returnObj = null;

	sg.mutex.lock(this.etag);
	try
	{
		if (! this._isCached())
		{
			//sg.log("debug", "textCache miss:" + this.etag);

			var fn = Array.prototype.shift.call(arguments);
			var t = Array.prototype.shift.call(arguments);
			returnObj = fn.apply(t, arguments);
			if (typeof(returnObj) != "object")
				throw("fetchFunc did not return an object");

			this._cacheWrite(sg.to_json(returnObj));
		}
	}
	finally
	{
		sg.mutex.unlock(this.etag);
	}

	if (returnObj === null)
	{
		sg.server.request_profiler_start(sg.server.CACHE_READ);
		try
		{
			//sg.log("debug", "textCache hit: " + this.etag);

			var cachedText = sg.file.read(this.cachefn);
			eval("returnObj=" + cachedText);
		}
		finally
		{
			sg.server.request_profiler_stop();
		}
	}
	else
	{
		/* We just wrote to the cache. It might be time to cleanup (but not while holding our etag mutex). */
		this._cleanOldCache(this.cachedir, this.urlHash, this.etag);
	}

	return returnObj;
};

textCache.prototype.get_text = function(fetchFunc, thisCtx)
{
	var returnText = null;
	sg.mutex.lock(this.etag);
	try
	{
		if (! this._isCached())
		{
			//sg.log("debug", "textCache miss: " + this.etag);

			var fn = Array.prototype.shift.call(arguments);
			var t = Array.prototype.shift.call(arguments);
			returnText = fn.apply(t, arguments);
			if (returnText !== null && typeof(returnText) != "string")
				throw("fetchFunc did not return a string");

			this._cacheWrite(returnText);
		}
	}
	finally
	{
		sg.mutex.unlock(this.etag);
	}

	if (returnText === null)
	{
		sg.server.request_profiler_start(sg.server.CACHE_READ);
		try
		{
			//sg.log("debug", "textCache hit: " + this.etag);
			returnText = sg.file.read(this.cachefn);
		}
		finally
		{
			sg.server.request_profiler_stop();
		}
	}
	else
	{
		/* We just wrote to the cache. It might be time to cleanup (but not while holding our etag mutex). */
		this._cleanOldCache(this.cachedir, this.urlHash, this.etag);
	}

	return returnText;
};

textCache.prototype.get_response = function(fetchFunc, thisCtx)
{
	var response = null;
	sg.mutex.lock(this.etag);
	try
	{
		if (! this._isCached())
		{
			//sg.log("debug", "textCache miss: " + this.etag);
			
			var fn = Array.prototype.shift.call(arguments);
			var t = Array.prototype.shift.call(arguments);
			try {
				response = textResponse(this._cacheWrite(fn.apply(t, arguments)), this.contentType);
			}
			catch (ex)
			{
				if (typeof(ex) == "object" && ex.statusCode !== undefined && ex.headers !== undefined)
					return ex;
				throw ex;
			}
		}
	}
	finally
	{
		sg.mutex.unlock(this.etag);
	}

	if (response === null)
	{
		sg.server.request_profiler_start(sg.server.CACHE_READ);
		try
		{
			//sg.log("debug", "textCache hit: " + this.etag);
			response = fileResponse(this.cachefn, false, this.contentType);
		}
		finally
		{
			sg.server.request_profiler_stop();
		}
	}
	else
	{
		/* We just wrote to the cache. It might be time to cleanup (but not while holding our etag mutex). */
		this._cleanOldCache(this.cachedir, this.urlHash, this.etag);
	}

	response.headers["ETag"] = this.etag;
	return response;
};

textCache.prototype.get_possible_304_response = function(fetchFunc, thisCtx)
{
	var response = null;
	sg.mutex.lock(this.etag);
	try
	{
		if (! this._isCached())
		{
			//sg.log("debug", "textCache miss: " + this.etag);

			var fn = Array.prototype.shift.call(arguments);
			var t = Array.prototype.shift.call(arguments);
			try 
			{
				response = textResponse(this._cacheWrite(fn.apply(t, arguments)), this.contentType);
				response.headers["ETag"] = this.etag;
			}
			catch (ex)
			{
				if (typeof(ex) == "object" && ex.statusCode !== undefined && ex.headers !== undefined)
					return ex;
				throw ex;
			}
		}
	}
	finally
	{
		sg.mutex.unlock(this.etag);
	}

	if (response === null)
	{
		sg.server.request_profiler_start(sg.server.CACHE_READ);
		try
		{
			if ( (this.etag == this.oldetag) )
			{
				//sg.log("debug", "textCache hit 304: " + this.etag);

				response = {
					statusCode: STATUS_CODE__NOT_MODIFIED,
					headers: {"Content-Length": 0}
				};
			}
			else
			{
				//sg.log("debug", "textCache hit: " + this.etag);

				response = fileResponse(this.cachefn, false, this.contentType);
				response.headers["ETag"] = this.etag;
			}
		}
		finally
		{
			sg.server.request_profiler_stop();
		}
	}
	else
	{
		/* We just wrote to the cache. It might be time to cleanup (but not while holding our etag mutex). */
		this._cleanOldCache(this.cachedir, this.urlHash, this.etag);
	}

	return response;
};
