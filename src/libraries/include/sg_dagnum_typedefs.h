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
 * @file sg_dagnum_typedefs.h
 *
 */

#ifndef H_SG_DAGNUM_TYPEDEFS_H
#define H_SG_DAGNUM_TYPEDEFS_H

BEGIN_EXTERN_C;

#define SG_DAGNUM__BUF_MAX__HEX  (17)

// 4 bits for the type of the dag.  that is 16 possible types.
// we currently use 2 of them (db and tree).  and we reserve
// dag type 0 to mean a dag that contains nothing
// which is probably only useful for testing.
#define sg_DAGNUM__TYPE__SHIFT   (0)
#define sg_DAGNUM__TYPE__NUM     (4)

// 8 bits for flags.  we currently use 5 of them.
#define sg_DAGNUM__FLAGS__SHIFT  (4)
#define sg_DAGNUM__FLAGS__NUM    (8)

// 8 bits for the dagid.  that is 256 possible dags in each
// area.
#define sg_DAGNUM__DAGID__SHIFT  (12)
#define sg_DAGNUM__DAGID__NUM    (8)

// the area ID is 28 bits long, and it is divided into two
// ranges, the vendor, and the grouping.  combined, these
// two form the area ID.
//
// the vendor is 20 bits, for over a million possible 
// organizations developing Veracity applications.  I
// really hope we run out of these.  :-)  SourceGear's
// vendor ID is 1.
//
// the grouping is 8 bits, meaning that each vendor can have
// up to 256 areas.
#define sg_DAGNUM__GROUPING__SHIFT  (20)
#define sg_DAGNUM__GROUPING__NUM    (8)

#define sg_DAGNUM__VENDOR__SHIFT (28)
#define sg_DAGNUM__VENDOR__NUM   (20)

// 16 bits remaining unused

#define sg_DAGNUM__AREA__SHIFT   (sg_DAGNUM__GROUPING__SHIFT)
#define sg_DAGNUM__AREA__NUM     (sg_DAGNUM__GROUPING__NUM + sg_DAGNUM__VENDOR__NUM)

// helper macros for manipulating regions of bits
#define sg_DAGNUM__MAKE_MASK(num_bits) ((((SG_uint64) 1) << (num_bits+1)) - 1)
#define sg_GET_REGION(x, num_bits, shift) (((SG_uint64) (x)) >> shift) & sg_DAGNUM__MAKE_MASK(num_bits)
#define sg_SET_REGION(x, val, num_bits, shift) (((SG_uint64) (x)) | (((val) & sg_DAGNUM__MAKE_MASK(num_bits)) << shift))

// higher level macros for manipulating the parts of a dagnum
#define SG_DAGNUM__SET_TYPE(t, x) sg_SET_REGION(x, t, sg_DAGNUM__TYPE__NUM, sg_DAGNUM__TYPE__SHIFT)
#define SG_DAGNUM__SET_DAGID(t, x) sg_SET_REGION(x, t, sg_DAGNUM__DAGID__NUM, sg_DAGNUM__DAGID__SHIFT)
#define SG_DAGNUM__SET_GROUPING(t, x) sg_SET_REGION(x, t, sg_DAGNUM__GROUPING__NUM, sg_DAGNUM__GROUPING__SHIFT)
#define SG_DAGNUM__SET_VENDOR(t, x) sg_SET_REGION(x, t, sg_DAGNUM__VENDOR__NUM, sg_DAGNUM__VENDOR__SHIFT)
#define SG_DAGNUM__SET_FLAG(f, x) ((f) | (x))

// flags
#define SG_DAGNUM__FLAG__OLDAUDIT            (((SG_uint64) 1) << (0 + sg_DAGNUM__FLAGS__SHIFT))
#define SG_DAGNUM__FLAG__ADMIN               (((SG_uint64) 1) << (1 + sg_DAGNUM__FLAGS__SHIFT))
#define SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE  (((SG_uint64) 1) << (2 + sg_DAGNUM__FLAGS__SHIFT))
#define SG_DAGNUM__FLAG__SINGLE_RECTYPE      (((SG_uint64) 1) << (3 + sg_DAGNUM__FLAGS__SHIFT))
#define SG_DAGNUM__FLAG__TRIVIAL_MERGE       (((SG_uint64) 1) << (4 + sg_DAGNUM__FLAGS__SHIFT))

// higher level macros for getting the parts of a dagnum
#define SG_DAGNUM__GET_TYPE(x) (sg_GET_REGION(x, sg_DAGNUM__TYPE__NUM, sg_DAGNUM__TYPE__SHIFT))
#define SG_DAGNUM__GET_AREA_ID(x) (sg_GET_REGION(x, sg_DAGNUM__AREA__NUM, sg_DAGNUM__AREA__SHIFT))

// definitions of the parts of our dagnums
#define SG_VENDOR__SOURCEGEAR (1)

#define SG_GROUPING__SOURCEGEAR__CORE (1)
#define SG_GROUPING__SOURCEGEAR__VERSION_CONTROL (2)
//#define SG_GROUPING__SOURCEGEAR__SCRUM (3) // TODO remove this
#define SG_GROUPING__SOURCEGEAR__TESTING (4) // TODO remove this

