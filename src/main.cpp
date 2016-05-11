#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include "Filemap.h"
#include "ChkptRegion.h"
#include "IMap.h"
#include "INode.h"

#define BLK_SIZE 1024
#define MAX_FILE_NUM 10000
#define MAX_INODE_DPNTR 128 //Maximum number of data block pointers in an inode
#define SEGMENT_SIZE BLK_SIZE * BLK_SIZE

#ifndef DEBUG
#define DEBUG 1
#endif

// Buffer
char* logBuffer = new char[BLK_SIZE * BLK_SIZE];
char* overflowBuffer = new char[130 * BLK_SIZE]; // 128 data blocks + 1 inode + 1 imap
int logBufferPos, overBufPos;
int currentSegment;
IMap currentIMap(0);
bool bufferFull;

// Filemap
Filemap filemap("./DRIVE/FILEMAP");
std::vector<std::pair<std::string, int> > filelist = filemap.populate();

// Checkpoint region
ChkptRegion chkptregion("./DRIVE/CHECKPOINT_REGION");

/**
 * Completes remaining writes and exits.
 */
void proper_exit()
{
    char* imapStr = currentIMap.convertToString();
    memcpy(&logBuffer[logBufferPos],
            imapStr,
            BLK_SIZE);
    delete imapStr;
    currentIMap.clear();
    chkptregion.addimap((BLK_SIZE * currentSegment) + logBufferPos/BLK_SIZE);

    //if (DEBUG) std::cerr << "Current imap: " << std::string(logBuffer.at(logBufferPos - 1)) << std::endl;

    // Update segment
    chkptregion.markSegment(currentSegment, true);

    // Write buffer into the segment file
    std::string segmentFile = "./DRIVE/SEGMENT" + std::to_string(currentSegment + 1);
    if (DEBUG) std::cerr << "Segment file: " << segmentFile << std::endl;
    std::ofstream ofs(segmentFile);
    if(!ofs.is_open())
    {
        std::cerr << "[ERROR] Could not open SEGMENT file for reading" << std::endl;
        exit(1);
    }
    ofs.write(logBuffer, BLK_SIZE * BLK_SIZE);
    ofs.close();

    // Successfully exit
    exit(0);
}

/**
 * Lists the files in the LFS.
 */
void list_files()
{
    // Lab 7 Code
    /*for (auto file : filelist)
    {
        std::cout << file.first << " (" << file.second << ")" << std::endl;
    }*/

    auto fileList = filemap.getFilemap();
    auto imapCR = chkptregion.getimapArray();
    for (auto it = fileList.begin(); it != fileList.end(); ++it)
    {
        std::string currName = it->first;
        int inodeNum = it->second;
        for (auto mapIt = imapCR.begin(); mapIt != imapCR.end(); ++mapIt)
        {
            if (*mapIt != -1)
            {
                int idx = ((int)(*mapIt / 1024.0)) + 1;
                std::ifstream in("./DRIVE/SEGMENT" + std::to_string(idx));
                std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                // const char *tmpBuffer = contents.c_str();
                // char ptr = tmpBuffer[inodeNum];
            }
        }
        std::cout << currName << "\t" << inodeNum << std::endl;
    }
}

/**
 * Import specified file into LFS.
 */
