#include "token.h"

#define PRIMITIVE_TYPES_TOTAL 7
const char *primitive_types[PRIMITIVE_TYPES_TOTAL] = {
    "void", "char", "short", "int", "long", "float", "double"};

bool token_is_keyword(token *token_instance, const char *keyword)
{
    return token_instance && token_instance->type == TOKEN_TYPE_KEYWORD && S_EQ(token_instance->sval, keyword);
}

bool token_is_symbol(struct token *token, char sym)
{
    return token && token->type == TOKEN_TYPE_SYMBOL && token->cval == sym;
}

bool token_is_operator(struct token *token, const char *op)
{
    return token && token->type == TOKEN_TYPE_OPERATOR && S_EQ(token->sval, op);
}

bool token_is_primitive_keyword(struct token *token)
{
    if (!token)
        return false;
    if (token->type != TOKEN_TYPE_KEYWORD)
        return false;
    for (int i = 0; i < PRIMITIVE_TYPES_TOTAL; i++)
    {
        if (S_EQ(primitive_types[i], token->sval))
            return true;
    }
    return false;
}