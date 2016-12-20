When in a working copy, BASELINE-REVSPEC defaults to the baseline parent.
OTHER-REVSCPEC defaults to the other parent, if applicable, or to the other
head of the currently attached branch, if applicable.

Overview
--------

The merge-preview command examines the history of the OTHER changeset and
reports which changesets it has that are not also in the history of BASELINE.
This shows you which changesets you would be bringing into the history of your
working copy if you were to perform `vv merge OTHER-REVSPEC` from a working
copy whose parent was BASELINE.

The command can also be used as a general query tool since it allows you to
specify a BASELINE other than a working copy parent. For example:

vv merge-preview --branch BranchA --branch BranchB
: shows which changesets are in BranchB but not in BranchA

vv merge-preview --branch BranchB --branch BranchA
: shows which changesets are in BranchA but not in BranchB