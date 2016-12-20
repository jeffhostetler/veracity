Displays the contents of one or more files.

On Linux and Mac the output is sent to stdout, which can be redirected
as needed. On Windows the output is sent directly to the console, which
cannot be redirected.  However, the -o option can be used to send the
output to a file.  (This restriction is necessary to allow
us to write binary and UNICODE-encoded files without being
altered by the Windows shell.)

There are 3 modes to 'vv cat':

[1] When the --repo option is used:

You must also specify a
changeset using either --rev, --tag, or --branch.  If using
--branch, it must resolve to a single head.  In this mode,
'vv cat' does not require or use a working copy.  All
files must be specified using a full repo-path "@/path/to/file"
and will be interpreted relative to the specified changeset;
relative paths are not permitted.
'vv cat' will display the version of each file as it existed
in that changeset.

[2] When the --repo option is omitted and you specify a
changeset using either --rev, --tag, or --branch:

In this mode, 'vv cat' will determine the repo-name using the current
working copy (which must exist) and look up the
requested changeset.  Absolute repo-paths of the form '@/path/to/file'
will be interpreted relative to the specified changeset; relative
and absolute paths (and extended-prefix repo-paths of the form '@b/...'
or '@c/...') will be interpreted relative to the CURRENT WORKING
COPY to identify the file.
Then the version of the file in the specified changeset
will be displayed.  That is, we only use the state of the working 
copy to identify/select
the file; we always get the content from the specified changeset.

[3] When the --repo option is omitted and no changeset is
specified (no --rev, --tag, or --branch):

In this mode, 'vv cat' will determine the repo-name using the current working copy (which must
exist).  'vv cat' will display the content of
each file as it existed in the working copy's parent changeset(s); normally,
this is the baseline, but if there is a pending merge you can also
'cat' the version in the other parent.  

[3a] When a qualified extended-prefix
repo-path of the form '@b/path/to/file/in/baseline' or '@c/path/to/file/in/other'
is given, the path will be interpreted relative to the baseline (@b) or other (@c)
parent.  Then 'vv cat' will display the content of the file as it was in that changeset.

[3b] When a relative or absolute path or a non-qualified
repo-path (of the form '@/path/to/file') is used, it will be
interpreted relative to the CURRENT WORKING COPY to identify the file.
If there is no merge pending, 'vv cat' will display the contents
of the file as it existed in the baseline parent.
If there is a merge pending, 'vv cat' will look up the file in both
parents.  If the file is identical in both versions or if it is only present
in one parent, the content will be displayed.  If the file is different in the
two parents, an error will be displayed recommending a qualified
repo-path be used.

At no point does 'vv cat' display the (possibly dirty)
version of the file as it exists in the current working copy;
use /bin/cat (or Windows "type") for that.  We only use the working copy to identify
the file.

