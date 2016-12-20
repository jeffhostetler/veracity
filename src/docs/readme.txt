To add markdown help for another vv command:

1. Create the markdown file, named vv-help-mycommand.md
2. In ../cmd/sg_help_source.h, add:

       extern const char *vv_help_mycommand;
       (note the underscores instead of dashes)
3. Reference this same variable in sg.c's commands[] list.  Add a
    pointer to this help variable as the extendedHelp member, e.g.

	{
		"mycommand", "msc", NULL, NULL, cmd_mycommand,
		"Do an awesome thing", "", &vv_help_mycommand,
		SG_FALSE, 
       ...
4. Re-run CMake, edit the markdown file, build cmd



We currently support the following subset of multi-markdown:

Body text
======

Paragraphs are separated
by double line breaks.  Single line
breaks are considered part of the same paragraph,
and will be reflowed.


Inline code
=======

Code, command-line, etc. snippets should be 
wrapped in `whatever you call these backward quotes`.


First- and second-level headers
====================

First-level header (subcommand)
===========

Since we have no actual need for first-level headlines in vv help,
any headlines seen will be taken as the name of a subcommand,
and following text will end up in its own subcommand help
block.


Second-level header
---

Note that the character counts on the "underlines" do not need to
match the text.

Unordered lists
==========

* A thing
* And another
- Or you can use dashes
* Or mix them up
* Entries can span lines
  if leading spaces are used for continuation.


Definition lists
==========

An item
: its definition
Another item
: And its definition, which 
  may span multiple lines if the second and subsequent
  lines begin with spaces
And another item
: And *its* definition.
