Displays status information on changes in the working copy or between two changesets.

There are 3 modes to 'vv status':

[1] When no changesets are specified:

In this mode, 'vv status' will display a summary of the changes between
the baseline (parent) changeset and the current working copy.

It will display the current repo-path of items using the '@/' format.
For items with pending moves and/or renames, it will also display
the item's baseline repo-path using the '@b/' format under the current
repo-path.  
For items with a pending delete, it will display only the
baseline repo-path using the '(@b/deleted/file)' format.

File or folder arguments are interpreted relative to the current
working copy for relative and absolute paths and for unqualified
repo-paths of the form '@/path/to/file'.  An argument of the form
'@b/path/in/baseline' or '@c/path/in/other' can also be used.

The --repo option may not be used in this mode.


[2] When 1 changeset is specified (using one of the --rev, --tag, or --branch options):

In this mode, 'vv status' will display a summary of the changes 
between the named changeset and the current working copy.
For example, this can be used to see the changes between the other
parent of the merge and the current working copy.

'vv status' will display the current repo-path of items using the '@/' format.
For moved and/or renamed items, it will also display the item's repo-path
relative to the specified changeset using the '@0/' qualifier.

File or folder arguments are interpreted relative to the current
working copy for relative and absolute paths, repo-paths of the
form '@/current/path', '@b/baseline/path', and '@c/other/path'.
Arguments of the form '@0/changeset/path' are interpreted relative
to the specified changeset.

The --repo option may not be used in this mode.


[3] When 2 changesets are specified using any combination of the --rev, --tag,
or --branch options:

In this mode,
'vv status' will display a summary of the changes between the 2 changesets.
Note that order matters when specifying the 2 changesets.  For example, if a file 
is added in rev 5, then '-r4 -r5' will show it as added, and '-r5 -r4' will 
show it as removed.

Repo-paths in this summary will be qualified with the "@0/" and "@1/" prefixes
to indicate the repo-path relative to each changeset.

Qualified repo-path file or folder arguments are interpreted relative
to the corresponding changeset.  Other repo-path forms and relative
pathnames are interpreted relative to the current working directory
(if present).

This --repo option may be used in this mode; if given, a working copy
is not required.


Basic statuses:

Added [1,2,3]:
:	Newly added items.
Attributes [1,2,3]:
:	Items with changed attributes.  (Currently, this just means the execute bit).
Found [1,2]:
:	Items that are not under version control, but exist on disk in the working copy.
Ignored [1,2]:
:	These are uncontrolled (found) items that match a pattern in '.vvignores'.  (These are only shown when the '--verbose' option is used.)
Locked (*) [1]:
:	These items have outstanding locks.
Lost [1,2]:
:	Items that are under version control, but missing from the working copy.
Modified [1,2,3]:
:	Files or (Unix) symlinks with modified content.
Moved [1,2,3]:
:	Moved items.
Removed [1,2,3]:
:	Items removed from version control and not present in the working copy.
Renamed [1,2,3]:
:	Renamed items.
Sparse [1,2]:
:	Items under version control, but not populated in the working copy (such as Unix symlinks on a Windows system).
Unchanged [1,2]:
:	Items that are unchanged.  (These are only shown when the '--list-unchanged' option is used.)


Additional statuses after a 'vv merge' or 'vv update':

Added (Merge) [1]:
:	These items were added by a previous merge because they were present in the other parent.
Added (Update) [1]:
:	These items were added by a previous update because they were present in the previous baseline and probably dirty in the working copy at the time of the update; they were not present in the new baseline, but have been preserved in the working copy to avoid losing uncommitted work.
Auto-Merged [1]:
:	These files were auto-merged by a previous merge.
Auto-Merged (Edited) [1]:
:	These files were auto-merged by a previous merge and subsequently edited.
Resolved [1]:
:	These items had one or more conflicts during a previous merge or update and all of the issues have been addressed.
Unresolved [1]:
:	These items had one or more conflicts during a previous merge or update and at least one of the issues still needs to be addressed.  Use 'vv resolve' to take care of them.
Choice Resolved (*) [1]:
:	A detailed listing of the individual conflict issues that have been addressed.  (These are only shown when the '--verbose' option is used.)
Choice Unresolved (*) [1]:
:	A detailed listing of the individual conflict issues that have not been addressed.  (These are only shown when the '--verbose' option is used.)


