#include "lexer.h"
#include <stdio.h>

struct token_comp {
    type_t ttype;
    const char* out;
} LUT[] = {{T_INVALID, ""},
           {RIGHT, "get_global $p \
                i32.const %u  \
                i32.add       \
                set_global $p"},
           {LEFT, "get_global $p \
                i32.const %u  \
                i32.sub       \
                set_global $p"},
           {INC, "get_global $p \
                get_global $p \
                i32.load8_u   \
                i32.const %u  \
                i32.add       \
                i32.store8    "},
           {DEC, "get_global $p \
                get_global $p \
                i32.load8_u   \
                i32.const %u  \
                i32.sub       \
                i32.store8    "},
           {OUT, "get_global $p \
                  i32.load8_u       \
                  call $put_out"},
           {IN, "get_global $p  \
                     call $get_in   \
                     i32.store8"},
           {L_START, "(block    \
                (loop %u     \
                get_global $p\
                i32.load8_u  \
                i32.const 0  \
                i32.eq       \
                br_if $%u"},
           {L_END, "br 0       \
                  )          \
                  )"},
           {T_EOF, ""}

};

int write_header(FILE* out) {

    const char* header_top = "(module \
                              (memory 1) \
                              %s \
                              %s \
                              (global $p (mut i32) (i32.const 0)) \
                              (func $main";

    const char* fd_write = "(import \"wasi_snapshot_preview1\" \"fd_write\" \
                            (func $fd_write (param i32 i32 i32 i32) (result i32))) \
                            \
                            (func $put_out (param $out i32) \
                                (i32.store8 (i32.const 65528) (local.get $out)) \
                                (i32.store (i32.const 65520) (i32.const 65528)) \
                                (i32.store (i32.const 65524) (i32.const 1)) \
                                (call $fd_write \
                                    (i32.const 1) \
                                    (i32.const 65520) \
                                    (i32.const 1) \
                                    (i32.const 65532)\
                                )\
                            drop\
                            )";

    const char* fd_read = "(import \"wasi_snapshot_preview1\" \"fd_read\" \
                            (func $fd_read (param i32 i32 i32 i32) (result i32))) \
                            (func $get_in (result i32) \
                                (i32.store (i32.const 65520) (i32.const 65528)) \
                                (i32.store (i32.const 65524) (i32.const 1)) \
                                (call $fd_read \
                                  (i32.const 0) \
                                  (i32.const 65520) \
                                  (i32.const 1) \
                                  (i32.const 65532) \
                                ) \
                                drop \
                            (i32.load8_u (i32.const 65528)) \
                            )";
    int written = fprintf(out, header_top, fd_write, fd_read);
    if (written < 0) {
        return -1;
    }
    return 0;
}

int write_footer(FILE* out) {
    const char* footer = ") \
                         (export \"_start\" (func $main)) \
                         )";
    int written = fprintf(out, "%s", footer);
    if(written < 0) {
        return -1;
    }
    return 0;
}

int write_body(token_stack* stack, FILE* out) {
    // For sequentially identical tokens
    size_t n_tok_seq = 0;
    token_t cur_tok = stack->data[0];
    for (size_t i = 0; i < stack->top;) {
        if (cur_tok.type == T_EOF) break;
        // Loop tokens are handled a little weird
        if (cur_tok.type == L_START || cur_tok.type == L_END) {
            n_tok_seq = cur_tok.jump;
        } else {
            n_tok_seq = cur_tok.count;
        }
        if(cur_tok.type == T_EOF) {
            printf("Shouldn't have hit EOF");
            return -1;
        } else if (cur_tok.type == T_INVALID) {
            i++; 
            cur_tok = stack->data[i];
            continue;
        }

        if(cur_tok.type == L_START) {
            fprintf(out, LUT[cur_tok.type].out, n_tok_seq, n_tok_seq);
        } else if(cur_tok.type == L_END) {
            fprintf(out, "%s", LUT[cur_tok.type].out);
        } else {
            fprintf(out, LUT[cur_tok.type].out, n_tok_seq);
        }
        i++;
        cur_tok = stack->data[i];
    }

    return 0;
}

int compile(token_stack* stack, FILE* out) {
    if(out == NULL) {
        printf("Compile received invalid file descriptor");
        return -1;
    }
    int ret = 0;
    ret = write_header(out);
    if(ret < 0) {
        printf("Failed to write WAT file header");
        return ret;
    }
    ret = write_body(stack, out);
    if(ret < 0) {
        printf("Compile failed on program body");
        return ret;
    }
    ret = write_footer(out);
    if (ret < 0) {
        printf("Failed to write WAT file footer");
        return ret;
    }
    printf("Successfully compiled WAT file");
    return ret;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: bf_compile <input_file> <output_file>");
        return -1;
    }
    char* f_in = argv[1];
    char* f_out = argv[2];

    FILE* fp = fopen(f_in, "r");
    // Get filesize
    fseek(fp, 0L, SEEK_END);
    size_t flen = ftell(fp);
    rewind(fp);

    // Read file to string for lexing
    char* fpstring = malloc(flen);
    if (!fpstring) {
        fclose(fp);
        free(fpstring);
        printf("Failed to read input file %s", f_in);
        return -1;
    }
    fread(fpstring, flen, 1, fp);
    fclose(fp);

    token_stack* stack = token_stack_factory(flen / 2);
    if (!stack) {
        printf("Failed to create token allocator for lexer.");
        free(fpstring);
        return -1;
    }
    // Lex da file
    if (lex_bf(stack, fpstring, flen) < 0) {
        printf("Failed to lex input file %s", f_in);
        free(fpstring);
        token_stack_destructor(stack);
        return -1;
    }
    free(fpstring);

    // Compile da file
    fp = fopen(f_out, "w");
    int compile_status = compile(stack, fp);
    if(compile_status < 0) {
        printf("Failed to compile file %s", f_in);
        token_stack_destructor(stack);
        return -1;
    }
    fclose(fp);

    return 0;
}
