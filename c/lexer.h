#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum { T_INVALID = 0, INC, DEC, RIGHT, LEFT, OUT, IN, L_START, L_END, T_EOF } type_t;

typedef struct {
    type_t type;
    union {
        size_t count;
        size_t jump;
    };
} token_t;

typedef struct {
    token_t* data;
    size_t top;
    size_t capacity;
} token_stack;

token_stack* token_stack_factory(size_t initial_capacity);

void token_stack_destructor(token_stack* stack);

int push_token(token_stack* stack, token_t tok);

int lex_bf(token_stack* tokens, char* input, size_t len);
