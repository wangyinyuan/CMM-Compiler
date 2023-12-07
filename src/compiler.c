#include "../include/compiler.h"

// 编译器主入口

int compile_file(const char *filename, const char *output_filename, int output_type)
{
    compile_process *process = compile_process_create(filename, output_filename, output_type);
    if (!process)
        return FAILURE;

    // lexical analysis

    // parsing

        return SUCCESS;
}