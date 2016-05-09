#ifndef INODE_H
#define INODE_H

#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <cstring>

class INode
{
    public:
        std::string filename;
        int filesize;
        std::vector<int> dataPointers;
        ~INode();
        INode(std::string filenameIn);
        void setSize(int size) {filesize = size;}
        char* convertToString();
};

#endif
