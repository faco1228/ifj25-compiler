// scanner_test.c
#include "scanner.h"
#include <stdio.h>

const char *token_type_str[] = {
    "IDENT", "KEY_WORD", "GLOB_VAR",
    "INT_LIT", "FLOAT_LIT", "NULL_LIT",
    "ONE_L_STRING", "MUL_L_STRING",
    "OPERATOR", "MINUS",
    "LEFT_PAR", "RIGHT_PAR", "LEFT_DOM_PAR", "RIGHT_DOM_PAR",
    "END_OF_LINE", "END_OF_FILE",
    "DOT", "DOUBLE_DOT", "TRIPLE_DOT",
    "Q_MARK", "EXC_MARK", "SEMICOLON", "COMMA"
};

int main(void) {
    token_ptr token;

    while ((token = get_token())->type != END_OF_FILE) {
        printf("TOKEN: %-15s", token_type_str[token->type]);

        switch (token->type) {
            case IDENT:
            case GLOB_VAR:
            case KEY_WORD:
            case ONE_L_STRING:
            case MUL_L_STRING:
                printf("  value: %s\n", token->value.str_value);
                break;
            case INT_LIT:
                printf("  value: %lld\n", token->value.int_value);
                break;
            case FLOAT_LIT:
                printf("  value: %Lf\n", token->value.float_value);
                break;
            case NULL_LIT:
            case OPERATOR:
            case MINUS:
            case LEFT_PAR: 
            case RIGHT_PAR:
            case LEFT_DOM_PAR: 
            case RIGHT_DOM_PAR:
            case DOT: 
            case DOUBLE_DOT: 
            case TRIPLE_DOT:
            case Q_MARK: 
            case EXC_MARK: 
            case SEMICOLON: 
            case COMMA:
            case END_OF_LINE:
                printf("  value code: %d\n", token->value.other_value);
                break;
            default:
                printf("\n");
                break;
        }

        free_token(token);
    }

    free_token(token); // posledný EOF token
    scanner_cleanup();

    return 0;
}
