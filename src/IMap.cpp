#include "IMap.h"

#include <cstring>

// Constructor
IMap::IMap(int mapPiece): mapPieceNumber(mapPiece), nextAvailinode(0), imapChanged(false)
{
    for(auto &inum : blockNumbers)
    {
        inum = -1;
    }
}

IMap::~IMap() {}

void IMap::calcNextinode()
{
    if(!imapChanged)
    {
        nextAvailinode++;
    }
    else
    {
        for(int iter = nextAvailinode+1; iter < 1024; ++iter)
        {
            if(blockNumbers[iter] == -1)
            {
                nextAvailinode = iter;
                break;
            }
        }

        imapChanged = false;
    }
}

int IMap::addinode(int blockNumber)
{
    int inodeNumber = nextAvailinode;
    blockNumbers[inodeNumber] = blockNumber;
    ++numInodes;
    calcNextinode();
    return inodeNumber + (mapPieceNumber) * 1024;
}

void IMap::removeinode(int inode)
{
    if(inode < nextAvailinode) nextAvailinode = inode;
    imapChanged = true;
    blockNumbers[inode] = -1;
    --numInodes;
}
char* IMap::convertToString()
{
    std::string imapData;
    int mask = 0xFF;
    for(int iter = 0, index = 0; iter < numInodes; ++iter, ++index)
    {
        //int inode = iter + (mapPieceNumber) * 1024;
        if(blockNumbers[index] != -1)
        {
            for(int i = 3; i >= 0; --i)
            {
                char dummy;
                dummy = (char)((blockNumbers[index] >> 8*i) & mask);
                imapData += dummy;
            }
        }
        else
        {
            --iter;
        }
    }

    return strdup(imapData.c_str());

}

void IMap::clear()
{
    mapPieceNumber++;
    nextAvailinode = 0;
    numInodes = 0;
    for(auto &b : blockNumbers)
    {
        b = -1;
    }
}

