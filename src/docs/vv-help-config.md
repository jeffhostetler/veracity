add-to
======

Some Veracity configuration settings, such as 'ignores' (which lists
patterns of files and directories vv will normally ignore), contain a
list of values.

For example, if 'ignores' currently contains the following:

    **/*.backups
    TAGS

.foo files would be added to the ignores list via:

vv config add-to ignores '**/*.foo'