void import_file(std::string& originalName, std::string& lfsName)
{
    // Open file for reading
    std::ifstream ifs(originalName, std::ifstream::binary);
    if(!ifs.is_open())
    {
        std::cerr << "[ERROR] Could not open file for reading" << std::endl;
        exit(1);
    }
    else if(lfsName.size() > 32)
    {
        std::cerr << "[ERROR] File name '" << lfsName << "' too long" << std::endl;
    }

    // Get file size
    ifs.seekg(0, ifs.end);
    long size = ifs.tellg();
    ifs.seekg(0);

    //// Allocate memory for file contents
    char* buffer = new char[size];
    ifs.read(buffer, size);

    // Setup inode
    INode inodeObj(lfsName);
    if(size / BLK_SIZE > 128 || (size / BLK_SIZE == 128 && size % BLK_SIZE > 0))
    {
        std::cerr << "[ERROR] File '" << originalName << "' too big." << std::endl;
        //std::cerr << "Filesize: " << size / BLK_SIZE << std::endl;
        return;
    }

    inodeObj.setSize(size);

    for (int bufferPos = 0; bufferPos < size; bufferPos += BLK_SIZE)
    {
        if(logBufferPos < SEGMENT_SIZE)
        {
            // Absolute position in memory
            inodeObj.addDataPointer((BLK_SIZE * currentSegment) + logBufferPos/BLK_SIZE);

            for (int offset = 0; offset < BLK_SIZE && bufferPos + offset < size; offset++)
            {
                logBuffer[logBufferPos + offset] = buffer[bufferPos + offset];
            }

            logBufferPos += BLK_SIZE;
        }
        else
        {
            if(!bufferFull) bufferFull = true;
            /* Did not check if currentSegment exceeds 32 */
            inodeObj.addDataPointer((BLK_SIZE * (currentSegment + 1)) + overBufPos/BLK_SIZE + 8);
            for (int offset = 0; offset < BLK_SIZE && bufferPos + offset < size; offset++)
            {
                overflowBuffer[overBufPos + offset] = buffer[bufferPos + offset];
            }
            overBufPos += BLK_SIZE;
        }
    }

    /* Write inode into buffer */
    char* inodeStr = inodeObj.convertToString();

    /* Record inode in imap */
    int createdInodeNum;
    if(!bufferFull)
    {
        memcpy(&logBuffer[logBufferPos], inodeStr, sizeof(INodeInfo));
        if(DEBUG) std::cerr << "[DEBUG] Logical inode block num: " << logBufferPos/BLK_SIZE << std::endl;
        logBufferPos += BLK_SIZE;
        createdInodeNum = currentIMap.addinode((BLK_SIZE * currentSegment) + logBufferPos/BLK_SIZE);
    }
    else
    {
        memcpy(&overflowBuffer[overBufPos], inodeStr, sizeof(INodeInfo));
        if(DEBUG) std::cerr << "[DEBUG] (Overflow) Logical inode block num: " << overBufPos/BLK_SIZE << std::endl;
        overBufPos += BLK_SIZE;
        createdInodeNum = currentIMap.addinode((BLK_SIZE * (currentSegment + 1)) + overBufPos/BLK_SIZE + 8);
    }

    //if (DEBUG)
    //{
        //std::cerr << "[DEBUG] Created INode" << std::endl;
        //inodeObj.printValues();
        //std::cerr << "Contents of string: " << inodeStr << std::endl;
    //}

    delete inodeStr;


    // Add file-inode association to filemap
    filemap.addFile(lfsName, createdInodeNum);
    filelist.push_back(std::make_pair(lfsName, size));
    if (DEBUG) std::cerr << "[DEBUG] INode # " << createdInodeNum << " is tracked in IMap # " << 0 << std::endl;
    if (DEBUG) std::cerr << "[DEBUG] Import Complete" << std::endl;
}

/**
 * Remove specified file from LFS.
 */
void remove_file(std::string& filename)
{
    std::string name = filename;
    auto it = std::find_if(filelist.begin(), filelist.end(), [&name](const std::pair<std::string, int>& pair)
    {
        return pair.first == name;
    });
    if (it == filelist.end())
    {
        std::cerr << "[ERROR] File < " << filename << " > not found (does it exist?)" << std::endl;
    }
    else
    {
        it = filelist.erase(it);
        filemap.removeFile(filename);
    }

    //IMap *map;

    //if (cRegion.empty())
    //{
        //std::cerr << "[ERROR] No files found" << std::endl;
    //}
    //// If INode at location has the specified filename, clear its reference in the IMap
    //int crSize = cRegion.size();
    //for (int i = 0; i < crSize; i++)
    //{
        //map = &cRegion[i];
        //int mapSize = map->inodeList.size();
        //for (int j = 0; j < mapSize; j++)
        //{
            //if (inodes[j].filename == filename)
            //{
                //map->inodeList.erase(map->inodeList.begin() + j);
                //if (DEBUG) std::cerr << "[DEBUG] Successfully removed < " << filename << " >" << std::endl;
                //return;
            //}
        //}
    //}
}

