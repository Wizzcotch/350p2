#ifndef IMAP_H
#define IMAP_H

#include <iostream>
#include <array>
#include <string>

class IMap
{
    private:
        std::array<int, 1024> blockNumbers;
        int mapPieceNumber, nextAvailinode, numInodes;
        bool imapChanged;
        void calcNextinode();

    public:
        IMap() = delete;
        IMap(int mapPiece);
        ~IMap();
        int addinode(int blockNumber);
        void removeinode(int inode);
        int getBlockNumber(int inodeNumber) { return blockNumbers[inodeNumber]; }
        int getMapPieceNumber() { return mapPieceNumber; }
        bool isFull() {return numInodes == 1024;}
        void clear();
        char* convertToString();

};

#endif
