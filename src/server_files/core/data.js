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

var dispatchBlobDownload = function (request, download)
{
    // todo: return 404 error if hid not found
    var blob = request.repo.fetch_blob(request.hid, 2 * 1024 * 1024);

    var response = {
        statusCode: STATUS_CODE__OK,
        headers: {
            "Content-Length": blob.length          
        },
        onChunk: function () { return blob.next_chunk(request.repo); },
        onFinish: function () { blob.abort(request.repo); },
        finalize: function () { blob.abort(request.repo); }
    };

    if (download)
    {
        if (request.filename)
            response.headers["Content-Disposition"] = "attachment; filename=" + request.filename;
        else
            response.headers["Content-Disposition"] = "attachment";
    }

    if (request.contenttype)
    {
        response.headers["Content-Type"] = request.contenttype;
        response.headers["X-Content-Type-Options"] = "nosniff";
    }

    return response;
};

registerRoutes({
	"/repos/<repoName>/blobs/<hid>": {
		"GET": {
			onDispatch: function (request) {
				return dispatchBlobDownload(request, false);
			}
		}
	},
	"/repos/<repoName>/blobs/<hid>/<filename>.$": { // the ".$" at the end makes it so that request.filename includes the file suffix
		"GET": {
			onDispatch: function (request) {
				return dispatchBlobDownload(request, false);
			}
		}
	},
	"/repos/<repoName>/blobs-download/<hid>": {
		"GET": {
			onDispatch: function (request) {
				return dispatchBlobDownload(request, true);
			}
		}
	},
	"/repos/<repoName>/blobs-download/<hid>/<filename>.$": { // the ".$" at the end makes it so that request.filename includes the file suffix
		"GET": {
			onDispatch: function (request) {
				return dispatchBlobDownload(request, true);
			}
		}
	}
});
