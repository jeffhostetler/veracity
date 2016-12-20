/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/**
 * Resolve Tests
 *
 * This file contains tests that setup divergent move conflicts.
 */

// a simple divergent move
// that accepts the "ancestor" value
TEST("divergent_move__simple__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/baseline/file.txt", "@/other/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/baseline/file.txt")
		VERIFY_GONE("@/other/file.txt")
TEST_END

// a simple divergent move
// that accepts the "baseline" value
TEST("divergent_move__simple__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/baseline/file.txt", "@/other/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/baseline/file.txt", NULL)
		VERIFY_GONE("@/other/file.txt")
TEST_END

// a simple divergent move
// that accepts the "other" value
TEST("divergent_move__simple__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/baseline/file.txt", "@/other/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/baseline/file.txt")
		VERIFY_FILE("@/other/file.txt", NULL)
TEST_END

// a simple divergent move
// that accepts the "working" value
TEST("divergent_move__simple__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/baseline/file.txt", "@/other/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/baseline/file.txt", NULL)
		VERIFY_GONE("@/other/file.txt")
TEST_END

// a divergent move
// that modifies the WD
// before accepting the "ancestor" value
TEST("divergent_move__modify__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/working", NULL)
		SETUP_MOVE("item_file", "@/working")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/baseline/file.txt", "@/other/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/working",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/baseline/file.txt")
		VERIFY_GONE("@/other/file.txt")
		VERIFY_GONE("@/working/file.txt")
TEST_END

// a divergent move
// that modifies the WD
// before accepting the "baseline" value
TEST("divergent_move__modify__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/working", NULL)
		SETUP_MOVE("item_file", "@/working")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/baseline/file.txt", "@/other/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/working",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/baseline/file.txt", NULL)
		VERIFY_GONE("@/other/file.txt")
		VERIFY_GONE("@/working/file.txt")
TEST_END

// a divergent move
// that modifies the WD
// before accepting the "other" value
TEST("divergent_move__modify__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/working", NULL)
		SETUP_MOVE("item_file", "@/working")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/baseline/file.txt", "@/other/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/working",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/baseline/file.txt")
		VERIFY_FILE("@/other/file.txt", NULL)
		VERIFY_GONE("@/working/file.txt")
TEST_END

// a divergent move
// that modifies the WD
// before accepting the "working" value
TEST("divergent_move__modify__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/working", NULL)
		SETUP_MOVE("item_file", "@/working")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/baseline/file.txt", "@/other/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/working",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/baseline/file.txt")
		VERIFY_GONE("@/other/file.txt")
		VERIFY_FILE("@/working/file.txt", NULL)
TEST_END

// a simple divergent move
// that accepts the "ancestor" value
// and then accepts the "baseline" value
TEST("divergent_move__simple__ancestor_baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/baseline/file.txt", "@/other/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/baseline/file.txt", NULL)
		VERIFY_GONE("@/other/file.txt")
TEST_END

// a simple divergent move
// that accepts the "ancestor" value
// and then accepts the "other" value
TEST("divergent_move__simple__ancestor_other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/baseline/file.txt", "@/other/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/baseline/file.txt")
		VERIFY_FILE("@/other/file.txt", NULL)
TEST_END

// a simple divergent move
// that accepts the "ancestor" value
// and then accepts the "working" value
TEST("divergent_move__simple__ancestor_working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/baseline/file.txt", "@/other/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/baseline/file.txt")
		VERIFY_GONE("@/other/file.txt")
TEST_END

// a simple divergent move
// that accepts the "working" value
// and then accepts the "ancestor" value
TEST("divergent_move__simple__working_ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/baseline/file.txt", "@/other/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/baseline/file.txt")
		VERIFY_GONE("@/other/file.txt")
TEST_END

#if defined(MAC) || defined(LINUX)

// a simple divergent move
// of a symlink
// that accepts the "ancestor" value
TEST("divergent_move__symlink__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/link", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/baseline/link", "@/other/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file.txt")
		VERIFY_GONE("@/baseline/link")
		VERIFY_GONE("@/other/link")
TEST_END

// a simple divergent move
// of a symlink
// that accepts the "baseline" value
TEST("divergent_move__symlink__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/link", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/baseline/link", "@/other/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_SYMLINK("@/baseline/link", "@/file.txt")
		VERIFY_GONE("@/other/link")
TEST_END

// a simple divergent move
// of a symlink
// that accepts the "other" value
TEST("divergent_move__symlink__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/link", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/baseline/link", "@/other/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_GONE("@/baseline/link")
		VERIFY_SYMLINK("@/other/link", "@/file.txt")
TEST_END

// a simple divergent move
// of a symlink
// that accepts the "working" value
TEST("divergent_move__symlink__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/link", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/baseline/link", "@/other/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_SYMLINK("@/baseline/link", "@/file.txt")
		VERIFY_GONE("@/other/link")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a divergent move
// of a symlink
// that modifies the WD
// before accepting the "ancestor" value
TEST("divergent_move__symlink_modify__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/link", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/working", NULL)
		SETUP_MOVE("item_link", "@/working")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/baseline/link", "@/other/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/working",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file.txt")
		VERIFY_GONE("@/baseline/link")
		VERIFY_GONE("@/other/link")
		VERIFY_GONE("@/working/link")
TEST_END

// a divergent move
// of a symlink
// that modifies the WD
// before accepting the "baseline" value
TEST("divergent_move__symlink_modify__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/link", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/working", NULL)
		SETUP_MOVE("item_link", "@/working")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/baseline/link", "@/other/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/working",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_SYMLINK("@/baseline/link", "@/file.txt")
		VERIFY_GONE("@/other/link")
		VERIFY_GONE("@/working/link")
TEST_END

// a divergent move
// of a symlink
// that modifies the WD
// before accepting the "other" value
TEST("divergent_move__symlink_modify__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/link", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/working", NULL)
		SETUP_MOVE("item_link", "@/working")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/baseline/link", "@/other/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/working",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_GONE("@/baseline/link")
		VERIFY_SYMLINK("@/other/link", "@/file.txt")
		VERIFY_GONE("@/working/link")
TEST_END

// a divergent move
// of a symlink
// that modifies the WD
// before accepting the "working" value
TEST("divergent_move__symlink_modify__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_ADD_FOLDER("@/baseline", NULL)
		SETUP_ADD_FOLDER("@/other", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/link", "@/baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/working", NULL)
		SETUP_MOVE("item_link", "@/working")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/baseline/link", "@/other/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",         "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/working",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_GONE("@/baseline/link")
		VERIFY_GONE("@/other/link")
		VERIFY_SYMLINK("@/working/link", "@/file.txt")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

