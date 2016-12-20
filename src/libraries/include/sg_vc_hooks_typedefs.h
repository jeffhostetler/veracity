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

#ifndef H_SG_VC_HOOKS_TYPEDEFS_H
#define H_SG_VC_HOOKS_TYPEDEFS_H

BEGIN_EXTERN_C;

#define SG_VC_HOOK__INTERFACE__BROADCAST__AFTER_COMMIT "broadcast__after_commit"

#define SG_VC_HOOK__INTERFACE__ASK__WIT__GET_TERMINOLOGY "ask__wit__get_terminology"
#define SG_VC_HOOK__INTERFACE__ASK__WIT__ADD_COMMENT "ask__wit__add_comment"
#define SG_VC_HOOK__INTERFACE__ASK__WIT__ADD_ASSOCIATIONS "ask__wit__add_associations"
#define SG_VC_HOOK__INTERFACE__ASK__WIT__VALIDATE_ASSOCIATIONS "ask__wit__validate_associations"
#define SG_VC_HOOK__INTERFACE__ASK__WIT__CHANGE_STATE "ask__wit__change_state"
#define SG_VC_HOOK__INTERFACE__ASK__WIT__LIST_ITEMS "ask__wit__list_items"
#define SG_VC_HOOK__INTERFACE__ASK__WIT__LIST_VALID_STATES "ask__wit__list_valid_states"
#define SG_VC_HOOK__INTERFACE__ASK__WIT__SUPPORTS_TIME "ask__wit__supports_time"
#define SG_VC_HOOK__INTERFACE__ASK__WIT__ADD_TIME "ask__wit__add_time"

END_EXTERN_C;

#endif//H_SG_VC_HOOKS_TYPEDEFS_H

