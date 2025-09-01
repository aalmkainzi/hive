#include "header.h"
#include "h2.h"
#include "ab.h"

int main()
{
    int_hive ih;
    float_hive fh;
    double_hive dh;
    AHive ah;
    BHive bh;
    
    int_hive_init(&ih);
    float_hive_init(&fh);
    double_hive_init(&dh);
    AHive_init(&ah);
    BHive_init(&bh);
    
    int_hive_deinit(&ih);
    float_hive_deinit(&fh);
    double_hive_deinit(&dh);
    AHive_deinit(&ah);
    BHive_deinit(&bh);
}
