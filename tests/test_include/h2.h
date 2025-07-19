// foo_hive.h
#ifndef FOO_HIVE_H
#define FOO_HIVE_H

#define HIVE_TYPE double
#define HIVE_NAME double_hive
#include "../../hive.h"

// struct MyStruct doesn't exist
// should still compile fine, should only fail if HIVE_IMPL is defined
// this allows you to use incomplete types that you define later
#define HIVE_TYPE struct MyStruct
#define HIVE_NAME MyStruct_hive
#include "../../hive.h"

#endif
