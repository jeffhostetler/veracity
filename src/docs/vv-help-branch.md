'vv branch' with no subcommand displays the working copy's current branch attachment.

Use 'vv help branch SUBCOMMAND' to see help on a specific subcommand.

new
===

On the next commit, a new branch BRANCHNAME will be created and attached. It is an error if BRANCHNAME already exists. 

'new' can only be used from within a working copy.

'vv branch' with no arguments displays the working copy's current branch attachment.

attach
===

Attach the working copy to an existing branch in order to continue, fork, or 
start a new instance of that branch. BRANCHNAME will be applied starting with 
the next commit. BRANCHNAME must refer to an existing branch. 

'attach' can only be used from within a working copy.

'vv branch' with no arguments displays the working copy's current branch attachment.

detach
===

The next commit will start an anonymous branch and will require the 
'--detached' option. 

'detach' can only be used from within a working copy.

'vv branch' with no arguments displays the working copy's current branch attachment.

add-head
===

BRANCHNAME's head count will be increased by one.

BRANCHNAME can refer to a new or an existing branch.

'add-head' acts on the current repoository when run from inside a working copy but it can act on any repository by using the '--repo' option.

remove-head
===

Use of either '--all' or '--REVSPEC [...]' is required.

When used with '--REVSPEC', remove the BRANCHNAME head at the specified changeset. That changeset will no 
longer be a head of BRANCHNAME. BRANCHNAME's head count will be decreased by one for each specified REVSPEC. 

When used with '--all', remove all BRANCHNAME heads, effectively deleting the BRANCHNAME branch. 

BRANCHNAME must refer to an existing branch. 

'remove-head' acts on the current repo when run from inside a working copy, but it can act on any repository by using the '--repo' option.

move-head
===

The use of '--all' precludes the use of '--REV_SOURCE'.

When used with '--all', consolidate and move all existing BRANCHNAME heads to the changeset indicated by REV_TARGET. BRANCHNAME will be left with exactly one head at the REV_TARGET.

When used without '--all', move a head of BRANCHNAME from REV_SOURCE to 
REV_TARGET. If BRANCHNAME has only one head, the source argument is optional. BRANCHNAME's total head count will be unchanged.

BRANCHNAME must refer to an existing branch. 

'move-head' acts on the current repository when run from inside a working copy, but it can act on any repository by using the '--repo' option.

close
===

BRANCHNAME's heads will no longer be included in 'vv heads' or 'vv branch list' displays unless '--all' is used with these commands, in which case BRANCHNAME will be displayed as "(closed)".

BRANCHNAME must refer to an existing branch. 

'close' acts on the current repo when run from inside a working copy, but it can act on any repository by using the '--repo' option.

reopen
===

If BRANCHNAME had been closed, its heads will again be visible to 
commands such as 'vv heads' and 'vv branch list'.

BRANCHNAME must refer to an existing branch.

'reopen' acts on the current repo when run from inside a working copy, but it can act on any repository by using the '--repo' option.

list
===

If '--all' is used, include closed branches, indicating their closed 
status.  

'list' acts on the current repo when run from inside a working copy, but it can act on any repository by using the '--repo' option.

The command 'vv branches' is an alias for 'vv branch list'.
