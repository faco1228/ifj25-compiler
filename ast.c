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


// definition of helper functions
static void *ast_malloc(size_t size);
static void *ast_realloc(void *p, size_t size);
static char *ast_strcpy(const char *str);


// implementation of core functions
ASTNode_ptr ast_create(NodeType type) {
    // allocate mem for new ast node
    ASTNode_ptr node = (ASTNode_ptr)ast_malloc(sizeof(ASTNode));

    // make sure, whole struct is clear
    memset(node, 0, sizeof(ASTNode));

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

    // free union child pointers
    if (type == NODE_ASSIGN) {
        ast_free(node->data.assign.lhs);
        ast_free(node->data.assign.rhs);
    } 
    else if (type == NODE_IF) {
        ast_free(node->data.if_statement.condition);
        ast_free(node->data.if_statement.block_then);
        ast_free(node->data.if_statement.block_else);
    }
    else if (type == NODE_WHILE) {
        ast_free(node->data.while_statement.condition);
        ast_free(node->data.while_statement.body);
    }
    else if (type == NODE_FOR) {
        ast_free(node->data.for_statement.expr_iter);
        ast_free(node->data.for_statement.body);
    }
    else if (type == NODE_RETURN) {
        ast_free(node->data.ret.value);
    }
    else if (type == NODE_EXPR_STMNT) {
        ast_free(node->data.exp_statement.exp);
    }
    else if (type == NODE_BINARY_OP) {
        ast_free(node->data.binary_operator.lhs);
        ast_free(node->data.binary_operator.rhs);
    } 
    else if (type == NODE_UNARY_OP) {
        ast_free(node->data.unary_operator.expres);
    }
    else if (type == NODE_RANGE) {
        ast_free(node->data.range.start);
        ast_free(node->data.range.stop);
    }
    else if (type == NODE_TERNARY) {
        ast_free(node->data.ternary.condition);
        ast_free(node->data.ternary.expr_then);
        ast_free(node->data.ternary.expr_else);
    }
    else if (type == NODE_FUNCTION_DEF) {
        ast_free(node->data.function_def.body);
    }

    // free all chilren[]
    for (size_t i = 0; i < node->child_count; i++) {
        ast_free(node->children[i]);
    }
    free(node->children);

    // free strings stored in union
    if (type == NODE_IDENTIFIER || type == NODE_VAR_DECL) {
        free(node->data.identifier.name);
    }
    else if (type == NODE_FUNCTION_DEF) {
        free(node->data.function_def.name);
    }
    else if (type == NODE_FOR) {
        free(node->data.for_statement.name_iter);
    }
    else if (type == NODE_STR_LIT) {
        free(node->data.literal.str_value);
    }
    else if (type == NODE_CALL) {
        free(node->data.function_call.name);
    }

    free(node);
}


// implementation oif walk-through functions

// void ast_walk(ASTNode_ptr root) {

// } 


// implementation of builders functions
ASTNode_ptr ast_create_program(void) {
    return ast_create(NODE_PROGRAM);
}

ASTNode_ptr ast_create_function(const char *name, unsigned args, function_type type, ASTNode_ptr body) {
   ASTNode_ptr new_node = ast_create(NODE_FUNCTION_DEF);
   
   new_node->data.function_def.name = ast_strcpy(name);
   new_node->data.function_def.arg_count = args;
   new_node->data.function_def.type = type;
   new_node->data.function_def.body = body;

   return new_node;
}

ASTNode_ptr ast_create_block(void) {
    return ast_create(NODE_BLOCK);
}

ASTNode_ptr ast_create_var_dec(const char *name) {
    ASTNode_ptr new_node = ast_create(NODE_VAR_DECL);

    new_node->data.identifier.name = ast_strcpy(name);

    return new_node;
}

ASTNode_ptr ast_create_assignment(ASTNode_ptr lhs, ASTNode_ptr rhs) {
    ASTNode_ptr new_node = ast_create(NODE_ASSIGN);

    new_node->data.assign.lhs = lhs;
    new_node->data.assign.rhs = rhs;

    return new_node;
}

ASTNode_ptr ast_create_if(ASTNode_ptr cond, ASTNode_ptr b_then, ASTNode_ptr b_else) {
    ASTNode_ptr new_node = ast_create(NODE_IF);

    new_node->data.if_statement.condition = cond;
    new_node->data.if_statement.block_then = b_then;
    new_node->data.if_statement.block_else = b_else;

    return new_node;
}

ASTNode_ptr ast_create_return(ASTNode_ptr val) {
    ASTNode_ptr new_node = ast_create(NODE_RETURN);

    new_node->data.ret.value = val;

    return new_node;
}

ASTNode_ptr ast_create_while(ASTNode_ptr cond, ASTNode_ptr body) {
    ASTNode_ptr new_node = ast_create(NODE_WHILE);

    new_node->data.while_statement.condition = cond;
    new_node->data.while_statement.body = body;

    return new_node;
}

ASTNode_ptr ast_create_for(const char *name, ASTNode_ptr iter, ASTNode_ptr body) {
    ASTNode_ptr new_node = ast_create(NODE_FOR);

    new_node->data.for_statement.name_iter = ast_strcpy(name);
    new_node->data.for_statement.expr_iter = iter;
    new_node->data.for_statement.body = body;

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

    new_node->data.exp_statement.exp = exp;
    new_node->data.exp_statement.result_type = TYPE_UNKNOWN;

    return new_node;
}

ASTNode_ptr ast_create_ident(const char *name) {
    ASTNode_ptr new_node = ast_create(NODE_IDENTIFIER);

    new_node->data.identifier.name = ast_strcpy(name);

    return new_node;
}


// implementation of builders functions for PSA

ASTNode_ptr ast_create_binary(ASTNode_ptr lhs, ASTNode_ptr rhs, operator_types op) {
    ASTNode_ptr new_node = ast_create(NODE_BINARY_OP);

    new_node->data.binary_operator.lhs = lhs;
    new_node->data.binary_operator.rhs = rhs;  
    new_node->data.binary_operator.op_type = op;

    return new_node;
}

ASTNode_ptr ast_create_unary(operator_types op, ASTNode_ptr exp) {
    ASTNode_ptr new_node = ast_create(NODE_UNARY_OP);

    new_node->data.unary_operator.op_type = op;
    new_node->data.unary_operator.expres = exp;

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

    new_node->data.ternary.condition = cond;
    new_node->data.ternary.expr_then = b_then;
    new_node->data.ternary.expr_else = b_else;

    return new_node;
}

ASTNode_ptr ast_create_range(ASTNode_ptr l, ASTNode_ptr r, bool inclusive) {
    ASTNode_ptr new_node = ast_create(NODE_RANGE);

    new_node->data.range.start = l;
    new_node->data.range.stop = r;
    new_node->data.range.inclusive = inclusive;

    return new_node;   
}

ASTNode_ptr ast_create_int(long long int val) {
    ASTNode_ptr new_node = ast_create(NODE_INT_LIT);

    new_node->data.literal.int_val = val;

    return new_node;
}

ASTNode_ptr ast_create_float(long double val) {
    ASTNode_ptr new_node = ast_create(NODE_FLOAT_LIT);

    new_node->data.literal.float_val = val;

    return new_node;
}

ASTNode_ptr ast_create_str(const char *string) {
    ASTNode_ptr new_node = ast_create(NODE_STR_LIT);

    new_node->data.literal.str_value = ast_strcpy(string);

    return new_node;
}

ASTNode_ptr ast_create_null(void) {
    return ast_create(NODE_NULL_LIT);
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