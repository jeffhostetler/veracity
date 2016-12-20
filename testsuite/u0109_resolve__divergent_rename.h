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
 * This file contains tests that setup divergent rename conflicts.
 */

// a simple divergent rename
// that accepts the "ancestor" value
TEST("divergent_rename__simple__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file.txt", "file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file.txt", "file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file_baseline.txt", "@/file_other.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file.txt",                "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file_baseline.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file_other.txt",          "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "file_baseline.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/file_baseline.txt")
		VERIFY_GONE("@/file_other.txt")
		VERIFY_TEMP_GONE("@/file_baseline.txt~L0~g~", "item_file")
TEST_END

// a simple divergent rename
// that accepts the "baseline" value
TEST("divergent_rename__simple__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file.txt", "file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file.txt", "file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file_baseline.txt", "@/file_other.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file.txt",                "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file_baseline.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file_other.txt",          "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "file_baseline.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_GONE("@/file_other.txt")
		VERIFY_TEMP_GONE("@/file_baseline.txt~L0~g~", "item_file")
TEST_END

// a simple divergent rename
// that accepts the "other" value
TEST("divergent_rename__simple__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file.txt", "file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file.txt", "file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file_baseline.txt", "@/file_other.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file.txt",                "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file_baseline.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file_other.txt",          "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "file_baseline.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/file_baseline.txt")
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_TEMP_GONE("@/file_baseline.txt~L0~g~", "item_file")
TEST_END

// a simple divergent rename
// that accepts the "working" value
TEST("divergent_rename__simple__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file.txt", "file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file.txt", "file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file_baseline.txt", "@/file_other.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file.txt",                "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file_baseline.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file_other.txt",          "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "file_baseline.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/file_baseline.txt")
		VERIFY_GONE("@/file_other.txt")
		VERIFY_TEMP_FILE("@/file_baseline.txt~L0~g~", "item_file", NULL)
TEST_END

// a divergent rename
// that modifies the WD
// before accepting the "ancestor" value
TEST("divergent_rename__modify__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file.txt", "file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file.txt", "file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file", "file_working.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file_baseline.txt", "@/file_other.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "file_working.txt",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/file_baseline.txt")
		VERIFY_GONE("@/file_other.txt")
		VERIFY_GONE("@/file_working.txt")
		VERIFY_TEMP_GONE("@/file_baseline.txt~L0~g~", "item_file")
TEST_END

// a divergent rename
// that modifies the WD
// before accepting the "baseline" value
TEST("divergent_rename__modify__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file.txt", "file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file.txt", "file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file", "file_working.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file_baseline.txt", "@/file_other.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "file_working.txt",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_GONE("@/file_other.txt")
		VERIFY_GONE("@/file_working.txt")
		VERIFY_TEMP_GONE("@/file_baseline.txt~L0~g~", "item_file")
TEST_END

// a divergent rename
// that modifies the WD
// before accepting the "other" value
TEST("divergent_rename__modify__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file.txt", "file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file.txt", "file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file", "file_working.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file_baseline.txt", "@/file_other.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "file_working.txt",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/file_baseline.txt")
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_GONE("@/file_working.txt")
		VERIFY_TEMP_GONE("@/file_baseline.txt~L0~g~", "item_file")
TEST_END

// a divergent rename
// that modifies the WD
// before accepting the "working" value
TEST("divergent_rename__modify__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", "item_file", NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file.txt", "file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file.txt", "file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_file", "file_working.txt")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file_baseline.txt", "@/file_other.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file.txt",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file_baseline.txt", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file_other.txt",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "file_working.txt",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/file_baseline.txt")
		VERIFY_GONE("@/file_other.txt")
		VERIFY_FILE("@/file_working.txt", NULL)
		VERIFY_TEMP_GONE("@/file_baseline.txt~L0~g~", "item_file")
TEST_END

// a simple divergent rename
// that accepts the "ancestor" value
// and then accepts the "baseline" value
TEST("divergent_rename__simple__ancestor_baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file.txt", "file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file.txt", "file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file_baseline.txt", "@/file_other.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file.txt",                "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file_baseline.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file_other.txt",          "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "file_baseline.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_FILE("@/file_baseline.txt", NULL)
		VERIFY_GONE("@/file_other.txt")
		VERIFY_TEMP_GONE("@/file_baseline.txt~L0~g~", "item_file")
TEST_END

// a simple divergent rename
// that accepts the "ancestor" value
// and then accepts the "other" value
TEST("divergent_rename__simple__ancestor_other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file.txt", "file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file.txt", "file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file_baseline.txt", "@/file_other.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file.txt",                "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file_baseline.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file_other.txt",          "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "file_baseline.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_GONE("@/file.txt")
		VERIFY_GONE("@/file_baseline.txt")
		VERIFY_FILE("@/file_other.txt", NULL)
		VERIFY_TEMP_GONE("@/file_baseline.txt~L0~g~", "item_file")
TEST_END

// a simple divergent rename
// that accepts the "ancestor" value
// and then accepts the "working" value
TEST("divergent_rename__simple__ancestor_working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file.txt", "file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file.txt", "file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file_baseline.txt", "@/file_other.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file.txt",                "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file_baseline.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file_other.txt",          "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "file_baseline.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/file_baseline.txt")
		VERIFY_GONE("@/file_other.txt")
		VERIFY_TEMP_GONE("@/file_baseline.txt~L0~g~", "item_file")
TEST_END

// a simple divergent rename
// that accepts the "working" value
// and then accepts the "ancestor" value
TEST("divergent_rename__simple__working_ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file.txt", "file_other.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file.txt", "file_baseline.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_file", SG_TREENODEENTRY_TYPE_REGULAR_FILE, "@/file.txt", "@/file_baseline.txt", "@/file_other.txt", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "file.txt",                "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "file_baseline.txt",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "file_other.txt",          "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "file_baseline.txt~L0~g~", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_working", NULL)
		RESOLVE_ACCEPT("item_file", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/file_baseline.txt")
		VERIFY_GONE("@/file_other.txt")
		VERIFY_TEMP_GONE("@/file_baseline.txt~L0~g~", "item_file")
TEST_END

#if defined(MAC) || defined(LINUX)

// a simple divergent rename
// of a symlink
// that accepts the "ancestor" value
TEST("divergent_rename__symlink__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link", "link_other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link", "link_baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link_baseline", "@/link_other", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link",                "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link_baseline",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link_other",          "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "link_baseline~L0~g~", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file.txt")
		VERIFY_GONE("@/link_baseline")
		VERIFY_GONE("@/link_other")
		VERIFY_TEMP_GONE("@/link_baseline~L0~g~", "item_link")
TEST_END

// a simple divergent rename
// of a symlink
// that accepts the "baseline" value
TEST("divergent_rename__symlink__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link", "link_other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link", "link_baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link_baseline", "@/link_other", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link",                "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link_baseline",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link_other",          "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "link_baseline~L0~g~", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_SYMLINK("@/link_baseline", "@/file.txt")
		VERIFY_GONE("@/link_other")
		VERIFY_TEMP_GONE("@/link_baseline~L0~g~", "item_link")
TEST_END

// a simple divergent rename
// of a symlink
// that accepts the "other" value
TEST("divergent_rename__symlink__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link", "link_other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link", "link_baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link_baseline", "@/link_other", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link",                "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link_baseline",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link_other",          "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "link_baseline~L0~g~", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_GONE("@/link_baseline")
		VERIFY_SYMLINK("@/link_other", "@/file.txt")
		VERIFY_TEMP_GONE("@/link_baseline~L0~g~", "item_link")
TEST_END

// a simple divergent rename
// of a symlink
// that accepts the "working" value
TEST("divergent_rename__symlink__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", NULL, "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link", "link_other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link", "link_baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link_baseline", "@/link_other", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link",                "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link_baseline",       "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link_other",          "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "link_baseline~L0~g~", SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_GONE("@/link_baseline")
		VERIFY_GONE("@/link_other")
		VERIFY_TEMP_SYMLINK("@/link_baseline~L0~g~", "item_link", "@/file.txt")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

#if defined(MAC) || defined(LINUX)

// a divergent rename
// of a symlink
// that modifies the WD
// before accepting the "ancestor" value
TEST("divergent_rename__symlink_modify__ancestor")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link", "link_other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link", "link_baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link", "link_working")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link_baseline", "@/link_other", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link_other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "link_working",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__NAME, "value_ancestor", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_SYMLINK("@/link", "@/file.txt")
		VERIFY_GONE("@/link_baseline")
		VERIFY_GONE("@/link_other")
		VERIFY_GONE("@/link_working")
		VERIFY_TEMP_GONE("@/link_baseline~L0~g~", "item_link")
TEST_END

// a divergent rename
// of a symlink
// that modifies the WD
// before accepting the "baseline" value
TEST("divergent_rename__symlink_modify__baseline")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link", "link_other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link", "link_baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link", "link_working")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link_baseline", "@/link_other", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link_other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "link_working",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__NAME, "value_baseline", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_SYMLINK("@/link_baseline", "@/file.txt")
		VERIFY_GONE("@/link_other")
		VERIFY_GONE("@/link_working")
		VERIFY_TEMP_GONE("@/link_baseline~L0~g~", "item_link")
TEST_END

// a divergent rename
// of a symlink
// that modifies the WD
// before accepting the "other" value
TEST("divergent_rename__symlink_modify__other")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link", "link_other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link", "link_baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link", "link_working")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link_baseline", "@/link_other", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link_other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "link_working",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__NAME, "value_other", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_GONE("@/link_baseline")
		VERIFY_SYMLINK("@/link_other", "@/file.txt")
		VERIFY_GONE("@/link_working")
		VERIFY_TEMP_GONE("@/link_baseline~L0~g~", "item_link")
TEST_END

// a divergent rename
// of a symlink
// that modifies the WD
// before accepting the "working" value
TEST("divergent_rename__symlink_modify__working")
	TEST_SETUP
		SETUP_ADD_FILE("@/file.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_SYMLINK("@/link", "item_link", "@/file.txt")
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/link", "link_other")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/link", "link_baseline")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
		SETUP_RENAME("item_link", "link_working")
	TEST_EXPECT
		EXPECT_ITEM("item_link", SG_TREENODEENTRY_TYPE_SYMLINK, "@/link", "@/link_baseline", "@/link_other", SG_FALSE)
			EXPECT_CHOICE(SG_RESOLVE__STATE__NAME, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME, SG_FALSE, ACCEPTED_VALUE_NONE)
				EXPECT_CHANGESET_VALUE__SZ("value_ancestor", "link",          "commit_ancestor", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_baseline", "link_baseline", "commit_baseline", SG_FALSE)
				EXPECT_CHANGESET_VALUE__SZ("value_other",    "link_other",    "commit_other",    SG_FALSE)
				EXPECT_WORKING_VALUE__SZ(  "value_working",  "link_working",  SG_FALSE)
				EXPECT_RELATED_NONE
			EXPECT_CHOICE_END
		EXPECT_ITEM_END
	TEST_RESOLVE
		RESOLVE_ACCEPT("item_link", SG_RESOLVE__STATE__NAME, "value_working", NULL)
	TEST_VERIFY
		VERIFY_FILE("@/file.txt", NULL)
		VERIFY_GONE("@/link")
		VERIFY_GONE("@/link_baseline")
		VERIFY_GONE("@/link_other")
		VERIFY_SYMLINK("@/link_working", "@/file.txt")
		VERIFY_TEMP_GONE("@/link_baseline~L0~g~", "item_link")
TEST_END

#endif // #if defined(MAC) || defined(LINUX)

