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

/*
This file is a drop-in replacement for vv_serve_process.js. It is for
running web tests against a server that you have started manually.

Normally, vv_serve_process():
(1) Sets server/enable_diagnostics
(2) cd's out of the working copy if !runFromWorkingCopy
(3) Starts the server on the given port
(4) Provides a stop() function that the suite uses to stop the server

Replace vv_serve_process.js with this file in order to take these
matters into your own hands.

Regarding (1):
You are still responsible for setting server/enable_diagnostics before
starting the server, if needed for the test(s) you are running.

Regarding (2):
Currently, only tests that don't require a working copy are supported.
(You can't have cd'ed into a working copy for the repo before starting
the server since the repo doesn't exist before starting a given test
run.) Mosts tests don't need a working copy anyway.

Regarding (3):
We assume port 8080, but you can change it in the code below. This is
where the tests get their url from, so it's the only change needed.

Regarding (4):
The stop function is a no-op, and we leave the server running. Shut it
down manually when desired. Leak checking is thus manual too :-) .
*/

function vv_serve_process(port, runFromWorkingCopy) {
	this.url = 'http://localhost:8080';

	this.stop = function () {}
}
