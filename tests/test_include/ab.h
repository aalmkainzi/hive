// ab.h
#ifndef MY_OBJ_H
#define MY_OBJ_H

typedef struct A
{
    int i;
} A;

typedef struct B
{
    int j;
} B;

#define HIVE_TYPE A
#define HIVE_NAME AHive
#include "hive.h"

#define HIVE_TYPE B
#define HIVE_NAME BHive
#include "hive.h"

#endif
