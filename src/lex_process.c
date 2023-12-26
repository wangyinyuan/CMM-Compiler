#include "compiler.h"
#include "../helpers/vector.h"

lex_process *lex_process_create(compile_process *compiler, lex_process_functions *functions, void *data)
{
    lex_process *process = (lex_process *)malloc(sizeof(lex_process));
    if (!process)
        return NULL;
    // printf("lex_process is not null\n");
    process->compiler = compiler;
    process->function = functions;
    // printf("%d", sizeof(struct token));
    process->token_vec = vector_create(sizeof(struct token));

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
