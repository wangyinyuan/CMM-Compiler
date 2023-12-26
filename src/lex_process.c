#include "compiler.h"
#include "../helpers/vector.h"
#include <stdlib.h>

lex_process *lex_process_create(compile_process *compiler, lex_process_functions *functions, void *data)
{
    lex_process *process = (lex_process *)malloc(sizeof(lex_process));
    if (!process)
        return NULL;

    process->compiler = compiler;
    process->function = functions;
    process->token_vec = vector_create(sizeof(token));
    process->private = data;
    process->pos.col = 1;
    process->pos.line = 1;

    return process;
}

void lex_process_free(lex_process *process)
{
    vector_free(process->token_vec);
    free(process);
}

void *lex_process_private(lex_process *process)
{
    return process->private;
}

struct vector *lex_process_tokens(lex_process *process)
{
    return process->token_vec;
}
