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
 * Generated from @/src/docs/vscript-usage.md
 * 
 * Not part of the build, since generation requires a working vscript.
 *
 * To regenerate the following code manually, run:
 *
 *     vscript ../docs/md2c.js ../docs/vscript-usage.md tmpfile.c
 *
 * and replace everything below this comment with the contents of tmpfile.c
 */

const char *vscript_usage = ""
	"usage: vscript [--no-modules] [--help] [-f scriptfile] [-e script] [-i]"
	"\n"
	"[scriptfile] [scriptarg ...]"
	"\n"
	"\n"
	"    -f scriptfile"
	"\n"
	"        Load and execute the contents of 'scriptfile'. This may be used more"
	"\n"
	"        than once;"
	" "
	"scripts will be evaluated in order"
	"\n"
	"    -e script"
	"\n"
	"        Parse and execute 'script' as literal javascript."
	"\n"
	"    -i"
	"\n"
	"        Run vscript's interactive console after loading scripts."
	"\n"
	"    --no-modules"
	"\n"
	"        Tells vscript not to attempt to load any external veracity modules."
	"\n"
	"    --help"
	"\n"
	"        Displays vscript's version number and this usage message."
	"\n"
	"\n"
	"Running vscript with no script arguments ("
	"'-e'"
	", "
	"'-f'"
	", "
	"'scriptfile'"
	") will enter"
	"\n"
	"vscript's interactive javascript console."
	"\n"
	"\n"
	"If any scripts *are* listed on the command line, by default vscript will"
	"\n"
	"execute them and exit immediately. To load"
	" "
	"scripts before launching the"
	"\n"
	"interactive console, add the "
	"'-i'"
	" parameter."
	"\n"
	"\n"

;
