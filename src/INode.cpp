#include "INode.h"

INode::~INode() {}

// Constructor
INode::INode(std::string filenameIn):filename(filenameIn),dataPointers() {}

// For packaging data into a string
char* INode::convertToString()
{
    std::string inodeData;
    inodeData += filename;
    inodeData += "\n";
    inodeData += std::to_string(filesize) + "\n";
    int size = dataPointers.size();
    for (int i = 0; i < size; i++)
    {
        inodeData += std::to_string(dataPointers[i]);
    }

    /*char dataPointer[4];
    int mask = 0xFF;
    for(auto dp : dataPointers)
    {
        dataPointer[3] = dp & mask;
        dataPointer[2] = (dp >> 8) & mask;
        dataPointer[1] = (dp >> 16) & mask;
        dataPointer[0] = (dp >> 24) & mask;
        inodeData += std::string(dataPointer) + "\n";
    }*/

    char* retChar = strdup(inodeData.c_str());
    return retChar;
}