int main(int argc, char *argv[])
{
    currentSegment = chkptregion.getNextFreeSeg();
    if (DEBUG) std::cerr << "Current segment: " << currentSegment << std::endl;

    // Grab contents of current segment
    std::string segmentFile = "./DRIVE/SEGMENT" + std::to_string(currentSegment + 1);

    // Open segment for reading
    std::ifstream ifs(segmentFile, std::ifstream::binary);
    if(!ifs.is_open())
    {
        std::cerr << "[ERROR] Could not open '" << segmentFile << "' file for reading" << std::endl;
        exit(1);
    }
    ifs.read(logBuffer, BLK_SIZE * BLK_SIZE);
    ifs.close();

    // Initialize buffers' positions
    logBufferPos = 8 * BLK_SIZE; // Start buffer after segment summary blocks
    overBufPos = 0;

    // Exit with an error message if argument count is incorrect (i.e. expecting one: input file path)
    if (argc != 1)
    {
        std::cerr << argv[0] << ": program does not take arguments; commands are sent as input, not arguments" << std::endl;
        exit(1);
    }

    /**
     * Process command from user input.
     */
    std::string command;
    if (DEBUG) std::cerr << "[DEBUG] Now accepting commands" << std::endl;
    while (std::getline(std::cin, command))
    {
        if (command.empty() || std::all_of(command.begin(), command.end(), isspace))
        {
            std::cout << "[ERROR] Command not recognized; please try again..." << std::endl;
            continue;
        }

        std::string buffer;
        std::stringstream ss(command);
        std::vector<std::string> tokens;
        while (ss >> buffer) tokens.push_back(buffer);

        if (tokens[0] == "exit" && tokens.size() == 1)
        {
            proper_exit();
        }
        else if (tokens[0] == "list" && tokens.size() == 1)
        {
            list_files();
        }
        else if (tokens[0] == "import" && tokens.size() == 3)
        {
            std::string name = tokens[2];
            auto it = std::find_if(filelist.begin(), filelist.end(), [&name](const std::pair<std::string, int>& pair)
            {
                return pair.first == name;
            });
            if (it != filelist.end())
            {
                std::cerr << "[ERROR] Cannot import with duplicate filename < " << tokens[2] << " >" << std::endl;
            }
            else
            {
                import_file(tokens[1], tokens[2]);
                if(bufferFull)
                {
                    if(currentIMap.isFull())
                    {
                        char* imapStr = currentIMap.convertToString();
                        memcpy(&overflowBuffer[overBufPos],
                                imapStr,
                                sizeof(int) * 256);
                        delete imapStr;
                        currentIMap.clear();
                        chkptregion.addimap((BLK_SIZE * (currentSegment+1)) +
                                overBufPos/BLK_SIZE + 8);
                        if(DEBUG) std::cerr << "[DEBUG] imap is full, writing to buffer" << std::endl;
                    }

                    std::string segmentFile = "./DRIVE/SEGMENT" + std::to_string(currentSegment+1);

                    if (DEBUG) std::cerr << "Segment file: " << segmentFile << std::endl;
                    std::ofstream ofs(segmentFile);
                    if(!ofs.is_open())
                    {
                        std::cerr << "[ERROR] Could not open SEGMENT file for reading" << std::endl;
                        exit(1);
                    }
                    ofs.write(logBuffer, BLK_SIZE * BLK_SIZE);
                    ofs.close();

                    if(DEBUG)
                    {
                        std::cerr << "[DEBUG] Buffer is full, writing to segment" << std::endl;
                        std::cerr << "[DEBUG] overBufPos: " << overBufPos/BLK_SIZE << std::endl;
                    }

                    //Put overflowBuffer in logBuffer
                    if(overBufPos > 0)
                    {
                        memcpy(&logBuffer[8 * BLK_SIZE],
                                overflowBuffer,
                                overBufPos);
                        logBufferPos = 8 * BLK_SIZE + overBufPos;
                        overBufPos = 0;
                        if(DEBUG) std::cerr << "[DEBUG] Overflow buffer successfully written" << std::endl;
                    }
                    else
                    {
                        logBufferPos = 0;
                    }

                    // Update segment
                    chkptregion.markSegment(currentSegment, true);
                    currentSegment = chkptregion.getNextFreeSeg();

                    // Reset
                    bufferFull = false;
                }

                if(currentIMap.isFull())
                {
                    // Copy imap piece into buffer
                    char* imapStr = currentIMap.convertToString();
                    memcpy(&logBuffer[logBufferPos],
                            imapStr,
                            sizeof(int) * 256);

                    // Ready imap object for next imap
                    currentIMap.clear();
                    chkptregion.addimap((BLK_SIZE * currentSegment) + logBufferPos/BLK_SIZE);

                    delete imapStr;

                    if(DEBUG) std::cerr << "[DEBUG] imap is full, writing to buffer" << std::endl;
                }

            }
        }
        else if (tokens[0] == "remove" && tokens.size() == 2)
        {
            remove_file(tokens[1]);
        }
        else
        {
            std::cout << "[ERROR] Command not recognized; please try again..." << std::endl;
        }
    }

    /**
     * Exit in case of CTRL+D or EOF.
     */
    proper_exit();

    return 0;
}
