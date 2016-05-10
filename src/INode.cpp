#include "INode.h"

// Destructor
INode::~INode() {}

// Constructor
INode::INode(std::string filenameIn) : nextDataPointer(0)
{
    //for(int i = 0; i < 32; ++i )
    //{
        //info.filename[i] = '\n';
    //}

    if(filenameIn.copy(info.filename, filenameIn.size()) <= 0)
    {
        std::cerr << "[ERROR]" << std::endl;
    }

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
    std::cout << "INode buffer: " << retChar << std::endl;
    std::cout << "INode filesize: " << info.filesize << std::endl;
    std::cout << "sizeof INode buffer: " << sizeof(INodeInfo) << std::endl;
    std::cout << "Num of data pointers: " << nextDataPointer << std::endl;
    return retChar;
}
