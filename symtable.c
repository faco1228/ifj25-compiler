/**
 * @file symtable.c
 * @author xmezeim00
 * @brief Implementation of symtable using AVL binary tree.
 * @version 0.1
 * @date 2025-11-17
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "symtable.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "error.h" // library with custom error handling

//**HELPER FUNCTION PROTOTYPES**//
static char *str_duplicate(char *to_duplicate);
static void node_dispose(ST_Node *node);
static ST_Node *find_max_node(ST_Node *node);
static ST_Node *left_rotation(ST_Node *root_ptr);
static ST_Node *right_rotation(ST_Node *root_ptr);
static ST_Node *balance_tree(ST_Node *critical_node);
static void tree_height(ST_Node *root_ptr, int *height);
static void set_balance_factor(ST_Node *node);
static int key_cmp(Key *key1, Key *key2);

/**
 * @brief Used for creating function, setter and getter keys.
 *
 * @param name
 * @param args_count
 * @param id_type Can be SETTER, GETTER or FUNCTION
 */
Key *st_create_function_key(char *name, int args_count, ID_Type id_type)
{
    Key *new_key = malloc(sizeof(Key));

    new_key->args_count = args_count;
    new_key->id_type = id_type;

    char *name_copy = str_duplicate(name);

    if (!name_copy)
    {
        free(new_key);
        return NULL;
    }

    new_key->name = name_copy;

    return new_key;
}

/**
 * @brief Used for creating local and global var keys.
 *
 * @param name
 */
Key *st_create_variable_key(char *name)
{
    Key *new_key = malloc(sizeof(Key));

    new_key->args_count = -1; // args count value of variables
    new_key->id_type = VAR; 

    char *name_copy = str_duplicate(name);

    if (!name_copy)
    {
        free(new_key);
        return NULL;
    }

    new_key->name = name_copy;

    return new_key;
}

/**
 * @brief Creates a new instance of a Variable_Node and initializes it's attributes.
 *
 * @param key Key of the new node. Needs to be created before using create_function_key or create_variable_key functions
 *
 * @return New Variable_Node.
 */
ST_Node *st_create_node(Key *key)
{
    ST_Node *node = malloc(sizeof(ST_Node));

    if (!node)
        return NULL;

    // newly created node has no children
    node->left = NULL;
    node->right = NULL;

    node->key.args_count = key->args_count;
    node->key.id_type = key->id_type;

    char *name_copy = str_duplicate(key->name);

    if (!name_copy)
    {
        free(node);
        return NULL;
    }

    node->key.name = name_copy;

    return node;
}

/**
 * @brief Inserts a new ST_Node.
 *
 * @param root_ptr Pointer to the root ST_Node of a symtable.
 * @param to_insert Pointer to a node we want to insert.
 *
 * @return Pointer to the root of the symtable.
 */
ST_Node *st_insert_node(ST_Node *root_ptr, ST_Node *to_insert)
{
    if (!root_ptr) // new node is created when NULL is detected
    {
        return to_insert;
    }
    else // cannot insert the node yet
    {

        int key_cmp_result = key_cmp(&to_insert->key, &root_ptr->key);

        if (key_cmp_result < 0) // go to left subtree
            root_ptr->left = st_insert_node(root_ptr->left, to_insert);

        else if (key_cmp_result > 0) // go to right subtree
            root_ptr->right = st_insert_node(root_ptr->right, to_insert);

        else // attempt to add already existing symbol made
            error_exit(ERR_SEM_REDEFINITION);
        // NOTE: If you encounter this error after calling insert, you have probably
        // forgotten to call search() before trying to insert new symbol
    }

    // balance factor of the critical node is calculated
    set_balance_factor(root_ptr);

    // if needed
    return balance_tree(root_ptr);
}

/**
 * @brief Removes an existing ST_Node.
 *
 * @param root_ptr Pointer to the root ST_Node of a symtable.
 * @param key Key that is used to locate the ST_Node that will be removed.
 *
 * @return Pointer to the (possibly new) root of the subtree after removal,
 *         or NULL if the subtree becomes empty or removal fails.
 */
