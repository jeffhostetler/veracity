Displays extended merge-related status information on the changes 
in the working copy or on a changeset that is the result of a merge.

There are 2 modes to 'vv mstatus':

[1] When no changesets are specified:

In this mode, if there is a pending merge, 'vv mstatus' will display
a summary of the changes in the working copy and relate them to the
parent changesets and the common ancestor changeset.  (If there is not
a pending merge, it will fall back to 'vv status'.)

'vv mstatus' will label each changeset and the working copy and qualify
repo-paths relative to each as follows:
* the common ancestor as "A" and "@a/",
* the baseline parent as "B" and "@b/",
* the other parent as "C" and "@c/", and
* the working copy as "WC" and "@/".

The --repo option may not be used in this mode.


[2] When 1 changeset is specified (using one of the --rev, --tag, or --branch options):

In this mode, 'vv mstatus' looks up the requested changeset and if it
is the result of a merge, it will display extended merge-related status
between this item and its 2 parents and their common ancestor.
(If this changeset is not the result of a merge, it will fall back to 'vv status'.)

'vv mstatus' will label each changeset and qualify repo-paths relative to each
changeset as follows:
* the common ancestor as "A" and "@a/",
* the first (arbitrary) parent as "B" and "@b/",
* the other parent as "C" and "@c/", and
* the merge result as "M" and '@m/'.


Augmented status headers:

'vv mstatus' augments the 'added', 'moved', ... status headers (described in 'vv help status')
with an indication of where the change came from.  
For example:

* An item present in the merge result that was also present in the C parent (but not in the ancestor
  or B parent), would be listed as "Added (C)".

* An item renamed in the C parent and with that value in the M (and with the original name in A and B),
  would be listed as "Renamed (M!=A,M!=B,M==C)".


Other changes to status headers:
* The "Added (Merge)" and "Added (Update)" headers reported by 'vv status' are replaced with the augmented headers.



