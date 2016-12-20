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

// a test that creates colliding files via moving
// and accepts the state of all items from "ancestor"
TEST("collision_move__simple__ancestor")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/collision.txt", NULL)
		VERIFY_FILE("@/folder2/collision.txt", NULL)
		VERIFY_GONE("@/collision.txt")
TEST_END

// a test that creates colliding files via moving
// and accepts the state of all items from "baseline"
TEST("collision_move__simple__baseline")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/folder1/collision.txt")
		VERIFY_FILE("@/folder2/collision.txt", NULL)
		VERIFY_FILE("@/collision.txt", NULL)
TEST_END

// a test that creates colliding files via moving
// and accepts the state of all items from "other"
TEST("collision_move__simple__other")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/collision.txt", NULL)
		VERIFY_GONE("@/folder2/collision.txt")
		VERIFY_FILE("@/collision.txt", NULL)
TEST_END

// a test that creates colliding files via moving
// and accepts the state of all items from "ancestor"
// in the reverse order
TEST("collision_move__simple__ancestor_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/collision.txt", NULL)
		VERIFY_FILE("@/folder2/collision.txt", NULL)
		VERIFY_GONE("@/collision.txt")
TEST_END

// a test that creates colliding files via moving
// and accepts the state of all items from "baseline"
// in the reverse order
TEST("collision_move__simple__baseline_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/folder1/collision.txt")
		VERIFY_FILE("@/folder2/collision.txt", NULL)
		VERIFY_FILE("@/collision.txt", NULL)
TEST_END

// a test that creates colliding files via moving
// and accepts the state of all items from "other"
// in the reverse order
TEST("collision_move__simple__other_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/collision.txt", NULL)
		VERIFY_GONE("@/folder2/collision.txt")
		VERIFY_FILE("@/collision.txt", NULL)
TEST_END

// a test that creates colliding files via moving
// and modifies the WD after merging
// and accepts the state of all items from "ancestor"
TEST("collision_move__modify__ancestor")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_file1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_file2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/collision.txt", NULL)
		VERIFY_FILE("@/folder2/collision.txt", NULL)
		VERIFY_GONE("@/collision.txt")
		VERIFY_GONE("@/folder3/collision.txt")
		VERIFY_GONE("@/folder4/collision.txt")
TEST_END

// a test that creates colliding files via moving
// and modifies the WD after merging
// and accepts the state of all items from "baseline"
TEST("collision_move__modify__baseline")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_file1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_file2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/folder1/collision.txt")
		VERIFY_FILE("@/folder2/collision.txt", NULL)
		VERIFY_FILE("@/collision.txt", NULL)
		VERIFY_GONE("@/folder3/collision.txt")
		VERIFY_GONE("@/folder4/collision.txt")
TEST_END

// a test that creates colliding files via moving
// and modifies the WD after merging
// and accepts the state of all items from "other"
TEST("collision_move__modify__other")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_file1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_file2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/collision.txt", NULL)
		VERIFY_GONE("@/folder2/collision.txt")
		VERIFY_FILE("@/collision.txt", NULL)
		VERIFY_GONE("@/folder3/collision.txt")
		VERIFY_GONE("@/folder4/collision.txt")
TEST_END

// a test that creates colliding files via moving
// and modifies the WD after merging
// and accepts the state of all items from "working"
TEST("collision_move__modify__working")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_file1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_file2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/folder1/collision.txt")
		VERIFY_GONE("@/folder2/collision.txt")
		VERIFY_GONE("@/collision.txt")
		VERIFY_FILE("@/folder3/collision.txt", NULL)
		VERIFY_FILE("@/folder4/collision.txt", NULL)
TEST_END

// a test that creates colliding files via moving
// and modifies the WD after merging
// and accepts the state of all items from "ancestor"
// in the reverse order
TEST("collision_move__modify__ancestor_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_file1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_file2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/collision.txt", NULL)
		VERIFY_FILE("@/folder2/collision.txt", NULL)
		VERIFY_GONE("@/collision.txt")
		VERIFY_GONE("@/folder3/collision.txt")
		VERIFY_GONE("@/folder4/collision.txt")
TEST_END

// a test that creates colliding files via moving
// and modifies the WD after merging
// and accepts the state of all items from "baseline"
// in the reverse order
TEST("collision_move__modify__baseline_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_file1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_file2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/folder1/collision.txt")
		VERIFY_FILE("@/folder2/collision.txt", NULL)
		VERIFY_FILE("@/collision.txt", NULL)
		VERIFY_GONE("@/folder3/collision.txt")
		VERIFY_GONE("@/folder4/collision.txt")
TEST_END

