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
 * This file contains tests that setup cycle conflicts.
 */

// a test that sets up a simple path cycle
// and accepts the state of all items from "ancestor"
TEST("cycle__simple__ancestor")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder1", "@/folder2")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder2", "@/folder1")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder1", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder1", "@/folder1", "@/folder2/folder1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "item_folder2", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",           SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_folder2", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder2", "@/folder1/folder2", "@/folder2", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "item_folder1", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "item_folder1", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder1", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_folder2", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder1")
		VERIFY_FOLDER("@/folder2")
		VERIFY_GONE("@/folder1/folder2")
		VERIFY_GONE("@/folder2/folder1")
TEST_END

// a test that sets up a simple path cycle
// and accepts the state of all items from "baseline"
TEST("cycle__simple__baseline")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder1", "@/folder2")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder2", "@/folder1")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder1", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder1", "@/folder1", "@/folder2/folder1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "item_folder2", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",           SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_folder2", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder2", "@/folder1/folder2", "@/folder2", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "item_folder1", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "item_folder1", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder1", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_folder2", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder1")
		VERIFY_GONE("@/folder2")
		VERIFY_FOLDER("@/folder1/folder2")
		VERIFY_GONE("@/folder2/folder1")
TEST_END

// a test that sets up a simple path cycle
// and accepts the state of all items from "other"
TEST("cycle__simple__other")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder1", "@/folder2")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder2", "@/folder1")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder1", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder1", "@/folder1", "@/folder2/folder1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "item_folder2", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",           SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_folder2", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder2", "@/folder1/folder2", "@/folder2", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "item_folder1", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "item_folder1", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		// need to accept folder2 first in this case
		// otherwise we'll try to move folder1 into its own subfolder (folder2)
		RESOLVE_ACCEPT("item_folder2", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
		RESOLVE_ACCEPT("item_folder1", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/folder1")
		VERIFY_FOLDER("@/folder2")
		VERIFY_GONE("@/folder1/folder2")
		VERIFY_FOLDER("@/folder2/folder1")
TEST_END

// a test that sets up a simple path cycle
// and accepts the state of all items from "working"
TEST("cycle__simple__working")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder1", "@/folder2")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder2", "@/folder1")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder1", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder1", "@/folder1", "@/folder2/folder1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "item_folder2", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",           SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_folder2", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder2", "@/folder1/folder2", "@/folder2", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "item_folder1", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "item_folder1", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder1", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
		RESOLVE_ACCEPT("item_folder2", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder1")
		VERIFY_GONE("@/folder2")
		VERIFY_FOLDER("@/folder1/folder2")
		VERIFY_GONE("@/folder2/folder1")
TEST_END

// a test that sets up a simple path cycle
// and modifies the WD after merging
// and accepts the state of all items from "ancestor"
TEST("cycle__simple_modify__ancestor")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", "item_folder1")
		SETUP_ADD_FOLDER("@/folder2", "item_folder2")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder1", "@/folder2")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder2", "@/folder1")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_folder1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_folder2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_folder1", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder1", "@/folder1", "@/folder2/folder1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "item_folder2", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3",    SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_folder2", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder2", "@/folder1/folder2", "@/folder2", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "item_folder1", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4",    SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder1", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_folder2", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder1")
		VERIFY_FOLDER("@/folder2")
		VERIFY_GONE("@/folder1/folder2")
		VERIFY_GONE("@/folder2/folder1")
		VERIFY_GONE("@/folder3/folder1")
		VERIFY_GONE("@/folder4/folder2")
TEST_END

// a test that sets up a simple path cycle
// and modifies the WD after merging
// and accepts the state of all items from "baseline"
TEST("cycle__simple_modify__baseline")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", "item_folder1")
		SETUP_ADD_FOLDER("@/folder2", "item_folder2")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder1", "@/folder2")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder2", "@/folder1")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_folder1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_folder2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_folder1", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder1", "@/folder1", "@/folder2/folder1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "item_folder2", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3",    SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_folder2", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder2", "@/folder1/folder2", "@/folder2", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "item_folder1", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4",    SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder1", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_folder2", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder1")
		VERIFY_GONE("@/folder2")
		VERIFY_FOLDER("@/folder1/folder2")
		VERIFY_GONE("@/folder2/folder1")
		VERIFY_GONE("@/folder3/folder1")
		VERIFY_GONE("@/folder4/folder2")
