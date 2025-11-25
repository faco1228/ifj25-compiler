#include "psa.h"
#include <stdio.h>
#include <string.h>

#include "scanner.h"
#include "ast.h"
#include "error.h"
#include "symtable.h"


/**
 * Tento testovací súbor:
 *  - NElinkuj so scanner.c (aby sa nebil get_token/push_token/free_token)
 *  - Definuje vlastný "fake scanner" nad statickým poľom tokenov.
 *
 * Poznámka: free_token je tu NO-OP, aby sme nefreeovali statické tokeny.
 * PSA si alokuje svoje vlastné tokeny (MARKER, NONTERMINAL_E, '$') a
 * tie sa nebudú freeovať → pre testy to nevadí (krátko žijúci proces).
 */

/* ───────────────────────── Fake scanner state ───────────────────────── */

ASTNode_ptr g_ast_root = NULL;
ST_Node *g_func_symtable = NULL;
ST_Node *g_global_symtable = NULL;


void consume_eols(void) { }


static token_t *g_stream = NULL;
static size_t   g_stream_len = 0;
static size_t   g_stream_pos = 0;

void init_token_stream(token_t *stream, size_t len)
{
    g_stream      = stream;
    g_stream_len  = len;
    g_stream_pos  = 0;
}

/* get_token/push_token/free_token – nahrádzajú originály zo scanner.c */

token_ptr get_token(void)
{
    if (g_stream_pos < g_stream_len)
        return &g_stream[g_stream_pos++];

    /* keď dôjdeme na koniec, vraciame END_OF_FILE */
    static token_t eof_token;
    eof_token.type = END_OF_FILE;
    /* ostatné polia nás nezaujímajú */
    return &eof_token;
}

void push_token(token_ptr token)
{
    (void)token; /* token ignorujeme, len sa posunieme dozadu v streame */

    if (g_stream_pos > 0)
        g_stream_pos--;
}

/* POZOR: v testoch free_token neurobí nič, aby sme nefreeovali statické tokeny */
void free_token(token_ptr token)
{
    (void)token;
}

/* scanner_cleanup sa v testoch nepoužíva, ale musíme ho definovať */
void scanner_cleanup(void)
{
    /* nič */
}

/* ─────────────────────────── Helper makro pre testy ─────────────────── */

#define RUN_TEST(name, expr)                                  \
    do {                                                      \
        printf("[TEST] %s ... ", (name));                     \
        if ((expr))                                           \
            printf("OK\n");                                   \
        else                                                  \
            printf("FAIL\n");                                 \
    } while (0)

/* ─────────────────────────── Pomocné shortcuty ─────────────────────── */

/* Vytvorí token INT_LIT s danou hodnotou */
static token_t make_int(int value)
{
    token_t t;
    memset(&t, 0, sizeof(t));
    t.type = INT_LIT;
    t.value.int_value = value;
    return t;
}

/* Vytvorí token IDENT s daným identifikátorom */
static token_t make_ident(const char *name)
{
    token_t t;
    memset(&t, 0, sizeof(t));
    t.type = IDENT;
    t.value.str_value = (char *)name;   /* literál stačí */
    return t;
}

/* Vytvorí OPERATOR token s danou other_value (PLUS_V, MINUS_V, ...) */
static token_t make_op(enum other_value_type op)
{
    token_t t;
    memset(&t, 0, sizeof(t));
    t.type = OPERATOR;
    t.value.other_value = op;
    return t;
}

/* KEY_WORD token (napr. "return", "in", "is", "Ifj") */
static token_t make_kw(const char *kw)
{
    token_t t;
    memset(&t, 0, sizeof(t));
    t.type = KEY_WORD;
    t.value.str_value = (char *)kw;
    return t;
}

/* Dvojbodky .. alebo ... (CYCLES) */
static token_t make_ddot(void)
{
    token_t t;
    memset(&t, 0, sizeof(t));
    t.type = DOUBLE_DOT;
    return t;
}

static token_t make_tdot(void)
{
    token_t t;
    memset(&t, 0, sizeof(t));
    t.type = TRIPLE_DOT;
    return t;
}

/* END_OF_LINE token – ukončuje výraz v assignment/return kontexte */
static token_t make_eol(void)
{
    token_t t;
    memset(&t, 0, sizeof(t));
    t.type = END_OF_LINE;
    return t;
}

/* RIGHT_PAR – pre testy s LEFT_PAR kontextom / "in (...)" */
static token_t make_rpar(void)
{
    token_t t;
    memset(&t, 0, sizeof(t));
    t.type = RIGHT_PAR;
    return t;
}

/* NULL literal */
static token_t make_null_lit(void)
{
    token_t t;
    memset(&t, 0, sizeof(t));
    t.type = NULL_LIT;
    return t;
}

