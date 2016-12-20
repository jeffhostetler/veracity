Stamps are similar to tags, but unlike tags, a single
STAMPNAME can be applied to multiple changesets.

Use 'vv help stamp SUBCOMMAND' to see help on a specific subcommand.


add
===

The stamp will be added to the changeset(s) indicated by REVSPEC.
Multiple REVSPECs can be used to add a stamp to several changesets at once.
If no REVSPEC is specified, the stamp will be added to the working copy's parent changeset.  

If used from within a working copy and --repo is not specified, 
the repository associated with the working copy will be used.


remove
===

The stamp will be removed from the changeset(s) indicated by REVSPEC.  
Multiple REVSPECs can be used to remove a stamp from several changesets at once.

--all can be used in place of --REVSPEC to remove STAMPNAME from all changesets to which it is applied.
If no REVSPEC is specified and --all is not used, the stamp will be removed from the working copy's parent changeset.

If used from within a working copy and --repo is not specified, 
the repository associated with the working copy will be used.


list
===

If STAMPNAME is not specified, print a summary of stamp usage by stamp name.
Specifying STAMPNAME will print all changesets where that stamp is applied.

If used from within a working copy and --repo is not specified, 
the repo associated with the working copy will be used.

The command 'vv stamps' is an alias for 'vv stamp list'.
