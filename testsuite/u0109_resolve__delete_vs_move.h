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
 * This file contains tests that setup delete-vs-move conflicts.
 */

// a simple delete-vs-move test
// where the delete occurs in the baseline
// which accepts the "ancestor" value
TEST("delete_vs_move__simple__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/folder/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
	TEST_VERIFY
		// we accepted the ancestor's existence state, not its location
		// so we still expect the location it had in the "other" changeset
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the baseline
// which accepts the "baseline" value
TEST("delete_vs_move__simple__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/folder/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/folder/file.txt")
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the baseline
// which accepts the "other" value
TEST("delete_vs_move__simple__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/folder/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the baseline
// which accepts the "working" value
TEST("delete_vs_move__simple__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/folder/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the baseline
// which accepts the "ancestor" value
// and then accepts the "baseline" value
TEST("delete_vs_move__simple__ancestor_baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/folder/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/folder/file.txt")
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the baseline
// which accepts the "ancestor" value
// and then accepts the "other" value
TEST("delete_vs_move__simple__ancestor_other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/folder/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the baseline
// which accepts the "ancestor" value
// and then accepts the "working" value
TEST("delete_vs_move__simple__ancestor_working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/folder/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the baseline
// which accepts the "working" value
// and then accepts the "ancestor" value
TEST("delete_vs_move__simple__working_ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/folder/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_working", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the "other"
// which accepts the "ancestor" value
TEST("delete_vs_move__reverse__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/folder/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
	TEST_VERIFY
		// we accepted the ancestor's existence state, not its location
		// so we still expect the location it had in the "baseline" changeset
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the "other"
// which accepts the "baseline" value
TEST("delete_vs_move__reverse__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/folder/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the "other"
// which accepts the "other" value
TEST("delete_vs_move__reverse__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/folder/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/folder/file.txt")
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the "other"
// which accepts the "working" value
TEST("delete_vs_move__reverse__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/folder/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the "other"
// which accepts the "ancestor" value
// and then accepts the "baseline" value
TEST("delete_vs_move__reverse__ancestor_baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/folder/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the "other"
// which accepts the "ancestor" value
// and then accepts the "other" value
TEST("delete_vs_move__reverse__ancestor_other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/folder/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/folder/file.txt")
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the "other"
// which accepts the "ancestor" value
// and then accepts the "working" value
TEST("delete_vs_move__reverse__ancestor_working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/folder/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the "other"
// which accepts the "working" value
// and then accepts the "ancestor" value
TEST("delete_vs_move__reverse__working_ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/folder/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_working", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple delete-vs-move test
// where the delete occurs in the baseline
// which resolves the file by deleting it
TEST("delete_vs_move__simple__delete")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file.txt", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/folder/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_DELETE_TEMP_FILE("@/folder/file.txt~A~g~", "item_file")
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/folder/file.txt")
		VERIFY_TEMP_GONE("@/folder/file.txt~A~g~", "item_file")
TEST_END

#if defined(MAC) || defined(LINUX)

// a delete-vs-move test
// with symlinks
// where the delete occurs in the baseline
// which accepts the "ancestor" value
TEST("delete_vs_move__symlink__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/folder/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
	TEST_VERIFY
		// we accepted the ancestor's existence state, not its location
		// so we still expect the location it had in the "other" changeset
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_SYMLINK("@/folder/link", "@/file.txt")
TEST_END

// a delete-vs-move test
// with symlinks
// where the delete occurs in the baseline
// which accepts the "baseline" value
TEST("delete_vs_move__symlink__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/folder/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_GONE("@/folder/link")
TEST_END

// a delete-vs-move test
// with symlinks
// where the delete occurs in the baseline
// which accepts the "other" value
TEST("delete_vs_move__symlink__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/folder/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_SYMLINK("@/folder/link", "@/file.txt")
TEST_END

// a delete-vs-move test
// with symlinks
// where the delete occurs in the baseline
// which accepts the "working" value
TEST("delete_vs_move__symlink__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/folder/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__EXISTENCE, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_SYMLINK("@/folder/link", "@/file.txt")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a delete-vs-move test
// with symlinks
// where the delete occurs in the other
// which accepts the "ancestor" value
TEST("delete_vs_move__symlink_reverse__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/link", "@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/folder/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
	TEST_VERIFY
		// we accepted the ancestor's existence state, not its location
		// so we still expect the location it had in the "baseline" changeset
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_SYMLINK("@/folder/link", "@/file.txt")
TEST_END

// a delete-vs-move test
// with symlinks
// where the delete occurs in the other
// which accepts the "baseline" value
TEST("delete_vs_move__symlink_reverse__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/link", "@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/folder/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_SYMLINK("@/folder/link", "@/file.txt")
TEST_END

// a delete-vs-move test
// with symlinks
// where the delete occurs in the other
// which accepts the "other" value
TEST("delete_vs_move__symlink_reverse__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/link", "@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/folder/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_GONE("@/folder/link")
TEST_END

// a delete-vs-move test
// with symlinks
// where the delete occurs in the other
// which accepts the "working" value
TEST("delete_vs_move__symlink_reverse__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/link", "@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/folder/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__EXISTENCE, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_SYMLINK("@/folder/link", "@/file.txt")
TEST_END

// a delete-vs-move test
// with symlinks
// where the delete occurs in the baseline
// which resolves the file by deleting it
TEST("delete_vs_move__symlink__delete")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/link", "@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/folder/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_DELETE_TEMP_FILE("@/folder/link~A~g~", "item_link")
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_GONE("@/folder/link")
		VERIFY_TEMP_GONE("@/folder/link~A~g~", "item_link")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

