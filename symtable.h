/**
 * @file symtable.h
 * @author Martin Mezei (xmezeim00)
 *
 * @brief Implementation of symtable using AVL binary tree.
 * 
 * @version 0.1
 * @date 2025-11-17
 *
 * @copyright Copyright (c) 2025
 */

#ifndef _SYMTABLE_H_
#define _SYMTABLE_H_

typedef enum ID_Types
{
    FUNCTION,
    SETTER,
    GETTER,
    VAR // no need to differentiate between local and global vars, because they will be stored inside different symtables at all times
} ID_Type;

// composite key to describe identifiers
typedef struct
{
    char *name;      // name of the identificator, primary key
    int args_count;  // args_count >= 0 for functions, args_count == -1 for variables, seconadary key
    ID_Type id_type; // used to differentiate between function, setter, getters, etc. which share the same name, tertiary key
} Key;

// defining the ST_Node data type
typedef struct ST_Node ST_Node;

// NOTE: There was no time to do a major redesign of the ST_Node structure, so some data can be unused in certain cases, such as arg_count with variables or
// block_id with functions and global variables. It does not cause any problems when it comes to the logic it just stores redundant data sometimes.

struct ST_Node
{
    Key key;            // contains information about the id that will help to differentiate between ids with the same name
    int balance_factor; // used to determine the balance of the Node's subtree
    ST_Node *left;      // left child pointer
    ST_Node *right;     // right child pointer

    unsigned block_id; // stores an id of the block to which the variable belongs to
};

//**FUNCTION FOR CREATING KEYS**//

/**
 * @brief Deallocates key struct and it's data.
 *
 * @param key Key to dispose.
 */
void key_dispose(Key *key);

/**
 * @brief Used for creating function, setter and getter keys.
 *
 * @param name
 * @param args_count
 * @param id_type Can be SETTER, GETTER or FUNCTION
 *
 * @return Pointer to a new key.
 */
Key *st_create_function_key(char *name, int args_count, ID_Type id_type);

/**
 * @brief Used for creating local and global var keys.
 *
 * @param name
 *
 * @return Pointer to a new key.
 */
Key *st_create_variable_key(char *name);

/**
 * @brief Creates a new instance of a Variable_Node and initializes it's attributes.
 *
 * @param key Key of the new node. Needs to be created before using st_create_function_key or st_create_variable_key functions.
 * @param block_id Block id is used to determined in which block exactly was a local variable declared.
 *
 * @return New Variable_Node.
 */
ST_Node *st_create_node(Key *key, unsigned block_id);

/**
 * @brief Inserts a new ST_Node.
 *
 * @param root_ptr Pointer to the root ST_Node of a symtable.
 * @param to_insert Pointer to a node we want to insert.
 *
 * @return Pointer to the root of the (possibly rebalanced) subtree.
 */
ST_Node *st_insert_node(ST_Node *root_ptr, ST_Node *to_insert);


/**
 * @brief Searches for a ST_Node based on a provided key. Can be used to verify existance of a ST_Node or to obtain a pointer to it's adress.
 *
 * @param root_ptr Pointer to the root ST_Node of a symtable.
 * @param key Pointer to a key that is used to locate the ST_Node.
 *
 * @return Pointer to a ST_Node or NULL if no ST_Node with corresponding key was found.
 */
ST_Node *st_search(ST_Node *root_ptr, Key *key);

/**
 * @brief Recursively disposes of all nodes in the tree using PostOrder tree traversal.
 *
 * @param root_ptr Root of the tree/subtree to dispose.
 */
void st_dispose_tree(ST_Node *root_ptr);

#endif
