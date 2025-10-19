#include "symtable.h"
#include <stdlib.h>
#include <string.h>

//**HELPER FUNCTION DECLARATIONS**//
char *str_duplicate(char *to_duplicate);
void store_data(Node *node, void *data, Data_Type data_type);

/**
 * @brief Creates a new instance of a Variable_Node and initializes it's attributes.
 *
 * @param key Name of the symbol that is stored inside the Node.
 * @param data Pointer to data of unknown data type.
 * @param data_type Helps to determine what type of data is going to be stored inside the Node.
 *
 * @return New Variable_Node.
 *
 * @note Data can store nums, strings or function args depending on the type of symbol.
 */
Node *Create_Node(char *key, void *data, Data_Type data_type)
{
    Node *node = malloc(sizeof(Node));

    if (!node)
        return NULL;

    // newly created node has no children
    node->left = NULL;
    node->right = NULL;

    // copy of the key is made
    char *copy = str_duplicate(key);

    // if str_duplicate fails function returns a NULL pointer to signal Node creation failure
    if (!copy)
        return NULL;

    node->key = copy;

    store_data(node, data, data_type);

    return node;
}

/**
 * @brief Inserts a new Node.
 * 
 * @param root_ptr Pointer to the root Node of a symtable.
 * @param key Name of the symbol.
 * @param data Pointer to data of unknown data type.
 * @param data_type Helps to determine what type of data is going to be stored inside the Node.
 */
Node *Insert_Node(Node *root_ptr, char *key, void *data, Data_Type data_type)
{
    if (!root_ptr) // new node is created when NULL is detected
    {
        return Create_Node(key, data, data_type);
    }
    else // cannot insert the node yet
    {
        if (strcmp(key, root_ptr->key) < 0) // go to left subtree
        {
            root_ptr->left = Insert_Node(root_ptr->left, key, data, data_type);
        }
        else if (strcmp(key, root_ptr->key) > 0) // go to right subtree
        {
            root_ptr->right = Insert_Node(root_ptr->right, key, data, data_type);
        }
        else // node with the same key found
        {
            // data inside the node will just be overwritten
            store_data(root_ptr, data, data_type);
        }
    }
}

//**HELPER FUNCTIONS DEFINITIONS**//
/**
 * @brief Duplicates string to a new adress
 *
 * @param to_duplicate String to duplicate.
 * @return Pointer to the copy.
 */
static char *str_duplicate(char *to_duplicate)
{
    // memory is allocated to store a copy of the passed string
    char *copy = malloc(strlen(to_duplicate) + 1);

    strcpy(copy, to_duplicate);

    // if allocation fails function returns NULL to signal Node creation failure
    if (!copy)
        return NULL;

    return copy;
}

/**
 * @brief Handles explicit typing and stores data inside the Node.
 *
 * @param node Pointer to a Node that will store the data.
 * @param data Pointer to data of unknown data type.
 * @param data_type Helps to determine what type of data is going to be stored inside the Node.
 */
static void store_data(Node *node, void *data, Data_Type data_type)
{
    switch (data_type)
    {
    case INT:
        node->data.int_value = *(int *)data;
        break;

    case FLOAT:
        node->data.float_value = *(float *)data;
        break;

    case STRING:
        // copy of the string is made
        char *copy = str_duplicate((char *)data);

        // if str_duplicate fails function returns a NULL pointer to signal Node creation failure
        if (!copy)
            return NULL;

        node->data.string_value = copy; // data inside the node points to the adress of the copy

    case FUNCTION:
        node->data.args_count = *(int *)data;
        break;

    default:
        break;
    }
}