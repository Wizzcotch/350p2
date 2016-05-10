#ifndef INODE_H
#define INODE_H

#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <cstring>

typedef struct {
    char filename[32];
    int filesize;
    int dataPointers[128];
} INodeInfo;

class INode
{
    public:
        ~INode();
        INodeInfo info;
        INode(const std::string filenameIn);
        void setSize(const int size) {info.filesize = size;}
        void addDataPointer(const int blockNum);
        char* convertToString();
        void printValues();

    private:
        int nextDataPointer;
};

#endif
