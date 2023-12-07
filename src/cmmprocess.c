#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"

compile_process *compile_process_create(const char *filename, const char *output_filename, int output_type)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("File %s not found.\n", filename);
        return NULL;
    }

    FILE *output_file = NULL;
    if (output_filename)
    {
        output_file = fopen(output_filename, "w");
        if (output_file == NULL)
        {
            printf("File %s cannot be opened.\n", output_filename);
            return NULL;
        }
    }

    compile_process *process = (compile_process *)malloc(sizeof(compile_process));
    process->input_file->file = file;
    process->ofile = output_file;
    process->output_type = output_type;
    return process;
}
