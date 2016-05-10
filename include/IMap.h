#ifndef IMAP_H
#define IMAP_H

#include <array>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <utility>

class IMap
{
    private:
        int blockNumbers[256];
        int mapPieceNumber, nextAvailinode, numInodes;
        bool imapChanged;
        void calcNextinode();

    public:
        IMap();
        IMap(int mapPiece);
        ~IMap();
        int addinode(int blockNumber);
        void removeinode(int inode);
        int getBlockNumber(int inodeNumber);
        int getMapPieceNumber() { return mapPieceNumber; }
        bool isFull() {return numInodes == 256;}
        void clear();
        char* convertToString();
        void setUpImap(std::pair<int, int> blockInfo, bool checkFull);

};

#endif
