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
 * This file contains tests that setup divergent edit conflicts.
 */

// a divergent edit
// that conflicts
// and accepts the "ancestor" value
TEST("divergent_edit__conflict__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_ancestor")
TEST_END

// a divergent edit
// that conflicts
// and accepts the "baseline" value
TEST("divergent_edit__conflict__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_baseline")
TEST_END

// a divergent edit
// that conflicts
// and accepts the "other" value
TEST("divergent_edit__conflict__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_other")
TEST_END

// a divergent edit
// that conflicts
// and accepts the "working" value
TEST("divergent_edit__conflict__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_merged")
TEST_END

// a divergent edit
// that conflicts
// and accepts the "merged" value
TEST("divergent_edit__conflict__merged")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merged", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_merged")
TEST_END

// multiple divergent edits
// that conflict
// and accept the "ancestor" value
TEST("divergent_edit__conflict_multiple__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, "contents_ancestor1", _content__append_numbered_lines, "4")
		SETUP_ADD_FILE("@/file2.txt", NULL, "contents_ancestor2", _content__append_numbered_lines, "6")
		SETUP_ADD_FILE("@/file3.txt", NULL, "contents_ancestor3", _content__append_numbered_lines, "8")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file1.txt", "contents_other1", _content__replace_first_line, "other1")
		SETUP_MODIFY_FILE("@/file2.txt", "contents_other2", _content__replace_first_line, "other2")
		SETUP_MODIFY_FILE("@/file3.txt", "contents_other3", _content__replace_first_line, "other3")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file1.txt", "contents_baseline1", _content__replace_first_line, "baseline1")
		SETUP_MODIFY_FILE("@/file2.txt", "contents_baseline2", _content__replace_first_line, "baseline2")
		SETUP_MODIFY_FILE("@/file3.txt", "contents_baseline3", _content__replace_first_line, "baseline3")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged1", "contents_ancestor1", "contents_baseline1", "contents_other1")
		SETUP_CONTENTS("contents_merged2", "contents_ancestor2", "contents_baseline2", "contents_other2")
		SETUP_CONTENTS("contents_merged3", "contents_ancestor3", "contents_baseline3", "contents_other3")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/file1.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline1", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other1",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged1", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged1", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/file2.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline2", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other2",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged2", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged2", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file3", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file3.txt", "@/file3.txt", "@/file3.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor3", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline3", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other3",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged3", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged3", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file3", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", "contents_ancestor1")
		VERIFY_FILE("@/file2.txt", "contents_ancestor2")
		VERIFY_FILE("@/file3.txt", "contents_ancestor3")
TEST_END

