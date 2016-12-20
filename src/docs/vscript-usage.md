usage: vscript [--no-modules] [--help] [-f scriptfile] [-e script] [-i] [scriptfile] [scriptarg ...]

\-f scriptfile
: Load and execute the contents of 'scriptfile'. This may be used more than once;
  scripts will be evaluated in order
\-e script
: Parse and execute 'script' as literal javascript.
\-i
: Run vscript's interactive console after loading scripts.
\--no-modules
: Tells vscript not to attempt to load any external Veracity modules.
\--help
: Displays vscript's version number and this usage message.

Running vscript with no script arguments (`-e`, `-f`, `scriptfile`) will enter vscript's interactive javascript console. 

If any scripts *are* listed on the command line, by default vscript will execute them and exit immediately. To load
scripts before launching the interactive console, add the `-i` parameter.