// a test that creates colliding files via moving
// and modifies the WD after merging
// and accepts the state of all items from "other"
// in the reverse order
TEST("collision_move__modify__other_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_file1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_file2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/collision.txt", NULL)
		VERIFY_GONE("@/folder2/collision.txt")
		VERIFY_FILE("@/collision.txt", NULL)
		VERIFY_GONE("@/folder3/collision.txt")
		VERIFY_GONE("@/folder4/collision.txt")
TEST_END

// a test that creates colliding files via moving
// and modifies the WD after merging
// and accepts the state of all items from "working"
// in the reverse order
TEST("collision_move__modify__working_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/collision.txt", "item_file1", NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/collision.txt", "item_file2", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision.txt", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision.txt", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_file1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_file2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder1/collision.txt", "@/collision.txt", "@/folder1/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/folder2/collision.txt", "@/folder2/collision.txt", "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_file1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/folder1/collision.txt")
		VERIFY_GONE("@/folder2/collision.txt")
		VERIFY_GONE("@/collision.txt")
		VERIFY_FILE("@/folder3/collision.txt", NULL)
		VERIFY_FILE("@/folder4/collision.txt", NULL)
TEST_END

#if defined(MAC) || defined(LINUX)

// a test that creates colliding symlinks via moving
// and accepts the state of all items from "ancestor"
TEST("collision_move__symlink__ancestor")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", NULL, "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", NULL, "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_SYMLINK("@/folder1/collision", "@/folder1/file.txt")
		VERIFY_SYMLINK("@/folder2/collision", "@/folder2/file.txt")
		VERIFY_GONE("@/collision")
TEST_END

// a test that creates colliding symlinks via moving
// and accepts the state of all items from "baseline"
TEST("collision_move__symlink__baseline")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", NULL, "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", NULL, "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_GONE("@/folder1/collision")
		VERIFY_SYMLINK("@/folder2/collision", "@/folder2/file.txt")
		VERIFY_SYMLINK("@/collision", "@/folder1/file.txt")
TEST_END

// a test that creates colliding symlinks via moving
// and accepts the state of all items from "other"
TEST("collision_move__symlink__other")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", NULL, "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", NULL, "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_SYMLINK("@/folder1/collision", "@/folder1/file.txt")
		VERIFY_GONE("@/folder2/collision")
		VERIFY_SYMLINK("@/collision", "@/folder2/file.txt")
TEST_END

