Overview
--------

This folder contains the import tests executed by u0111_fast_import.
Every file with a .test extension is usually loaded and processed.
However, if there is a file in this folder called _priority.test, then ONLY IT is run.
That is useful for debugging a single test - just rename it to _priority.test temporarily.
Each test file indicates a fast-import file to import, and a set of expectations about the resulting repo.


File Format
-----------

The test file import is as follows:
Any line in a test file that starts with # is considered a comment, and ignored.
Each non-blank, non-comment line in a file is a one-word command, followed by one or more arguments.
A command and its arguments are basically whitespace-delimited.
The last argument to a command, however, always extends to the end of the line, whitespace included.
Commands are grouped into blocks which are separated by blank lines.
Generally, each command in a block references the same object and further describes it.

The first block in every file must be an "import" block, as follows:

import FILENAME
option NAME VALUE
error ERRNUM
shared-user USERNAME

The "import" command indicates the start of the block, and may only appear once.
FILENAME is the fast-import file to import for this test, relative to the test file's folder.
The "option" command may appear any number of times.
Each option is a name/value pair that specifies an aspect of how the test is to be processed.
No options are implemented yet, but at least one is planned.
The "error" command is optional, and may appear once in the block.
ERRNUM specifies the error code that is expected when importing the file (to expect specific import failures).
The "shared-user" command is optional, and may appear any number of times.
It specifies a username that should exist in a repo whose users will be shared with the imported repo.
This causes a second repo to be created, the specified user to be added to it, and that repo to be specified as if by the --shared-users flag.
If "shared-user" is used multiple times, all the specified users are created in the same second repo.

After the import block, there may be any number of other blocks that specify expectations of the imported repo.
The available blocks are as follows:

commit NAME
user USERNAME
time TIMESTAMP
comment COMMENT
parent COMMIT
wc FOLDER

A commit block specifies the details of a single commit.
A commit block by itself doesn't cause any checks against the imported repo, other blocks must reference it.
NAME is a simple name to refer to the commit throughout the test file.
Often, the names match the "mark" IDs in the corresponding fast-import file.
The "user" command is optional and may appear once in each commit block.
USERNAME is the name of the user that created the commit.
The "time" command is optional and may appear once in each commit block.
TIMESTAMP is the POSIX-style timestamp of when the commit was created.
The "comment" command is optional and may appear any number of times in each commit block.
COMMENT is the first line of the first comment associated with the commit.
The "parent" command is optional and may appear any number of times in each commit block.
COMMIT is the name of a previously defined commit block that is this commit's parent.
The "wc" command is optional and may appear once in each commit block.
FOLDER is a folder, relative to the .test file, that contains the working copy expected from checking out this commit.
The expected working copy need not contain an .sgdrawer folder.

leaf COMMIT

A leaf block specifies a commit that is expected to be a leaf in the imported repo.
COMMIT is the name of a previously defined commit block that is expected to be a leaf.
The leaf block will cause all leaves in the repo to be compared to the details of the referenced commit block.
If none of the leaves matches those details, the test will fail.

tag TAG
commit COMMIT
wc FOLDER

A tag block specifies a tag that is expected to exist in the imported repo.
TAG is the name of the tag that is expected to exist.
The "commit" command is optional and may appear once in each tag block.
COMMIT is the name of a previously defined commit block that the tag is expected to point to in the imported repo.
The "commit" command here will cause the test to compare the tagged commit in the repo to the specified named commit.
If the tagged commit in the repo doesn't match the details of the named commit, the test will fail.
The "wc" command is optional and may appear once in each tag block.
FOLDER is a folder, relative to the .test file, that contains the working copy expected from checking out this tag.
The expected working copy need not contain an .sgdrawer folder.

branch BRANCH
status STATUS
commit COMMIT
wc FOLDER

A branch block specifies a branch that is expected to exist in the imported repo.
BRANCH is the name of the branch that is expected to exist.
The "status" command is optional and may appear once in each branch block.
STATUS is either "open" or "closed", and will cause the test to verify that the named branch is in that state.
The "commit" command is optional and may appear any number of times in each branch block.
COMMIT is the name of a previously defined commit block that is expected to be a head of the branch.
Each "commit" command will cause all of the branch's heads to be compared to the details of the named commit.
If none of the branch heads matches those details, the test will fail.
The "wc" command is optional and may appear once in each branch block.
FOLDER is a folder, relative to the .test file, that contains the working copy expected from checking out this branch.
The expected working copy need not contain an .sgdrawer folder.
Note that "wc" won't work on branches with multiple heads, because checking the branch out for comparison will fail.


TODO
----

The following is a list of things I'd like to add to the test infrastructure.

Option "long-test".
A boolean value that, if true, would cause the test to only be processed during "long" test runs.
Would default to false.

Generators
Programs could be written to generate .test files from existing repos.
Different generator programs would be needed for different DVCS software.
This would allow existing foreign repos to easily be used as import tests.
Just export the repo to a fast-import file, and use a generator to create the corresponding .test expectations.
