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
 *
 * @file sg_sync_typedefs.h
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_SYNC_TYPEDEFS_H
#define H_SG_SYNC_TYPEDEFS_H

BEGIN_EXTERN_C;

#define SG_SYNC_REPO_INFO_KEY__PROTOCOL_VERSION "ver"
#define SG_SYNC_REPO_INFO_KEY__REPO_ID			"RepoID"
#define SG_SYNC_REPO_INFO_KEY__ADMIN_ID			"AdminID"
#define SG_SYNC_REPO_INFO_KEY__HASH_METHOD		"HashMethod"

#define SG_SYNC_REPO_INFO_KEY__ACCOUNT			"account"
#define SG_SYNC_REPO_INFO_KEY__ACCOUNT_STATUS	"status"
#define SG_SYNC_REPO_INFO_KEY__ACCOUNT_MSG		"msg"

#define SG_SYNC_STATUS_KEY__CLONE				"clone"
#define SG_SYNC_STATUS_KEY__SINCE				"since"
#define SG_SYNC_STATUS_KEY__DAGS				"dags"
#define SG_SYNC_STATUS_KEY__BLOBS				"blobs"
#define SG_SYNC_STATUS_KEY__NEW_NODES			"new-nodes"
#define SG_SYNC_STATUS_KEY__LEAVES				"leaves"
#define SG_SYNC_STATUS_KEY__CLONE_REQUEST		"clone_request"

#define SG_SYNC_STATUS_KEY__COUNTS				"counts"
#define SG_SYNC_STATUS_KEY__BLOBS_REFERENCED	"blobs-referenced"
#define SG_SYNC_STATUS_KEY__BLOBS_PRESENT		"blobs-present"
#define SG_SYNC_STATUS_KEY__ROUNDTRIPS			"roundtrips"

#define SG_SYNC_REQUEST_VALUE_TAG				"tag"
#define SG_SYNC_REQUEST_VALUE_HID_PREFIX		"prefix"

#define SG_SYNC_DESCRIPTOR_KEY__CLONE_ID		"CloneID"
#define SG_SYNC_CLONE_TO_REMOTE_KEY__STATUS		"status"

END_EXTERN_C;

#endif//H_SG_SYNC_TYPEDEFS_H