// multiple divergent edits
// that conflict
// and accept the "baseline" value
TEST("divergent_edit__conflict_multiple__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, "contents_ancestor1", _content__append_numbered_lines, "4")
		SETUP_ADD_FILE("@/file2.txt", NULL, "contents_ancestor2", _content__append_numbered_lines, "6")
		SETUP_ADD_FILE("@/file3.txt", NULL, "contents_ancestor3", _content__append_numbered_lines, "8")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file1.txt", "contents_other1", _content__replace_first_line, "other1")
		SETUP_MODIFY_FILE("@/file2.txt", "contents_other2", _content__replace_first_line, "other2")
		SETUP_MODIFY_FILE("@/file3.txt", "contents_other3", _content__replace_first_line, "other3")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file1.txt", "contents_baseline1", _content__replace_first_line, "baseline1")
		SETUP_MODIFY_FILE("@/file2.txt", "contents_baseline2", _content__replace_first_line, "baseline2")
		SETUP_MODIFY_FILE("@/file3.txt", "contents_baseline3", _content__replace_first_line, "baseline3")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged1", "contents_ancestor1", "contents_baseline1", "contents_other1")
		SETUP_CONTENTS("contents_merged2", "contents_ancestor2", "contents_baseline2", "contents_other2")
		SETUP_CONTENTS("contents_merged3", "contents_ancestor3", "contents_baseline3", "contents_other3")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/file1.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline1", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other1",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged1", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged1", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/file2.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline2", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other2",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged2", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged2", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file3", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file3.txt", "@/file3.txt", "@/file3.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor3", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline3", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other3",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged3", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged3", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__CONTENTS, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__CONTENTS, "value_baseline", NULL)
		RESOLVE_ACCEPT("item_file3", SG_RESOLVE__STATE__CONTENTS, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", "contents_baseline1")
		VERIFY_FILE("@/file2.txt", "contents_baseline2")
		VERIFY_FILE("@/file3.txt", "contents_baseline3")
TEST_END

// multiple divergent edits
// that conflict
// and accept the "other" value
TEST("divergent_edit__conflict_multiple__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, "contents_ancestor1", _content__append_numbered_lines, "4")
		SETUP_ADD_FILE("@/file2.txt", NULL, "contents_ancestor2", _content__append_numbered_lines, "6")
		SETUP_ADD_FILE("@/file3.txt", NULL, "contents_ancestor3", _content__append_numbered_lines, "8")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file1.txt", "contents_other1", _content__replace_first_line, "other1")
		SETUP_MODIFY_FILE("@/file2.txt", "contents_other2", _content__replace_first_line, "other2")
		SETUP_MODIFY_FILE("@/file3.txt", "contents_other3", _content__replace_first_line, "other3")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file1.txt", "contents_baseline1", _content__replace_first_line, "baseline1")
		SETUP_MODIFY_FILE("@/file2.txt", "contents_baseline2", _content__replace_first_line, "baseline2")
		SETUP_MODIFY_FILE("@/file3.txt", "contents_baseline3", _content__replace_first_line, "baseline3")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged1", "contents_ancestor1", "contents_baseline1", "contents_other1")
		SETUP_CONTENTS("contents_merged2", "contents_ancestor2", "contents_baseline2", "contents_other2")
		SETUP_CONTENTS("contents_merged3", "contents_ancestor3", "contents_baseline3", "contents_other3")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/file1.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline1", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other1",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged1", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged1", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/file2.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline2", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other2",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged2", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged2", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file3", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file3.txt", "@/file3.txt", "@/file3.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor3", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline3", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other3",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged3", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged3", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__CONTENTS, "value_other", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__CONTENTS, "value_other", NULL)
		RESOLVE_ACCEPT("item_file3", SG_RESOLVE__STATE__CONTENTS, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", "contents_other1")
		VERIFY_FILE("@/file2.txt", "contents_other2")
		VERIFY_FILE("@/file3.txt", "contents_other3")
TEST_END

// multiple divergent edits
// that conflict
// and accept the "working" value
TEST("divergent_edit__conflict_multiple__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, "contents_ancestor1", _content__append_numbered_lines, "4")
		SETUP_ADD_FILE("@/file2.txt", NULL, "contents_ancestor2", _content__append_numbered_lines, "6")
		SETUP_ADD_FILE("@/file3.txt", NULL, "contents_ancestor3", _content__append_numbered_lines, "8")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file1.txt", "contents_other1", _content__replace_first_line, "other1")
		SETUP_MODIFY_FILE("@/file2.txt", "contents_other2", _content__replace_first_line, "other2")
		SETUP_MODIFY_FILE("@/file3.txt", "contents_other3", _content__replace_first_line, "other3")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file1.txt", "contents_baseline1", _content__replace_first_line, "baseline1")
		SETUP_MODIFY_FILE("@/file2.txt", "contents_baseline2", _content__replace_first_line, "baseline2")
		SETUP_MODIFY_FILE("@/file3.txt", "contents_baseline3", _content__replace_first_line, "baseline3")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged1", "contents_ancestor1", "contents_baseline1", "contents_other1")
		SETUP_CONTENTS("contents_merged2", "contents_ancestor2", "contents_baseline2", "contents_other2")
		SETUP_CONTENTS("contents_merged3", "contents_ancestor3", "contents_baseline3", "contents_other3")
	TEST_EXPECT
		EXPECT_ITEM("item_file1", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file1.txt", "@/file1.txt", "@/file1.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor1", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline1", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other1",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged1", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged1", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file2", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file2.txt", "@/file2.txt", "@/file2.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor2", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline2", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other2",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged2", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged2", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
		EXPECT_ITEM("item_file3", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file3.txt", "@/file3.txt", "@/file3.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor3", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline3", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other3",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged3", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged3", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file1", SG_RESOLVE__STATE__CONTENTS, "value_working", NULL)
		RESOLVE_ACCEPT("item_file2", SG_RESOLVE__STATE__CONTENTS, "value_working", NULL)
		RESOLVE_ACCEPT("item_file3", SG_RESOLVE__STATE__CONTENTS, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", "contents_merged1")
		VERIFY_FILE("@/file2.txt", "contents_merged2")
		VERIFY_FILE("@/file3.txt", "contents_merged3")
TEST_END

// a divergent edit
// that successfully automerges
TEST("divergent_edit__automerge")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__append_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_TRUE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_OK, SG_FALSE, ACCEPTED_VALUE_AUTOMERGE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_FALSE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_FILETOOL__RESULT__SUCCESS, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_NONE
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_merged")
TEST_END

// a divergent edit
// that successfully automerged
// and is then edited
TEST_CONTINUE("divergent_edit__automerge__edit", "divergent_edit__automerge")
	TEST_SETUP
		SETUP_MODIFY_FILE("@/file.txt", "contents_edited", _content__append_line, "edit")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_TRUE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_OK, SG_FALSE, ACCEPTED_VALUE_AUTOMERGE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_FALSE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_FILETOOL__RESULT__SUCCESS, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_edited", SG_FALSE, SG_TRUE, "value_merged", SG_TRUE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_NONE
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_edited")
TEST_END

// a divergent edit
// of binary files
// that conflicts
// and accepts the "ancestor" value
TEST("divergent_edit__binary__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.bin", NULL, "contents_ancestor", _content__append_nulls, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.bin", "contents_other", _content__append_nulls, "3")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.bin", "contents_baseline", _content__append_nulls, "7")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.bin", "@/file.bin", "@/file.bin", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_baseline", SG_TRUE, SG_TRUE, "value_baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.bin", "contents_ancestor")
TEST_END

// a divergent edit
// of binary files
// that conflicts
// and accepts the "baseline" value
TEST("divergent_edit__binary__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.bin", NULL, "contents_ancestor", _content__append_nulls, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.bin", "contents_other", _content__append_nulls, "3")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.bin", "contents_baseline", _content__append_nulls, "7")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.bin", "@/file.bin", "@/file.bin", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_baseline", SG_TRUE, SG_TRUE, "value_baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.bin", "contents_baseline")
TEST_END

// a divergent edit
// of binary files
// that conflicts
// and accepts the "other" value
TEST("divergent_edit__binary__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.bin", NULL, "contents_ancestor", _content__append_nulls, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.bin", "contents_other", _content__append_nulls, "3")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.bin", "contents_baseline", _content__append_nulls, "7")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.bin", "@/file.bin", "@/file.bin", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_baseline", SG_TRUE, SG_TRUE, "value_baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.bin", "contents_other")
TEST_END

// a divergent edit
// of binary files
// that conflicts
// and accepts the "working" value
TEST("divergent_edit__binary__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.bin", NULL, "contents_ancestor", _content__append_nulls, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.bin", "contents_other", _content__append_nulls, "3")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.bin", "contents_baseline", _content__append_nulls, "7")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.bin", "@/file.bin", "@/file.bin", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_baseline", SG_TRUE, SG_TRUE, "value_baseline", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.bin", "contents_baseline")
TEST_END

// a divergent edit
// with conflicts
// that modifies the WD
// before accepting the "ancestor" value
TEST("divergent_edit__conflict_modify__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
		SETUP_MODIFY_FILE("@/file.txt", "contents_working", _content__append_line, "working line")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_working", SG_TRUE, SG_TRUE, "value_merged", SG_TRUE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", "value_working2")
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_ancestor")
TEST_END

// a divergent edit
// with conflicts
// that modifies the WD
// before accepting the "baseline" value
TEST("divergent_edit__conflict_modify__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
		SETUP_MODIFY_FILE("@/file.txt", "contents_working", _content__append_line, "working line")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_working", SG_TRUE, SG_TRUE, "value_merged", SG_TRUE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working2")
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_baseline")
TEST_END

// a divergent edit
// with conflicts
// that modifies the WD
// before accepting the "other" value
TEST("divergent_edit__conflict_modify__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
		SETUP_MODIFY_FILE("@/file.txt", "contents_working", _content__append_line, "working line")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_working", SG_TRUE, SG_TRUE, "value_merged", SG_TRUE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_other", "value_working2")
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_other")
TEST_END

// a divergent edit
// with conflicts
// that modifies the WD
// before accepting the "working" value
TEST("divergent_edit__conflict_modify__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
		SETUP_MODIFY_FILE("@/file.txt", "contents_working", _content__append_line, "working line")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_working", SG_TRUE, SG_TRUE, "value_merged", SG_TRUE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_working")
TEST_END

// a divergent edit
// with conflicts
// that modifies the WD
// before accepting the "merged" value
TEST("divergent_edit__conflict_modify__merged")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
		SETUP_MODIFY_FILE("@/file.txt", "contents_working", _content__append_line, "working line")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_working", SG_TRUE, SG_TRUE, "value_merged", SG_TRUE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merged", "value_working2")
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_merged")
TEST_END

// a divergent edit
// of binary files
// with conflicts
// that modifies the WD
// before accepting the "ancestor" value
TEST("divergent_edit__binary_modify__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.bin", NULL, "contents_ancestor", _content__append_nulls, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.bin", "contents_other", _content__append_nulls, "3")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.bin", "contents_baseline", _content__append_nulls, "7")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MODIFY_FILE("@/file.bin", "contents_working", _content__append_nulls, "1")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.bin", "@/file.bin", "@/file.bin", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_working", SG_TRUE, SG_TRUE, "value_baseline", SG_TRUE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", "value_working2")
	TEST_VERIFY
		VERIFY_FILE("@/file.bin", "contents_ancestor")
TEST_END

// a divergent edit
// of binary files
// with conflicts
// that modifies the WD
// before accepting the "baseline" value
TEST("divergent_edit__binary_modify__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.bin", NULL, "contents_ancestor", _content__append_nulls, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.bin", "contents_other", _content__append_nulls, "3")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.bin", "contents_baseline", _content__append_nulls, "7")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MODIFY_FILE("@/file.bin", "contents_working", _content__append_nulls, "1")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.bin", "@/file.bin", "@/file.bin", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_working", SG_TRUE, SG_TRUE, "value_baseline", SG_TRUE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working2")
	TEST_VERIFY
		VERIFY_FILE("@/file.bin", "contents_baseline")
TEST_END

// a divergent edit
// of binary files
// with conflicts
// that modifies the WD
// before accepting the "other" value
TEST("divergent_edit__binary_modify__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.bin", NULL, "contents_ancestor", _content__append_nulls, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.bin", "contents_other", _content__append_nulls, "3")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.bin", "contents_baseline", _content__append_nulls, "7")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MODIFY_FILE("@/file.bin", "contents_working", _content__append_nulls, "1")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.bin", "@/file.bin", "@/file.bin", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_working", SG_TRUE, SG_TRUE, "value_baseline", SG_TRUE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_other", "value_working2")
	TEST_VERIFY
		VERIFY_FILE("@/file.bin", "contents_other")
TEST_END

// a divergent edit
// of binary files
// with conflicts
// that modifies the WD
// before accepting the "working" value
TEST("divergent_edit__binary_modify__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.bin", NULL, "contents_ancestor", _content__append_nulls, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.bin", "contents_other", _content__append_nulls, "3")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.bin", "contents_baseline", _content__append_nulls, "7")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MODIFY_FILE("@/file.bin", "contents_working", _content__append_nulls, "1")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.bin", "@/file.bin", "@/file.bin", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_working", SG_TRUE, SG_TRUE, "value_baseline", SG_TRUE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.bin", "contents_working")
TEST_END

// a divergent edit
// that conflicts
// and accepts the "ancestor" value
// and then accepts the "baseline" value
TEST("divergent_edit__conflict__ancestor_baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_baseline")
TEST_END

// a divergent edit
// that conflicts
// and accepts the "ancestor" value
// and then accepts the "other" value
TEST("divergent_edit__conflict__ancestor_other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_other")
TEST_END

// a divergent edit
// that conflicts
// and accepts the "ancestor" value
// and then accepts the "working" value
TEST("divergent_edit__conflict__ancestor_working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_ancestor")
TEST_END

// a divergent edit
// that conflicts
// and accepts the "working" value
// and then accepts the "ancestor" value
TEST("divergent_edit__conflict__working_ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_working", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_ancestor")
TEST_END

// a divergent edit
// that conflicts
// and merges the working value
// before accepting the "ancestor" value
TEST("divergent_edit__working__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working", "value_merge2", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_merged", SG_FALSE, SG_FALSE, "value_working2")
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_ancestor")
TEST_END

// a divergent edit
// that conflicts
// and merges the working value
// before accepting the "baseline" value
TEST("divergent_edit__working__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working", "value_merge2", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_merged", SG_FALSE, SG_FALSE, "value_working2")
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_baseline")
TEST_END

// a divergent edit
// that conflicts
// and merges the working value
// before accepting the "other" value
TEST("divergent_edit__working__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working", "value_merge2", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_merged", SG_FALSE, SG_FALSE, "value_working2")
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_other")
TEST_END

// a divergent edit
// that conflicts
// and merges the working value
// before accepting the "working" value
TEST("divergent_edit__working__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working", "value_merge2", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_merged", SG_FALSE, SG_FALSE, "value_working2")
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_merged")
TEST_END

// a divergent edit
// that conflicts
// and merges the working value
// before accepting the "working2" value
TEST("divergent_edit__working__working2")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working", "value_merge2", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_merged", SG_FALSE, SG_FALSE, "value_working2")
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_working2", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_merged")
TEST_END

// a divergent edit
// that conflicts
// and merges the working value multiple times
// before accepting the "ancestor" value
TEST("divergent_edit__working_multiple__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_MODIFY_FILE("item_file", "value_working", "contents_working", _content__append_line, "working1")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working", "value_merge2", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working", SG_FALSE, SG_FALSE, "value_working2")
		RESOLVE_MODIFY_FILE("item_file", "value_working2", "contents_working2", _content__append_line, "working2")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge2", "value_working2", "value_merge3", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working2", SG_FALSE, SG_FALSE, "value_working3")
		RESOLVE_MODIFY_FILE("item_file", "value_working3", "contents_working3", _content__append_line, "working3")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge3", "value_working3", "value_merge4", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working3", SG_FALSE, SG_FALSE, "value_working4")
		RESOLVE_MODIFY_FILE("item_file", "value_working4", "contents_working4", _content__append_line, "working4")
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", "value_working5")
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_ancestor")
TEST_END

// a divergent edit
// that conflicts
// and merges the working value multiple times
// before accepting the "baseline" value
TEST("divergent_edit__working_multiple__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_MODIFY_FILE("item_file", "value_working", "contents_working", _content__append_line, "working1")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working", "value_merge2", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working", SG_FALSE, SG_FALSE, "value_working2")
		RESOLVE_MODIFY_FILE("item_file", "value_working2", "contents_working2", _content__append_line, "working2")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge2", "value_working2", "value_merge3", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working2", SG_FALSE, SG_FALSE, "value_working3")
		RESOLVE_MODIFY_FILE("item_file", "value_working3", "contents_working3", _content__append_line, "working3")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge3", "value_working3", "value_merge4", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working3", SG_FALSE, SG_FALSE, "value_working4")
		RESOLVE_MODIFY_FILE("item_file", "value_working4", "contents_working4", _content__append_line, "working4")
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working5")
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_baseline")
TEST_END

// a divergent edit
// that conflicts
// and merges the working value multiple times
// before accepting the "other" value
TEST("divergent_edit__working_multiple__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_MODIFY_FILE("item_file", "value_working", "contents_working", _content__append_line, "working1")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working", "value_merge2", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working", SG_FALSE, SG_FALSE, "value_working2")
		RESOLVE_MODIFY_FILE("item_file", "value_working2", "contents_working2", _content__append_line, "working2")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge2", "value_working2", "value_merge3", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working2", SG_FALSE, SG_FALSE, "value_working3")
		RESOLVE_MODIFY_FILE("item_file", "value_working3", "contents_working3", _content__append_line, "working3")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge3", "value_working3", "value_merge4", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working3", SG_FALSE, SG_FALSE, "value_working4")
		RESOLVE_MODIFY_FILE("item_file", "value_working4", "contents_working4", _content__append_line, "working4")
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_other", "value_working5")
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_other")
TEST_END

// a divergent edit
// that conflicts
// and merges the working value multiple times
// before accepting the "working" value
TEST("divergent_edit__working_multiple__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_MODIFY_FILE("item_file", "value_working", "contents_working", _content__append_line, "working1")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working", "value_merge2", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working", SG_FALSE, SG_FALSE, "value_working2")
		RESOLVE_MODIFY_FILE("item_file", "value_working2", "contents_working2", _content__append_line, "working2")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge2", "value_working2", "value_merge3", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working2", SG_FALSE, SG_FALSE, "value_working3")
		RESOLVE_MODIFY_FILE("item_file", "value_working3", "contents_working3", _content__append_line, "working3")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge3", "value_working3", "value_merge4", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working3", SG_FALSE, SG_FALSE, "value_working4")
		RESOLVE_MODIFY_FILE("item_file", "value_working4", "contents_working4", _content__append_line, "working4")
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_working", "value_working5")
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_working")
TEST_END

