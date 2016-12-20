
File Classes
----

A "file class" is just a type of file, such as "text" or "binary".  Known classes are defined in config.  Files are divided into classes so that some file-based behaviors can be customized and/or optimized appropriately.


File Class Configuration Settings
---

The following settings are used to configure file classes.

`fileclasses/CLASS`
: The settings for a class named CLASS are stored under this tree.  A class' name is basically arbitrary, but by convention the names of built-in classes start with a colon.
`fileclasses/CLASS/patterns`
: A list/array of filename patterns that are used to recognize files of the class CLASS.  Patterns should never include path information.  When setting this value using `vv config`, remember to use `add-to` instead of `set` because this is a list.  Additionally, if you're adding any patterns that contain wildcards which your shell can expand, remember to escape them appropriately to get them past the shell.


Built In Classes
---

There are two built in classes called `:text` and `:binary`, which are defined in the `default` config scope.  If a file cannot be classified by any other means, then a binary file detection algorithm is used to place the file in one of these two classes.


Tools
---

A "tool" is a single external program that can be invoked in a particular manner to view and/or operate on a file.  Tools can either be built into the system or they can be defined in config.  Multiple tools can be configured that call the same external program using different parameters, if necessary.  Tools are divided into "types", based on their purpose.  The types of tools that are currently implemented are listed below, along with more details about each.  Each tool has a unique name among all tools of the same type.

Some types of tools can be used in different "contexts".  For example, merge tools might be used during a `merge` command or a `resolve` command, and it can be helpful to configure different tools to be used in each context (for example, you probably want an interactive merge tool for `resolve`, but an automated one for `merge`).  Each type of tool defines its own contexts, see each type's details below for more information.


Tool Configuration Settings
---

The following settings are used to configure tools:

filetools/TYPE/TOOL
: The settings for a tool named TOOL of type TYPE are stored under this tree.  For example, the settings for a merge tool called `diffmerge` are stored under `filetools/merge/diffmerge`.  A tool's name is basically arbitrary, but by convention the names of internal tools start with a colon.
filetools/TYPE/TOOL/path
: The name of the tool's executable.  Must include the full path unless it's in the user's environment PATH.  May also be a list of path/filenames, in which case the first listed file that exists on disk will be used.  If none of the files in the list exist then the last one will still be used, in case it is a plain executable that is expected to be in the user's PATH.
filetools/TYPE/TOOL/args
: A list of command line arguments to pass to the tool when invoked.  Each flag and argument must be a separate entry in this list.  These arguments may contain special tokens to pass dynamic data to the tool (see Tokens, below).  When adding items to this list using the `vv config` command, remember to use `add-to` instead of `set` because this is a list.  You will also need to escape flags (arguments beginning with `-`), otherwise `vv` will interpret the flag as an argument to itself, rather than interpreting it as config data.  To escape the flag, prepend a backslash (i.e. '\-').  Note that if your shell recognizes backslashes, you may have to escape it with another backslash.
filetools/TYPE/TOOL/stdout
: A filename that should be bound to the tool's STDOUT stream when it is invoked.  This value may contain special tokens to pass dynamic data to the tool (see Tokens, below).
filetools/TYPE/TOOL/stderr
: A filename that should be bound to the tool's STDERR stream when it is invoked.  This value may contain special tokens to pass dynamic data to the tool (see Tokens, below).
filetools/TYPE/TOOL/stdin
: A filename that should be bound to the tool's STDIN stream when it is invoked.  This value may contain special tokens to pass dynamic data to the tool (see Tokens, below).
filetools/TYPE/TOOL/flags
: A list of flags that customize how the system invokes the tool.  Different flags are available for different types of tools, see each tool's section below for more information about available flags.  When adding items to this list using the `vv config` command, remember to use `add-to` instead of `set` because this is a list.
filetools/TYPE/TOOL/exit/CODE
: An optional mapping to convert program-specific EXIT codes into a canonical RESULT.  CODE represents an integer and RESULT is a symbol name; these are explained under each type of tool.

Tokens
---

Most of a tool's config settings may contain special tokens that will be replaced with actual data when the tool is invoked.  These tokens are how Veracity passes dynamic information to a tool, such as the filename(s) it must operate on.  The set of available tokens depends on the type of tool, so check each tool type's details below for a list.

