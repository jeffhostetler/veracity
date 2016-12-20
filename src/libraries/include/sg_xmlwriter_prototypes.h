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

#ifndef H_SG_XMLWRITER_PROTOTYPES_H
#define H_SG_XMLWRITER_PROTOTYPES_H

BEGIN_EXTERN_C

#define SG_XMLWRITER_NULLFREE(c,w) SG_STATEMENT( if (w) { SG_xmlwriter__free((c),(w)); w = NULL; } )
void SG_xmlwriter__alloc(SG_context* pCtx, SG_xmlwriter** ppResult, SG_string* pDest, SG_bool bFormatted);
void SG_xmlwriter__free(SG_context* pCtx, SG_xmlwriter*);

void SG_xmlwriter__write_start_document(SG_context* pCtx, SG_xmlwriter* pxml);
void SG_xmlwriter__write_end_document(SG_context* pCtx, SG_xmlwriter* pxml);

void SG_xmlwriter__write_start_element__sz(SG_context* pCtx, SG_xmlwriter* pxml, const char* putf8Name);
void SG_xmlwriter__write_content__sz(SG_context* pCtx, SG_xmlwriter* pxml, const char* putf8Content);
void SG_xmlwriter__write_content__buflen(SG_context* pCtx, SG_xmlwriter* pxml, const char* putf8Text, SG_uint32 len_bytes);
void SG_xmlwriter__write_end_element(SG_context* pCtx, SG_xmlwriter* pxml);
void SG_xmlwriter__write_full_end_element(SG_context* pCtx, SG_xmlwriter* pxml);
void SG_xmlwriter__write_element__sz(SG_context* pCtx, SG_xmlwriter* pxml, const char* putf8Name, const char* putf8Content);

void SG_xmlwriter__write_attribute__sz(SG_context* pCtx, SG_xmlwriter* pxml, const char* putf8Name, const char* putf8Value);

void SG_xmlwriter__write_comment__sz(SG_context* pCtx, SG_xmlwriter* pxml, const char* putf8Text);

END_EXTERN_C

#endif

