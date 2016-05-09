#ifndef CHKPTREGION_H
#define CHKPTREGION_H
#include <iostream>
#include <string>
#include <array>

class ChkptRegion
{
    private:
        std::string filename;
        std::array<int, 40> imap;
        std::array<bool, 32> segInfo;
        int nextimapPiece;
        int nextFreeSeg;

    public:
        ChkptRegion() = delete;
        ChkptRegion(std::string filename);
        ~ChkptRegion();
        void addimap(int blkNumber);
        int getimap(int imapPiece);
        int getNextFreeSeg();
        void markSegment(int segment, bool state);
        std::array<int, 40> getimapArray() { return imap; }
};

#endif