Each token has a name and a type, which determines how you use the token in the config settings.  The following types are currently in use:

String
: The vast majority of tokens have string values.  These are also the easiest types of tokens to use in the config settings.  Just surround the token's name with @ symbols and it will be replaced with the string value before invocation.  For example, for a string token named "FILENAME", place the string "@FILENAME@" in the config settings and it will be replaced with an actual filename when the tool is invoked.
Integer
: Tokens with integer values are simply converted to strings and then treated as string values.  Use them in the same fashion as string tokens.
Boolean
: Tokens with boolean values are a little tricky because they can't use straight replacement like string tokens.  A boolean token could have a true or a false value, but what should be substituted in place of the token's name, the string "true" or "false"?  Most often a tool will want to specify a command-line flag or not depending on the value of a boolean token.  To use a boolean token in the config settings, use a token of the form "@TOKEN|TRUE|FALSE@".  As with other types of tokens, the @ symbols delimit the beginning and end.  TOKEN is the name of the token, TRUE is the value that should be substituted in if the token's value is true, and FALSE is the value that should be used if the token's value is false.  Either TRUE or FALSE may be an empty string, resulting in tokens like "@TOKEN||FALSE@" or "@TOKEN|TRUE|@" (in the latter case, you can omit the last | symbol, resulting in "@TOKEN|TRUE@").  For example, to use a read-only boolean token, you might use the following: "@READONLY|--read-only@", which results in the "--read-only" flag being used if the READONLY token is true, and nothing being used if the READONLY token is false.


Tool Bindings
---

Once file classes and tools are configured, you can bind certain tools to be used on certain classes of files in certain situations using the settings described below.

filetoolbindings/TYPE/CLASS
: The name of the tool of type TYPE to invoke for files of class CLASS in all contexts.
filetoolbindings/TYPE/CLASS/CONTEXT
: The name of the tool of type TYPE to invoke for files of class CLASS in context CONTEXT.  Overrides any setting for the same type and class that is not context-specific.


diff
---
Tools of type "diff" are used by `diff` and `resolve` to compare two different files, or two different versions of the same file.

Diff tools support two contexts.  The "gui" context will be used from graphical clients, such as the Windows Tortoise Client, or from the command line when --interactive is used.  The "console" context will be used when called from a command line.

Diff tools are invoked with values for the following argument tokens.  See Tokens, above, for information about inserting them into config settings.

FROM (string)
: The full path and filename of the "from" file, usually the older of two versions of a file being diffed.
TO (string)
: The full path and filename of the "to" file, usually the newer of two versions of a file being diffed.
FROM_LABEL (string)
: A user-friendly label for the "from" file.  Sometimes this might just be the filename.
TO_LABEL (string)
: A user-friendly label for the "to" file.  Sometimes this might just be the filename.
TO_WRITABLE (boolean)
: True if the TO file should be writable, false if the TO file should be read-only.

Diff tools don't use any flags.

Diff tools support the following exit RESULT values.

successful
: This RESULT indicates that the tool successfully compared the files and exited normally.
same
: This RESULT indicates that the tool successfully compared the files, exited normally, and reported that they were the same (or equivalent).
different
: This RESULT indicates that the tool successfully compared the files, exited normally, and reported that they were different.
failure
: This RESULT indicates that the tool had an error and exited.  (This usually means there was an invalid argument, so check the values in filetools/TYPE/TOOL/args.)
error
: This RESULT indicates that Veracity was not able to launch the external tool.  (This usually means that something is wrong with the value in filetools/TYPE/TOOL/path.)

By default the EXIT mappings for all diff tools are set to: { 0 ==> successful, otherwise ==> failure }.
For tools like gnu-diff that exit with 1 when the inputs are different, you'll want to add "filetools/diff/TOOL/exit/1" with value "different".

The following diff tools are built in:

