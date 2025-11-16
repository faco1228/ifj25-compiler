/**
 * @file symtable.h
 * @author xmezeim00
 *
 * @brief Implementation of symtable using AVL binary tree.
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

// defining the ST_Node data type
typedef struct ST_Node ST_Node;

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

struct ST_Node
{
    Key key;             // contains information about the id that will help to differentiate between ids with the same name
    int balance_factor; // used to determine the balance of the Node's subtree
    ST_Node *left;      // left child pointer
    ST_Node *right;     // right child pointer
};

//**FUNCTION FOR CREATING KEYS**//

/**
 * @brief Used for creating function, setter and getter keys.
 * 
 * @param name
 * @param args_count 
 * @param id_type Can be SETTER, GETTER or FUNCTION
 * 
 * @return Pointer to a new key.
 */
Key *create_function_key(char *name, int args_count, ID_Type id_type);

/**
 * @brief Used for creating local and global var keys.
 *
 * @param name
 * 
 * @return Pointer to a new key.
 */
Key *create_variable_key(char *name);


/**
 * @brief Creates a new instance of a Variable_Node and initializes it's attributes.
 *
 * @param key Key of the new node. Needs to be created before using create_function_key or create_variable_key functions
 *
 * @return New Variable_Node.
 */
ST_Node *create_node(Key *key);

/**
 * @brief Inserts a new ST_Node.
 *
 * @param root_ptr Pointer to the root ST_Node of a symtable.
 * @param to_insert Pointer to a node we want to insert.
 *
 * @return Pointer to the root of the (possibly rebalanced) subtree.
 */
ST_Node *insert_node(ST_Node *root_ptr, ST_Node *to_insert);

/**
 * @brief Removes an existing Node.
 *
 * @param root_ptr Pointer to the root Node of a symtable.
 * @param key Key that is used to locate the Node that will be removed.
 *
 * @return Pointer to the (possibly new) root of the subtree after removal,
 *         or NULL if the subtree becomes empty or removal fails.
 */
ST_Node *remove_node(ST_Node *root_ptr, Key *key);

/**
 * @brief Searches for a ST_Node based on a provided key. Can be used to verify existance of a ST_Node or to obtain a pointer to it's adress.
 *
 * @param root_ptr Pointer to the root ST_Node of a symtable.
 * @param key Pointer to a key that is used to locate the ST_Node.
 *
 * @return Pointer to a ST_Node or NULL if no ST_Node with corresponding key was found.
 */
ST_Node *search(ST_Node *root_ptr, Key *key);

/**
 * @brief Recursively disposes of all nodes in the tree using PostOrder tree traversal.
 *
 * @param root_ptr Root of the tree/subtree to dispose.
 */
void dispose_tree(ST_Node *root_ptr);

#endif