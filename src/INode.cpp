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
    int size = (32 * sizeof(char)) + (129 * sizeof(int));
    char* retChar = new char[size];

    int sizeInt = sizeof(int);
    //memcpy(retChar, &info, sizeof(INodeInfo));
    for (int i = 0; i < 32; i++)
    {
        retChar[i] = info.filename[i];
    }
    int pos = 32;
    for (int i = 0; i < 128; i++)
    {
        char dataPointerInt[sizeInt];
        sprintf(dataPointerInt, "%d", info.dataPointers[i]);
        //std::cerr << "Read in: " << dataPointerInt[0] << ", " << dataPointerInt[1] << ", " << dataPointerInt[2] << ", " << dataPointerInt[3] << std::endl;
        for (int j = 0; j < sizeInt; j++)
        {
            retChar[pos + j] = dataPointerInt[j];
        }
        pos += 4;
    }
    char filesizeInt[sizeInt];
    sprintf(filesizeInt, "%d", info.filesize);
    for (int j = 0; j < sizeInt; j++)
    {
        retChar[pos + j] = filesizeInt[j];
    }
    std::cerr << retChar << std::endl;
    return retChar;
}
