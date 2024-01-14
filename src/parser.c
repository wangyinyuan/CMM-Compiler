#include "compiler.h"
#include "../helpers/vector.h"
#include "token.h"

// history
struct history
{
    int flags;
};

struct history *history_begin(int flags)
{
    struct history *history = (struct history *)malloc(sizeof(struct history));
    history->flags = flags;
    return history;
}

struct history *history_down(struct history *history, int flags)
{
    struct history *new_history = (struct history *)malloc(sizeof(struct history));
    memcpy(new_history, history, sizeof(struct history));
    new_history->flags = flags;
    return new_history;
}

static compile_process *current_process;
static struct token *parser_last_token;
static bool token_is_nl_or_comment_or_newline_seperator(struct token *token);

static bool token_is_nl_or_comment_or_newline_seperator(struct token *token)
{
    return token->type == TOKEN_TYPE_NEWLINE ||
           token->type == TOKEN_TYPE_COMMENT ||
           token_is_symbol(token, '\\');
}

static void parser_ignore_nl_or_comment(struct token *token)
{
    while (token && token_is_nl_or_comment_or_newline_seperator(token))
    {
        vector_peek(current_process->token_vec);
        token = vector_peek_no_increment(current_process->token_vec);
    }
}

static struct token *token_next()
{
    struct token *next_token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_nl_or_comment(next_token);
    current_process->pos = next_token->pos;
    parser_last_token = next_token;
    return vector_peek(current_process->token_vec);
}

static token *token_peek_next()
{
    struct token *next_token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_nl_or_comment(next_token);
    return vector_peek_no_increment(current_process->token_vec);
}

void parse_single_token_to_node()
{
    struct token *token = token_next();
    struct node *node = NULL;
    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
        node = node_create(&(struct node){.type = NODE_TYPE_NUMBER, .llnum = token->llnum});
        break;

    case TOKEN_TYPE_IDENTIFIER:
        node = node_create(&(struct node){.type = NODE_TYPE_IDENTIFIER, .sval = token->sval});
        break;

    case TOKEN_TYPE_STRING:
        node = node_create(&(struct node){.type = NODE_TYPE_STRING, .sval = token->sval});
        break;

    default:
        compiler_error(current_process, "此 token 无法转换为 node");
    }
}

int parse_next()
{
    token *token = token_peek_next();

    if (!token)
    {
        return -1;
    }

    int res = 0;

    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
    case TOKEN_TYPE_STRING:
    case TOKEN_TYPE_IDENTIFIER:
        parse_single_token_to_node();
        break;
    default:
        break;
    }

    return 0;
}

int parse(compile_process *compiler)
{
    current_process = compiler;
    parser_last_token = NULL;
    node_set_vector(compiler->node_vec, compiler->node_tree_vec);
    struct node *node = NULL;

    vector_set_peek_pointer(compiler->token_vec, 0);

    while (parse_next() == 0)
    {
        node = node_peek();
        vector_push(compiler->node_tree_vec, &node);
    }
    return PARSE_ALL_OK;
}