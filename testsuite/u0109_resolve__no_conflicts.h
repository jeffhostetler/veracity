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
 * This file contains tests that don't setup any conflicts.
 */

// a delete test that results in no conflicts
TEST("no_conflicts__delete")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_DELETE("@/file2.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_DELETE("@/file1.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_NONE
	TEST_RESOLVE
		RESOLVE_NONE
	TEST_VERIFY
		VERIFY_GONE("@/file1.txt")
		VERIFY_GONE("@/file2.txt")
TEST_END

// a move test that results in no conflicts
TEST("no_conflicts__move")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FOLDER("@/folder1", NULL)
		SETUP_ADD_FOLDER("@/folder2", NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_MOVE("@/file2.txt", "@/folder2")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MOVE("@/file1.txt", "@/folder1")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_NONE
	TEST_RESOLVE
		RESOLVE_NONE
	TEST_VERIFY
		VERIFY_GONE("@/file1.txt")
		VERIFY_GONE("@/file2.txt")
		VERIFY_FILE("@/folder1/file1.txt", NULL)
		VERIFY_FILE("@/folder2/file2.txt", NULL)
TEST_END

// a rename test that results in no conflicts
TEST("no_conflicts__rename")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, NULL, NULL)
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, NULL, NULL)
		SETUP_COMMIT("commit_ancestor")

		SETUP_RENAME("@/file2.txt", "file2_renamed.txt")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_RENAME("@/file1.txt", "file1_renamed.txt")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_NONE
	TEST_RESOLVE
		RESOLVE_NONE
	TEST_VERIFY
		VERIFY_GONE("@/file1.txt")
		VERIFY_GONE("@/file2.txt")
		VERIFY_FILE("@/file1_renamed.txt", NULL)
		VERIFY_FILE("@/file2_renamed.txt", NULL)
TEST_END

// a modify test that results in no conflicts
TEST("no_conflicts__modify")
	TEST_SETUP
		SETUP_ADD_FILE("@/file1.txt", NULL, NULL, _content__append_numbered_lines, "10")
		SETUP_ADD_FILE("@/file2.txt", NULL, NULL, _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_ancestor")

		SETUP_MODIFY_FILE("@/file2.txt", "content_file2_other", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_other")

		SETUP_UPDATE("commit_ancestor")
		SETUP_MODIFY_FILE("@/file1.txt", "content_file1_baseline", _content__append_numbered_lines, "10")
		SETUP_COMMIT("commit_baseline")

		SETUP_MERGE("merge_1", "commit_other")
	TEST_EXPECT
		EXPECT_NONE
	TEST_RESOLVE
		RESOLVE_NONE
	TEST_VERIFY
		VERIFY_FILE("@/file1.txt", "content_file1_baseline")
		VERIFY_FILE("@/file2.txt", "content_file2_other")
TEST_END
