#include "header.h"
#include "h2.h"

int main()
{
    int_hive ih;
    float_hive fh;
    double_hive dh;
    
    int_hive_init(&ih);
    float_hive_init(&fh);
    double_hive_init(&dh);
    
    int_hive_deinit(&ih);
    float_hive_deinit(&fh);
    double_hive_deinit(&dh);
}
