#include "buddySystem_UT.h"

#define true 1
#define false 0

MemoryTreeNode *createMemoryTree()
{
    MemoryTreeNode *memoryTreeRoot;
    memoryTreeRoot = malloc(sizeof(MemoryTreeNode));
    memoryTreeRoot->left = NULL;
    memoryTreeRoot->right = NULL;
    memoryTreeRoot->size = 1024;
    memoryTreeRoot->startindex = 0;
    memoryTreeRoot->free = true;
    return memoryTreeRoot;
}

// Recursively searches for the best fitting free block [in the current tree.. does not split] for a given size
MemoryTreeNode *getBestFit(MemoryTreeNode *root, int size)
{
    if (!root)
        return NULL;

    // If current node is free and big enough, return it (potential fit)
    if (root->free && root->size >= size)
        return root;

    if (root->free)
        return NULL;

    // Recursively search left and right subtrees
    MemoryTreeNode *leftfit = getBestFit(root->left, size);
    MemoryTreeNode *rightfit = getBestFit(root->right, size);

    if (leftfit == NULL)
        return rightfit;

    if (rightfit == NULL)
        return leftfit;

    // Return the smaller block that fits (best fit)
    if (leftfit->size <= rightfit->size)
        return leftfit;
    else
        return rightfit;
}
// Merges a node by freeing its children and marking itself as free
void merge(MemoryTreeNode *root)
{
    free(root->left);
    free(root->right);
    root->left = NULL;
    root->right = NULL;
    root->free = true;
}

int getNearestPowerOfTwo(int memsize)
{
    int i = 1;
    while (i < memsize)
    {
        // shifts i bits to the left... 001 -> 010 .. 1 2 4 etc
        i <<= 1;
    }
    return i;
}
// Splits a node into two children (buddies), each half the size
void split(MemoryTreeNode *root)
{
    root->left = malloc(sizeof(MemoryTreeNode));
    root->left->free = true;
    root->left->size = root->size / 2;
    root->left->startindex = root->startindex;
    root->left->left = NULL;
    root->left->right = NULL;

    root->right = malloc(sizeof(MemoryTreeNode));
    root->right->free = true;
    root->right->size = root->size / 2;
    root->right->startindex = root->startindex + root->size / 2;
    root->right->left = NULL;
    root->right->right = NULL;
    // Once split, root is no longer a free leaf
    root->free = false;
}
// Recursively merges buddy nodes if both children are free
void recursiveMerge(MemoryTreeNode *root)
{
    if (root == NULL || root->left == NULL)
        return;

    recursiveMerge(root->left);
    recursiveMerge(root->right);

    if (root->left->free && root->right->free)
        merge(root);
}
// Allocates memory of at least memsize, returns start index or -1 if not possible
int allocateMemory(MemoryTreeNode *root, int memsize)
{
    int assignedaddress = -1;

    // if root is occupied try to allocate in one of its children
    if (root->free == false)
    {
        assignedaddress = allocateMemory(root->left, memsize);
        if (assignedaddress == -1)
            assignedaddress = allocateMemory(root->right, memsize);

        return assignedaddress;
    }
    if (root->size < memsize)
    {
        return -1;
    }
    // if root is free first try to split then allocate
    if (root->free == true)
    {
        split(root);
        assignedaddress = allocateMemory(root->left, memsize);
        if (assignedaddress == -1)
            assignedaddress = allocateMemory(root->right, memsize);
        if (assignedaddress != -1)
            return assignedaddress;

        // If split didn't work out, undo the split and allocate the entire block
        merge(root);
        root->free = false;
        return root->startindex;
    }
    return assignedaddress;
}
// Deallocates memory and sets the corresponding block as free
void deallocateMemory(MemoryTreeNode *root, int startindex, int size)
{
    if (root == NULL)
        return;

    // If we found the node that matches the block
    if (root->startindex == startindex && root->size == size)
    {
        root->free = true;
        return;
    }

    // Recurse to the correct child
    if (root->left && startindex < root->right->startindex)
        deallocateMemory(root->left, startindex, size);
    else if (root->right)
        deallocateMemory(root->right, startindex, size);

    // After deallocation, try to merge the buddies
    if (root->left && root->right && root->left->free && root->right->free)
        merge(root);
}