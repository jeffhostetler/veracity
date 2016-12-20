If no subcommand is used, an interactive resolve prompt is presented for each unresolved conflict in the working copy.

FILENAME
========

Specifying a filename instead of a subcommand presents an interactive resolve prompt for each conflict on that file, resolved or not.

list
====

Use this command to review conflicts that exist in the working copy.
If a filename is specified, only conflicts on that file are listed.

next
====

Use this command to review the next unresolved conflict in the working copy.
If a filename is specified, the next unresolved conflict on that file is listed.

accept
======

Use of either `[FILENAME]` or `--all` is required.

To specify a resolution value, use its 'label', not the value itself. Valid labels are listed with each conflict, but usually include:
* `ancestor` (the value from the merge's common ancestor changeset)
* `baseline` (the value from the working copy's original parent)
* `other` (the value from the changeset merged into the working copy)
* `working` (the value as it currently exists in the working copy)

For example, if the `baseline` value on a `name` conflict is `file.txt`, the label would be specified as `baseline`, not `file.txt`.

On `contents` conflicts, the following labels are also usually included:
* `automerge` (an automatic merge that may include conflict markers)
* `merge` (a manual merge created using `vv resolve merge`)

To specify a conflict on a file that has several conflicts, use one of the following values for the `CONFLICT` parameter:
* `existence`
* `name`
* `location`
* `attrs`
* `contents`

diff
====

This command may only be used on `contents` conflicts on standard files.

To specify a resolution value, use its 'label', not the value itself. Valid labels are listed with each conflict, but usually include:
* `ancestor` (the value from the merge's common ancestor changeset)
* `baseline` (the value from the working copy's original parent)
* `other` (the value from the changeset merged into the working copy)
* `working` (the value as it currently exists in the working copy)

For example, if the `baseline` value on a `name` conflict is `file.txt`, the label would be specified as `baseline`, not `file.txt`.

On `contents` conflicts, the following labels are also usually included:
* `automerge` (an automatic merge that may include conflict markers)
* `merge` (a manual merge created using `vv resolve merge`)

To specify a conflict on a file that has several conflicts, use one of the following values for the `CONFLICT` parameter:
* `existence`
* `name`
* `location`
* `attrs`
* `contents`

merge
=====

This command may only be used on `contents` conflicts on standard files.

To specify a resolution value, use its 'label', not the value itself. Valid labels are listed with each conflict, but usually include:
* `ancestor` (the value from the merge's common ancestor changeset)
* `baseline` (the value from the working copy's original parent)
* `other` (the value from the changeset merged into the working copy)
* `working` (the value as it currently exists in the working copy)

For example, if the `baseline` value on a `name` conflict is `file.txt`, the label would be specified as `baseline`, not `file.txt`.

On `contents` conflicts, the following labels are also usually included:
* `automerge` (an automatic merge that may include conflict markers)
* `merge` (a manual merge created using `vv resolve merge`)

To specify a conflict on a file that has several conflicts, use one of the following values for the `CONFLICT` parameter:
* `existence`
* `name`
* `location`
* `attrs`
* `contents`
