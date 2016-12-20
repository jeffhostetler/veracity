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
 * ajaxQueue Object for chained ajax execution
 * 
 * This Object allows you to create a chain of ajax requests that will be 
 * executed in insertion order.  Each request will be executed once the previous
 * ajax request has returned successfully.
 * 
 * = Queue Setup
 * To add items to the ajax queue you simply pass a js object of ajax options
 * (similar to those that you would use to call vCore.ajax() or $.ajax) to the 
 * addQueueRequest() method.  This set of options does include one additional
 * call back
 * 
 * == beforeQueueDispatch Callback
 * 
 * This callback will be called by the execute method before the ajax call is
 * to be dispatched.  This callback takes the ajax request options as it's only
 * argument.  This call back allows the request to be modified before it is
 * dispatched.
 * 
 * <pre>
 * var queue = new ajaxQueue();
 * var recid = null
 * 
 * queue.addQueueRequest({
 * 		url: <save record url>,
 * 		success: function(data, xhr, status) {
 * 			recid = data.recid;
 * 		}
 * });
 * 
 * queue.addQueueRequest({
 * 		beforeQueueDispatch: function(request) {
 * 			request.url = vCore.urlForRecord("sprint", recid, "update")
 * 		}
 * });
 * </pre>
 * 
 * = Callbacks
 * 
 * 	onSuccess -	Called when the queue has successfully executed all queued items
 * 
 * 	onError - 	Called when any of the queue's requests errors.  When the queue
 * 				errors the queue will be emptied of all pending requests.
 */
function ajaxQueue(options)
{
	this.options = $.extend({
		onSuccess: null,
		onError: null
	}, (options || {}));
	
	this.queue = [];
	
	/**
	 * Executed the queue
	 * 
	 * This method will executed the first queued item after the beforeQueueDispatch
	 * callback has been executed.  This method will be called recursively after 
	 * each successful request until there are not any requests in the queue.
	 */
	this.execute = function()
	{
		
		if (this.queue.length == 0)
		{
			if (this.options.onSuccess)
				this.options.onSuccess();
			return;
		}

		var request = this.queue.shift();
		
		if (request.beforeQueueDispatch)
			request.beforeQueueDispatch(request);
		
		vCore.ajax(request);
	};

	this.writesPending = function()
	{
		var count = 0;

		for ( var i = 0; i < this.queue.length; ++i )
		{
			var options = this.queue[i] || {};

			if (options.type && (options.type.toLowerCase() != "get"))
				++count;
		}

		return( count );
	};
	
	/**
	 * Add a request to the ajaxQueue
	 * 
	 * This method takes a JS object representing the ajax request.
	 */
	this.addQueueRequest = function(ajaxOptions)
	{
		var onSuccess = ajaxOptions.success;
		var onError = ajaxOptions.error;
		var self = this;
		
		ajaxOptions.success = function(data, status, xhr)
		{
			if (onSuccess)
				onSuccess(data, status, xhr);
			
			self.execute();
		};
		
		ajaxOptions.error = function(xhr, status, errorthrown)
		{
			vCore.consoleLog("ajaxQueue: ajax error");

			if (onError)
				onError(xhr, status, errorthrown);
			
			self._onError(xhr, status, errorthrown);
		};
		
		this.queue.push(ajaxOptions);
	};
	
	this._onError = function(xhr, status, errorthrown)
	{
		if (! this.options.saveQueue)
			this.queue = [];
		if (this.options.onError)
			this.options.onError(xhr, status, errorthrown);
	};
	
	this.clearQueue = function()
	{
		this.queue = [];
	};
}
