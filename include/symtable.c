#include "symtable.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//**HELPER FUNCTION DECLARATIONS**//
char *str_duplicate(char *to_duplicate);
bool store_data(Node *node, void *data, Data_Type data_type);
static void Remove_Node_No_Children(Node *node);
static bool Remove_Node_Both_Children(Node *to_remove);
static void Node_Dispose(Node *node);
static Node *Find_Min_Node(Node *node);

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

    // determines what type of data to store and stores it inside the node
    if (!store_data(node, data, data_type))
        return NULL;

    return node;
}

/**
 * @brief Inserts a new Node.
 *
 * @param root_ptr Pointer to the root Node of a symtable.
 * @param key Name of the symbol.
 * @param data Pointer to data of unknown data type.
 * @param data_type Helps to determine what type of data is going to be stored inside the Node.
 *
 * @return Pointer to the inserted Node.
 */
Node *Insert_Node(Node *root_ptr, char *key, void *data, Data_Type data_type)
{
    if (!root_ptr) // new node is created when NULL is detected
    {
        return Create_Node(key, data, data_type);
    }
    else // cannot insert the node yet
    {
        int cmp = strcmp(key, root_ptr->key);

        if (cmp < 0) // go to left subtree
            root_ptr->left = Insert_Node(root_ptr->left, key, data, data_type);

        else if (cmp > 0) // go to right subtree
            root_ptr->right = Insert_Node(root_ptr->right, key, data, data_type);

        else                                            // node with the same key found
            if (!store_data(root_ptr, data, data_type)) // failed to store_data so NULL is returned
                return NULL;
    }
}

/**
 * @brief Removes an existing Node.
 *
 * @param root_ptr Pointer to the root Node of a symtable.
 * @param key Key that is used to locate the Node that will be removed.
 * 
 * @return Pointer to the (possibly new) root of the subtree after removal,
 *         or NULL if the subtree becomes empty or removal fails.
 */
Node *Remove_Node(Node *root_ptr, char *key)
{
    if (!root_ptr) // Attempt to remove non-existing Node is counted as success
    {
        return NULL;
    }

    // here we can try to look for the node to remove
    int cmp = strcmp(key, root_ptr->key);

    if (cmp < 0) // go to the left subtree
    {
        root_ptr->left = Remove_Node(root_ptr->left, key);
        return root_ptr;
    }
    else if (cmp > 0) // go to the right subtree
    {
        root_ptr->right = Remove_Node(root_ptr->right, key);
        return root_ptr;
    }
    else // node found
    {
        if (!root_ptr->right && !root_ptr->left) // Node has no children
        {
            Remove_Node_No_Children(root_ptr);
            return NULL;
        }
        else if (root_ptr->right && root_ptr->left) // Node has both children
        {
            // we need to know the removal_success value becaue copying data might fail here
            bool removal_success = Remove_Node_Both_Children(root_ptr);

            if (!removal_success)
                return NULL;

            return root_ptr;
        }
        else if (root_ptr->left && !root_ptr->right) // only left child present
        {
            Node *onlyChild = root_ptr->left;
            free_node(root_ptr); // tvoja funkcia, ktorá uvoľní pamäť Node (vrátane key a data)
            return onlyChild;
        }
        else if (!root_ptr->left && root_ptr->right) // only right child present
        {
            Node *onlyChild = root_ptr->right;
            free_node(root_ptr);
            return onlyChild;
        }
    }
}

/**
 * @brief Searches for a Node based on a provided key.
 *
 * @param root_ptr Pointer to the root Node of a symtable.
 * @param key Pointer to a key that is used to locate the Node.
 *
 * @return Pointer to a Node or NULL if no Node with corresponding key was found.
 */
Node *Search(Node *root_ptr, char *key)
{
    if (!root_ptr) // Node not found
    {
        return NULL;
    }
    else
    {
        int cmp = strcmp(key, root_ptr->key);

        if (cmp < 0) // go to the left subtree
            return Search(root_ptr->left, key);

        else if (cmp > 0) // go to the right subtree
            return Search(root_ptr->right, key);

        else // node found
            return root_ptr;
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

    // if allocation fails function returns NULL to signal Node creation failure
    if (!copy)
        return NULL;

    return strcpy(copy, to_duplicate);
}

/**
 * @brief Handles explicit typing and stores data inside the Node.
 *
 * @param node Pointer to a Node that will store the data.
 * @param data Pointer to data of unknown data type.
 * @param data_type Helps to determine what type of data is going to be stored inside the Node.
 *
 * @return False if storing the data fails.
 */
static bool store_data(Node *node, void *data, Data_Type data_type)
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
            return false;

        node->data.string_value = copy; // data inside the node points to the adress of the copy

    case FUNCTION:
        node->data.args_count = *(int *)data;
        break;

    default:
        break;
    }

    return true;
}

/**
 * @brief Finds the most right Node of the left subtree.
 *
 * @param root_ptr Root node of the subtree in which we want to find the min Node.
 * @note Root of the left subtree needs to be passed!
 *
 * @return Min Node pointer.
 */
static Node *Find_Min_Node(Node *node)
{
    if (!node->right) // no more right children
        return NULL;

    else
        return Find_Min_Node(node->right);
}

/**
 * @brief Helper function for the Remove_Node function that handles deleting a Node with no children.
 *
 * @param node Pointer to a node that will be removed.
 */
static void Remove_Node_No_Children(Node *node)
{
    free(node);
    node = NULL;
}

/**
 * @brief Helper function for the Remove_Node function that handles deleting a Node with both children present.
 *
 * @param to_remove Pointer to the Node we want to remove.
 * 
 * @return False if removal of the node fails, true otherwise.
 */
static bool Remove_Node_Both_Children(Node *to_remove)
{
    Node *min_node = Find_Min_Node(to_remove);

    // copy of the key is made
    char *key_copy = str_duplicate(min_node->key);

    // if str_duplicate fails function returns a NULL pointer to signal Node creation failure
    if (!key_copy)
        return false;

    // copies data from a terminal Node to the to_remove Node which effectively removed the Node we wanted to remove
    to_remove->key = key_copy;
    to_remove->data_type = min_node->data_type;

    bool store_data_successful = store_data(to_remove, &min_node->data, min_node->data_type);
    if (!store_data_successful)
        return false;

    // now that the data copied we can remove the terminal node
    Node_Dispose(min_node);

    return true;
}

/**
 * @brief Deallocates data inside the node and the node itself.
 * 
 * @param node Pointer to Node we want to clean up after.
 */
static void Node_Dispose(Node *node)
{
    free(node->key);
    node->key = NULL;

    free(node);
    node = NULL;
}