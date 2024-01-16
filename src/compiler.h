#ifndef CMM_COMPILER_H
#define CMM_COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../helpers/buffer.h"

#define S_EQ(str1, str2) \
    (str1 && str2 && strcmp(str1, str2) == 0)

// compile_process 成败返回值
enum
{
    SUCCESS,
    FAILURE
};

// lex_process 成败返回值
enum
{
    LEXICAL_ANALYSIS_ALL_OK,
    LEXICAL_ANALYSIS_ERROR
};

enum output_type
{
    OUTPUT_TYPE_ASSEMBLY,
    OUTPUT_TYPE_OBJECT,
    OUTPUT_TYPE_EXECUTABLE
};

#define NUMERIC_CASES \
    case '0':         \
    case '1':         \
    case '2':         \
    case '3':         \
    case '4':         \
    case '5':         \
    case '6':         \
    case '7':         \
    case '8':         \
    case '9'

// 操作符
#define OPERATOR_CASES_EXCLUDING_DIVISION \
    case '+':                             \
    case '-':                             \
    case '*':                             \
    case '>':                             \
    case '<':                             \
    case '^':                             \
    case '%':                             \
    case '!':                             \
    case '=':                             \
    case '~':                             \
    case '|':                             \
    case '&':                             \
    case '(':                             \
    case '[':                             \
    case ',':                             \
    case '.':                             \
    case '?'

#define SYMBOL_CASE \
    case '{':       \
    case '}':       \
    case ':':       \
    case ';':       \
    case '#':       \
    case '\\':      \
    case ')':       \
    case ']'

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

enum
{
    NUMBER_TYPE_NORMAL,
    NUMBER_TYPE_LONG,
    NUMBER_TYPE_FLOAT,
    NUMBER_TYPE_DOUBLE
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
    pos pos;

    union
    {
        char cval;
        const char *sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void *any;
    };

    struct token_number
    {
        int type;
    } num;

    /**
     * @brief 表示是否包含空白字符的布尔值。
     */
    bool whitespace;

    const char *between_brackets;
} token;

/**
 * @brief Represents the compile process information.
 *
 * This struct contains the necessary information for the compile process,
 * including the filename, output filename,
 * and output type.
 */
typedef struct compile_process compile_process;
bool token_is_primitive_keyword(struct token *token);
bool token_is_operator(struct token *token, const char *op);

typedef struct compile_process_input_file
{
    FILE *file;
    const char *abs_path;
} cfile;

// scope
struct scope
{
    int flags;

    struct vector *entities;

    size_t size;

    struct scope *parent;
};

enum
{
    SYMBOL_TYPE_NODE,
    SYMBOL_TYPE_NATIVE_FUNCTION,
    SYMBOL_TYPE_UNKNOWN
};

struct symbol
{
    const char *name;
    int type;
    void *data;
};

struct compile_process
{

    cfile *input_file; ///< The input file.
    FILE *ofile;       ///< The output file.
    int output_type;   ///< The type of output for the compiler.

    pos pos; ///< The current position of the compiler.

    /**
     * @brief 编译器结构体
     *
     * 该结构体包含了编译器所需的向量数据结构。
     */
    struct vector *token_vec;     /**< 词法分析结果向量 */
    struct vector *node_vec;      /**< 语法分析结果向量 */
    struct vector *node_tree_vec; /**< 语法树向量 */

    struct
    {
        struct scope *root;
        struct scope *current;
    } scope;

    struct
    {
        struct vector *tables;
        struct vector *table;
    } symbols;
};

// 词法分析器结构体定义

typedef struct lex_process_functions lex_process_functions;
typedef struct lex_process lex_process;
struct lex_process
{
    pos pos;
    struct vector *token_vec;
    compile_process *compiler;

    int current_expression_count;
    struct buffer *parentheses_buffer;
    lex_process_functions *function;

    void *private;
};

enum
{
    PARSE_ALL_OK,
    PARSE_GENERAL_ERROR
};

