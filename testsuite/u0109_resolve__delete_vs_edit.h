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
 * This file contains tests that setup delete-vs-edit conflicts.
 */

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// which accepts the "ancestor" value
TEST("delete_vs_edit__simple__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", "contents_other")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// which accepts the "baseline" value
TEST("delete_vs_edit__simple__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// which accepts the "other" value
TEST("delete_vs_edit__simple__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", "contents_other")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// which accepts the "working" value
TEST("delete_vs_edit__simple__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", "contents_other")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// which accepts the "ancestor" value
// and then accepts the "baseline" value
TEST("delete_vs_edit__simple__ancestor_baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// which accepts the "ancestor" value
// and then accepts the "other" value
TEST("delete_vs_edit__simple__ancestor_other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", "contents_other")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// which accepts the "ancestor" value
// and then accepts the "working" value
TEST("delete_vs_edit__simple__ancestor_working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", "contents_other")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// which accepts the "working" value
// and then accepts the "ancestor" value
TEST("delete_vs_edit__simple__working_ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", "contents_other")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// which accepts the "ancestor" value
TEST("delete_vs_edit__reverse__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", "contents_baseline")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// which accepts the "baseline" value
TEST("delete_vs_edit__reverse__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", "contents_baseline")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// which accepts the "other" value
TEST("delete_vs_edit__reverse__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// which accepts the "working" value
TEST("delete_vs_edit__reverse__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", "contents_baseline")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// which accepts the "ancestor" value
// and then accepts the "baseline" value
TEST("delete_vs_edit__reverse__ancestor_baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", "contents_baseline")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// which accepts the "ancestor" value
// and then accepts the "other" value
TEST("delete_vs_edit__reverse__ancestor_other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// which accepts the "ancestor" value
// and then accepts the "working" value
TEST("delete_vs_edit__reverse__ancestor_working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", "contents_baseline")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// which accepts the "working" value
// and then accepts the "ancestor" value
TEST("delete_vs_edit__reverse__working_ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", "contents_baseline")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// that renames the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__rename__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the file's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/file_renamed.txt", "contents_other")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// that renames the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__rename__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the file's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/file_renamed.txt")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// that renames the item after merging
// which accepts the "other" value
TEST("delete_vs_edit__rename__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the file's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/file_renamed.txt", "contents_other")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// that renames the item after merging
// which accepts the "working" value
TEST("delete_vs_edit__rename__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the file's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/file_renamed.txt", "contents_other")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// that renames the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__reverse_rename__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the file's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/file_renamed.txt", "contents_baseline")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// that renames the item after merging
// which accepts the "baseline" value
TEST("delete_vs_edit__reverse_rename__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the file's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/file_renamed.txt", "contents_baseline")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// that renames the item after merging
// which accepts the "other" value
TEST("delete_vs_edit__reverse_rename__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the file's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/file_renamed.txt")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// that renames the item after merging
// which accepts the "working" value
TEST("delete_vs_edit__reverse_rename__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the file's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/file_renamed.txt", "contents_baseline")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// that moves the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__move__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/folder/file.txt", "contents_other")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// that moves the item after merging
// which accepts the "baseline" value
TEST("delete_vs_edit__move__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/folder/file.txt")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// that moves the item after merging
// which accepts the "other" value
TEST("delete_vs_edit__move__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/folder/file.txt", "contents_other")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// that moves the item after merging
// which accepts the "working" value
TEST("delete_vs_edit__move__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/folder/file.txt", "contents_other")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// that moves the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__reverse_move__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/folder/file.txt", "contents_baseline")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// that moves the item after merging
// which accepts the "baseline" value
TEST("delete_vs_edit__reverse_move__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/folder/file.txt", "contents_baseline")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// that moves the item after merging
// which accepts the "other" value
TEST("delete_vs_edit__reverse_move__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/folder/file.txt")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// that moves the item after merging
// which accepts the "working" value
TEST("delete_vs_edit__reverse_move__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/folder/file.txt", "contents_baseline")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// that moves and renames the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__move_rename__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/folder/file.txt")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/folder/file_renamed.txt", "contents_other")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// that moves and renames the item after merging
// which accepts the "baseline" value
TEST("delete_vs_edit__move_rename__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/folder/file.txt")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/folder/file_renamed.txt")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// that moves and renames the item after merging
// which accepts the "other" value
TEST("delete_vs_edit__move_rename__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/folder/file.txt")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/folder/file_renamed.txt", "contents_other")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// that moves and renames the item after merging
// which accepts the "working" value
TEST("delete_vs_edit__move_rename__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/folder/file.txt")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/folder/file_renamed.txt", "contents_other")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// that moves and renames the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__reverse_move_rename__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/folder/file.txt")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/folder/file_renamed.txt", "contents_baseline")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// that moves and renames the item after merging
// which accepts the "baseline" value
TEST("delete_vs_edit__reverse_move_rename__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/folder/file.txt")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/folder/file_renamed.txt", "contents_baseline")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// that moves and renames the item after merging
// which accepts the "other" value
TEST("delete_vs_edit__reverse_move_rename__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/folder/file.txt")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/folder/file_renamed.txt")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "other"
// that moves and renames the item after merging
// which accepts the "working" value
TEST("delete_vs_edit__reverse_move_rename__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_file", "@/folder")
		SETUP_RENAME("item_file", "file_renamed.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_TEMP_GONE("@/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/file.txt~L1~g~", "item_file")
		VERIFY_GONE("@/folder/file.txt")
		VERIFY_TEMP_GONE("@/folder/file.txt~L0~g~", "item_file")
		VERIFY_TEMP_GONE("@/folder/file.txt~L1~g~", "item_file")
		VERIFY_FILE("@/folder/file_renamed.txt", "contents_baseline")
TEST_END

// a simple delete-vs-edit test
// where the delete occurs in the "baseline"
// which resolves the file by deleting it
TEST("delete_vs_edit__simple__delete")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", NULL, "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_DELETE_TEMP_FILE("@/file.txt~A~g~", "item_file")
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_TEMP_GONE("@/file.txt~A~g~", "item_file")
TEST_END

#if defined(MAC) || defined(LINUX)

// a simple delete-vs-edit test
// with a symlink
// where the delete occurs in the "baseline"
// which accepts the "ancestor" value
TEST("delete_vs_edit__symlink__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// with a symlink
// where the delete occurs in the "baseline"
// which accepts the "baseline" value
TEST("delete_vs_edit__symlink__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
TEST_END

// a simple delete-vs-edit test
// with a symlink
// where the delete occurs in the "baseline"
// which accepts the "other" value
TEST("delete_vs_edit__symlink__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// with a symlink
// where the delete occurs in the "baseline"
// which accepts the "working" value
TEST("delete_vs_edit__symlink__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file_edited.txt")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a simple delete-vs-edit test
// with a symlink
// where the delete occurs in the "other"
// which accepts the "ancestor" value
TEST("delete_vs_edit__symlink_reverse__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// with a symlink
// where the delete occurs in the "other"
// which accepts the "baseline" value
TEST("delete_vs_edit__symlink_reverse__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// with a symlink
// where the delete occurs in the "other"
// which accepts the "other" value
TEST("delete_vs_edit__symlink_reverse__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
TEST_END

// a simple delete-vs-edit test
// with a symlink
// where the delete occurs in the "other"
// which accepts the "working" value
TEST("delete_vs_edit__symlink_reverse__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file_edited.txt")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "baseline"
// that renames the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__symlink_rename__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the link's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/link_renamed", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "baseline"
// that renames the item after merging
// which accepts the "baseline" value
TEST("delete_vs_edit__symlink_rename__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the link's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_GONE("@/link_renamed")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "baseline"
// that renames the item after merging
// which accepts the "other" value
TEST("delete_vs_edit__symlink_rename__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the link's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/link_renamed", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "baseline"
// that renames the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__symlink_rename__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the link's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/link_renamed", "@/file_edited.txt")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "other"
// that renames the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__symlink_reverse_rename__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the link's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/link_renamed", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "other"
// that renames the item after merging
// which accepts the "baseline" value
TEST("delete_vs_edit__symlink_reverse_rename__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the link's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/link_renamed", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "other"
// that renames the item after merging
// which accepts the "other" value
TEST("delete_vs_edit__symlink_reverse_rename__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the link's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_GONE("@/link_renamed")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "other"
// that renames the item after merging
// which accepts the "working" value
TEST("delete_vs_edit__symlink_reverse_rename__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		// The main point of this test is to make sure that resolving the
		// conflict reverts the link's temporary name to the name it was
		// given post-merge, not its original name.
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/link_renamed", "@/file_edited.txt")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "baseline"
// that moves the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__symlink_move__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/folder/link", "@/file_edited.txt")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "baseline"
// that moves the item after merging
// which accepts the "baseline" value
TEST("delete_vs_edit__symlink_move__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_GONE("@/folder/link")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "baseline"
// that moves the item after merging
// which accepts the "other" value
TEST("delete_vs_edit__symlink_move__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/folder/link", "@/file_edited.txt")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "baseline"
// that moves the item after merging
// which accepts the "working" value
TEST("delete_vs_edit__symlink_move__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/folder/link", "@/file_edited.txt")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "other"
// that moves the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__symlink_reverse_move__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/folder/link", "@/file_edited.txt")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "other"
// that moves the item after merging
// which accepts the "baseline" value
TEST("delete_vs_edit__symlink_reverse_move__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/folder/link", "@/file_edited.txt")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "other"
// that moves the item after merging
// which accepts the "other" value
TEST("delete_vs_edit__symlink_reverse_move__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_GONE("@/folder/link")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "other"
// that moves the item after merging
// which accepts the "working" value
TEST("delete_vs_edit__symlink_reverse_move__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/folder/link", "@/file_edited.txt")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "baseline"
// that moves and renames the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__symlink_move_rename__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_GONE("@/folder/link")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/folder/link_renamed", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "baseline"
// that moves and renames the item after merging
// which accepts the "baseline" value
TEST("delete_vs_edit__symlink_move_rename__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_GONE("@/folder/link")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
		VERIFY_GONE("@/folder/link_renamed")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "baseline"
// that moves and renames the item after merging
// which accepts the "other" value
TEST("delete_vs_edit__symlink_move_rename__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_GONE("@/folder/link")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/folder/link_renamed", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "baseline"
// that moves and renames the item after merging
// which accepts the "working" value
TEST("delete_vs_edit__symlink_move_rename__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_GONE("@/folder/link")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/folder/link_renamed", "@/file_edited.txt")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "other"
// that moves and renames the item after merging
// which accepts the "ancestor" value
TEST("delete_vs_edit__symlink_reverse_move_rename__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_GONE("@/folder/link")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/folder/link_renamed", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "other"
// that moves and renames the item after merging
// which accepts the "baseline" value
TEST("delete_vs_edit__symlink_reverse_move_rename__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_GONE("@/folder/link")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/folder/link_renamed", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "other"
// that moves and renames the item after merging
// which accepts the "other" value
TEST("delete_vs_edit__symlink_reverse_move_rename__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_GONE("@/folder/link")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
		VERIFY_GONE("@/folder/link_renamed")
TEST_END

// a simple delete-vs-edit test
// of symlinks
// where the delete occurs in the "other"
// that moves and renames the item after merging
// which accepts the "working" value
TEST("delete_vs_edit__symlink_reverse_move_rename__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder", NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MOVE("item_link", "@/folder")
		SETUP_RENAME("item_link", "link_renamed")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", NULL, SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
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
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/link~L1~g~", "item_link")
		VERIFY_GONE("@/folder/link")
		VERIFY_TEMP_GONE("@/folder/link~L0~g~", "item_link")
		VERIFY_TEMP_GONE("@/folder/link~L1~g~", "item_link")
		VERIFY_SYMLINK("@/folder/link_renamed", "@/file_edited.txt")
TEST_END

// a simple delete-vs-edit test
// with a symlink
// where the delete occurs in the "baseline"
// which resolves the file by deleting it
TEST("delete_vs_edit__symlink__delete")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_edited.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_edited.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/link")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", NULL, "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__EXISTENCE, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__BOOL("value_ancestor", SG_TRUE,  "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_baseline", SG_FALSE, "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__BOOL("value_other",    SG_TRUE,  "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__BOOL(  "value_working",  SG_TRUE,  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_DELETE_TEMP_FILE("@/link~A~g~", "item_link")
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_edited.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_TEMP_GONE("@/link~A~g~", "item_link")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

