#include <algorithm>
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
#define DEBUG 1
#endif

// Checkpoint Region
std::vector<IMap> cRegion;

// INodes
std::vector<INode> inodes;

// Buffer
std::vector<char*> logBuffer(BLK_SIZE);
std::vector<char*> overflowBuffer(130); //128 data blocks + 1 inode + 1 imap
int logSeekPos;
int currentSegment;
IMap currentIMap(0);

// Filemap
Filemap filemap("./DRIVE/FILEMAP");

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
    std::cout << "[TO DO] Proper exit here" << std::endl;
    // Write buffer into the segment file

    // Successfully exit
    exit(0);
}

/**
 * Lists the files in the LFS.
 */
void list_files()
{
    //std::cout << "[TO DO] List files here" << std::endl;
    //for (auto map : cRegion)
    //{
        //for (auto location : map.inodeList)
        //{
            //std::cout << inodes[location].filename << std::endl;
        //}
    //}
}

/**
 * Import specified file into LFS.
 */
void import_file(std::string& originalName, std::string& lfsName)
{
    //std::cout << "[TO DO] Import file < " << originalName << " > as < " << lfsName << " >" << std::endl;
    const char *fname = originalName.c_str();
    FileProcessor processor(fname); // Open file
    char *bytes;

    /* Setup inode */
    INode i(lfsName);
    i.setSize(processor.filesize);

    while (!processor.isDone())
    {
        bytes = processor.readBytes();

        /* Write data into buffer here */
        if(logSeekPos < 1024)
        {
            logBuffer.at(logSeekPos) = strdup(bytes); //Replace c_string w/ string, shallow copy
        }
        else
        {
            //Do something because buffer is full
            //Write into overflowBuffer
            overflowBuffer.at(logSeekPos-1024) = strdup(bytes);
        }

        i.dataPointers.push_back((BLK_SIZE * currentSegment-1)+logSeekPos); //Absolute position in memory
        logSeekPos++;
    }

    /* Insert inode at end of data blocks */
    if(logSeekPos < 1024)
    {
        logBuffer.at(logSeekPos) = i.convertToString();
    }
    else
    {
        overflowBuffer.at(logSeekPos-1024) = i.convertToString();
    }

    if (DEBUG) std::cerr << "[DEBUG] Created INode" << std::endl;

    int createdInodeNum = currentIMap.addinode(logSeekPos+((currentSegment-1) * BLK_SIZE));
    ++logSeekPos;
    if (DEBUG) std::cerr << "[DEBUG] INode # " << createdInodeNum << " is tracked in IMap # " << 0 << std::endl;
    if (DEBUG) std::cerr << "[DEBUG] Import Complete" << std::endl;
}

/**
 * Remove specified file from LFS.
 */
void remove_file(std::string& filename)
{
    //std::cout << "[TO DO] Remove file < " << filename << " >" << std::endl;
    /*IMap *map;*/

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
    /*}*/
    std::cerr << "[ERROR] File < " << filename << " > not found (does it exist?)" << std::endl;
}

int main(int argc, char *argv[])
{
    /* Setup */
    logSeekPos = 0;
    currentSegment = 1;
    currentIMap = IMap(0);

    // Exit with an error message if argument count is incorrect (i.e. expecting one: input file path)
    if (argc != 1)
    {
        std::cerr << argv[0] << ": program does not take arguments; commands are sent as input, not arguments" << std::endl;
        exit(1);
    }

    /**
     * Complete setup here
     *     - Check if DRIVE folder exists
     *         - Yes: Load metadata, and maintain LFS from last session.
     *         - No: Set up LFS with segments.
     */
    struct stat st = {0};

    /**
     * Create DRIVE folder in current directory
     * if it does not already exist
     */
    if (stat("DRIVE", &st) == -1)
    {
        if (mkdir("DRIVE", 0755) != 0)
        {
            perror("[ERROR] Could not create DRIVE folder during setup");
            exit(1);
        }
        else if (DEBUG)
        {
            std::cerr << "[DEBUG] Created DRIVE folder" << std::endl;
        }

        /**
         * Set up 32 segments of 1 MB size each
         */
        for (int i = 1; i <= 32; i++)
        {
            std::string name = "./DRIVE/SEGMENT" + std::to_string(i);
            FILE *fp = fopen(name.c_str(), "w");

            if (fp == NULL)
            {
                perror("[ERROR] Could not open file during setup");
                exit(1);
            }

            if (ftruncate(fileno(fp), 1024 * 1024) != 0)
            {
                perror("[ERROR] Could not write to file during setup");
                exit(1);
            }

            fclose(fp);
            if (DEBUG) std::cerr << "Wrote file " << i << " of 32" << std::endl;
        }
    }
    else if (DEBUG)
    {
        std::cerr << "[DEBUG] DRIVE folder exists" << std::endl;
    }

    /**
     * Process command
     */
    std::string command;
    if (DEBUG) std::cerr << "[DEBUG] Now accepting commands" << std::endl;
    while (std::getline(std::cin, command))
    {
        if (command.empty() || std::all_of(command.begin(), command.end(), isspace))
        {
            std::cout << "Command not recognized; please try again..." << std::endl;
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
            import_file(tokens[1], tokens[2]);
        }
        else if (tokens[0] == "remove" && tokens.size() == 2)
        {
            remove_file(tokens[1]);
        }
        else
        {
            std::cout << "Command not recognized; please try again..." << std::endl;
        }

        /* Perform write operations here as necessary */
    }

    /**
     * Exit in case of CTRL+D or EOF.
     */
    proper_exit();

    return 0;
}