// a divergent edit
// that conflicts
// and merges the working value multiple times
// before accepting the "working2" value
TEST("divergent_edit__working_multiple__working2")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_MODIFY_FILE("item_file", "value_working", "contents_working", _content__append_line, "working1")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working", "value_merge2", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working", SG_FALSE, SG_FALSE, "value_working2")
		RESOLVE_MODIFY_FILE("item_file", "value_working2", "contents_working2", _content__append_line, "working2")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge2", "value_working2", "value_merge3", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working2", SG_FALSE, SG_FALSE, "value_working3")
		RESOLVE_MODIFY_FILE("item_file", "value_working3", "contents_working3", _content__append_line, "working3")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge3", "value_working3", "value_merge4", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working3", SG_FALSE, SG_FALSE, "value_working4")
		RESOLVE_MODIFY_FILE("item_file", "value_working4", "contents_working4", _content__append_line, "working4")
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_working2", "value_working5")
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_working2")
TEST_END

// a divergent edit
// that conflicts
// and merges the working value multiple times
// before accepting the "working3" value
TEST("divergent_edit__working_multiple__working3")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_MODIFY_FILE("item_file", "value_working", "contents_working", _content__append_line, "working1")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working", "value_merge2", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working", SG_FALSE, SG_FALSE, "value_working2")
		RESOLVE_MODIFY_FILE("item_file", "value_working2", "contents_working2", _content__append_line, "working2")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge2", "value_working2", "value_merge3", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working2", SG_FALSE, SG_FALSE, "value_working3")
		RESOLVE_MODIFY_FILE("item_file", "value_working3", "contents_working3", _content__append_line, "working3")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge3", "value_working3", "value_merge4", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working3", SG_FALSE, SG_FALSE, "value_working4")
		RESOLVE_MODIFY_FILE("item_file", "value_working4", "contents_working4", _content__append_line, "working4")
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_working3", "value_working5")
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_working3")
TEST_END

