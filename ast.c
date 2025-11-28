/**
 * @file ast.c
 * @author Samuel Facka (xfackas00)
 * @brief 
 * @version 0.1
 * @date 2025-11-10
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "ast.h"
#include "global_structures.h"


// definition of helper functions
static void *ast_malloc(size_t size);
static void *ast_realloc(void *p, size_t size);
static char *ast_strcpy(const char *str);


// implementation of core functions
ASTNode_ptr ast_create(NodeType type) {
    // allocate mem for new ast node
    ASTNode_ptr node = (ASTNode_ptr)ast_malloc(sizeof(ASTNode_t));

    // make sure, whole struct is clear
    memset(node, 0, sizeof(ASTNode_t));

    // set the values for type, num of children and pointer
    node->type = type;
    node->child_count = 0;
    node->children = NULL;

    return node;
}

void add_child(ASTNode_ptr parent, ASTNode_ptr child) {
    if (parent == NULL || child == NULL) return;

    // increment number of children
    size_t new_child_count = parent->child_count + 1; 

    // reallocate array of children to new_child_count 
    parent->children = (ASTNode_ptr*)ast_realloc(parent->children, new_child_count*sizeof(ASTNode_ptr));
    
    // new child is stored at the index of the previous child count and update the child count
    parent->children[parent->child_count] = child;
    parent->child_count = new_child_count;
}

void ast_free(ASTNode_ptr node) {
    if (node == NULL) return;

    NodeType type = node->type;

    // free every child
    for (size_t i = 0; i < node->child_count; i++) {
        ast_free(node->children[i]);
    }
    free(node->children);

    // free allocated strings
    if (type == NODE_IDENTIFIER || type == NODE_VAR_DECL) {
        free(node->data.identifier.name);
    }
    else if (type == NODE_FUNCTION_DEF) {
        free(node->data.function_def.name);
    }
    else if (type == NODE_FOR) {
        free(node->data.for_statement.name_iter);
    }
    else if (type == NODE_STR_LIT || type == NODE_TYPE_LIT) {
        free(node->data.literal.data.str_value);
    }
    else if (type == NODE_CALL) {
        free(node->data.function_call.name);
    }

    // free the node
    free(node);
}


// implementation of builders functions
ASTNode_ptr ast_create_program(void) {
    return ast_create(NODE_PROGRAM);
}

ASTNode_ptr ast_create_function(const char *name, unsigned args, function_type type, ASTNode_ptr body) {
   ASTNode_ptr new_node = ast_create(NODE_FUNCTION_DEF);
   
   new_node->data.function_def.name = ast_strcpy(name);
   new_node->data.function_def.arg_count = args;
   new_node->data.function_def.type = type;

   add_child(new_node, body);

   return new_node;
}

ASTNode_ptr ast_create_block(void) {
    return ast_create(NODE_BLOCK);
}

ASTNode_ptr ast_create_var_dec(const char *name) {
    ASTNode_ptr new_node = ast_create(NODE_VAR_DECL);

    new_node->data.identifier.name = ast_strcpy(name);
    new_node->data.identifier.id_type = VAR; // todo ID_UNDEFINED ??

    return new_node;
}

ASTNode_ptr ast_create_assignment(ASTNode_ptr lhs, ASTNode_ptr rhs) {
    ASTNode_ptr new_node = ast_create(NODE_ASSIGN);

    add_child(new_node, lhs);
    add_child(new_node, rhs);

    return new_node;
}

ASTNode_ptr ast_create_if(ASTNode_ptr cond, ASTNode_ptr b_then, ASTNode_ptr b_else) {
    ASTNode_ptr new_node = ast_create(NODE_IF);

    add_child(new_node, cond);
    add_child(new_node, b_then);
    
    // else blok is necesarry
    if (b_else != NULL) {
        add_child(new_node, b_else);
    }

    return new_node;
}

ASTNode_ptr ast_create_return(ASTNode_ptr val) {
    ASTNode_ptr new_node = ast_create(NODE_RETURN);

    if (val != NULL) {
        add_child(new_node, val);
    }

    return new_node;
}

ASTNode_ptr ast_create_while(ASTNode_ptr cond, ASTNode_ptr body) {
    ASTNode_ptr new_node = ast_create(NODE_WHILE);

    add_child(new_node, cond);
    add_child(new_node, body);

    return new_node;
}

ASTNode_ptr ast_create_for(const char *name, ASTNode_ptr iter, ASTNode_ptr body) {
    ASTNode_ptr new_node = ast_create(NODE_FOR);

    new_node->data.for_statement.name_iter = ast_strcpy(name);

    ASTNode_ptr iter_ident = ast_create_ident(name);
    add_child(new_node, iter_ident);

    add_child(new_node, iter);
    add_child(new_node, body);

    return new_node;
}

ASTNode_ptr ast_create_break(void) {
    return ast_create(NODE_BREAK);
}

ASTNode_ptr ast_create_continue(void) {
    return ast_create(NODE_CONTINUE);
}

ASTNode_ptr ast_create_exp_statement(ASTNode_ptr exp) {
    ASTNode_ptr new_node = ast_create(NODE_EXPR_STMNT);

    add_child(new_node, exp);

    new_node->data.exp_statement.exp_type = TYPE_UNKNOWN;

    return new_node;
}

ASTNode_ptr ast_create_ident(const char *name) {
    ASTNode_ptr new_node = ast_create(NODE_IDENTIFIER);

    new_node->data.identifier.name = ast_strcpy(name);
    new_node->data.identifier.id_type = VAR; // todo ID_UNDEFINED ??

    return new_node;
}


// implementation of builders functions for PSA

ASTNode_ptr ast_create_binary(ASTNode_ptr lhs, ASTNode_ptr rhs, operator_types op) {
    ASTNode_ptr new_node = ast_create(NODE_BINARY_OP);

    add_child(new_node, lhs);
    add_child(new_node, rhs);

    new_node->data.binary_operator.op_type = op;

    return new_node;
}

ASTNode_ptr ast_create_unary(operator_types op, ASTNode_ptr exp) {
    ASTNode_ptr new_node = ast_create(NODE_UNARY_OP);

    new_node->data.unary_operator.op_type = op;

    add_child(new_node, exp);

    return new_node;
}

ASTNode_ptr ast_create_call(const char *name, unsigned param_c, bool builtin) {
    ASTNode_ptr new_node = ast_create(NODE_CALL);

    new_node->data.function_call.name = ast_strcpy(name);
    new_node->data.function_call.param_count = param_c;
    new_node->data.function_call.is_builtin = builtin;

    return new_node;
}

ASTNode_ptr ast_create_ternary(ASTNode_ptr cond, ASTNode_ptr b_then, ASTNode_ptr b_else) {
    ASTNode_ptr new_node = ast_create(NODE_TERNARY);

    add_child(new_node, cond);
    add_child(new_node, b_then);
    add_child(new_node, b_else);

    return new_node;
}

ASTNode_ptr ast_create_range(ASTNode_ptr l, ASTNode_ptr r, bool inclusive) {
    ASTNode_ptr new_node = ast_create(NODE_RANGE);

    add_child(new_node, l);
    add_child(new_node, r);

    new_node->data.range.inclusive = inclusive;

    return new_node;   
}

ASTNode_ptr ast_create_int(long long int val) {
    ASTNode_ptr new_node = ast_create(NODE_INT_LIT);

    new_node->data.literal.data.int_val = val;

    return new_node;
}

ASTNode_ptr ast_create_float(long double val) {
    ASTNode_ptr new_node = ast_create(NODE_FLOAT_LIT);

    new_node->data.literal.data.float_val = val;

    return new_node;
}

ASTNode_ptr ast_create_str(const char *string) {
    ASTNode_ptr new_node = ast_create(NODE_STR_LIT);

    new_node->data.literal.data.str_value = ast_strcpy(string);

    return new_node;
}

ASTNode_ptr ast_create_null(void) {
    return ast_create(NODE_NULL_LIT);
}

ASTNode_ptr ast_create_type_lit(const char *type_name) {
    ASTNode_ptr new_node = ast_create(NODE_TYPE_LIT);

    new_node->data.literal.data.str_value = ast_strcpy(type_name);

    return new_node;
}


//////////////////////
// helper functions //
//////////////////////
static void *ast_malloc(size_t size) {
    void *p = malloc(size);

    if(p == NULL) error_exit(ERR_INTERNAL);
    return p;
}

static void *ast_realloc(void *p, size_t size) {
    void *new_p = realloc(p, size);

    if(new_p == NULL) error_exit(ERR_INTERNAL);
    return new_p;
} 

static char *ast_strcpy(const char *str) {
    if (str == NULL) return NULL;

    size_t l = strlen(str) + 1;
    char *cpy = (char*)ast_malloc(l);
    memcpy(cpy, str, l);

    return cpy;
}