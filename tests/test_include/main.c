#include "header.h"

int main()
{
    int_hive ih;
    float_hive fh;
    
    int_hive_init(&ih);
    float_hive_init(&fh);
    
    int_hive_deinit(&ih);
    float_hive_deinit(&fh);
}
