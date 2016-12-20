Veracity supports repository path prefixes to eliminate ambiguity when the path
to a versioned directory or file has changed.


@0 and @1
---------
The @0 and @1 prefixes are used with commands that take 1 or 2 revisions, like
diff and status. They refer to repository paths in the context of the first or
second changeset provided, respectively. For example:

`originalName` was renamed to `newName` in revision 3.

    $ vv status -r 2 -r 3
    Renamed:    @1/newName
                # Changeset (1): @1/newName
                # Changeset (0): @0/originalName

Status reports that in the first changeset given, revision 2, the item's path
was `@/originalName`. In the second changeset given, revision 3, the item's path
was `@/newName`.

If we reverse the arguments, the output changes as you'd expect:

    $ vv status -r 3 -r 2
    Renamed:    @1/originalName
                # Changeset (1): @1/originalName
                # Changeset (0): @0/newName

You can use these path prefixes yourself. If the file was both modified and 
renamed and you wanted to see the changes, you can specify either path to diff.
Given this change:

    $ vv status -r 2 -r 3
    Modified:  + @1/newName
     Renamed:  + @1/newName
                 # Changeset (1): @1/newName
                 # Changeset (0): @0/originalName

Both of these commands will print the same diff:

    $ vv diff -r -2 -r 3 @0/originalName
    $ vv diff -r -2 -r 3 @1/newName

With no prefix, the path corresponds to the current working copy state.

    $ vv rename newName evenNewerName

Now there's a third path we can use that will print the same diff:

    $ vv diff -r 2 -r 3 @/evenNewerName

The @0 prefix works when only a single revision is given. If I want to diff
a file in my working copy against an old revision, but specify the file's path
in terms of the old revision, I can:

    $ vv diff -r 2 @0/originalName


@b
--
The @b prefix causes a path to be resolved using the working copy's baseline
(or parent) changeset.

Continuing the example from above, where our baseline changeset is revision 3 
and we've renamed `newName` to `evenNewerName`:

	$ vv parent
    Parents of pending changes in working copy:
    
        revision:  3:49e850ae4a1516bda94377d1c603443c2d8aa8e4
          branch:  master
             who:  user@example.com
            when:  2013/01/18 08:43:29.239 -0600
         comment:  newName
          parent:  2:ed924423267fd2869ef66d38498ca572771d107f
    
    $ vv status
    Renamed:    @/evenNewerName
                # Baseline (B): @b/newName

For any command where you want to refer to the `evenNewerName` file, you could
use the path to the file as it existed in revision 3 using the @b prefix. These
commands are equivalent:

    $ vv cat @b/newName
    $ vv cat @/evenNewerName


@a and @c
---------
The @a and @c prefixes are available in working copies with a pending merge. 
@a refers to the merge ancestor changeset. @c refers to the changeset being 
merged.

While we renamed `originalName` to `newName`, someone else renamed it 
`yetAnotherName`:

                  (2) @a/originalName
                 /   \
    @b/newName (3)   (4) @c/yetAnotherName
                 \    /
                  (WC) @/mergedName

In this working copy, we can refer to the file using @a, which will use revision
2 to resolve the path to the file:

    $ vv cat @a/originalName

The @b prefix will refer to revision 3 or 4, depending on which was the baseline
when the merge was performed:

    $ vv merge -r 4
    1 updated, 0 deleted, 0 added, 0 merged, 0 restored, 0 resolved, 1 unresolved
    $ vv rename @b/newName mergedName
    $ vv status
     Renamed: .  @/mergedName
                 # Baseline (B): @b/newName
    Resolved: .  @/mergedName

@c refers to the changeset being merged, which is revision 4 in our example:

    $ vv status @c/yetAnotherName
     Renamed: .  @/mergedName
                 # Baseline (B): @b/newName
    Resolved: .  @/mergedName

In a real life merge with lots of moves and renames, it can get messy. The 
`mstatus` command can help. It displays all 4 prefixed paths:

    $ vv mstatus
     Renamed (WC!=A,WC!=B,WC!=C): .+ @/mergedName
                                     # Baseline (B): @b/newName
                                     #    Other (C): @c/yetAnotherName
                                     # Ancestor (A): @a/originalName
                        Resolved: .+ @/mergedName
