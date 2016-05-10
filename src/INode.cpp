#include "INode.h"

// Destructor
INode::~INode() {}

// Constructor
INode::INode(std::string filenameIn) : nextDataPointer(0)
{
    for(int i = 0; i < 32; ++i )
    {
        info.filename[i] = (char)-1;
    }

    if(filenameIn.copy(info.filename, filenameIn.size()) <= 0)
    {
        std::cerr << "[ERROR]" << std::endl;
    }
    //info.filename[31] = '9';
    //info.filename[23] = 'B';

    info.filesize = -1;
    for(int i = 0; i < 128; ++i)
    {
        info.dataPointers[i] = -1;
    }
}

// Don't need to check for filesize
void INode::addDataPointer(const int blockNum)
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

void INode::printValues()
{
    std::cerr << "[DEBUG] INode filename: " << info.filename << std::endl;
    std::cerr << "[DEBUG] INode filesize: " << info.filesize << std::endl;
    for(int i = 0; i < nextDataPointer; ++i)
    {
        std::cerr << info.dataPointers[i] << " ";
    }
    std::cerr << "\n" << "[DEBUG] Number of data pointers: " << nextDataPointer << std::endl;
}
