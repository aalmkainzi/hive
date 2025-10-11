#define COLONY_SIZE 64
#define COLONY_ALIGNMENT 64
#define HANDLE_SIZE 64
#define HANDLE_ALIGNMENT 64
#define ITERATOR_SIZE 64
#define ITERATOR_ALIGNMENT 64
typedef struct Big Big;
#include "rs_col.h"

#include <stdio.h>
int main()
{
    printf("col-size = %zu\n", colony_get_colony_size());
    printf("col-alignment = %zu\n", colony_get_colony_alignment());
    printf("handle-size = %zu\n", colony_get_handle_size());
    printf("handle-alignment = %zu\n", colony_get_handle_alignment());
    printf("iter-size = %zu\n", colony_get_iterator_size());
    printf("iter-alignment = %zu\n", colony_get_iterator_alignment());
}