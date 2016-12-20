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
 * This file contains tests that setup orphan conflicts.
 */

// a simple orphaned file test
// where the delete occurs in the "baseline"
// which accepts the "ancestor" value
TEST("orphan__simple__ancestor")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_FILE("@/folder/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder", NULL, "@/folder", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple orphaned file test
// where the delete occurs in the "baseline"
// which accepts the "baseline" value
TEST("orphan__simple__baseline")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_FILE("@/folder/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder", NULL, "@/folder", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder", SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/folder")
		VERIFY_GONE("@/folder/file.txt")
TEST_END

// a simple orphaned file test
// where the delete occurs in the "baseline"
// which accepts the "other" value
TEST("orphan__simple__other")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_FILE("@/folder/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder", NULL, "@/folder", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder", SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple orphaned file test
// where the delete occurs in the "baseline"
// which accepts the "working" value
TEST("orphan__simple__working")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_FILE("@/folder/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/folder")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder", NULL, "@/folder", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder", SG_RESOLVE__STATE__EXISTENCE, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple orphaned file test
// where the delete occurs in the "other"
// which accepts the "ancestor" value
TEST("orphan__reverse__ancestor")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_FILE("@/folder/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder", "@/folder", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder", SG_RESOLVE__STATE__EXISTENCE, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple orphaned file test
// where the delete occurs in the "other"
// which accepts the "baseline" value
TEST("orphan__reverse__baseline")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_FILE("@/folder/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder", "@/folder", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder", SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

// a simple orphaned file test
// where the delete occurs in the "other"
// which accepts the "other" value
TEST("orphan__reverse__other")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_FILE("@/folder/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder", "@/folder", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder", SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/folder")
		VERIFY_GONE("@/folder/file.txt")
TEST_END

// a simple orphaned file test
// where the delete occurs in the "other"
// which accepts the "working" value
TEST("orphan__reverse__working")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/folder")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_FILE("@/folder/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder", "@/folder", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder", SG_RESOLVE__STATE__EXISTENCE, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder")
		VERIFY_FILE("@/folder/file.txt", NULL)
TEST_END

