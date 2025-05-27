#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
int main(int argc, char **argv)
{
    if(argc < 2)
        return 1;
    FILE *f = fopen("OUT", "r");
    FILE *f2 = fopen(argv[1], "w");
    
    if(ferror(f) || ferror(f2))
    {
        puts(strerror(errno));
        return 1;
    }
    
    char line[512] = {0};
    
    int mode = 0; // hv=1, plf=2, ls=3
    int i = 0;
    while(!feof(f))
    {
        printf("i=%d\n", i++);
        fgets(line, sizeof(line), f);
        if(line[0] == 'S')
        {
            fputs(line, f2);
            mode = 0;
        }
        else if(line[0] == 'H' && (mode == 0 || mode == 2 || mode == 3))
        {
            fputs(line, f2);
            mode = 1;
        }
        else if(line[0] == 'P' && (mode == 0 || mode == 1 || mode == 3))
        {
            fputs(line, f2);
            mode = 2;
        }
        else if(line[0] == 'L' && (mode == 0 || mode == 1 || mode == 2))
        {
            fputs(line, f2);
            mode = 3;
        }
    }
}