/* String literal (jednoriadkový) */
static token_t make_str(const char *s)
{
    token_t t;
    memset(&t, 0, sizeof(t));
    t.type = ONE_L_STRING;
    t.value.str_value = (char *)s;
    return t;
}

/* ─────────────────────────────── Konkrétne testy ────────────────────── */

/* 1) Jednoduchý aritmetický výraz v "obyčajnom" (NULL) kontexte:
 *    1 + 2 * 3
 */
static int test_simple_arith(void)
{
    token_t stream[] = {
        make_int(1),
        make_op(PLUS_V),
        make_int(2),
        make_op(STAR_V),
        make_int(3),
        make_eol()   /* delim, ktorý parse_expression(NULL) uvidí a pushne späť */
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    ASTNode_ptr ast = parse_expression(NULL);
    return (ast != NULL);  /* aspoň ne-NULL; hlbšie kontroly môžeš doplniť podľa ast.h */
}

/* 2) Typový operátor: a is null
 *    – testuje KEY_WORD "is" a NULL_LIT.
 */
static int test_is_operator(void)
{
    token_t stream[] = {
        make_ident("a"),
        make_kw("is"),
        make_null_lit(),
        make_eol()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    ASTNode_ptr ast = parse_expression(NULL);
    return (ast != NULL);
}

/* 3) Range s CYCLES: 1 .. 10 v kontexte "in" (for (... in 1 .. 10))
 *    recognition_token = KEY_WORD "in"
 */
static int test_range_in_cycles_inclusive(void)
{
    token_t stream[] = {
        make_int(1),
        make_ddot(),     /* .. */
        make_int(10),
        make_rpar()      /* parse_expression("in") zastaví pri ')' a pushne ju späť */
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    token_t rec_in = make_kw("in");
    ASTNode_ptr ast = parse_expression(&rec_in);
    return (ast != NULL);
}

/* 4) Range s ... (exkluzívny) – 1 ... 10 */
static int test_range_in_cycles_exclusive(void)
{
    token_t stream[] = {
        make_int(1),
        make_tdot(),     /* ... */
        make_int(10),
        make_rpar()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    token_t rec_in = make_kw("in");
    ASTNode_ptr ast = parse_expression(&rec_in);
    return (ast != NULL);
}


/* 7) Assignment kontext: x = 1 + 2
 *    – parse_expression sa volá s recognition_token '='
 */
static int test_assignment_context(void)
{
    token_t stream[] = {
        make_int(1),
        make_op(PLUS_V),
        make_int(2),
        make_eol()
    };
    
    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));
    
    token_t rec_eq;
    memset(&rec_eq, 0, sizeof(rec_eq));
    rec_eq.type = OPERATOR;
    rec_eq.value.other_value = EQUAL_SIGN_V;
    
    ASTNode_ptr ast = parse_expression(&rec_eq);
    return (ast != NULL);
}

/* 8) return kontext: return 1 + 2
 *    – parse_expression sa volá s recognition_token "return"
 */
static int test_return_context(void)
{
    token_t stream[] = {
        make_int(1),
        make_op(PLUS_V),
        make_int(2),
        make_eol()
    };
    
    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));
    
    token_t rec_ret = make_kw("return");
    
    ASTNode_ptr ast = parse_expression(&rec_ret);
    return (ast != NULL);
}

/* 9) if/while podmienka – LEFT_PAR kontext: (1 < 2)
 *   parse_expression sa volá s recognition_token = LEFT_PAR
 */
static int test_paren_condition_context(void)
{
    token_t stream[] = {
        make_int(1),
        make_op(LESS_THAN_V),
        make_int(2),
        make_rpar()
    };
    
    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));
    
    token_t rec_lpar;
    memset(&rec_lpar, 0, sizeof(rec_lpar));
    rec_lpar.type = LEFT_PAR;
    
    ASTNode_ptr ast = parse_expression(&rec_lpar);
    return (ast != NULL);
}

/* 5) FUNEXP – user volanie funkcie: foo(1, 2+3)
 *
 *    foo ( 1 , 2 + 3 )
 */
