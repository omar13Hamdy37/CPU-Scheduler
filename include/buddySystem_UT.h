#ifndef BUDDYSYSTEM_H
#define BUDDYSYSTEM_H

#include "../models/memoryTreeNode.h"
#include <stdlib.h>

MemoryTreeNode* createMemoryTree();
MemoryTreeNode* getBestFit(MemoryTreeNode* root, int size);
void merge(struct MemoryTreeNode* root);
int getNearestPowerOfTwo(int memsize);
void split(MemoryTreeNode* root);
void recursiveMerge(MemoryTreeNode* root);
int allocateMemory(MemoryTreeNode* root,int memsize);
void deallocateMemory(MemoryTreeNode* root,int startindex,int size);

#endif