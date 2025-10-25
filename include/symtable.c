#include "symtable.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../error.h" // library with custom error handling

//**HELPER FUNCTION DECLARATIONS**//
static char *str_duplicate(char *to_duplicate);
static bool store_data(Node *node, void *data, Data_Type data_type);
static void Remove_Node_Both_Children(Node *to_remove);
static void Node_Dispose(Node *node);
static Node *Find_Max_Node(Node *node);
static Node *Left_Rotation(Node *root_ptr);
static Node *Right_Rotation(Node *root_ptr);
static void Tree_Height(Node *root_ptr, int *height);
static void Set_Balance_Factor(Node *node);

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
Node *Create_Node(char *name, int args_count, ID_Type id_type, void *data, Data_Type data_type)
{
    Node *node = malloc(sizeof(Node));

    if (!node)
        return NULL;

    // newly created node has no children
    node->left = NULL;
    node->right = NULL;

    // copy of the primary key (name) is made
    char *name_copy = str_duplicate(name);

    // if str_duplicate fails function returns a NULL pointer to signal Node creation failure
    if (!name_copy)
        return NULL;

    // node key struct init
    node->key.name = name_copy;
    node->key.args_count = args_count;
    node->key.id_type = id_type;

    // determines what type of data to store and stores it inside the node
    if (!store_data(node, data, data_type))
        error_exit(99);

    return node;
}

/**
 * @brief Inserts a new Node.
 *
 * @param root_ptr Pointer to the root Node of a symtable.
 * @param to_insert Pointer to a node we want to add.
 *
 * @return Pointer to the inserted Node.
 */
