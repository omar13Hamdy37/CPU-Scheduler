#ifndef MEMORYTREENODE_H
#define MEMORYTREENODE_H

typedef struct MemoryTreeNode{
    struct MemoryTreeNode* left;
    struct MemoryTreeNode* right;
    int size;         // Size of the block
    int startindex;   // Starting memory index
    int free;                 // Whether the block is free... 1 if no allocations in current block
} MemoryTreeNode;


#endif