ST_Node *st_remove_node(ST_Node *root_ptr, Key *key)
{
    if (!root_ptr)
    {
        return NULL;
    }

    // here we can try to look for the node to remove
    int key_cmp_result = key_cmp(key, &root_ptr->key);

    if (key_cmp_result < 0) // go to the left subtree
    {
        root_ptr->left = st_remove_node(root_ptr->left, key);
    }
    else if (key_cmp_result > 0) // go to the right subtree
    {
        root_ptr->right = st_remove_node(root_ptr->right, key);
    }
    else // node found
    {
        if (!root_ptr->right && !root_ptr->left) // ST_Node has no children
        {
            node_dispose(root_ptr);
            return NULL;
        }
        else if (root_ptr->right && root_ptr->left) // ST_Node has both children
        {
            ST_Node *max_node = find_max_node(root_ptr->left);

            free(root_ptr->key.name); // old name needs to be freed in case str_duplicate fails

            // copy of the key is made
            root_ptr->key.name = str_duplicate(max_node->key.name);

            if (!root_ptr->key.name)
                // has to be exited because there is no way to know that removal failed based on the return value of remove_node
                error_exit(ERR_INTERNAL);

            root_ptr->key.args_count = max_node->key.args_count;
            root_ptr->key.id_type = max_node->key.id_type;

            // max node is removed
            root_ptr->left = st_remove_node(root_ptr->left, &max_node->key);
        }
        else if (root_ptr->left && !root_ptr->right) // only left child present
        {
            ST_Node *onlyChild = root_ptr->left;
            node_dispose(root_ptr);

            // tree needs to be balanced after removal
            set_balance_factor(onlyChild);
            return balance_tree(onlyChild);
        }
        else // only right child present
        {
            ST_Node *onlyChild = root_ptr->right;
            node_dispose(root_ptr);

            // tree needs to be balanced after removal
            set_balance_factor(onlyChild);
            return balance_tree(onlyChild);
        }
    }

    // tree needs to be balanced after removal
    set_balance_factor(root_ptr);

    return balance_tree(root_ptr);
}

/**
 * @brief Searches for a ST_Node based on a provided key. Can be used to verify existance of a ST_Node or to obtain a pointer to it's adress.
 *
 * @param root_ptr Pointer to the root ST_Node of a symtable.
 * @param key Pointer to a key that is used to locate the ST_Node.
 *
 * @return Pointer to a ST_Node or NULL if no ST_Node with corresponding key was found.
 */
ST_Node *st_search(ST_Node *root_ptr, Key *key)
{
    if (!root_ptr) // ST_Node not found
    {
        return NULL;
    }
    else
    {
        int key_cmp_result = key_cmp(key, &root_ptr->key);

        if (key_cmp_result < 0) // go to the left subtree
            return st_search(root_ptr->left, key);

        else if (key_cmp_result > 0) // go to the right subtree
            return st_search(root_ptr->right, key);

        else // node found
            return root_ptr;
    }
}

/**
 * @brief Recursively disposes of all nodes in the tree using PostOrder tree traversal.
 *
 * @param root_ptr Root of a tree to dispose.
 */
