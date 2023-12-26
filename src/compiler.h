#ifndef CMM_COMPILER_H
#define CMM_COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include "../helpers/buffer.h"

enum
{
    SUCCESS,
    FAILURE
};

enum output_type
{
    OUTPUT_TYPE_ASSEMBLY,
    OUTPUT_TYPE_OBJECT,
    OUTPUT_TYPE_EXECUTABLE
};

// token types
enum
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE,
    TOKEN_TYPE_SYMBOL,
};

typedef struct pos
{
    int line;
    int col;
    const char *filename;
} pos;

typedef struct token
{
    int type;
    int flags;

    union
    {
        char cval;
        const char *sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void *any;
    };

    /**
     * @brief 表示是否包含空白字符的布尔值。
     */
    bool whitespace;

    const char *between_brackets;
} token;

struct lex_process;
typedef char (*LEX_PROCESS_NEXT_CHAR)(lex_process *process);
typedef char (*LEX_PROCESS_PEEK_CHAR)(lex_process *process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(lex_process *process, char c);

typedef struct lex_process_functions
{
    LEX_PROCESS_NEXT_CHAR next_char;
    LEX_PROCESS_PEEK_CHAR peek_char;
    LEX_PROCESS_PUSH_CHAR push_char;
} lex_process_functions;

typedef struct lex_process
{
    pos pos;
    struct vector *token_vec;
    compile_process *compiler;

    int current_expression_count;
    struct buffer *parentheses_buffer;
    lex_process_functions *function;

    void *private;
} lex_process;

/**
 * @brief Represents the compile process information.
 *
 * This struct contains the necessary information for the compile process,
 * including the filename, output filename,
 * and output type.
 */
typedef struct compile_process_input_file
{
    FILE *file;
    const char *abs_path;
} cfile;

typedef struct compile_process
{

    cfile *input_file; ///< The input file.
    FILE *ofile;       ///< The output file.
    int output_type;   ///< The type of output for the compiler.

    pos pos; ///< The current position of the compiler.
} compile_process;

int compile_file(const char *filename, const char *output_filename, int output_type);
compile_process *compile_process_create(const char *filename, const char *output_filename, int output_type);

// lex_process_functions
char compile_process_next_char(lex_process *process);
char compile_process_peek_char(lex_process *process);
void compile_process_push_char(lex_process *process, char c);

#endif // CMM_COMPILER_H