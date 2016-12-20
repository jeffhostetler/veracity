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

function vv_serve_process() {
	var port = arguments[0];
	var options = arguments[1] || {};

	this.port = port.toString();
	this.url = 'http://localhost:'+this.port;

	if (repInfo)
	{
		sg.fs.cd(repInfo.workingPath);
		if (!options.runFromWorkingCopy)
			sg.fs.cd("..");
	}

	sg.set_local_setting("server/enable_diagnostics", "true");
	sg.set_local_setting("server/enable_auth", options.enableAuth===true ? "true" : "false");

	var pathPrefix = ".";
	if (options.runFromWorkingCopy)
		pathPrefix = "..";

	this.stdoutfilepath = pathPrefix + "/serve-" + this.port + "-stdout.txt";
	this.stderrfilepath = pathPrefix + "/serve-" + this.port + "-stderr.txt";
	/* If these files are sitting around from a previous test run, their (old) contents can end up in a log causing much confusion. So we empty them. */
	sg.file.write(this.stdoutfilepath, "");
	sg.file.write(this.stderrfilepath, "");

	print("Starting server on port " + port + " in working dir " + sg.fs.getcwd());
	this.pid = sg.exec_nowait(["output=" + this.stdoutfilepath, "error=" + this.stderrfilepath], "vv", "serve", "--port=" + this.port);

	if (repInfo)
		sg.fs.cd(repInfo.workingPath);

	// Wait for the server to finish starting up before running tests against it.
	var startTime = sg.time();
	var status = curl("http://localhost:"+this.port+"/ui/sg.css").status;
	while(status!="200 OK"){
		print(status);
		if(sg.time()-startTime > 15000)
			throw "Web server took longer than 15 seconds to start up!";
		sg.sleep_ms(10);
		status = curl("http://localhost:"+this.port+"/ui/sg.css").status;
	}
	print("We waited " + (sg.time()-startTime).toString() + "ms for the server to start up.");

	this.stop = function (wait_ms) {
		print ("In vv_serve_process.stop()");

		sg.set_local_setting("server/enable_diagnostics", "");

		if (sg.config.debug) {
			if (repInfo && repInfo.secretKey)
				sg.exec("curl", "-d", "{\"test\":true}", "-H", "X-vv-cirrus-key: " + repInfo.secretKey, "http://localhost:" + this.port + "/test/remote-shutdown");
			else
				sg.exec("curl", "-d", "{\"test\":true}", "http://localhost:" + this.port + "/test/remote-shutdown");

			//Reset enable_auth after the sever has shutdown
			sg.set_local_setting("server/enable_auth", "");

			// Print server's output so that its memleaks will be caught.
			print();
			print("Waiting for 'vv serve' to shut down before we print its stdout/stderr output...");
			var stopTime = sg.time();
			var result = sg.exec_result(this.pid);
			while(result===undefined) {
				if(sg.time()-stopTime > 120000)
					throw "Web server took longer than 2 minutes to shut down!";
				sg.sleep_ms(10);
				result = sg.exec_result(this.pid);
			}
			print("We waited " + (sg.time()-stopTime).toString() + "ms for the server to shut down. exec_result was "+result+".");

			print();

			if (repInfo)
			{
				sg.fs.cd(repInfo.workingPath);
				if (!options.runFromWorkingCopy)
					sg.fs.cd("..");
			}
			print();
			print(sg.file.read(this.stdoutfilepath));
			print();
			print();
			print(sg.file.read(this.stderrfilepath));
		}
		else {
			// Just kill the server.
			if (sg.platform() == "WINDOWS")
				sg.exec("taskkill", "/PID", this.pid.toString(), "/F");
			else
				sg.exec("kill", "-s", "SIGINT", this.pid.toString());
		}
	};
}
