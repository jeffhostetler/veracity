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
 * This file contains tests that setup collision conflicts.
 */

// a test that creates colliding files via renaming
// and accepts the state of all items from "ancestor"
TEST("collision_rename__simple__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_GONE("@/collision.txt")
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and accepts the state of all items from "baseline"
TEST("collision_rename__simple__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file1.txt")
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_FILE("@/collision.txt", NULL)
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and accepts the state of all items from "other"
TEST("collision_rename__simple__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_other", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_GONE("@/file2.txt")
		VERIFY_FILE("@/collision.txt", NULL)
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and accepts the state of all items from "working"
TEST("collision_rename__simple__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_working", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file1.txt")
		VERIFY_GONE("@/file2.txt")
		VERIFY_GONE("@/collision.txt")
		VERIFY_TEMP_FILE("@/collision.txt~L0~g~", "item_file1", NULL)
		VERIFY_TEMP_FILE("@/collision.txt~L1~g~", "item_file2", NULL)
TEST_END

// a test that creates colliding files via renaming
// and accepts the state of all items from "ancestor"
// in the reverse order
TEST("collision_rename__simple__ancestor_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_GONE("@/collision.txt")
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and accepts the state of all items from "baseline"
// in the reverse order
TEST("collision_rename__simple__baseline_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file1.txt")
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_FILE("@/collision.txt", NULL)
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and accepts the state of all items from "other"
// in the reverse order
TEST("collision_rename__simple__other_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_other", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_GONE("@/file2.txt")
		VERIFY_FILE("@/collision.txt", NULL)
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and accepts the state of all items from "working"
// in the reverse order
TEST("collision_rename__simple__working_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision.txt~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_working", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file1.txt")
		VERIFY_GONE("@/file2.txt")
		VERIFY_GONE("@/collision.txt")
		VERIFY_TEMP_FILE("@/collision.txt~L0~g~", "item_file1", NULL)
		VERIFY_TEMP_FILE("@/collision.txt~L1~g~", "item_file2", NULL)
TEST_END

// a test that creates colliding files via renaming
// and modifies the WD after merging
// and accepts the state of all items from "ancestor"
TEST("collision_rename__modify__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file1", "collision1.txt")
		SETUP_RENAME("item_file2", "collision2.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_GONE("@/collision.txt")
		VERIFY_GONE("@/collision1.txt")
		VERIFY_GONE("@/collision2.txt")
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and modifies the WD after merging
// and accepts the state of all items from "baseline"
TEST("collision_rename__modify__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file1", "collision1.txt")
		SETUP_RENAME("item_file2", "collision2.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file1.txt")
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_FILE("@/collision.txt", NULL)
		VERIFY_GONE("@/collision1.txt")
		VERIFY_GONE("@/collision2.txt")
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and modifies the WD after merging
// and accepts the state of all items from "other"
TEST("collision_rename__modify__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file1", "collision1.txt")
		SETUP_RENAME("item_file2", "collision2.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_other", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_GONE("@/file2.txt")
		VERIFY_FILE("@/collision.txt", NULL)
		VERIFY_GONE("@/collision1.txt")
		VERIFY_GONE("@/collision2.txt")
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and modifies the WD after merging
// and accepts the state of all items from "working"
TEST("collision_rename__modify__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file1", "collision1.txt")
		SETUP_RENAME("item_file2", "collision2.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_working", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file1.txt")
		VERIFY_GONE("@/file2.txt")
		VERIFY_GONE("@/collision.txt")
		VERIFY_FILE("@/collision1.txt", NULL)
		VERIFY_FILE("@/collision2.txt", NULL)
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and modifies the WD after merging
// and accepts the state of all items from "ancestor"
// in the reverse order
TEST("collision_rename__modify__ancestor_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file1", "collision1.txt")
		SETUP_RENAME("item_file2", "collision2.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_GONE("@/collision.txt")
		VERIFY_GONE("@/collision1.txt")
		VERIFY_GONE("@/collision2.txt")
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and modifies the WD after merging
// and accepts the state of all items from "baseline"
// in the reverse order
TEST("collision_rename__modify__baseline_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file1", "collision1.txt")
		SETUP_RENAME("item_file2", "collision2.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file1.txt")
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_FILE("@/collision.txt", NULL)
		VERIFY_GONE("@/collision1.txt")
		VERIFY_GONE("@/collision2.txt")
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and modifies the WD after merging
// and accepts the state of all items from "other"
// in the reverse order
TEST("collision_rename__modify__other_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file1", "collision1.txt")
		SETUP_RENAME("item_file2", "collision2.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_other", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_GONE("@/file2.txt")
		VERIFY_FILE("@/collision.txt", NULL)
		VERIFY_GONE("@/collision1.txt")
		VERIFY_GONE("@/collision2.txt")
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

// a test that creates colliding files via renaming
// and modifies the WD after merging
// and accepts the state of all items from "working"
// in the reverse order
TEST("collision_rename__modify__working_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "collision.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "collision.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file1", "collision1.txt")
		SETUP_RENAME("item_file2", "collision2.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/collision.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file1.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision.txt",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file1.txt",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file2.txt",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file2.txt",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision.txt",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2.txt", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__NAME, "value_working", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file1.txt")
		VERIFY_GONE("@/file2.txt")
		VERIFY_GONE("@/collision.txt")
		VERIFY_FILE("@/collision1.txt", NULL)
		VERIFY_FILE("@/collision2.txt", NULL)
		VERIFY_TEMP_GONE("@/collision.txt~L0~g~", "item_file1")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_file2")
TEST_END

#if defined(MAC) || defined(LINUX)

// a test that creates colliding symlinks via renaming
// and accepts the state of all items from "ancestor"
TEST("collision_rename__symlink__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", NULL, "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", NULL, "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_SYMLINK("@/link1", "@/file1.txt")
		VERIFY_SYMLINK("@/link2", "@/file2.txt")
		VERIFY_GONE("@/collision")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and accepts the state of all items from "baseline"
TEST("collision_rename__symlink__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", NULL, "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", NULL, "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_GONE("@/link1")
		VERIFY_SYMLINK("@/link2", "@/file2.txt")
		VERIFY_SYMLINK("@/collision", "@/file1.txt")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and accepts the state of all items from "other"
TEST("collision_rename__symlink__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", NULL, "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", NULL, "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_other", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_SYMLINK("@/link1", "@/file1.txt")
		VERIFY_GONE("@/link2")
		VERIFY_SYMLINK("@/collision", "@/file2.txt")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and accepts the state of all items from "working"
TEST("collision_rename__symlink__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", NULL, "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", NULL, "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_working", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_GONE("@/link1")
		VERIFY_GONE("@/link2")
		VERIFY_GONE("@/collision")
		VERIFY_TEMP_SYMLINK("@/collision~L0~g~", "item_link1", "@/file1.txt")
		VERIFY_TEMP_SYMLINK("@/collision~L1~g~", "item_link2", "@/file2.txt")
TEST_END

// a test that creates colliding symlinks via renaming
// and accepts the state of all items from "ancestor"
// in the reverse order
TEST("collision_rename__symlink__ancestor_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", NULL, "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", NULL, "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_SYMLINK("@/link1", "@/file1.txt")
		VERIFY_SYMLINK("@/link2", "@/file2.txt")
		VERIFY_GONE("@/collision")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and accepts the state of all items from "baseline"
// in the reverse order
TEST("collision_rename__symlink__baseline_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", NULL, "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", NULL, "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_GONE("@/link1")
		VERIFY_SYMLINK("@/link2", "@/file2.txt")
		VERIFY_SYMLINK("@/collision", "@/file1.txt")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and accepts the state of all items from "other"
// in the reverse order
TEST("collision_rename__symlink__other_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", NULL, "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", NULL, "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_other", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_SYMLINK("@/link1", "@/file1.txt")
		VERIFY_GONE("@/link2")
		VERIFY_SYMLINK("@/collision", "@/file2.txt")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and accepts the state of all items from "working"
// in the reverse order
TEST("collision_rename__symlink__working_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", NULL, "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", NULL, "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",           "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L0~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",           "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",           "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",       "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision~L1~g~", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_working", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_GONE("@/link1")
		VERIFY_GONE("@/link2")
		VERIFY_GONE("@/collision")
		VERIFY_TEMP_SYMLINK("@/collision~L0~g~", "item_link1", "@/file1.txt")
		VERIFY_TEMP_SYMLINK("@/collision~L1~g~", "item_link2", "@/file2.txt")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a test that creates colliding symlinks via renaming
// and modifies the WD after merging
// and accepts the state of all items from "ancestor"
TEST("collision_rename__symlink_modify__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", "item_link1", "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", "item_link2", "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link1", "collision1")
		SETUP_RENAME("item_link2", "collision2")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_SYMLINK("@/link1", "@/file1.txt")
		VERIFY_SYMLINK("@/link2", "@/file2.txt")
		VERIFY_GONE("@/collision")
		VERIFY_GONE("@/collision1")
		VERIFY_GONE("@/collision2")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and modifies the WD after merging
// and accepts the state of all items from "baseline"
TEST("collision_rename__symlink_modify__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", "item_link1", "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", "item_link2", "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link1", "collision1")
		SETUP_RENAME("item_link2", "collision2")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_GONE("@/link1")
		VERIFY_SYMLINK("@/link2", "@/file2.txt")
		VERIFY_SYMLINK("@/collision", "@/file1.txt")
		VERIFY_GONE("@/collision1")
		VERIFY_GONE("@/collision2")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and modifies the WD after merging
// and accepts the state of all items from "other"
TEST("collision_rename__symlink_modify__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", "item_link1", "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", "item_link2", "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link1", "collision1")
		SETUP_RENAME("item_link2", "collision2")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_other", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_SYMLINK("@/link1", "@/file1.txt")
		VERIFY_GONE("@/link2")
		VERIFY_SYMLINK("@/collision", "@/file2.txt")
		VERIFY_GONE("@/collision1")
		VERIFY_GONE("@/collision2")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and modifies the WD after merging
// and accepts the state of all items from "working"
TEST("collision_rename__symlink_modify__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", "item_link1", "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", "item_link2", "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link1", "collision1")
		SETUP_RENAME("item_link2", "collision2")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_working", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_GONE("@/link1")
		VERIFY_GONE("@/link2")
		VERIFY_GONE("@/collision")
		VERIFY_SYMLINK("@/collision1", "@/file1.txt")
		VERIFY_SYMLINK("@/collision2", "@/file2.txt")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and modifies the WD after merging
// and accepts the state of all items from "ancestor"
// in the reverse order
TEST("collision_rename__symlink_modify__ancestor_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", "item_link1", "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", "item_link2", "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link1", "collision1")
		SETUP_RENAME("item_link2", "collision2")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_SYMLINK("@/link1", "@/file1.txt")
		VERIFY_SYMLINK("@/link2", "@/file2.txt")
		VERIFY_GONE("@/collision")
		VERIFY_GONE("@/collision1")
		VERIFY_GONE("@/collision2")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and modifies the WD after merging
// and accepts the state of all items from "baseline"
// in the reverse order
TEST("collision_rename__symlink_modify__baseline_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", "item_link1", "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", "item_link2", "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link1", "collision1")
		SETUP_RENAME("item_link2", "collision2")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_GONE("@/link1")
		VERIFY_SYMLINK("@/link2", "@/file2.txt")
		VERIFY_SYMLINK("@/collision", "@/file1.txt")
		VERIFY_GONE("@/collision1")
		VERIFY_GONE("@/collision2")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and modifies the WD after merging
// and accepts the state of all items from "other"
// in the reverse order
TEST("collision_rename__symlink_modify__other_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", "item_link1", "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", "item_link2", "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link1", "collision1")
		SETUP_RENAME("item_link2", "collision2")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_other", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_SYMLINK("@/link1", "@/file1.txt")
		VERIFY_GONE("@/link2")
		VERIFY_SYMLINK("@/collision", "@/file2.txt")
		VERIFY_GONE("@/collision1")
		VERIFY_GONE("@/collision2")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

// a test that creates colliding symlinks via renaming
// and modifies the WD after merging
// and accepts the state of all items from "working"
// in the reverse order
TEST("collision_rename__symlink_modify__working_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link1", "item_link1", "@/file1.txt")
		SETUP_ADD_SYMLINK("@/link2", "item_link2", "@/file2.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link2", "collision")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link1", "collision")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link1", "collision1")
		SETUP_RENAME("item_link2", "collision2")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link1", "@/collision", "@/link1", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link1",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "collision",  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link1",      "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision1", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link2", "@/link2", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link2",      "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link2",      "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "collision",  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "collision2", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__NAME, "value_working", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", NULL)
		VERIFY_FILE("@/file2.txt", NULL)
		VERIFY_GONE("@/link1")
		VERIFY_GONE("@/link2")
		VERIFY_GONE("@/collision")
		VERIFY_SYMLINK("@/collision1", "@/file1.txt")
		VERIFY_SYMLINK("@/collision2", "@/file2.txt")
		VERIFY_TEMP_GONE("@/collision~L0~g~", "item_link1")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_link2")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