enum
{
    NODE_TYPE_EXPRESSION,
    NODE_TYPE_EXPRESSION_PARENTHESIS,
    NODE_TYPE_NUMBER,
    NODE_TYPE_IDENTIFIER,
    NODE_TYPE_STRING,
    NODE_TYPE_VARIABLE,
    NODE_TYPE_VARIABLE_LIST,
    NODE_TYPE_FUNCTION,
    NODE_TYPE_BODY,
    NODE_TYPE_STATEMENT_RETURN,
    NODE_TYPE_STATEMENT_IF,
    NODE_TYPE_STATEMENT_ELSE,
    NODE_TYPE_STATEMENT_WHILE,
    NODE_TYPE_STATEMENT_DO_WHILE,
    NODE_TYPE_STATEMENT_FOR,
    NODE_TYPE_STATEMENT_BREAK,
    NODE_TYPE_STATEMENT_CONTINUE,
    NODE_TYPE_STATEMENT_SWITCH,
    NODE_TYPE_STATEMENT_CASE,
    NODE_TYPE_STATEMENT_DEFAULT,
    NODE_TYPE_STATEMENT_GOTO,
    NODE_TYPE_TENARY,
    NODE_TYPE_LABEL,
    NODE_TYPE_UNARY,
    NODE_TYPE_STRUCT,
    NODE_TYPE_UNION,
    NODE_TYPE_BRACKET,
    NODE_TYPE_CAST,
    NODE_TYPE_BLANK
};

struct node;
struct datatype
{
    int flags;
    int type;

    struct datatype *secondary;

    const char *type_str;

    size_t size;

    int pointer_depth;

    union
    {
        struct node *struct_node;
        struct node *union_node;
    };

    struct array
    {
        struct array_brackets *brackets;
        size_t size;
    } array;
};
bool keyword_is_datatype(const char *str);

struct node
{
    int type;
    int flags;

    struct pos pos;

    struct node_binded
    {
        struct node *owner;
        struct node *function;
    } binded;

    union
    {
        struct exp
        {
            struct node *left;
            struct node *right;
            const char *op;
        } exp;

        struct var
        {
            struct datatype type;
            const char *name;
            struct node *val;
        } var;

        struct varlist
        {
            struct vector *list;
        } var_list;

        struct body
        {
            struct vector *statements;
            size_t size;
            /**
             * @brief 表示是否进行了填充的布尔值。
             */
            bool padded;
            struct node *largest_var_node;
        } body;
    };

    union
    {
        char cval;
        const char *sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
    };
};

enum
{
    DATATYPE_FLAG_IS_SIGNED = 0b00000001,
    DATATYPE_FLAG_IS_STATIC = 0b00000010,
    DATATYPE_FLAG_IS_CONST = 0b00000100,
    DATATYPE_FLAG_IS_POINTER = 0b00001000,
    DATATYPE_FLAG_IS_ARRAY = 0b00010000,
    DATATYPE_FLAG_IS_EXTERN = 0b00100000,
    DATATYPE_FLAG_IS_RESTRICT = 0b01000000,
    DATATYPE_FLAG_IGNORE_TYPE_CHECKING = 0b10000000,
    DATATYPE_FLAG_SECONDARY = 0b100000000,
    DATATYPE_FLAG_STRUCT_UNION_NO_NAME = 0b1000000000,
    DATATYPE_FLAG_IS_LITERAL = 0b10000000000
};

enum
{
    DATA_TYPE_VOID,
    DATA_TYPE_CHAR,
    DATA_TYPE_SHORT,
    DATA_TYPE_INTEGER,
    DATA_TYPE_FLOAT,
    DATA_TYPE_DOUBLE,
    DATA_TYPE_LONG,
    DATA_TYPE_STRUCT,
    DATA_TYPE_UNION,
    DATA_TYPE_UNKNOWN,
};

enum
{
    DATA_TYPE_EXPECT_PRIMITIVE,
    DATA_TYPE_EXPECT_UNION,
    DATA_TYPE_EXPECT_STRUCT,
};

