#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "compiler.h"
#include "../helpers/vector.h"

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
    process->node_vec = vector_create(sizeof(struct node *));
    process->node_tree_vec = vector_create(sizeof(struct node *));
    process->input_file = (cfile *)malloc(sizeof(cfile));
    process->input_file->file = file;
    process->ofile = output_file;
    process->output_type = output_type;
    return process;
}

char compile_process_next_char(lex_process *process)
{
    compile_process *compiler = process->compiler;
    compiler->pos.col++;
    char c = getc(compiler->input_file->file);
    if (c == '\n')
    {
        compiler->pos.col = 1;
        compiler->pos.line++;
    }
    return c;
}

char compile_process_peek_char(lex_process *process)
{
    compile_process *compiler = process->compiler;
    char c = getc(compiler->input_file->file);
    ungetc(c, compiler->input_file->file);
    return c;
}

void compile_process_push_char(lex_process *process, char c)
{
    compile_process *compiler = process->compiler;
    ungetc(c, compiler->input_file->file);
    compiler->pos.col--;
}