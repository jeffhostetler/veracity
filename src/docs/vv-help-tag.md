A tag is a unique label applied to a single changeset.
Tags are used to associate a meaningful name with a specific version in the repository.
For example, you might use "Release 1.0" might be used to tag the changeset from which 
the 1.0 release was built.

Use 'vv help tag SUBCOMMAND' to see help on a specific subcommand.

The command 'vv tags' is an alias for 'vv tag list'.

add
===

The tag will be added to the changeset indicated by REVSPEC. If no
REVSPEC is provided, then the tag will be added to the working copy's parent
changeset.

If used from within a working copy and --repo is not specified, the
repository associated with the working copy will be used.





move
===

The existing tag will be moved to the changeset indicated by REVSPEC. If no
REVSPEC is provided, then the tag will be moved to the working copy's parent
changeset.

If used from within a working copy and --repo is not specified, the
repository associated with the working copy will be used.



remove
===

An existing tag will be removed from its associated changeset.

If used from within a working copy and --repo is not specified, the
repository associated with the working copy will be used.



list
====

If used from within a working copy and --repo is not specified, the
repository associated with the working copy will be used.


