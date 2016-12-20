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

// a test that creates colliding files via addition
// and accepts the state of all items from "baseline"
TEST("collision_add__simple__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/dummy.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_FILE("@/collision.txt", NULL, "contents_other", _content__append_numbered_lines, "7")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_FILE("@/collision.txt", NULL, "contents_baseline", _content__append_numbered_lines, "3")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_baseline", SG_TREENODEENTRY_TYPE_REGULAR_FILE, NULL, "@/collision.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_other")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_other", SG_TREENODEENTRY_TYPE_REGULAR_FILE, NULL, NULL, "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_baseline")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_baseline", SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_other",    SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/dummy.txt", NULL)
		VERIFY_FILE("@/collision.txt", "contents_baseline")
TEST_END

// a test that creates colliding files via addition
// and accepts the state of all items from "other"
TEST("collision_add__simple__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/dummy.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_FILE("@/collision.txt", NULL, "contents_other", _content__append_numbered_lines, "7")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_FILE("@/collision.txt", NULL, "contents_baseline", _content__append_numbered_lines, "3")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_baseline", SG_TREENODEENTRY_TYPE_REGULAR_FILE, NULL, "@/collision.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_other")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_other", SG_TREENODEENTRY_TYPE_REGULAR_FILE, NULL, NULL, "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_baseline")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_baseline", SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
		RESOLVE_ACCEPT("item_other",    SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/dummy.txt", NULL)
		VERIFY_FILE("@/collision.txt", "contents_other")
TEST_END

// a test that creates colliding files via addition
// and accepts the state of all items from "baseline"
// in the reverse order
TEST("collision_add__simple__baseline_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/dummy.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_FILE("@/collision.txt", NULL, "contents_other", _content__append_numbered_lines, "7")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_FILE("@/collision.txt", NULL, "contents_baseline", _content__append_numbered_lines, "3")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_baseline", SG_TREENODEENTRY_TYPE_REGULAR_FILE, NULL, "@/collision.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_other")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_other", SG_TREENODEENTRY_TYPE_REGULAR_FILE, NULL, NULL, "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_baseline")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_other",    SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_baseline", SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/dummy.txt", NULL)
		VERIFY_FILE("@/collision.txt", "contents_baseline")
TEST_END

// a test that creates colliding files via addition
// and accepts the state of all items from "other"
// in the reverse order
TEST("collision_add__simple__other_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/dummy.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_FILE("@/collision.txt", NULL, "contents_other", _content__append_numbered_lines, "7")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_FILE("@/collision.txt", NULL, "contents_baseline", _content__append_numbered_lines, "3")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_baseline", SG_TREENODEENTRY_TYPE_REGULAR_FILE, NULL, "@/collision.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_other")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_other", SG_TREENODEENTRY_TYPE_REGULAR_FILE, NULL, NULL, "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_baseline")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_other",    SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
		RESOLVE_ACCEPT("item_baseline", SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/dummy.txt", NULL)
		VERIFY_FILE("@/collision.txt", "contents_other")
TEST_END

// a test that creates colliding files via addition
// and resolves one of them by deleting it
TEST("collision_add__simple__delete")
	TEST_SETUP
		SETUP_ADD_FILE("@/dummy.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_FILE("@/collision.txt", NULL, "contents_other", _content__append_numbered_lines, "7")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_FILE("@/collision.txt", NULL, "contents_baseline", _content__append_numbered_lines, "3")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_baseline", SG_TREENODEENTRY_TYPE_REGULAR_FILE, NULL, "@/collision.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_other")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_other", SG_TREENODEENTRY_TYPE_REGULAR_FILE, NULL, NULL, "@/collision.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_baseline")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_DELETE_TEMP_FILE("@/collision.txt~L1~g~", "item_other")
	TEST_VERIFY
		VERIFY_FILE("@/dummy.txt", NULL)
		VERIFY_GONE("@/collision.txt")
		VERIFY_TEMP_GONE("@/collision.txt~L1~g~", "item_other")
TEST_END

#if defined(MAC) || defined(LINUX)

// a test that creates colliding symlinks via addition
// and accepts the state of all items from "baseline"
TEST("collision_add__symlink__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_SYMLINK("@/collision", NULL, "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_SYMLINK("@/collision", NULL, "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_baseline", SG_TREENODEENTRY_TYPE_SYMLINK, NULL, "@/collision", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_other")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_other", SG_TREENODEENTRY_TYPE_SYMLINK, NULL, NULL, "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_baseline")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_baseline", SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_other",    SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_SYMLINK("@/collision", "@/file_baseline.txt")
TEST_END

// a test that creates colliding symlinks via addition
// and accepts the state of all items from "other"
TEST("collision_add__symlink__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_SYMLINK("@/collision", NULL, "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_SYMLINK("@/collision", NULL, "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_baseline", SG_TREENODEENTRY_TYPE_SYMLINK, NULL, "@/collision", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_other")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_other", SG_TREENODEENTRY_TYPE_SYMLINK, NULL, NULL, "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_baseline")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_baseline", SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
		RESOLVE_ACCEPT("item_other",    SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_SYMLINK("@/collision", "@/file_other.txt")
TEST_END

// a test that creates colliding symlinks via addition
// and accepts the state of all items from "baseline"
// in the reverse order
TEST("collision_add__symlink__baseline_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_SYMLINK("@/collision", NULL, "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_SYMLINK("@/collision", NULL, "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_baseline", SG_TREENODEENTRY_TYPE_SYMLINK, NULL, "@/collision", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_other")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_other", SG_TREENODEENTRY_TYPE_SYMLINK, NULL, NULL, "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_baseline")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_other",    SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_baseline", SG_RESOLVE__STATE__EXISTENCE, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_SYMLINK("@/collision", "@/file_baseline.txt")
TEST_END

// a test that creates colliding symlinks via addition
// and accepts the state of all items from "other"
// in the reverse order
TEST("collision_add__symlink__other_reverse")
	TEST_SETUP
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_SYMLINK("@/collision", NULL, "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_SYMLINK("@/collision", NULL, "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_baseline", SG_TREENODEENTRY_TYPE_SYMLINK, NULL, "@/collision", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_other")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_other", SG_TREENODEENTRY_TYPE_SYMLINK, NULL, NULL, "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_baseline")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_other",    SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
		RESOLVE_ACCEPT("item_baseline", SG_RESOLVE__STATE__EXISTENCE, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_SYMLINK("@/collision", "@/file_other.txt")
TEST_END

// a test that creates colliding symlinks via addition
// and resolves one of them by deleting it
TEST("collision_add__symlink__delete")
	TEST_SETUP
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_ADD_SYMLINK("@/collision", NULL, "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_ADD_SYMLINK("@/collision", NULL, "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_baseline", SG_TREENODEENTRY_TYPE_SYMLINK, NULL, "@/collision", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_TRUE,  "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_FALSE, "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_other")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_other", SG_TREENODEENTRY_TYPE_SYMLINK, NULL, NULL, "@/collision", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, 0, SG_TRUE, NULL)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_BEGIN
					EXPECT_RELATED_COLLISION("item_baseline")
				EXPECT_RELATED_END
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_DELETE_TEMP_FILE("@/collision~L1~g~", "item_other")
	TEST_VERIFY
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_GONE("@/collision")
		VERIFY_TEMP_GONE("@/collision~L1~g~", "item_other")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