Node *Insert_Node(Node *root_ptr, Node *to_insert)
{
    if (!root_ptr) // new node is created when NULL is detected
    {
        return to_insert;
    }
    else // cannot insert the node yet
    {

        int key_cmp_result = key_cmp(&to_insert->key, &root_ptr->key);

        if (key_cmp_result < 0) // go to left subtree
            root_ptr->left = Insert_Node(root_ptr->left, to_insert);

        else if (key_cmp_result > 0) // go to right subtree
            root_ptr->left = Insert_Node(root_ptr->right, to_insert);

        else // attempt to add already existing symbol made
            error_exit(ERR_SEM_REDEFINITION);
        // NOTE: If you encounter this error when calling Insert, you have probably
        // forgotten to call Search(new sy) before trying to insert new symbol
    }

    Set_Balance_Factor(root_ptr);

    // RL case
    if (root_ptr->balance_factor < -1 && key_cmp(&to_insert->key, &root_ptr->right->key) < 0)
    {
        root_ptr->right = Right_Rotation(root_ptr->right);
        return Left_Rotation(root_ptr);
    }

    // LR case
    if (root_ptr->balance_factor > 1 && key_cmp(&to_insert->key, &root_ptr->left->key) > 0)
    {
        root_ptr->left = Left_Rotation(root_ptr->left);
        return Right_Rotation(root_ptr);
    }

    // RR case
    if (root_ptr->balance_factor < -1 && key_cmp(&to_insert->key, &root_ptr->right->key) > 0)
    {
        return Left_Rotation(root_ptr);
    }

    // LL case
    if (root_ptr->balance_factor > 1 && key_cmp(&to_insert->key, &root_ptr->left->key) < 0)
    {
        return Right_Rotation(root_ptr);
    }

    return root_ptr;
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
Node *Remove_Node(Node *root_ptr, Key *key)
{
    if (!root_ptr)
    {
        return NULL;
    }

    // here we can try to look for the node to remove
    int key_cmp_result = key_cmp(key, &root_ptr->key);

    if (key_cmp_result < 0) // go to the left subtree
    {
        root_ptr->left = Remove_Node(root_ptr->left, key);
        return root_ptr;
    }
    else if (key_cmp_result > 0) // go to the right subtree
    {
        root_ptr->right = Remove_Node(root_ptr->right, key);
        return root_ptr;
    }
    else // node found
    {
        if (!root_ptr->right && !root_ptr->left) // Node has no children
        {
            Node_Dispose(root_ptr);
            root_ptr = NULL;
            return root_ptr;
        }
        else if (root_ptr->right && root_ptr->left) // Node has both children
        {
            Remove_Node_Both_Children(root_ptr);
            return root_ptr;
        }
        else if (root_ptr->left && !root_ptr->right) // only left child present
        {
            Node *onlyChild = root_ptr->left;
            Node_Dispose(root_ptr);
            root_ptr = NULL;
            return onlyChild;
        }
        else // only right child present
        {
            Node *onlyChild = root_ptr->right;
            Node_Dispose(root_ptr);
            root_ptr = NULL;
            return onlyChild;
        }
    }

    if (root_ptr)
    {
        Set_Balance_Factor(root_ptr);

        // LL
        if (root_ptr->balance_factor > 1 && root_ptr->left->balance_factor >= 0)
            return Right_Rotation(root_ptr);

        // LR
        if (root_ptr->balance_factor > 1 && root_ptr->left->balance_factor < 0)
        {
            root_ptr->left = Left_Rotation(root_ptr->left);
            return Right_Rotation(root_ptr);
        }

        // RR
        if (root_ptr->balance_factor < -1 && root_ptr->right->balance_factor <= 0)
            return Left_Rotation(root_ptr);

        // RL
        if (root_ptr->balance_factor < -1 && root_ptr->right->balance_factor > 0)
        {
            root_ptr->right = Right_Rotation(root_ptr->right);
            return Left_Rotation(root_ptr);
        }
    }
}

/**
 * @brief Searches for a Node based on a provided key. Can be used to verify existance of a Node or to obtain a pointer to it's adress.
 *
 * @param root_ptr Pointer to the root Node of a symtable.
 * @param key Pointer to a key that is used to locate the Node.
 *
 * @return Pointer to a Node or NULL if no Node with corresponding key was found.
 */
Node *Search(Node *root_ptr, Key *key)
{
    if (!root_ptr) // Node not found
    {
        return NULL;
    }
    else
    {
        int key_cmp_result = key_cmp(key, &root_ptr->key);

        if (key_cmp_result < 0) // go to the left subtree
            return Search(root_ptr->left, key);

        else if (key_cmp_result > 0) // go to the right subtree
            return Search(root_ptr->right, key);

        else // node found
            return root_ptr;
    }
}

//**HELPER FUNCTIONS DEFINITIONS**//

/**
 * @brief Compares to provided Key structs. Start with primary key and ends with tertiary key
 *
 * @param key1 Pointer to first key.
 * @param key2 Pointer to second key.
 *
 * @return -1 if key1 < key2, 0 if key1 == key2, 1 if key1 > key2
 */
static int key_cmp(Key *key1, Key *key2)
{
    int name_cmp = strcmp(key1->name, key2->name);

    if (name_cmp < 0) // we compare names
        return -1;
    else if (name_cmp > 0)
        return 1;

    int args_count_cmp = key1->args_count - key2->args_count;

    if (args_count_cmp < 0) // we compare args_counts
        return -1;
    else if (args_count_cmp > 0)
        return 1;

    int id_type_cmp = key1->id_type - key2->id_type;

    if (id_type_cmp < 0) // we compare id_types
        return -1;
    else if (id_type_cmp > 0)
        return 1;

    return 0; // keys are identical
}

/**
 * @brief Finds the height of a tree using recursive calls.
 *
 * @param root_ptr Root node of the tree.
 *
 * @return Height of the tree.
 */
static void Tree_Height(Node *root_ptr, int *height)
{
    int height_l = 0, height_r = 0;

    if (root_ptr)
    {
        Tree_Height(root_ptr->left, &height_l);
        Tree_Height(root_ptr->right, &height_r);

        if (height_l > height_r)
            *height = height_l + 1;
        else
            *height = height_r + 1;
    }
    else
    {
        *height = 0;
    }
}

/**
 * @brief Using the Tree_height function this function finds Height of both subtrees of the passed node, and determines its balance factor.
 *
 * @param node Balance factor of this node will be set.
 */
static void Set_Balance_Factor(Node *node)
{
    if (!node)
        return;

    int left_subtree_height, right_subtree_height;

    // finds height of both subtrees
    Tree_Height(node->left, &left_subtree_height);
    Tree_Height(node->right, &right_subtree_height);

    node->balance_factor = left_subtree_height - right_subtree_height;
}

/**
 * @brief Performes Right_Rotation around the critical node (also called pivot node).
 *
 * @param root_ptr Critical node.
 *
 * @return Pointer to the new root node of the subtree that was rotated.
 */
Node *Right_Rotation(Node *root_ptr)
{
    Node *left_child = root_ptr->left; // left child will become the new root_node of the subtree
    Node *temp = left_child->right;    // right subtree of the left_child will be connected to current root_node->left

    left_child->right = root_ptr; // left_child now becomes the new root node
    root_ptr->left = temp;        // connects left ptr of the old root node to the right subtree of the new root node

    // balance factor is calculated again after rotation, only root_ptr and left_child should be effected by the rotation
    Set_Balance_Factor(root_ptr);
    Set_Balance_Factor(left_child);

    return left_child; // new root_node is always the left child of the former root_node
}

/**
 * @brief Performes Left_Rotation around the critical node (also called pivot node).
 *
 * @param root_ptr Critical node.
 *
 * @return Pointer to the new root node of the subtree that was rotated.
 */
Node *Left_Rotation(Node *root_ptr)
{
    Node *right_child = root_ptr->right; // right child will become the new root_node of the subtree
    Node *temp = right_child->left;      // left subtree of the right_child will be connected to current root_ptr->right

    right_child->left = root_ptr; // right_child now becomes the new root node
    root_ptr->right = temp;       // connects right ptr of the old root node to the right subtree of the new root node

    // balance factor is calculated again after rotation, only root_ptr and right_child should be effected by the rotation
    Set_Balance_Factor(root_ptr);
    Set_Balance_Factor(right_child);

    return right_child; // new root_node is always the right child of the former root_node
}

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
static Node *Find_Max_Node(Node *node)
{
    if (!node->right) // no more right children
        return node;
    else
        return Find_Min_Node(node->right);
}

/**
 * @brief Helper function for the Remove_Node function that handles deleting a Node with both children present.
 *
 * @param to_remove Pointer to a Node we want to remove.
 */
static void Remove_Node_Both_Children(Node *to_remove)
{
    Node *min_node = Find_Max_Node(to_remove->left);

    // copies data from a terminal Node to the to_remove Node
    to_remove->key = min_node->key;
    to_remove->data_type = min_node->data_type;

    if (!store_data(to_remove, &min_node->data, min_node->data_type))
        error_exit(99); // internal compiler error

    Node_Dispose(min_node);
    min_node = NULL;
}

/**
 * @brief Deallocates data inside the node and the node itself.
 *
 * @param node Pointer to Node we want to clean up after.
 */
static void Node_Dispose(Node *node)
{
    free(node->key.name);
    node->key.name = NULL;

    free(node);
}