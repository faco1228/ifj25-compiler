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
    STRING,
    FUNCTION
} Data_Type;

typedef struct BinarySearchTree
{
    Node *root_ptr;

} Symtable;

typedef struct Node
{
    char *key; // name of the identificator
    Data_Type data_type; // helps identifying what kind of data is stored inside the node
    union // used to store different types of data inside the node 
    {
        int int_value;
        float float_value;
        char *string_value;
        int args_count;
    } data;
    int balance_factor; // used to determine the balance of the Node's subtree
    Node *left;         // left child pointer
    Node *right;        // right child pointer
} Node;

/**
 * @brief Creates a new instance of a Variable_Node and initializes it's attributes.
 *
 * @param key Name of the symbol that is stored inside the Node.
 * @param data Data pointer stored inside the Node. 
 * @param data_type Data type code to determine what type of data is stored.
 *
 * @return New Node.
 */
Node *Create_Node(char *key, void *data, Data_Type data_type);

/**
 * @brief Inserts a new Node.
 *
 * @param symtable Pointer to a Symtable.
 * @param new_node Pointer to the Node that will be added.
 */
Node *Insert_Node(Node *root_ptr, char *key, void *data, Data_Type data_type);

/**
 * @brief Removes an existing Node.
 *
 * @param symtable Pointer to a Symtable.
 * @param key Pointer to a key that is used to locate the Node that wil be removed.
 */
void Remove_Node(Node *root_ptr, char *key);

/**
 * @brief Searches for a Node based on a provided key.
 *
 * @param symtable Pointer to a Symtable.
 * @param key Pointer to a key that is used to locate the Node.
 *
 * @return Pointer to a Node or NULL if no Node with corresponding key was found.
 */
Node *Search(Node *root_ptr, char *key);

/**
 * @brief Finds the height of the left subtree of the passed Node.
 *
 * @param node_ptr Node of which subtree height we want to find.
 * @param value_ptr Height of the subtree will be stored at the adress of this pointer.
 *
 */
void Left_Subtree_Height(Node *node_ptr, int *value_ptr);

/**
 * @brief Finds the height of the right subtree of the passed Node.
 *
 * @param node_ptr Node of which subtree height we want to find.
 * @param value_ptr Height of the subtree will be stored at the adress of this pointer.
 *
 */
void Right_Subtree_Height(Node *node_ptr, int *value_ptr);

#endif