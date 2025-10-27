/**
 * @file symtable.h
 * @author xmezeim00
 *
 * @brief Implementation of symtable using AVL binary tree.
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

// Purpose of Data_Type code is to determine what information about the symbol should stored
typedef enum Data_Types
{
    INT,
    FLOAT,
    STRING
} Data_Type;

typedef enum ID_Types
{
    FUNCTION,
    SETTER,
    GETTER,
    GLOBAL_VAR,
    LOCAL_VAR
} ID_Type;

// creates a composite key to describe identifiers
// primary key - name, secondary key - args_count
typedef struct
{
    char *name;      // name of the identificator, primary key
    int args_count;  // args_count >= 0 for functions, args_count == -1 for variables, seconadary key
    ID_Type id_type; // used to differentiate between function, setter, getters, etc. which share the same name, tertiary key
} Key;

typedef struct
{
    Key key;             // contains infromation about the id that will help to differentiate between ids with the same name
    Data_Type data_type; // helps identifying what kind of data is stored inside the node
    union                // used to store different types of data inside the node
    {
        int int_value;
        float float_value;
        char *string_value;
    } data;
    int balance_factor; // used to determine the balance of the Node's subtree
    ST_Node *left;         // left child pointer
    ST_Node *right;        // right child pointer
} ST_Node;



/**
 * @brief Creates a new instance of a Variable_Node and initializes it's attributes.
 *
 * @param name Name of the symbol that is stored inside the Node.
 * @param args_count Num of arguments of the current symbol. -1 for for global and local variables, non-negative int for others.
 * @param id_type Type of the currently passed identifier.
 * @param data Pointer to data of unknown type.
 * @param data_type Helps to determine what type of data is going to be stored inside the Node.
 *
 * @return New Variable_Node.
 *
 * @note Data can store nums, strings or function args depending on the type of symbol.
 */
ST_Node *Create_Node(char *name, int args_count, ID_Type id_type, void *data, Data_Type data_type);

/**
 * @brief Inserts a new Node.
 *
 * @param root_ptr Pointer to the root Node of a symtable.
 * @param to_insert Pointer to a node we want to add.
 *
 * @return Pointer to the inserted Node.
 */
ST_Node *Insert_Node(ST_Node *root_ptr, ST_Node *to_insert);

/**
 * @brief Removes an existing Node.
 *
 * @param root_ptr Pointer to the root Node of a symtable.
 * @param key Key that is used to locate the Node that will be removed.
 *
 * @return Pointer to the (possibly new) root of the subtree after removal,
 *         or NULL if the subtree becomes empty or removal fails.
 */
ST_Node *Remove_Node(ST_Node *root_ptr, Key *key);

/**
 * @brief Searches for a Node based on a provided key. Can be used to verify existance of a Node or to obtain a pointer to it's adress.
 *
 * @param root_ptr Pointer to the root Node of a symtable.
 * @param key Pointer to a key that is used to locate the Node.
 *
 * @return Pointer to a Node or NULL if no Node with corresponding key was found.
 */
ST_Node *Search(ST_Node *root_ptr, Key *key);

/**
 * @brief Recursively disposes of all nodes in the tree.
 *
 * @param root_ptr Root of the tree/subtree to dispose.
 */
void Dispose_Tree(ST_Node *root_ptr);

#endif