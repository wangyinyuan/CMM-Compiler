#ifndef TOKEN_H
#define TOKEN_H
#include "compiler.h"

bool token_is_keyword(token *token_instance, const char *keyword);
bool token_is_symbol(struct token *token, char sym);

#endif