#define sg_DAGNUM__TYPE__NOTHING        (0)
#define sg_DAGNUM__TYPE__TREE           (1)
#define sg_DAGNUM__TYPE__DB             (2)

// macros for querying flags and capabilities of a dagnum

#define SG_DAGNUM__IS_OLDAUDIT(x)    ((x) & SG_DAGNUM__FLAG__OLDAUDIT) 

#define SG_DAGNUM__IS_NOTHING(x) (sg_DAGNUM__TYPE__NOTHING == SG_DAGNUM__GET_TYPE(x))
#define SG_DAGNUM__IS_TREE(x) ( (sg_DAGNUM__TYPE__TREE == SG_DAGNUM__GET_TYPE(x)) )
#define SG_DAGNUM__IS_DB(x) ( (sg_DAGNUM__TYPE__DB == SG_DAGNUM__GET_TYPE(x)) )

#define SG_DAGNUM__IS_ADMIN(x)       ((x) & SG_DAGNUM__FLAG__ADMIN) 

#define SG_DAGNUM__USE_TRIVIAL_MERGE(x) ( ((x) & SG_DAGNUM__FLAG__TRIVIAL_MERGE) )

#define SG_DAGNUM__HAS_SINGLE_RECTYPE(x) ( ((x) & SG_DAGNUM__FLAG__SINGLE_RECTYPE) )

#define SG_DAGNUM__HAS_NO_RECID(x) ( SG_DAGNUM__USE_TRIVIAL_MERGE(x) )

#define SG_DAGNUM__ALLOWS_UNIQUE_CONSTRAINTS(x) ( !SG_DAGNUM__USE_TRIVIAL_MERGE(x) )

#define SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(x) ( ((x) & SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE) )

// definitions of built-in dagnums
#define SG_DAGNUM__NONE              (  0x0000  )

#define SG_DAGNUM__VERSION_CONTROL   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__VERSION_CONTROL, \
        SG_DAGNUM__SET_DAGID(1, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__TREE, \
        ((SG_uint64) 0)))))

#define SG_DAGNUM__VC_COMMENTS   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__VERSION_CONTROL, \
        SG_DAGNUM__SET_DAGID(2, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__SINGLE_RECTYPE, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__TRIVIAL_MERGE, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE, \
        ((SG_uint64) 0))))))))

#define SG_DAGNUM__VC_STAMPS   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__VERSION_CONTROL, \
        SG_DAGNUM__SET_DAGID(3, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__SINGLE_RECTYPE, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__TRIVIAL_MERGE, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE, \
        ((SG_uint64) 0))))))))

#define SG_DAGNUM__VC_TAGS   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__VERSION_CONTROL, \
        SG_DAGNUM__SET_DAGID(4, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__SINGLE_RECTYPE, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE, \
        ((SG_uint64) 0)))))))

#define SG_DAGNUM__VC_BRANCHES   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__VERSION_CONTROL, \
        SG_DAGNUM__SET_DAGID(5, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__TRIVIAL_MERGE, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE, \
        ((SG_uint64) 0)))))))

#define SG_DAGNUM__VC_LOCKS   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__VERSION_CONTROL, \
        SG_DAGNUM__SET_DAGID(6, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__SINGLE_RECTYPE, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE, \
        ((SG_uint64) 0)))))))

#define SG_DAGNUM__OLD__VC_HOOKS   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__VERSION_CONTROL, \
        SG_DAGNUM__SET_DAGID(7, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__SINGLE_RECTYPE, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__TRIVIAL_MERGE, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE, \
        ((SG_uint64) 0))))))))

#define SG_DAGNUM__VC_HOOKS   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__VERSION_CONTROL, \
        SG_DAGNUM__SET_DAGID(8, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__SINGLE_RECTYPE, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__TRIVIAL_MERGE, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE, \
        ((SG_uint64) 0))))))))

#define SG_DAGNUM__AREAS   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__CORE, \
        SG_DAGNUM__SET_DAGID(1, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE, \
        ((SG_uint64) 0))))))

#define SG_DAGNUM__USERS   \
        SG_DAGNUM__SET_VENDOR(SG_VENDOR__SOURCEGEAR, \
        SG_DAGNUM__SET_GROUPING(SG_GROUPING__SOURCEGEAR__CORE, \
        SG_DAGNUM__SET_DAGID(2, \
        SG_DAGNUM__SET_TYPE(sg_DAGNUM__TYPE__DB, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__HARDWIRED_TEMPLATE, \
        SG_DAGNUM__SET_FLAG(SG_DAGNUM__FLAG__ADMIN, \
        ((SG_uint64) 0)))))))

// TODO not sure "core" is the right name here
#define SG_AREA_NAME__CORE "core"

#define SG_AREA_NAME__VERSION_CONTROL "version_control"

END_EXTERN_C;

#endif //H_SG_DAGNUM_TYPEDEFS_H

