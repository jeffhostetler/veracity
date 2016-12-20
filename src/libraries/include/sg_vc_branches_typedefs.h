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
 * @file sg_vc_branches_typedefs.h
 *
 */

#ifndef H_SG_VC_BRANCHES_TYPEDEFS_H
#define H_SG_VC_BRANCHES_TYPEDEFS_H

BEGIN_EXTERN_C;

#define SG_VC_BRANCHES__DEFAULT "master"

enum _SG_vc_branches__check_attach_name__flags
{
	SG_VC_BRANCHES__CHECK_ATTACH_NAME__DONT_CARE       = 0,
	SG_VC_BRANCHES__CHECK_ATTACH_NAME__MUST_EXIST      = 1,
	SG_VC_BRANCHES__CHECK_ATTACH_NAME__MUST_NOT_EXIST  = 2,
};

typedef enum _SG_vc_branches__check_attach_name__flags SG_vc_branches__check_attach_name__flags;

END_EXTERN_C;

#endif //H_SG_VC_BRANCHES_TYPEDEFS_H

