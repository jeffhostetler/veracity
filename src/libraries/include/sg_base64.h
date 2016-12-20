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

#ifndef H_SG_BASE64_H
#define H_SG_BASE64_H

BEGIN_EXTERN_C

void SG_base64__space_needed_for_encode(
    SG_context* pCtx,
    SG_uint32 len_binary,
    SG_uint32* pi
    );

void SG_base64__space_needed_for_decode(
    SG_context* pCtx,
    const char* p_encoded,
    SG_uint32* pi
    );

void SG_base64__encode(
    SG_context* pCtx,
    const SG_byte* p_binary,
    SG_uint32 len_binary,
    char* p_encoded_buf,
    SG_uint32 len_encoded_buf
    );

void SG_base64__decode(
    SG_context* pCtx,
    const char* p_encoded,
    SG_byte* p_binary_buf,
    SG_uint32 len_binary_buf,
    SG_uint32* p_actual_len_binary
    );

END_EXTERN_C

#endif//H_SG_BASE64_H

