/*
 * Lexes a brainfuck file into tokens for the compiler
 * This code is ugly, but I do not care!
 */

#include "lexer.h"

// An expository note here: I love C. I love C so very much.
// It is so damn annoying to me that I cannot make allocators
// with the sort of syntax I would in rust and instead create
// shit like token_stack_factory. I'd much prefer
// token_stack::new(), but we can't have everything beautiful.

token_stack* token_stack_factory(size_t initial_capacity) {
    token_t* token_stack_data = malloc(initial_capacity * sizeof(token_t));
    if (!token_stack_data) {
        return NULL;
    }
    token_stack* stack = malloc(sizeof(token_stack));
    if (!stack) {
        free(token_stack_data);
        return NULL;
    }
    stack->capacity = initial_capacity;
    stack->top = 0;
    stack->data = token_stack_data;
    return stack;
}

void token_stack_destructor(token_stack* stack) {
    free(stack->data);
    free(stack);
}

int push_token(token_stack* stack, token_t tok) {
    if (stack->top + 1 > stack->capacity) {
        size_t new_cap = stack->capacity * 2;
        token_t* data = realloc(stack->data, new_cap);
        if (!data) {
            return -1;
        }
        stack->capacity = new_cap;
    }
    stack->data[stack->top] = tok;
    stack->top++;
    return 0;
}

static const uint8_t c_to_t[256] = {
    ['+'] = INC, ['-'] = DEC, ['>'] = RIGHT,   ['<'] = LEFT,
    ['.'] = OUT, [','] = IN,  ['['] = L_START, [']'] = L_END,
};

/*
  Take in an input byte stream and convert it to an array of tokens.
  Wee!!! I haven't written something like this in a while.
  Am I forgetting how the hell to write C?
*/
int lex_bf(token_stack* tokens, char* input, size_t len) {
    // Invalid token stack
    if (!tokens) return -1;
    // Empty file
    if (len == 0) return 0;
    type_t cur_ty = c_to_t[(unsigned char)input[0]];
    size_t rc = 0;

    size_t* lstack = malloc(len * sizeof(size_t));
    if (!lstack) return -1;
    size_t l_top = 0;

    for (size_t i = 0; i < len; i++) {
        cur_ty = c_to_t[(unsigned char)input[i]];

        if (cur_ty == L_START) {
            if (rc > 0) {
                token_t pending
                    = {.type = c_to_t[(unsigned char)input[i - 1]], .count = rc};
                if (push_token(tokens, pending) < 0) {
                    free(lstack);
                    return -1;
                }
                rc = 0;
            }
            size_t start = tokens->top;
            token_t tok = {.type = L_START, .count = 1,};
            if (push_token(tokens, tok) < 0) {
                free(lstack);
                return -1;
            }
            lstack[l_top++] = start;
            continue;
        } else if (cur_ty == L_END) {

            if (rc > 0) {
                token_t pending
                    = {.type = c_to_t[(unsigned char)input[i - 1]], .count = rc};
                if (push_token(tokens, pending) < 0) {
                    free(lstack);
                    return -1;
                }
                rc = 0;
            }
            if (l_top == 0) {
                free(lstack);
                return -1;
            }
            size_t start = lstack[--l_top];
            size_t end = tokens->top;
            token_t tok = {.type = L_END, .jump = end};
            if (push_token(tokens, tok) < 0) {
                free(lstack);
                return -1;
            }
            tokens->data[start].jump = end;
            continue;
        }

        if (cur_ty != T_EOF && cur_ty != c_to_t[(unsigned char)input[i > 0 ? i - 1 : 0]]) {
            if (rc > 0) {
                token_t tok = {.type = c_to_t[(unsigned char)input[i - 1]], .count = rc};
                if (push_token(tokens, tok) < 0) {
                    free(lstack);
                    return -1;
                }
            }
            rc = 0;
        }

        if (cur_ty != T_EOF) rc++;
    }
    
    if (rc > 0 && cur_ty != T_EOF) {
        token_t tok = {.type = cur_ty, .count = rc};
        if (push_token(tokens, tok) < 0) { free(lstack); return -1; }
    }

    if (l_top != 0) { free(lstack); return -1; }

    free(lstack);
    return 0;

    }
