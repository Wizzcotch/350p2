#include "chkptregion.h"

#include <fstream>
#include <sstream>

ChkptRegion::ChkptRegion(std::string filename): filename(filename)
{
    std::ifstream ifs(filename);

    if(ifs.is_open())
    {
        std::string contents((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        const char *tmpBuffer = contents.c_str();

        /*std::string inputBuffer;
        std::getline(ifs, inputBuffer);
        std::stringstream ss(inputBuffer);*/

        //char segInfoBit;
        nextFreeSeg = -1;
        for(int i = 0; i < 32; ++i)
        {
            //ss.get(segInfoBit);
            //segInfo[i] = (segInfoBit == '1'); //True means it has live data
            segInfo[i] = (tmpBuffer[i] == '1'); //True means it has live data
            if(nextFreeSeg == -1 && segInfo[i] == false)
            {
                nextFreeSeg = i;
            }

            /*if(ss.fail())
            {
                std::cerr << "[ERROR] Wrong format detected in filemap" << std::endl;
                ifs.close();
                exit(1);
            }*/
        }

        //char digit;
        int imapPiece = 0;
        for (int i = 32; i < 192; i+= 4)
        {
            int blkNumber = 0;
            for(int j = 3; j >= 0; --j)
            {
                blkNumber += tmpBuffer[i + j] << j*8;
            }
            imap[imapPiece] = blkNumber;
            imapPiece++;
        }
        /*while(ss.get(digit))
        {
            if(imapPiece > 40)
            {
                std::cerr << "[ERROR] CHECKPOINT_REGION file is too large" << std::endl;
                exit(1);
            }
            int blkNumber = 0;
            for(int i = 3; i >= 0; --i)
            {
                blkNumber += digit << i*8;
            }
            imap[imapPiece] = blkNumber;
            imapPiece++;
        }*/

        //std::cerr << "CHKPTREGION: Read " << imapPiece << " imap pieces" << std::endl;
        /*nextimapPiece = imapPiece+1;
        for(int i = nextimapPiece; i < 40; ++i)
        {
            imap[i] = -1;
        }*/

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
        for(int iter = 0; iter < 40; ++iter)
        {
            for(int i = 3; i >= 0; --i)
            {
                ofs << (char)((imap[iter] >> i*4) & 0xFF);
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

int ChkptRegion::getNextFreeSeg()
{
    return nextFreeSeg;
}

void ChkptRegion::markSegment(int segment, bool state)
{
    if(segment < 40 && segment >= 0)
    {
        segInfo[segment] = state;
        /* Calculate next free seg */
        for(int i = 0; i < 40; ++i)
        {
            if(segInfo[i] == false)
            {
                nextFreeSeg = i;
                break;
            }
        }
    }
}
