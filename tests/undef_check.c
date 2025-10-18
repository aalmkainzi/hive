#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define STB_DS_IMPLEMENTATION
#include "benchmarks/external/stb_ds.h"

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        return 1;
    }
    char *filename = argv[1];
    
    FILE *file = fopen(filename, "r");
    
    char **macros = NULL;
    
    while(!feof(file))
    {
        char line[1024] = {0};
        fgets(line, sizeof(line), file);
        size_t line_len = strlen(line);
        int i = 0;
        for( ; i < line_len && isspace(line[i]) ; i++);
        
        if(strlen(line) < strlen("#undef"))
            continue;
        if(memcmp(line + i, "#define", strlen("#define")) != 0)
        {
            if(memcmp(line + i, "#undef", strlen("#undef")) != 0)
            {
                continue;
            }
            else
            {
                // an undef
                
                char *undef = line + i + strlen("#undef ");
                char *undef_dup = malloc(strlen(undef) - 1);
                memcpy(undef_dup, undef, strlen(undef) - 2);
                undef_dup[strlen(undef) - 2] = 0;
                size_t i;
                for(i = 0 ; i < arrlen(macros) ; i++)
                {
                    if(strlen(macros[i]) == (strlen(undef_dup)) && strcmp(undef_dup, macros[i]) == 0)
                    {
                        free(macros[i]);
                        arrdelswap(macros, i);
                        goto safe_done;
                    }
                }
                fprintf(stderr, "[ERROR]: found undef of macro that wasn't defined: \"%s\"\n", undef_dup);
                // exit(69);
                safe_done:;
                free(undef_dup);
            }
        }
        else
        {
            char *macro_name = line + i + strlen("#define ");
            int j = 0;
            for( ; isalpha(macro_name[j]) || macro_name[j] == '_' ; j++);
            char *macro_dup = malloc(j + 1);
            memcpy(macro_dup, macro_name, j);
            macro_dup[j] = 0;
            arrpush(macros, macro_dup);
            printf("[INFO]: found macro: \"%s\"\n", macro_dup);
        }
    }
    
    if(arrlen(macros) != 0)
    {
        printf("Found macros that were never undefined:\n");
        for(size_t i = 0 ; i < arrlen(macros) ; i++)
        {
            printf("%s\n", macros[i]);
        }
    }
    
    fclose(file);
    
    for(size_t i = 0 ; i < arrlen(macros) ; i++)
    {
        free(macros[i]);
    }
    arrfree(macros);
}