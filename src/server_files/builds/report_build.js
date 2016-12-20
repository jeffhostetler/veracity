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

load("report_build_lib.js");

function printUsageError()
{
	print_parseable("Usage: report_build.js <REPO NAME> new <NAME> <SERIES> <ENVIRONMENT> <STATUS> <CHANGESET> [<BRANCH NAME>] [<PRIORITY>]");
	print_parseable("                    <REPO NAME> update <BUILD RECID> <STATUS> [<PRIORITY>]");
	print_parseable("                  <REPO NAME> add-urls <BUILD RECID> <URL> <NAME> [<URL> <NAME>]...");
	print_parseable("        <REPO NAME> add-urls-from-file <BUILD RECID> <FILE PATH>");
	print_parseable("                    <REPO NAME> delete <BUILD RECID> [<BUILD RECID>]...");
	print_parseable("                      <REPO NAME> list [<QUERY>] [<SORT>] [<LIMIT>]");
	print_parseable("         <REPO NAME> get-pending-build <ENVIRONMENT> <OLDEST|NEWEST> <STATUS> [<STATUS>]...");
	print_parseable("        <REPO NAME> update-prev-builds <BUILD RECID> <FIND STATUS> <UPDATE TO STATUS>");

	return -1;
}

function main(arguments)
{
	var repo;
	var exitCode;
	var query = null;
	var sort = null;
	var limit = null;

	if (arguments.length >= 2)
	{
		repo = sg.open_repo(arguments[0]);
		try
		{
			switch(arguments[1])
			{
			case "new":
				if (arguments.length != 7 && arguments.length != 8 && arguments.length != 9)
					exitCode = printUsageError();
				else
				{
					var branchArg = null;
					var priorityArg = null;
					if (arguments.length > 7 && arguments[7] != "null")
						branchArg = arguments[7];
					if (arguments.length > 8 && arguments[8] != "null")
						priorityArg = arguments[8];
					exitCode = new_build(repo, arguments[2], arguments[3], arguments[4], arguments[5], arguments[6], branchArg, priorityArg);
				}
				break;
			case "update":
				if (arguments.length != 4 && arguments.length != 5)
					exitCode = printUsageError();
				else
				{
					var statusArg = null;
					priorityArg = null;

					if (arguments.length > 3 && arguments[3] != "null")
						statusArg = arguments[3];
					if (arguments.length > 4 && arguments[4] != "null")
						priorityArg = arguments[4];

					exitCode = update_build(repo, arguments[2], statusArg, priorityArg);
				}
				break;
			case "add-urls":
				if (arguments.length == 4 || (arguments.length > 4 && arguments.length % 2 == 1))
					exitCode = add_urls(repo, arguments[2], arguments.slice(3));
				else
					exitCode = printUsageError();
				break;
			case "add-urls-from-file":
				if (arguments.length != 4)
					exitCode = printUsageError();
				else
					exitCode = add_urls_from_file(repo, arguments[2], arguments[3]);
				break;
			case "delete":
				if (arguments.length < 3)
					exitCode = printUsageError();
				else
					exitCode = delete_builds(repo, arguments.slice(2));
				break;
			case "list":
				if (arguments.length < 2 || arguments.length > 5)
					exitCode = printUsageError();
				else
				{
					if (arguments.length > 2 && arguments[2] != "null")
						query = arguments[2];
					if (arguments.length > 3 && arguments[3] != "null")
						sort = arguments[3];
					if (arguments.length > 4 && arguments[4] != "null")
						limit = parseInt(arguments[4]);
					exitCode = list_builds(repo, query, sort, limit);
				}
				break;
			case "get-pending-build":
				if (arguments.length < 5)
					exitCode = printUsageError();
				else
					exitCode = get_pending_build(repo, arguments[2], arguments[3], arguments.slice(4));
				break;
			case "update-prev-builds":
				if (arguments.length < 5)
					exitCode = printUsageError();
				else
					exitCode = update_prev_builds(repo, arguments[2], arguments[3], arguments[4]);
				break;
			default:
				exitCode = printUsageError();
			}
		}
		finally
		{
			repo.close();
		}

	}
	else
	{
		exitCode = printUsageError();
	}

	output.exitCode = exitCode;
	print(sg.to_json__pretty_print(output));
	quit(exitCode);
}

main(arguments);
