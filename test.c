#include <stdio.h>
#include "scanner.h"
#include "error.h"
#include "lex_funs.h"

int main(void) {
    token_ptr token;

    while (1) {
        token = get_token();
        if (token == NULL) {
            fprintf(stderr, "Error: NULL token returned.\n");
            return 1;
        }

        switch (token->type) {
            case IDENT:
                printf("IDENT: %s\n", token->value.name);
                break;
            case KEY_WORD:
                printf("KEYWORD: %s\n", token->value.str_value);
                break;
            case GLOB_VAR:
                printf("GLOBAL: %s\n", token->value.name);
                break;
            case INT_LIT:
                printf("INT: %lld\n", token->value.int_value);
                break;
            case FLOAT_LIT:
                printf("FLOAT: %Lf\n", token->value.float_value);
                break;
            case ONE_L_STRING:
                printf("STRING: \"%s\"\n", token->value.str_value);
                break;
            case MUL_L_STRING:
                printf("MULTILINE STRING: \"%s\"\n", token->value.str_value);
                break;
            case OPERATOR:
                printf("OPERATOR: '%c'\n", token->value.other_value);
                break;
            case LEFT_PAR: printf("LEFT_PAR\n"); break;
            case RIGHT_PAR: printf("RIGHT_PAR\n"); break;
            case LEFT_DOM_PAR: printf("LEFT_DOM_PAR\n"); break;
            case RIGHT_DOM_PAR: printf("RIGHT_DOM_PAR\n"); break;
            case DOUBLE_DOT: printf("DOUBLE_DOT\n"); break;
            case TRIPE_DOT: printf("TRIPLE_DOT\n"); break;
            case DOT: printf("DOT\n"); break;
            case COMMA: printf("COMMA\n"); break;
            case Q_MARK: printf("Q_MARK\n"); break;
            case SEMICOLON: printf("SEMICOLON\n"); break;
            case MINUS: printf("MINUS\n"); break;
            case EOL: printf("EOL\n"); break;
            case END_OF_FILE:
                printf("EOF\n");
                free_token(token);
                return 0;
            default:
                printf("UNKNOWN TOKEN\n");
                break;
        }

        free_token(token);
    }
}