TEST_END

// a test that sets up a simple path cycle
// and modifies the WD after merging
// and accepts the state of all items from "other"
TEST("cycle__simple_modify__other")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", "item_folder1")
		SETUP_ADD_FOLDER("@/folder2", "item_folder2")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder1", "@/folder2")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder2", "@/folder1")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_folder1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_folder2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_folder1", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder1", "@/folder1", "@/folder2/folder1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "item_folder2", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3",    SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_folder2", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder2", "@/folder1/folder2", "@/folder2", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "item_folder1", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4",    SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder1", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
		RESOLVE_ACCEPT("item_folder2", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/folder1")
		VERIFY_FOLDER("@/folder2")
		VERIFY_GONE("@/folder1/folder2")
		VERIFY_FOLDER("@/folder2/folder1")
		VERIFY_GONE("@/folder3/folder1")
		VERIFY_GONE("@/folder4/folder2")
TEST_END

// a test that sets up a simple path cycle
// and modifies the WD after merging
// and accepts the state of all items from "working"
TEST("cycle__simple_modify__working")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", "item_folder1")
		SETUP_ADD_FOLDER("@/folder2", "item_folder2")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder1", "@/folder2")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder2", "@/folder1")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_folder1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_folder2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_folder1", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder1", "@/folder1", "@/folder2/folder1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "item_folder2", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3",    SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_folder2", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder2", "@/folder1/folder2", "@/folder2", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "item_folder1", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4",    SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder1", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
		RESOLVE_ACCEPT("item_folder2", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/folder1")
		VERIFY_GONE("@/folder2")
		VERIFY_GONE("@/folder1/folder2")
		VERIFY_GONE("@/folder2/folder1")
		VERIFY_FOLDER("@/folder3/folder1")
		VERIFY_FOLDER("@/folder4/folder2")
TEST_END

// a test that sets up a deep path cycle
// and accepts the state of all items from "ancestor"
TEST("cycle__deep__ancestor")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder1/xx", NULL)
		SETUP_ADD_FOLDER("@/folder1/xx/yy", "item_folder1_child")
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FOLDER("@/folder2/aa", NULL)
		SETUP_ADD_FOLDER("@/folder2/aa/bb", "item_folder2_child")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder1", "@/folder2/aa/bb")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder2", "@/folder1/xx/yy")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder1", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder1", "@/folder1", "@/folder2/aa/bb/folder1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",                 "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",                 "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "item_folder2_child", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",                 SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_folder2", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder2", "@/folder1/xx/yy/folder2", "@/folder2", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",                 "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "item_folder1_child", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",                 "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "item_folder1_child", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder1", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_folder2", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder1")
		VERIFY_FOLDER("@/folder1/xx")
		VERIFY_FOLDER("@/folder1/xx/yy")
		VERIFY_FOLDER("@/folder2")
		VERIFY_FOLDER("@/folder2/aa")
		VERIFY_FOLDER("@/folder2/aa/bb")
		VERIFY_GONE("@/folder1/xx/yy/folder2")
		VERIFY_GONE("@/folder1/xx/yy/folder2/aa")
		VERIFY_GONE("@/folder1/xx/yy/folder2/aa/bb")
		VERIFY_GONE("@/folder2/aa/bb/folder1")
		VERIFY_GONE("@/folder2/aa/bb/folder1/xx")
		VERIFY_GONE("@/folder2/aa/bb/folder1/xx/yy")
TEST_END

// a test that sets up a deep path cycle
// and accepts the state of all items from "baseline"
TEST("cycle__deep__baseline")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder1/xx", NULL)
		SETUP_ADD_FOLDER("@/folder1/xx/yy", "item_folder1_child")
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FOLDER("@/folder2/aa", NULL)
		SETUP_ADD_FOLDER("@/folder2/aa/bb", "item_folder2_child")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder1", "@/folder2/aa/bb")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder2", "@/folder1/xx/yy")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder1", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder1", "@/folder1", "@/folder2/aa/bb/folder1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",                 "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",                 "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "item_folder2_child", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",                 SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_folder2", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder2", "@/folder1/xx/yy/folder2", "@/folder2", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",                 "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "item_folder1_child", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",                 "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "item_folder1_child", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder1", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_folder2", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder1")
		VERIFY_FOLDER("@/folder1/xx")
		VERIFY_FOLDER("@/folder1/xx/yy")
		VERIFY_GONE("@/folder2")
		VERIFY_GONE("@/folder2/aa")
		VERIFY_GONE("@/folder2/aa/bb")
		VERIFY_FOLDER("@/folder1/xx/yy/folder2")
		VERIFY_FOLDER("@/folder1/xx/yy/folder2/aa")
		VERIFY_FOLDER("@/folder1/xx/yy/folder2/aa/bb")
		VERIFY_GONE("@/folder2/aa/bb/folder1")
		VERIFY_GONE("@/folder2/aa/bb/folder1/xx")
		VERIFY_GONE("@/folder2/aa/bb/folder1/xx/yy")