void st_dispose_tree(ST_Node *root_ptr)
{
    if (!root_ptr)
        return;

    st_dispose_tree(root_ptr->left);
    st_dispose_tree(root_ptr->right);
    node_dispose(root_ptr);
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
static void tree_height(ST_Node *root_ptr, int *height)
{
    int height_l = 0, height_r = 0;

    if (root_ptr)
    {
        tree_height(root_ptr->left, &height_l);
        tree_height(root_ptr->right, &height_r);

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
 * @brief Using the tree_height function this function finds Height of both subtrees of the passed node, and determines its balance factor.
 *
 * @param node Balance factor of this node will be set.
 */
static void set_balance_factor(ST_Node *node)
{
    if (!node)
        return;

    int left_subtree_height, right_subtree_height;

    // finds height of both subtrees
    tree_height(node->left, &left_subtree_height);
    tree_height(node->right, &right_subtree_height);

    node->balance_factor = left_subtree_height - right_subtree_height;
}

/**
 * @brief Performes right_rotation around the critical node (also called pivot node).
 *
 * @param root_ptr Critical node.
 *
 * @return Pointer to the new root node of the subtree that was rotated.
 */
static ST_Node *right_rotation(ST_Node *root_ptr)
{
    ST_Node *left_child = root_ptr->left; // left child will become the new root_node of the subtree
    ST_Node *temp = left_child->right;    // right subtree of the left_child will be connected to current root_node->left

    left_child->right = root_ptr; // left_child now becomes the new root node
    root_ptr->left = temp;        // connects left ptr of the old root node to the right subtree of the new root node

    // balance factor is calculated again after rotation, only root_ptr and left_child should be effected by the rotation
    set_balance_factor(root_ptr);
    set_balance_factor(left_child);

    return left_child; // new root_node is always the left child of the former root_node
}

/**
 * @brief Performes left_rotation around the critical node (also called pivot node).
 *
 * @param root_ptr Critical node.
 *
 * @return Pointer to the new root node of the subtree that was rotated.
 */
static ST_Node *left_rotation(ST_Node *root_ptr)
{
    ST_Node *right_child = root_ptr->right; // right child will become the new root_node of the subtree
    ST_Node *temp = right_child->left;      // left subtree of the right_child will be connected to current root_ptr->right

    right_child->left = root_ptr; // right_child now becomes the new root node
    root_ptr->right = temp;       // connects right ptr of the old root node to the right subtree of the new root node

    // balance factor is calculated again after rotation, only root_ptr and right_child should be effected by the rotation
    set_balance_factor(root_ptr);
    set_balance_factor(right_child);

    return right_child; // new root_node is always the right child of the former root_node
}

/**
 * @brief Balances the tree according to the type of imbalance.
 *
 * @param root_ptr Root node of an unbalanced subtree that has balance factor higher than 1 or lower than -1.
 *
 * @return New root_ptr of the subtree after balancing.
 */
static ST_Node *balance_tree(ST_Node *critical_node)
{
    // avoids NULL ptr dereference
    if (!critical_node)
        return NULL;

    // RL case
    if (critical_node->balance_factor < -1 && critical_node->right->balance_factor > 0)
    {
        critical_node->right = right_rotation(critical_node->right);
        return left_rotation(critical_node);
    }

    // LR case
    if (critical_node->balance_factor > 1 && critical_node->left->balance_factor < 0)
    {
        critical_node->left = left_rotation(critical_node->left);
        return right_rotation(critical_node);
    }

    // RR case
    if (critical_node->balance_factor < -1 && critical_node->right->balance_factor <= 0)
    {
        return left_rotation(critical_node);
    }

    // LL case
    if (critical_node->balance_factor > 1 && critical_node->left->balance_factor >= 0)
    {
        return right_rotation(critical_node);
    }

    return critical_node; // no balancing was required
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

    // if allocation fails function returns NULL to signal ST_Node creation failure
    if (!copy)
        return NULL;

    return strcpy(copy, to_duplicate);
}

/**
 * @brief Finds the most right ST_Node of the left subtree.
 *
 * @param root_ptr Root node of the subtree in which we want to find the min ST_Node.
 * @note Root of the left subtree needs to be passed!
 *
 * @return Min ST_Node pointer.
 */
static ST_Node *find_max_node(ST_Node *node)
{
    if (!node->right) // no more right children
        return node;
    else
        return find_max_node(node->right);
}

/**
 * @brief Deallocates data inside the node and the node itself.
 *
 * @param node Pointer to ST_Node we want to clean up after.
 */
void node_dispose(ST_Node *node)
{
    if (node)
    {
        free(node->key.name);
        free(node);
    }
}