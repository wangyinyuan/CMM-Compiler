#include "compiler.h"

int compile_file(const char *filename, const char *output_filename, int output_type)
{
    compile_process *process = compile_process_create(filename, output_filename, output_type);
    return 0;
}