TEST_END

// a test that sets up a deep path cycle
// and accepts the state of all items from "other"
TEST("cycle__deep__other")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder1/xx", NULL)
		SETUP_ADD_FOLDER("@/folder1/xx/yy", "item_folder1_child")
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FOLDER("@/folder2/aa", NULL)
		SETUP_ADD_FOLDER("@/folder2/aa/bb", "item_folder2_child")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder1", "@/folder2/aa/bb")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder2", "@/folder1/xx/yy")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder1", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder1", "@/folder1", "@/folder2/aa/bb/folder1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",                 "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",                 "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "item_folder2_child", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",                 SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_folder2", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder2", "@/folder1/xx/yy/folder2", "@/folder2", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",                 "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "item_folder1_child", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",                 "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "item_folder1_child", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		// need to accept folder2 first in this case
		// otherwise we'll try to move folder1 into its own subfolder (folder2)
		RESOLVE_ACCEPT("item_folder2", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
		RESOLVE_ACCEPT("item_folder1", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/folder1")
		VERIFY_GONE("@/folder1/xx")
		VERIFY_GONE("@/folder1/xx/yy")
		VERIFY_FOLDER("@/folder2")
		VERIFY_FOLDER("@/folder2/aa")
		VERIFY_FOLDER("@/folder2/aa/bb")
		VERIFY_GONE("@/folder1/xx/yy/folder2")
		VERIFY_GONE("@/folder1/xx/yy/folder2/aa")
		VERIFY_GONE("@/folder1/xx/yy/folder2/aa/bb")
		VERIFY_FOLDER("@/folder2/aa/bb/folder1")
		VERIFY_FOLDER("@/folder2/aa/bb/folder1/xx")
		VERIFY_FOLDER("@/folder2/aa/bb/folder1/xx/yy")
TEST_END

// a test that sets up a deep path cycle
// and accepts the state of all items from "working"
TEST("cycle__deep__working")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder1/xx", NULL)
		SETUP_ADD_FOLDER("@/folder1/xx/yy", "item_folder1_child")
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FOLDER("@/folder2/aa", NULL)
		SETUP_ADD_FOLDER("@/folder2/aa/bb", "item_folder2_child")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder1", "@/folder2/aa/bb")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder2", "@/folder1/xx/yy")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_folder1", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder1", "@/folder1", "@/folder2/aa/bb/folder1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",                 "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",                 "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "item_folder2_child", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",                 SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_folder2", SG_TREENODEENTRY_TYPE_DIRECTORY, "@/folder2", "@/folder1/xx/yy/folder2", "@/folder2", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/",                 "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "item_folder1_child", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",                 "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "item_folder1_child", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_CYCLE("item_folder1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_folder1", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
		RESOLVE_ACCEPT("item_folder2", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FOLDER("@/folder1")
		VERIFY_FOLDER("@/folder1/xx")
		VERIFY_FOLDER("@/folder1/xx/yy")
		VERIFY_GONE("@/folder2")
		VERIFY_GONE("@/folder2/aa")
		VERIFY_GONE("@/folder2/aa/bb")
		VERIFY_FOLDER("@/folder1/xx/yy/folder2")
		VERIFY_FOLDER("@/folder1/xx/yy/folder2/aa")
		VERIFY_FOLDER("@/folder1/xx/yy/folder2/aa/bb")
		VERIFY_GONE("@/folder2/aa/bb/folder1")
		VERIFY_GONE("@/folder2/aa/bb/folder1/xx")
		VERIFY_GONE("@/folder2/aa/bb/folder1/xx/yy")
TEST_END
