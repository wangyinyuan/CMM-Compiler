#include "compiler.h"

lex_process_functions compiler_lex_functinos = {
    .next_char = compile_process_next_char,
    .peek_char = compile_process_peek_char,
    .push_char = compile_process_push_char,
};

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