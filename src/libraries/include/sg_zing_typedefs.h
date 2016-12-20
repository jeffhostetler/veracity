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
 * @file sg_dbmanager_typedefs.h
 *
 */

#ifndef H_SG_ZING_TYPEDEFS_H
#define H_SG_ZING_TYPEDEFS_H

BEGIN_EXTERN_C;

typedef struct SG_zingrecord SG_zingrecord;
typedef struct SG_zingtx SG_zingtx;
typedef struct SG_zingtemplate SG_zingtemplate;
typedef struct SG_zingfieldattributes SG_zingfieldattributes;
typedef struct SG_zingrectypeinfo SG_zingrectypeinfo;

#define SG_ZING_MAX_FIELD_NAME_LENGTH   63

#define SG_ZING_TYPE__UNKNOWN       0
#define SG_ZING_TYPE__INT           1
#define SG_ZING_TYPE__STRING        2
#define SG_ZING_TYPE__BOOL          3
#define SG_ZING_TYPE__ATTACHMENT    4
#define SG_ZING_TYPE__DATETIME      5
#define SG_ZING_TYPE__USERID        6
#define SG_ZING_TYPE__REFERENCE     7

#define SG_ZING_TYPE_NAME__INT           "int"
#define SG_ZING_TYPE_NAME__STRING        "string"
#define SG_ZING_TYPE_NAME__BOOL          "bool"
#define SG_ZING_TYPE_NAME__ATTACHMENT    "attachment"
#define SG_ZING_TYPE_NAME__DATETIME      "datetime"
#define SG_ZING_TYPE_NAME__USERID        "userid"
#define SG_ZING_TYPE_NAME__REFERENCE     "reference"

#define SG_ZING_FIELD__RECID        "recid"
#define SG_ZING_FIELD__HIDREC       "hidrec"
#define SG_ZING_FIELD__RECTYPE      "rectype"
#define SG_ZING_FIELD__HISTORY      "history"
#define SG_ZING_FIELD__LAST_TIMESTAMP      "last_timestamp"

#define SG_ZING_FIELD__GIMME_ALL_FIELDS       "*"

END_EXTERN_C;

#endif //H_SG_ZING_TYPEDEFS_H

