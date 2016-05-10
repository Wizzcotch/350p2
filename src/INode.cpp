#include "INode.h"

INode::~INode() {}

// Constructor
INode::INode(std::string filenameIn) : nextDataPointer(0)
{
    if(filenameIn.copy(info.filename, filenameIn.size()) <= 0)
    {
        std::cerr << "[ERROR]" << std::endl;
    }
}

// Don't need to check for filesize
void INode::addDataPointer(int blockNum)
{
    info.dataPointers[nextDataPointer++] = blockNum;
}

// For packaging data into a string
char* INode::convertToString()
{
    char* retChar = new char[sizeof(INodeInfo)];
    memcpy(retChar, &info, sizeof(INodeInfo));
    return retChar;
}