// a test that creates colliding symlinks via moving
// and accepts the state of all items from "ancestor"
// in the reverse order
TEST("collision_move__symlink__ancestor_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", NULL, "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", NULL, "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_SYMLINK("@/folder1/collision", "@/folder1/file.txt")
		VERIFY_SYMLINK("@/folder2/collision", "@/folder2/file.txt")
		VERIFY_GONE("@/collision")
TEST_END

// a test that creates colliding symlinks via moving
// and accepts the state of all items from "baseline"
// in the reverse order
TEST("collision_move__symlink__baseline_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", NULL, "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", NULL, "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_GONE("@/folder1/collision")
		VERIFY_SYMLINK("@/folder2/collision", "@/folder2/file.txt")
		VERIFY_SYMLINK("@/collision", "@/folder1/file.txt")
TEST_END

// a test that creates colliding symlinks via moving
// and accepts the state of all items from "other"
// in the reverse order
TEST("collision_move__symlink__other_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", NULL, "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", NULL, "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/",        SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_SYMLINK("@/folder1/collision", "@/folder1/file.txt")
		VERIFY_GONE("@/folder2/collision")
		VERIFY_SYMLINK("@/collision", "@/folder2/file.txt")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a test that creates colliding symlinks via moving
// and modifies the WD after merging
// and accepts the state of all items from "ancestor"
TEST("collision_move__symlink_modify__ancestor")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", "item_link1", "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", "item_link2", "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_link1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_link2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_SYMLINK("@/folder1/collision", "@/folder1/file.txt")
		VERIFY_SYMLINK("@/folder2/collision", "@/folder2/file.txt")
		VERIFY_GONE("@/collision")
		VERIFY_GONE("@/folder3/collision")
		VERIFY_GONE("@/folder4/collision")
TEST_END

// a test that creates colliding symlinks via moving
// and modifies the WD after merging
// and accepts the state of all items from "baseline"
TEST("collision_move__symlink_modify__baseline")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", "item_link1", "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", "item_link2", "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_link1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_link2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_GONE("@/folder1/collision")
		VERIFY_SYMLINK("@/folder2/collision", "@/folder2/file.txt")
		VERIFY_SYMLINK("@/collision", "@/folder1/file.txt")
		VERIFY_GONE("@/folder3/collision")
		VERIFY_GONE("@/folder4/collision")
TEST_END

// a test that creates colliding symlinks via moving
// and modifies the WD after merging
// and accepts the state of all items from "other"
TEST("collision_move__symlink_modify__other")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", "item_link1", "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", "item_link2", "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_link1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_link2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_SYMLINK("@/folder1/collision", "@/folder1/file.txt")
		VERIFY_GONE("@/folder2/collision")
		VERIFY_SYMLINK("@/collision", "@/folder2/file.txt")
		VERIFY_GONE("@/folder3/collision")
		VERIFY_GONE("@/folder4/collision")
TEST_END

// a test that creates colliding symlinks via moving
// and modifies the WD after merging
// and accepts the state of all items from "working"
TEST("collision_move__symlink_modify__working")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", "item_link1", "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", "item_link2", "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_link1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_link2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_GONE("@/folder1/collision")
		VERIFY_GONE("@/folder2/collision")
		VERIFY_GONE("@/collision")
		VERIFY_SYMLINK("@/folder3/collision", "@/folder1/file.txt")
		VERIFY_SYMLINK("@/folder4/collision", "@/folder2/file.txt")
TEST_END

// a test that creates colliding symlinks via moving
// and modifies the WD after merging
// and accepts the state of all items from "ancestor"
// in reverse order
TEST("collision_move__symlink_modify__ancestor_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", "item_link1", "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", "item_link2", "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_link1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_link2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_SYMLINK("@/folder1/collision", "@/folder1/file.txt")
		VERIFY_SYMLINK("@/folder2/collision", "@/folder2/file.txt")
		VERIFY_GONE("@/collision")
		VERIFY_GONE("@/folder3/collision")
		VERIFY_GONE("@/folder4/collision")
TEST_END

// a test that creates colliding symlinks via moving
// and modifies the WD after merging
// and accepts the state of all items from "baseline"
// in reverse order
TEST("collision_move__symlink_modify__baseline_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", "item_link1", "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", "item_link2", "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_link1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_link2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_GONE("@/folder1/collision")
		VERIFY_SYMLINK("@/folder2/collision", "@/folder2/file.txt")
		VERIFY_SYMLINK("@/collision", "@/folder1/file.txt")
		VERIFY_GONE("@/folder3/collision")
		VERIFY_GONE("@/folder4/collision")
TEST_END

// a test that creates colliding symlinks via moving
// and modifies the WD after merging
// and accepts the state of all items from "other"
// in reverse order
TEST("collision_move__symlink_modify__other_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", "item_link1", "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", "item_link2", "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_link1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_link2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_SYMLINK("@/folder1/collision", "@/folder1/file.txt")
		VERIFY_GONE("@/folder2/collision")
		VERIFY_SYMLINK("@/collision", "@/folder2/file.txt")
		VERIFY_GONE("@/folder3/collision")
		VERIFY_GONE("@/folder4/collision")
TEST_END

// a test that creates colliding symlinks via moving
// and modifies the WD after merging
// and accepts the state of all items from "working"
// in reverse order
TEST("collision_move__symlink_modify__working_reverse")
	TEST_SETUP
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_ADD_FILE("@/folder1/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/folder2/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/folder1/collision", "item_link1", "@/folder1/file.txt")
		SETUP_ADD_SYMLINK("@/folder2/collision", "item_link2", "@/folder2/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/folder2/collision", "@/")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/folder1/collision", "@/")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_ADD_FOLDER("@/folder3", NULL)
		SETUP_MOVE("item_link1", "@/folder3")
		SETUP_ADD_FOLDER("@/folder4", NULL)
		SETUP_MOVE("item_link2", "@/folder4")
	TEST_EXPECT
		EXPECT_ITEM("item_link1", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder1/collision", "@/collision", "@/folder1/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/",        "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/folder1", "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder3", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link2")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_link2", SG_TREENODEENTRY_TYPE_SYMLINK, "@/folder2/collision", "@/folder2/collision", "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__LOCATION, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/folder2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/folder2", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/",        "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "@/folder4", SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_link1")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link2", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
		RESOLVE_ACCEPT("item_link1", SG_RESOLVE__STATE__LOCATION, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/folder1/file.txt", NULL)
		VERIFY_FILE("@/folder2/file.txt", NULL)
		VERIFY_GONE("@/folder1/collision")
		VERIFY_GONE("@/folder2/collision")
		VERIFY_GONE("@/collision")
		VERIFY_SYMLINK("@/folder3/collision", "@/folder1/file.txt")
		VERIFY_SYMLINK("@/folder4/collision", "@/folder2/file.txt")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