enum
{
    DATA_SIZE_ZERO = 0,
    DATA_SIZE_BYTE = 1,
    DATA_SIZE_WORD = 2,
    DATA_SIZE_DWORD = 4,
    DATA_SIZE_DDWORD = 8
};

typedef char (*LEX_PROCESS_NEXT_CHAR)(lex_process *process);
typedef char (*LEX_PROCESS_PEEK_CHAR)(lex_process *process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(lex_process *process, char c);

struct lex_process_functions
{
    LEX_PROCESS_NEXT_CHAR next_char;
    LEX_PROCESS_PEEK_CHAR peek_char;
    LEX_PROCESS_PUSH_CHAR push_char;
};

int compile_file(const char *filename, const char *output_filename, int output_type);
compile_process *compile_process_create(const char *filename, const char *output_filename, int output_type);

// lex_process_functions
char compile_process_next_char(lex_process *process);
char compile_process_peek_char(lex_process *process);
void compile_process_push_char(lex_process *process, char c);

// lex_process
lex_process *lex_process_create(compile_process *compiler, lex_process_functions *functions, void *data);
void lex_process_free(lex_process *process);
void *lex_process_private(lex_process *process);
struct vector *lex_process_tokens(lex_process *process);

// lexer
int lex(lex_process *process);

// error and warning
void compiler_error(compile_process *compiler, const char *msg, ...);
void compiler_warning(compile_process *compiler, const char *msg, ...);

lex_process *token_build_for_string(compile_process *compiler, const char *str);

// parser
int parse(compile_process *compiler);

// node

void node_set_vector(struct vector *vec, struct vector *root_vec);
void node_push(struct node *node);
struct node *node_peek_or_null();
struct node *node_peek();
struct node *node_pop();
struct node *node_create(struct node *node);
bool node_is_expressionable(struct node *node);
struct node *node_peek_expressionable_or_null();
void make_exp_node(struct node *left_node, struct node *right_node, const char *op);
void make_body_node(struct vector *body_vec, size_t size, bool padded, struct node *largest_var_node);

// history
enum
{
    NODE_FLAG_INSIDE_EXPRESSION = 0b00000001,
    NODE_FLAG_CLONED = 0b00000010,
    NODE_FLAG_IS_FORWARD_DECLARATION = 0b00000100,
    NODE_FLAG_HAS_VARIABLE_COMBINED = 0b00001000
};

// expression
#define TOTAL_OPERATOR_GROUPS 14
#define MAX_OPERATORS_IN_GROUP 12

enum
{
    ASSOCIATIVITY_LEFT_TO_RIGHT,
    ASSOCIATIVITY_RIGHT_TO_LEFT
};
struct expressionable_op_precedence_group
{
    char *operators[MAX_OPERATORS_IN_GROUP];
    int associativity;
};

// datatype
bool datatype_is_struct_or_union_for_name(const char *name);
size_t datatype_element_size(struct datatype *dtype);
size_t datatype_size_no_ptr(struct datatype *dtype);
size_t datatype_size(struct datatype *dtype);

// scope functions
struct scope *scope_alloc();
struct scope *scope_create_root(struct compile_process *process);
void scope_free_root(struct compile_process *process);
struct scope *scope_new(struct compile_process *process, int flags);
void scope_iteration_start(struct scope *scope);
void *scope_iterate_back(struct scope *scope);
void *scope_last_entity_at_scope(struct scope *scope);
void *scope_last_entity_from_scope_stop_at(struct scope *scope, struct scope *stop_scope);
void *scope_last_entity_stop_at(struct compile_process *process, struct scope *stop_scope);
void *scope_last_entity(struct compile_process *process);
void scope_push(struct compile_process *process, void *ptr, size_t elem_size);
void scope_finish(struct compile_process *process);
struct scope *scope_current(struct compile_process *process);

// helper
size_t variable_size(struct node *var_node);
size_t variable_size_for_list(struct node *var_list_node);

#endif // CMM_COMPILER_H