static int test_funexp_user_call(void)
{
    token_t stream[] = {
        make_ident("foo"),
        { .type = LEFT_PAR },
        make_int(1),
        { .type = COMMA },
        make_int(2),
        make_op(PLUS_V),
        make_int(3),
        make_rpar(),
        make_eol()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    ASTNode_ptr ast = parse_expression(NULL);
    return (ast != NULL);
}

/* FUNEXP – user volanie bez argumentov: foo() */
static int test_funexp_user_call_no_args(void)
{
    token_t stream[] = {
        make_ident("foo"),
        { .type = LEFT_PAR },
        make_rpar(),
        make_eol()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    ASTNode_ptr ast = parse_expression(NULL);
    return (ast != NULL);
}

/* FUNEXP – vnorené volanie: foo(bar(1), 2) */
static int test_funexp_user_call_nested(void)
{
    token_t stream[] = {
        make_ident("foo"),
        { .type = LEFT_PAR },
            make_ident("bar"),
            { .type = LEFT_PAR },
                make_int(1),
            make_rpar(),
            { .type = COMMA },
            make_int(2),
        make_rpar(),
        make_eol()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    ASTNode_ptr ast = parse_expression(NULL);
    return (ast != NULL);
}

/* FUNEXP – funkcia v rámci aritmetiky: 1 + foo(2, 3*4) */
static int test_funexp_user_call_in_arith(void)
{
    token_t stream[] = {
        make_int(1),
        make_op(PLUS_V),
        make_ident("foo"),
        { .type = LEFT_PAR },
            make_int(2),
            { .type = COMMA },
            make_int(3),
            make_op(STAR_V),
            make_int(4),
        make_rpar(),
        make_eol()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    ASTNode_ptr ast = parse_expression(NULL);
    return (ast != NULL);
}

/* 6) FUNEXP – built-in Ifj.read_str() bez argumentov */
static int test_funexp_builtin_call(void)
{
    token_t stream[] = {
        make_kw("Ifj"),     /* KEY_WORD "Ifj" */
        { .type = DOT },
        make_ident("read_str"),
        { .type = LEFT_PAR },
        make_rpar(),
        make_eol()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    ASTNode_ptr ast = parse_expression(NULL);
    return (ast != NULL);
}

/* FUNEXP – built-in Ifj.write("x") s jedným string argumentom */
static int test_funexp_builtin_call_with_string(void)
{
    token_t stream[] = {
        make_kw("Ifj"),
        { .type = DOT },
        make_ident("write"),
        { .type = LEFT_PAR },
        make_str("x"),
        make_rpar(),
        make_eol()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    ASTNode_ptr ast = parse_expression(NULL);
    return (ast != NULL);
}

/* FUNEXP – built-in Ifj.write(1+2) s aritmetickým argumentom */
static int test_funexp_builtin_call_with_expr_arg(void)
{
    token_t stream[] = {
        make_kw("Ifj"),
        { .type = DOT },
        make_ident("write"),
        { .type = LEFT_PAR },
        make_int(1),
        make_op(PLUS_V),
        make_int(2),
        make_rpar(),
        make_eol()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    ASTNode_ptr ast = parse_expression(NULL);
    return (ast != NULL);
}

/* 10) if podmienka – simulácia: if (1 < 2) { ... }
 *    parse_expression sa volá s recognition_token = LEFT_PAR
 */
static int test_if_condition_context(void)
{
    token_t stream[] = {
        make_int(1),
        make_op(LESS_THAN_V),
        make_int(2),
        make_rpar()   /* parser by mal ) pushnuť späť – tu ho dáme do streamu */
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    token_t rec_lpar;
    memset(&rec_lpar, 0, sizeof(rec_lpar));
    rec_lpar.type = LEFT_PAR;

    ASTNode_ptr ast = parse_expression(&rec_lpar);
    return (ast != NULL);
}

/* if podmienka – (a is null) */
static int test_if_condition_is_operator(void)
{
    token_t stream[] = {
        make_ident("a"),
        make_kw("is"),
        make_null_lit(),
        make_rpar()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    token_t rec_lpar;
    memset(&rec_lpar, 0, sizeof(rec_lpar));
    rec_lpar.type = LEFT_PAR;

    ASTNode_ptr ast = parse_expression(&rec_lpar);
    return (ast != NULL);
}

/* if podmienka – komplexná aritmetika: (1 + 2 * 3 < 4 * 5) */
static int test_if_condition_complex_arith(void)
{
    token_t stream[] = {
        make_int(1),
        make_op(PLUS_V),
        make_int(2),
        make_op(STAR_V),
        make_int(3),
        make_op(LESS_THAN_V),
        make_int(4),
        make_op(STAR_V),
        make_int(5),
        make_rpar()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    token_t rec_lpar;
    memset(&rec_lpar, 0, sizeof(rec_lpar));
    rec_lpar.type = LEFT_PAR;

    ASTNode_ptr ast = parse_expression(&rec_lpar);
    return (ast != NULL);
}

/* 11) while podmienka – simulácia: while (a is null) { ... }
 *     parse_expression sa volá s recognition_token = LEFT_PAR
 */
static int test_while_condition_context(void)
{
    token_t stream[] = {
        make_ident("a"),
        make_kw("is"),
        make_null_lit(),
        make_rpar()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    token_t rec_lpar;
    memset(&rec_lpar, 0, sizeof(rec_lpar));
    rec_lpar.type = LEFT_PAR;

    ASTNode_ptr ast = parse_expression(&rec_lpar);
    return (ast != NULL);
}

/* while podmienka – (a + 1 < b * 2) */
static int test_while_condition_complex_arith(void)
{
    token_t stream[] = {
        make_ident("a"),
        make_op(PLUS_V),
        make_int(1),
        make_op(LESS_THAN_V),
        make_ident("b"),
        make_op(STAR_V),
        make_int(2),
        make_rpar()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    token_t rec_lpar;
    memset(&rec_lpar, 0, sizeof(rec_lpar));
    rec_lpar.type = LEFT_PAR;

    ASTNode_ptr ast = parse_expression(&rec_lpar);
    return (ast != NULL);
}

/* 12) for-range kontext – simulácia: for (i in 1 .. 10) { ... }
 *     parse_expression sa volá s recognition_token = "in"
 */
static int test_for_in_range_context(void)
{
    token_t stream[] = {
        make_int(1),
        make_ddot(),    /* .. */
        make_int(10),
        make_rpar()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    token_t rec_in = make_kw("in");
    ASTNode_ptr ast = parse_expression(&rec_in);
    return (ast != NULL);
}

/* for-range kontext – exkluzívny: for (i in 1 ... 10) { ... } */
static int test_for_in_range_exclusive_context(void)
{
    token_t stream[] = {
        make_int(1),
        make_tdot(),    /* ... */
        make_int(10),
        make_rpar()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    token_t rec_in = make_kw("in");
    ASTNode_ptr ast = parse_expression(&rec_in);
    return (ast != NULL);
}

/* for-range kontext – výrazové hranice: for (i in 1+1 .. 10-1) { ... } */
static int test_for_in_range_expr_bounds(void)
{
    token_t stream[] = {
        make_int(1),
        make_op(PLUS_V),
        make_int(1),
        make_ddot(),        /* .. */
        make_int(10),
        make_op(MINUS_V),
        make_int(1),
        make_rpar()
    };

    init_token_stream(stream, sizeof(stream) / sizeof(stream[0]));

    token_t rec_in = make_kw("in");
    ASTNode_ptr ast = parse_expression(&rec_in);
    return (ast != NULL);
}

/* ───────────────────────────── main() – spúšťa testy ────────────────── */

int main(void)
{
    RUN_TEST("simple arithmetic: 1 + 2 * 3",                  test_simple_arith());
    RUN_TEST("is operator: a is null",                        test_is_operator());
    RUN_TEST("range in cycles: 1 .. 10 (in)",                 test_range_in_cycles_inclusive());
    RUN_TEST("range in cycles: 1 ... 10 (in)",                test_range_in_cycles_exclusive());

    RUN_TEST("assignment context: = 1 + 2",                   test_assignment_context());
    RUN_TEST("return context: return 1 + 2",                  test_return_context());

    RUN_TEST("paren condition: (1 < 2)",                      test_paren_condition_context());

    RUN_TEST("if condition context: if (1 < 2)",              test_if_condition_context());
    RUN_TEST("if condition with is: if (a is null)",          test_if_condition_is_operator());
    RUN_TEST("if condition complex: if (1 + 2 * 3 < 4 * 5)",  test_if_condition_complex_arith());

    RUN_TEST("while condition context: while (a is null)",    test_while_condition_context());
    RUN_TEST("while condition complex: while (a + 1 < b*2)",  test_while_condition_complex_arith());

    RUN_TEST("for range context: for (i in 1 .. 10)",         test_for_in_range_context());
    RUN_TEST("for range exclusive: for (i in 1 ... 10)",      test_for_in_range_exclusive_context());
    RUN_TEST("for range expr bounds: for (i in 1+1 .. 10-1)", test_for_in_range_expr_bounds());

    RUN_TEST("FUNEXP user call: foo(1, 2+3)",                 test_funexp_user_call());
    RUN_TEST("FUNEXP user call: foo()",                       test_funexp_user_call_no_args());
    RUN_TEST("FUNEXP user call: foo(bar(1), 2)",              test_funexp_user_call_nested());
    RUN_TEST("FUNEXP user call in expr: 1 + foo(2,3*4)",      test_funexp_user_call_in_arith());

    RUN_TEST("FUNEXP builtin call: Ifj.read_str()",           test_funexp_builtin_call());
    RUN_TEST("FUNEXP builtin call: Ifj.write(\"x\")",         test_funexp_builtin_call_with_string());
    RUN_TEST("FUNEXP builtin call: Ifj.write(1+2)",           test_funexp_builtin_call_with_expr_arg());

    return 0;
}
