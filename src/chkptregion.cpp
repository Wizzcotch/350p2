#include "chkptregion.h"

#include <fstream>
#include <sstream>

ChkptRegion::ChkptRegion(std::string filename): filename(filename)
{
    std::ifstream ifs(filename);
    if(ifs.is_open())
    {
        std::string inputBuffer;
        std::getline(ifs, inputBuffer);
        std::stringstream ss(inputBuffer);

        char segInfoBit;
        for(int i = 0; i < 32; ++i)
        {
            ss.get(segInfoBit);
            segInfo[i] = (segInfoBit == '0'); //Dunno what true and false mean here
            if(ss.fail())
            {
                std::cerr << "[ERROR] Wrong format detected in filemap" << std::endl;
                ifs.close();
                exit(1);
            }
        }

        char digit;
        int imapPiece = 0;
        while(!ss.eof())
        {
            ss.get(digit);
            int blkNumber = 0;
            for(int i = 3; i >= 0; --i)
            {
                blkNumber += digit << i*8;
            }
            imap[imapPiece] = blkNumber;
        }

        nextimapPiece = imapPiece+1;
        for(int i = nextimapPiece; i < 40; ++i)
        {
            imap[i] = -1;
        }

        ifs.close();
    }
    else
    {
        std::cerr << "[ERROR] Could not open CHECKPOINT_REGION file" << std::endl;
        exit(1);
    }
}

ChkptRegion::~ChkptRegion()
{
    std::ofstream ofs(filename);
    if(ofs.is_open())
    {
        for(auto iter = segInfo.begin(); iter != segInfo.end(); ++iter)
        {
            ofs << *iter;
        }
        for(auto iter = imap.begin(); iter != imap.end(); ++iter)
        {
            for(int i = 3; i >= 0; ++i)
            {
                ofs << (char)((*iter >> i*4) & 0xFF);
            }
        }
        ofs.close();
    }
    else
    {
        std::cerr << "[ERROR] Could not open CHECKPOINT_REGION file" << std::endl;
        exit(1);
    }
}

void ChkptRegion::addimap(int blkNumber)
{
    imap[nextimapPiece++] = blkNumber;
}

int ChkptRegion::getimap(int imapPiece)
{
    return imap[imapPiece];
}
