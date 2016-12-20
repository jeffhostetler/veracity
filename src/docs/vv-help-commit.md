The '--assoc' (or '-a') parameter may specify either a foreign or local work
item by its ID. Foreign work items are those in a repository other than the
target of the commit.

For local items, use the work item ID, e.g. 'vv commit --assoc Z1234'

For foreign items, prefix the item ID with the other repository's name and a colon,
e.g. 'vv commit --assoc myotherrepo:X6789'
