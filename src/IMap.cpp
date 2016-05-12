#include "IMap.h"

// Default Constructor
IMap::IMap()
{
    for(auto &inum : blockNumbers)
    {
        inum = -1;
    }
}

// Constructor
IMap::IMap(int mapPiece): mapPieceNumber(mapPiece), nextAvailinode(0), imapChanged(false)
{
    for(auto &inum : blockNumbers)
    {
        inum = -1;
    }
}


IMap::~IMap() {}

/**
 * Pair of ints: block number, and map piece number
 * Bool: Clear if necessary
 */
void IMap::setUpImap(std::pair<int, int> blockInfo, bool checkFull)
{
    int blockNum = blockInfo.first;
    if (!checkFull) std::cerr << "Block #: " << blockNum << std::endl;

    // First time run, so previous imap does not exist
    if (blockNum <= 0)
    {
        return;
    }

    mapPieceNumber = blockInfo.second;

    // Divide by 1MB to figure out what segment our piece is in
    int segmentNum = blockNum / 1024;
    std::string segmentFile = "./DRIVE/SEGMENT" + std::to_string(segmentNum + 1);

    if (!checkFull) std::cerr << "Fetching imap at segment #: " << segmentNum + 1 << std::endl;

    std::ifstream ifs(segmentFile, std::ifstream::binary);
    if(!ifs.is_open())
    {
        std::cerr << "[ERROR] Could not open SEGMENT file for reading" << std::endl;
        exit(1);
    }
    //char* imapBuffer = new char[1024];
    //ifs.read(imapBuffer, 1024);
    int localPos = blockNum % 1024;
    ifs.seekg(localPos * 1024);

    int inodeNum = 0;
    for (int i = 0; i < 256; i++)
    {
        ifs.read((char*)&inodeNum, sizeof(inodeNum));
        if (inodeNum > 0)
        {
            addinode(inodeNum);
            if (!checkFull) std::cerr << "Read in inode location: " << inodeNum << std::endl;
        }
    }
    ifs.close();

    if (checkFull && isFull())
    {
        clear();
    }
}

void IMap::calcNextinode()
{
    if(!imapChanged)
    {
        nextAvailinode++;
    }
    else
    {
        for(int iter = 0; iter < 256; ++iter)
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

int IMap::setinode(int blockNumber, int oldNum)
{
    int inodeNumber = oldNum;
    blockNumbers[inodeNumber] = blockNumber;
    return inodeNumber + (mapPieceNumber) * 1024;
}

int IMap::getNextINodeNumber(){
  return nextAvailinode + mapPieceNumber * 1024;
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
    /*std::string imapData;*/
    //int mask = 0xFF;
    //for(int iter = 0, index = 0; iter < numInodes; ++iter, ++index)
    //{
        ////int inode = iter + (mapPieceNumber) * 1024;
        //if(blockNumbers[index] != -1)
        //{
            //for(int i = 3; i >= 0; --i)
            //{
                //char dummy;
                //dummy = (char)((blockNumbers[index] >> 8*i) & mask);
                //imapData += dummy;
            //}
        //}
        //else
        //{
            //--iter;
        //}
    /*}*/

    char* retChar = new char[sizeof(int) * 256];
    memcpy(retChar, blockNumbers, sizeof(int) * 256);
    return retChar;

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

