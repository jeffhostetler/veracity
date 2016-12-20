Importing data requires specifying three key pieces of information:

* Which repository should the data be imported into?
* Which changeset should be the parent of the first imported changeset?
* Which branch should imported changesets be included in?

The Subversion dump data to import from must be provided on STDIN.  The dump data can be generated from a Subversion repository using "svnadmin dump" or "svnrdump dump".


Inside a Working Copy
---

If the command is invoked from inside a working copy, then the required data is entirely implied and may not be overridden.

* Data is imported into the repository associated with the working copy.
* The initial parent changeset is the working copy's baseline parent.
* Imported changesets are included in the working copy's attached branch.


Outside any Working Copy
---

If the command is invoked from outside a working copy, then the required data must be explicitly specified:

* Use --repo to specify the repository.
* Use --rev, --branch, or --tag to specify the initial parent changeset.
* Use --attach or --detached to specify the branch to import into.

If you specify the initial parent using --branch, then --attach on the same branch is implied, but may be overridden by explicitly specifying --attach or --detached.


Importing a Single Path
---

Data can be imported from only a specific Subversion path using --path.  The given path must exactly match the path as it exists in Subversion, including case and path separator.  The path should not have a leading path separator.  For example, to import from the root trunk directory, use `--path trunk`.

If specified, the given path will also be mapped to the root during the import.  For example, if you import from `trunk/my_folder`, then the file `trunk/my_folder/my_file` will end up being `@/my_file` in Veracity.
