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

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VV2__JSGLUE__PUBLIC_PROTOTYPES_H
#define H_SG_VV2__JSGLUE__PUBLIC_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Public routine to install all of the various "vv2"
 * APIs/Classes/Methods into JavaScript.
 *
 */
void SG_vv2__jsglue__install_scripting_api(SG_context * pCtx,
										   JSContext * cx,
										   JSObject * glob,
										   JSObject * pjsobj_sg);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV2__JSGLUE__PUBLIC_PROTOTYPES_H