\:skip
: A tool that makes no attempt to diff the file, it simply returns that the diff was skipped/not attempted.
\:diff
: A console diff tool that is functionally the same as GNU diff.  It also ignores end-of-line differences.
\:diff
: Works like ":diff" but respects end-of-line differences.
\:diff_ignore_whitespace
: Works like ":diff" but ignores whitespace differences.
\:diff_ignore_case
: Works like ":diff" but ignores case differences.
\:diff_ignore_case_and_whitespace
: Works like ":diff" but ignores case and whitespace differences.
\:diffmerge
: A tool that finds and execute an external installation of SourceGear's DiffMerge program.


merge
---
Tools of type "merge" are used by `merge` and `resolve` to perform 3-way file merges.

Merge tools are always invoked in one of the following contexts:

merge
: The tool is being run as the result of a `vv merge` command or equivalent API call.  In other words, a collection of files is being merged into a working copy.  Since this implies a potentially large number of merges happening at once, using interactive tools in this context is unusual.  Generally the tools selected for this context will do the best that they can without user input, and leave complicated situations for the user to resolve later.
resolve
: The tool is being run as the result of a `vv resolve` command or equivalent API call.  In other words, the user is resolving different changes to a single file.  This is usually the context where user interactive tools are used to guide the user through resolving conflicting changes.

Merge tools are invoked with values for the following argument tokens.  See Tokens, above, for information about inserting them into config settings.

ANCESTOR (string)
: The full path and filename of the "ancestor" version of the file involved in the merge.  This is the most recent common version of the file, before the "baseline" and "other" changesets were created.
BASELINE (string)
: The full path and filename of the "baseline" version of the file involved in the merge.  This is the version of the file from the merge's "baseline" changeset, usually the version you had before running a merge.
OTHER (string)
: The full path and filename of the "other" file involved in the merge.  This is the version of the file from the merge's "other", usually the version you merged in from elsewhere.
RESULT (string)
: The full path and filename where the tool should write the merged version of the file.
ANCESTOR_LABEL (string)
: A user-friendly label for the "ancestor" file.  Sometimes this might just be the filename.
BASELINE_LABEL (string)
: A user-friendly label for the "baseline" file.  Sometimes this might just be the filename.
OTHER_LABEL (string)
: A user-friendly label for the "other" file.  Sometimes this might just be the filename.
RESULT_LABEL (string)
: A user-friendly label for the "result" file.  Sometimes this might just be the filename.

The following flags can be specified for merge tools:

premake-result
: If this flag is in the list, then an empty "result" file will be created before invoking the external program.  Some merge programs require this file to already exist when they are executed.
ancestor-overwrite
: This flag indicates that the merge tool saves its output over the ancestor file that is specified to it.  This causes the system to copy the ancestor to the result before invocation and then expect that file to be modified by the tool.  The configuration for such a tool should therefore pass the RESULT token values to the tool as if it were the ancestor.

Merge tools support the following exit RESULT values.

successful
: This RESULT indicates that the tool successfully merged the files and exited normally.
conflict
: This RESULT indicates that the merge conflicts still exist in the result file.
cancel
: This RESULT indicates that the merge was canceled.
failure
: This RESULT indicates that the tool had an error and exited.  (This usually means there was an invalid argument, so check the values in filetools/TYPE/TOOL/args.)
error
: This RESULT indicates that Veracity was not able to launch the external tool.  (This usually means that something is wrong with the value in filetools/TYPE/TOOL/path.)

By default the EXIT mappings for all merge tools are set to: { 0 ==> successful, otherwise ==> failure }.
If for example, your tool exits with 1 when you cancel the merge, you'll want to add "filetools/diff/TOOL/exit/1" with value "cancel".

The following merge tools are built in:

\:skip
: A tool that makes no attempt to merge the file, it simply returns that the merge was skipped/not attempted.  This tool is generally used for file classes that cannot be meaningfully merged, like most binary files.
\:merge
: An automatic 3-way merge tool, which is functionally the same as GNU diff3.  It also ignores end-of-line differences.
\:merge_strict
: Works like `:merge` except that it respects end-of-line differences.
\:baseline
: A tool that resolves merges by simply accepting the "baseline" file as the result, rejecting any changes in the "other" file.
\:other
: A tool that resolves merges by simply accepting the "other" file as the result, rejecting any changes in the "baseline" file.
\:diffmerge
: A tool that finds and execute an external installation of SourceGear's DiffMerge program.
