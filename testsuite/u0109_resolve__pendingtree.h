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
 * This file contains tests related to resolve's pendingtree data.
 */

// a test that commits a resolved conflict
// and ensures that no saved choice data remains in pendingtree
TEST_CONTINUE("pendingtree__data_reset__commit", "delete_vs_edit__simple__baseline")
	TEST_SETUP
		SETUP_COMMIT("commit_merge")
	TEST_EXPECT
		EXPECT_NONE
	TEST_RESOLVE
		RESOLVE_NONE
	TEST_VERIFY
		VERIFY_CHOICES(SG_FALSE)
TEST_END

// a test that reverts a resolve conflict
// and ensures that no saved choice data remains in pendingtree
TEST_CONTINUE("pendingtree__data_reset__revert", "delete_vs_edit__simple__other")
	TEST_SETUP
		SETUP_REVERT_ALL
	TEST_EXPECT
		EXPECT_NONE
	TEST_RESOLVE
		RESOLVE_NONE
	TEST_VERIFY
		VERIFY_CHOICES(SG_FALSE)
TEST_END
