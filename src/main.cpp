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
#include "FileProcessor.h"
#include "filemap.h"
#include "chkptregion.h"
#include "IMap.h"
#include "INode.h"

#define BLK_SIZE 1024
#define MAX_FILE_NUM 10000
#define MAX_INODE_DPNTR 128 //Maximum number of data block pointers in an inode

#ifndef DEBUG
#define DEBUG 0
#endif

// Buffer
std::vector<char*> logBuffer(BLK_SIZE);
std::vector<char*> overflowBuffer(130); //128 data blocks + 1 inode + 1 imap
int logSeekPos, overflowSeekPos;
int currentSegment;
IMap currentIMap(0);
bool bufferFull;

// Filemap
Filemap filemap("./DRIVE/FILEMAP");
std::vector<std::pair<std::string, int> > filelist = filemap.populate();

// Checkpoint region
ChkptRegion chkptregion("./DRIVE/CHECKPOINT_REGION");

/**
 * TODO:
 * Handle overwrites
 * Write full buffer to segment
 */

void initialPad(std::vector<char*> buffer)
{
    char zeros[1024] = {0};
    for(int i = 0; i < 8; ++i)
    {
        buffer[i] = strdup(zeros);
    }

}

/**
 * Clear buffer
 */
void clear(std::vector<char*> buffer)
{
    for(auto iter = buffer.begin(); iter != buffer.end(); ++iter)
    {
        delete *iter;
    }
}

/**
 * Completes remaining writes and exits.
 */
void proper_exit()
{
    logBuffer.at(logSeekPos) = currentIMap.convertToString();
    chkptregion.addimap(logSeekPos+(currentSegment * BLK_SIZE));

    if (DEBUG) std::cerr << "Current imap: " << std::string(logBuffer.at(logSeekPos-1)) << std::endl;

    // Write buffer into the segment file
    std::string segmentFile = "./DRIVE/SEGMENT" + std::to_string(currentSegment+1);
    if (DEBUG) std::cerr << "Segment file: " << segmentFile << std::endl;
    std::ofstream ofs(segmentFile);

    if(ofs.is_open())
    {
        for(int i = 8; i < logSeekPos; ++i)
        {
            if (DEBUG) std::cerr << "index: " << i << std::endl;
            ofs << std::string(logBuffer[i]);
        }
        ofs.close();
    }
    clear(logBuffer);

    // Successfully exit
    exit(0);
}

/**
 * Lists the files in the LFS.
 */
void list_files()
{
    for (auto file : filelist)
    {
        std::cout << file.first << " (" << file.second << ")" << std::endl;
    }

    /*auto fileList = filemap.getFilemap();
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
                char *tmpBuffer = contents.c_str();
                char ptr = tmpBuffer[inodeNum];
            }
        }
        std::cout << currName << "\t" << inodeNum << std::endl;
    }*/
}

/**
 * Import specified file into LFS.
 */
void import_file(std::string& originalName, std::string& lfsName)
{
    const char *fname = originalName.c_str();
    FileProcessor processor(fname); // Open file
    char *bytes;

    /* Setup inode */
    INode i(lfsName);
    i.setSize(processor.filesize);
    if (DEBUG) std::cerr << "Filesize: " << processor.filesize << std::endl;

    while (!processor.isDone())
    {
        bytes = processor.readBytes();
        //std::cerr << "Finished reading bytes" << std::endl;
        //std::cerr << "logSeekPos: " << logSeekPos << ", logBuffer.size(): " << logBuffer.size() << std::endl;

        /* Write data into buffer here */
        if(logSeekPos < 1024)
        {
            logBuffer.at(logSeekPos) = strdup(bytes); //Replace c_string w/ string, shallow copy
        }
        else
        {
            //Write into overflowBuffer
            overflowBuffer.at(logSeekPos-1016) = strdup(bytes);
            overflowSeekPos = logSeekPos-1016;
            if(!bufferFull) bufferFull = true;
        }

        i.dataPointers.push_back((BLK_SIZE * currentSegment)+logSeekPos); //Absolute position in memory
        logSeekPos++;
    }

    // Insert inode at end of data blocks
    if(logSeekPos < 1024)
    {
        logBuffer.at(logSeekPos) = i.convertToString();
    }
    else
    {
        bufferFull = true;
        overflowSeekPos = logSeekPos-1016;
        overflowBuffer.at(overflowSeekPos) = i.convertToString();
    }

    if (DEBUG) std::cerr << "[DEBUG] Created INode" << std::endl;

    // Add inode to imap
    int createdInodeNum = currentIMap.addinode(logSeekPos+((currentSegment) * BLK_SIZE));

    // Add file-inode association to filemap
    filemap.addFile(lfsName, createdInodeNum);
    filelist.push_back(std::make_pair(lfsName, processor.filesize));
    ++logSeekPos;
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
    // Setup
    logSeekPos = 8; // Blocks 0-7 in each segment is the segment summary
    bufferFull = false;
    currentSegment = chkptregion.getNextFreeSeg();
    if (DEBUG) std::cerr << "Current segment: " << currentSegment << std::endl;
    initialPad(logBuffer);

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
        else if (tokens[0] == "show" && DEBUG)
        {
            for(int i = 0; i < logSeekPos+1; ++i)
                std::cout << std::string(logBuffer.at(i)) << std::endl;
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
                if(currentIMap.isFull())
                {
                    if(bufferFull)
                    {
                        overflowBuffer.at(overflowSeekPos) = currentIMap.convertToString();
                        overflowSeekPos = 0;
                    }
                    else
                    {
                        logBuffer.at(logSeekPos) = currentIMap.convertToString();
                    }
                    chkptregion.addimap(logSeekPos++);
                    currentIMap.clear();
                }
                if(bufferFull)
                {
                    std::string segmentFile = "./DRIVE/SEGMENT" + std::to_string(currentSegment+1);
                    std::ofstream ofs(segmentFile);

                    if(ofs.is_open())
                    {
                        for(auto iter : logBuffer)
                        {
                            ofs << std::to_string(*iter);
                        }
                        ofs.close();
                    }
                    clear(logBuffer);
                    initialPad(logBuffer);
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