// a divergent edit
// that conflicts
// and merges the working value multiple times
// before accepting the "working4" value
TEST("divergent_edit__working_multiple__working4")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_MODIFY_FILE("item_file", "value_working", "contents_working", _content__append_line, "working1")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_baseline", "value_working", "value_merge2", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working", SG_FALSE, SG_FALSE, "value_working2")
		RESOLVE_MODIFY_FILE("item_file", "value_working2", "contents_working2", _content__append_line, "working2")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge2", "value_working2", "value_merge3", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working2", SG_FALSE, SG_FALSE, "value_working3")
		RESOLVE_MODIFY_FILE("item_file", "value_working3", "contents_working3", _content__append_line, "working3")
		RESOLVE_MERGE("item_file", SG_RESOLVE__STATE__CONTENTS, "value_merge3", "value_working3", "value_merge4", SG_FALSE, ":other", SG_FILETOOL__RESULT__SUCCESS, "contents_working3", SG_FALSE, SG_FALSE, "value_working4")
		RESOLVE_MODIFY_FILE("item_file", "value_working4", "contents_working4", _content__append_line, "working4")
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__CONTENTS, "value_working4", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", "contents_working4")
TEST_END

// a divergent edit
// that conflicts
// and resolves the file by deleting it
TEST("divergent_edit__simple__delete")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, "contents_ancestor", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file.txt", "contents_other", _content__replace_first_line, "other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file.txt", "contents_baseline", _content__replace_first_line, "baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_CONTENTS("contents_merged", "contents_ancestor", "contents_baseline", "contents_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file.txt", "@/file.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "contents_ancestor", "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "contents_baseline", "commit_baseline", SG_TRUE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "contents_other",    "commit_other",    SG_TRUE)
				EXPECT_MERGE_VALUE("value_merged", "contents_merged", "merge_1", "value_baseline", "value_other", SG_MERGETOOL__RESULT__CONFLICT, SG_FALSE)
				EXPECT_MERGEABLE_WORKING_VALUE__SZ("value_working", "contents_merged", SG_FALSE, SG_TRUE, "value_merged", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_DELETE("item_file")
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
TEST_END

#if defined(MAC) || defined(LINUX)

// a divergent edit
// of symlinks
// that conflicts
// and accepts the "ancestor" value
TEST("divergent_edit__symlink__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ("value_working", "@/file_baseline.txt", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file.txt")
TEST_END

// a divergent edit
// of symlinks
// that conflicts
// and accepts the "baseline" value
TEST("divergent_edit__symlink__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ("value_working", "@/file_baseline.txt", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__CONTENTS, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file_baseline.txt")
TEST_END

// a divergent edit
// of symlinks
// that conflicts
// and accepts the "other" value
TEST("divergent_edit__symlink__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ("value_working", "@/file_baseline.txt", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__CONTENTS, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file_other.txt")
TEST_END

// a divergent edit
// of symlinks
// that conflicts
// and accepts the "working" value
TEST("divergent_edit__symlink__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ("value_working", "@/file_baseline.txt", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__CONTENTS, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file_baseline.txt")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a divergent edit
// of symlinks
// that conflicts
// and modifies the WD
// before accepting the "ancestor" value
TEST("divergent_edit__symlink_modify__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_working.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MODIFY_SYMLINK("item_link", "@/file_working.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ("value_working", "@/file_working.txt", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__CONTENTS, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_FILE("@/file_working.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file.txt")
TEST_END

// a divergent edit
// of symlinks
// that conflicts
// and modifies the WD
// before accepting the "baseline" value
TEST("divergent_edit__symlink_modify__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_working.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MODIFY_SYMLINK("item_link", "@/file_working.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ("value_working", "@/file_working.txt", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__CONTENTS, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_FILE("@/file_working.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file_baseline.txt")
TEST_END

// a divergent edit
// of symlinks
// that conflicts
// and modifies the WD
// before accepting the "other" value
TEST("divergent_edit__symlink_modify__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_working.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MODIFY_SYMLINK("item_link", "@/file_working.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ("value_working", "@/file_working.txt", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__CONTENTS, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_FILE("@/file_working.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file_other.txt")
TEST_END

// a divergent edit
// of symlinks
// that conflicts
// and modifies the WD
// before accepting the "working" value
TEST("divergent_edit__symlink_modify__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_working.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_MODIFY_SYMLINK("item_link", "@/file_working.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ("value_working", "@/file_working.txt", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__CONTENTS, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_FILE("@/file_working.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file_working.txt")
TEST_END

// a divergent edit
// of symlinks
// that conflicts
// and resolves the file by deleting it
TEST("divergent_edit__symlink__delete")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_baseline.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file_other.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_SYMLINK("@/link", "@/file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_SYMLINK("@/link", "@/file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link", "@/link", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__CONTENTS, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "@/file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "@/file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "@/file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ("value_working", "@/file_baseline.txt", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_DELETE("item_link")
	TEST_VERIFY
		VERIFY_GONE("@/link")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

