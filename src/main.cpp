#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <vector>
#include "Filemap.h"
#include "ChkptRegion.h"
#include "IMap.h"
#include "INode.h"

#define BLK_SIZE 1024
#define MAX_FILE_NUM 10000
#define MAX_INODE_DPNTR 128 // Maximum number of data block pointers in an inode
#define SEGMENT_SIZE BLK_SIZE * BLK_SIZE
#define IMAP_PIECE_COUNT 40

#ifndef DEBUG
#define DEBUG 1
#endif

// Buffer
char* logBuffer = new char[SEGMENT_SIZE];
char* overflowBuffer = new char[130 * BLK_SIZE]; // 128 data blocks + 1 inode + 1 imap
int logBufferPos, overBufPos;
int currentSegment;
IMap currentIMap(0);
IMap* listIMap = new IMap[IMAP_PIECE_COUNT];
bool bufferFull;

// Flags
bool completedOperations = false; // Changes were made

// Filemap
Filemap filemap("./DRIVE/FILEMAP");
std::vector<std::pair<std::string, int> > filelist; //Only contains newly imported files

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
    chkptregion.addimap((BLK_SIZE * currentSegment) + (logBufferPos/BLK_SIZE));
    if (DEBUG) std::cerr << "Imap located at: " << (BLK_SIZE * currentSegment) + (logBufferPos/BLK_SIZE) << std::endl;
    //logBuffer.at(logBufferPos) = currentIMap.convertToString();
    //chkptregion.addimap(logBufferPos + (currentSegment * BLK_SIZE));

    //if (DEBUG) std::cerr << "Current imap: " << std::string(logBuffer.at(logBufferPos - 1)) << std::endl;

    // Update segment
    chkptregion.markSegment(currentSegment, true);

    // Write buffer into the segment file
    std::string segmentFile = "./DRIVE/SEGMENT" + std::to_string(currentSegment + 1);
    if (DEBUG) std::cerr << "Segment file: " << segmentFile << std::endl;
    std::ofstream ofs(segmentFile);
    if(!ofs.is_open())
    {
        std::cerr << "[ERROR] Could not open SEGMENT file for writing" << std::endl;
        exit(1);
    }
    ofs.write(logBuffer, SEGMENT_SIZE);
    ofs.close();

    // Successfully exit
    exit(0);
}

/**
 * Cats the specified file or displays a certain number of bytes.
 *
 * NEED TO DO:
 *  Check if startLoc exceed filesize
 *  Functions properly on data blocks between segments (as a result of overflow)
 *  Functions properly on data blocks currently in memory instead of in disk
 */
void cat_file(std::string& filename, int numBytes, int startLoc)
{
    int currentBlk = 0, currentPos = 0, howMany = -1;
    if (numBytes > -1) howMany = numBytes;
    if (startLoc > -1)
    {
        currentBlk = ((int)(startLoc / BLK_SIZE));
        currentPos = startLoc % BLK_SIZE;
    }

    // Get filenames and their respective inode numbers
    auto diskFileMap = filemap.getFilemap();

    // Ensure it's not a file being dealt in memory
    std::unordered_map<std::string, int>::const_iterator fileIt = diskFileMap.find(filename);

    if (fileIt == diskFileMap.end())
    {
        std::cerr << "[ERROR] File '" << filename << "' not found." << std::endl;
        return;
    }

    // Inode number for filename
    int inodeNum = fileIt->second;

    int blockNum = 0;
    for (int i = 0; i < IMAP_PIECE_COUNT; i++)
    {
        blockNum = listIMap[i].getBlockNumber(inodeNum);
        if (blockNum != -1)
        {
            //if (DEBUG) std::cerr << "INode Location: " << blockNum << std::endl;
            break;
        }
    }

    // Find inode entry in imap piece buffer
    if(blockNum == -1)
    {
        blockNum = currentIMap.getBlockNumber(inodeNum);
        if(DEBUG && blockNum != -1)
            std::cerr << "Block number found in currentIMap: " << blockNum
                << std::endl;
    }

    // Calculate inode location (i.e. segment)
    int idx = ((int)(blockNum / BLK_SIZE)) + 1;

    // Important variables
    int* dataPointers; //Array of data block pointers
    int remainingSeek = -1; //Remaining blocks to seek
    std::string segmentFile = "./DRIVE/SEGMENT" + std::to_string(idx);
    std::ifstream in;
    char inodeBuffer[BLK_SIZE];
    bool inodeInBuffer = false, segmentFileOpen = false;

    // Check if in memory or stored in segment
    if(idx-1 == currentSegment)
    {
        memcpy(&inodeBuffer, &logBuffer[(blockNum % BLK_SIZE) * BLK_SIZE], BLK_SIZE);
        memcpy((char*)&remainingSeek, &inodeBuffer[32], sizeof(int));
        if(DEBUG)
        {
            std::cerr << "blockNum: " << blockNum << std::endl;
            std::cerr << "remainingSeek: " << remainingSeek << std::endl;
            std::cerr << "INode location at: " << blockNum % BLK_SIZE << std::endl;
        }
        inodeInBuffer = true;
    }
    else
    {
        // Open segment file for reading inode information
        in.open(segmentFile, std::ifstream::binary);
        if(!in.is_open())
        {
            std::cerr << "[ERROR] Could not open segment file for reading" << std::endl;
            return;
        }

        segmentFileOpen = true;
        // Seek to inode information in segment file
        //in.seekg(((blockNum % BLK_SIZE) * BLK_SIZE) + 32 + sizeof(int));
        //int remainingSeek = (BLK_SIZE - 32 - sizeof(int)) / sizeof(int);
        in.seekg(((blockNum % BLK_SIZE) * BLK_SIZE) + 32);

        // Read filesize
        in.read((char*)&remainingSeek, sizeof(remainingSeek));
        if(DEBUG)
        {
            std::cerr << "[DEBUG](main-cat) Filesize: " <<  remainingSeek << std::endl;
        }

    }

    //int filesize = remainingSeek; //Filesize in bytes
    remainingSeek = (remainingSeek % BLK_SIZE == 0) ? remainingSeek / BLK_SIZE : remainingSeek / BLK_SIZE + 1;

    if(DEBUG)
    {
        std::cerr << "[DEBUG](main-cat) Filesize in blocks: " << remainingSeek << std::endl;
    }

    if(remainingSeek > 0) dataPointers = new int[remainingSeek];
    else
    {
        std::cout << "[ERROR] Invalid filesize" << std::endl;
        return;
    }

    if(!dataPointers)
    {
        std::cout << "[ERROR] Problem allocating memory in cat_file()" << std::endl;
        return;
    }

    // Put data block numbers in an array
    for (int i = 0; i < remainingSeek; i++)
    {
        dataPointers[i] = -1;
        if(!inodeInBuffer) in.read((char*)&dataPointers[i], sizeof(int));
        else memcpy((char*)&dataPointers[i], &inodeBuffer[32 + (i+1) * sizeof(int)], sizeof(int));
    }


    int fileFound = false;
    int catSegment = idx; //Segment that cat is currently in
    for (int i = 0; i < remainingSeek; i++)
    {
        //int dataPointers[i] = -1;
        //in.read((char*)&dataPointers[i], sizeof(dataPointers[i]));
        if(DEBUG)
        {
            std::cerr << "[DEBUG] dataPointers[i]: " << dataPointers[i] << std::endl;
            std::cerr << "catSegment: " << catSegment << std::endl;
            std::cerr << "fetching segment: " << dataPointers[i] / BLK_SIZE + 1 << std::endl;
        }

        int savePos = in.tellg();
        if (dataPointers[i] == -1) break;
        fileFound = true;
        //if (DEBUG) std::cerr << "Data Block Number: " << (dataPointers[i] % BLK_SIZE) * BLK_SIZE << std::endl;
        char* textBuffer = new char[BLK_SIZE];

        if((dataPointers[i] / BLK_SIZE) == currentSegment)
        {
            memcpy(textBuffer, &logBuffer[(dataPointers[i] % BLK_SIZE) * BLK_SIZE], BLK_SIZE);
        }
        /* ===== Start here for overflow ===== */
        else if((dataPointers[i] / BLK_SIZE) + 1 != catSegment) {
            if (DEBUG)
            {
                std::cerr << "[DEBUG] Data in different place" << std::endl;
                std::cerr << "catSegment: " << catSegment << std::endl;
                std::cerr << "fetching segment: " << dataPointers[i] / BLK_SIZE + 1 << std::endl;
            }

            // Load up the new segment and continue
            if(segmentFileOpen) in.close();
            catSegment = dataPointers[i] / BLK_SIZE + 1;
            segmentFile = "./DRIVE/SEGMENT" + std::to_string(catSegment);
            in.open(segmentFile, std::ifstream::binary);
            if(!in.is_open())
            {
                std::cerr << "[ERROR] Could not open segment file for reading" << std::endl;
                delete[] textBuffer;
                return;
            }

        }

        if(dataPointers[i] / BLK_SIZE != currentSegment)
        {
            in.seekg((dataPointers[i] % BLK_SIZE) * BLK_SIZE);
            in.read(textBuffer, BLK_SIZE);
        }

        if (currentBlk > 0)
        {
            currentBlk--;
        }
        else
        {
            std::string s = "";

            // Display portion of file
            if (howMany != -1)
            {
                for (int j = currentPos; j < BLK_SIZE; j++)
                {
                    // Force quit reading if we hit our threshold of number of bytes to read
                    if (howMany <= 0)
                    {
                        remainingSeek = 0;
                        break;
                    }
                    s += textBuffer[j];
                    howMany--;
                }
            }
            // Display whole file
            else
            {
                for (int j = currentPos; j < BLK_SIZE; j++)
                {
                    s += textBuffer[j];
                }
            }

            std::cout << s;
            currentPos = 0;
        }
        delete[] textBuffer;
        if(segmentFileOpen && dataPointers[i] / BLK_SIZE != currentSegment) in.seekg(savePos);
    }
    std::cout << std::endl;

    if (!fileFound)
    {
        std::cerr << "[ERROR] File '" << filename << "' not found." << std::endl;
        in.close();
        return;
    }
    if(segmentFileOpen) in.close();
}

/**
 * Lists the files in the LFS.
 */
void list_files()
{
    // Get filenames and their respective inode numbers
    auto diskFileMap = filemap.getFilemap();

    // Traverse over filenames
    for (auto it = diskFileMap.begin(); it != diskFileMap.end(); ++it)
    {
        std::string currName = it->first;   // Filename

        // Ensure it's not a file being dealt in memory
        int filelistSize = filelist.size();
        bool fileFound = false;
        for (int i = 0; i < filelistSize; i++)
        {
            if (filelist[i].first.compare(currName) == 0)
            {
                fileFound = true;
                break;
            }
        }

        if (!fileFound)
        {
            int inodeNum = it->second;  // Inode number for filename
            //if (DEBUG) std::cerr << "Listing Block #: " << inodeNum << std::endl;

            int blockNum = 0;
            for (int i = 0; i < IMAP_PIECE_COUNT; i++)
            {
                blockNum = listIMap[i].getBlockNumber(inodeNum);
                if (blockNum != -1)
                {
                    //if (DEBUG) std::cerr << "INode Location: " << blockNum << std::endl;
                    break;
                }
            }

            // Calculate inode location
            int idx = ((int)(blockNum / BLK_SIZE)) + 1;
            //std::cerr << "[DEBUG] blockNum: " << blockNum << std::endl;
            // Open segment file for reading inode information
            std::string segmentFile = "./DRIVE/SEGMENT" + std::to_string(idx);
            std::ifstream in(segmentFile, std::ifstream::binary);
            if(!in.is_open())
            {
                std::cerr << "[ERROR] Could not open segment file for reading" << std::endl;
                std::cerr << "Filename causing error: " <<  segmentFile << std::endl;
                exit(1);
            }

            // Seek to inode information in segment file
            in.seekg(((blockNum % BLK_SIZE) * BLK_SIZE) + 32);
            int filesize;
            in.read((char*)&filesize, sizeof(filesize));
            std::cout << currName << "\t\t" << filesize << " bytes" << std::endl;
            // Close file
            in.close();
        }
    }

    int filelistSize = filelist.size();
    for (int i = 0; i < filelistSize; i++)
    {
        std::cout << filelist[i].first << "\t\t" << filelist[i].second << " bytes" << std::endl;
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
            //if (DEBUG) std::cerr << (BLK_SIZE * currentSegment) + logBufferPos/BLK_SIZE << std::endl;
            for (int offset = 0; offset < BLK_SIZE && bufferPos + offset < size; offset++)
            {
                logBuffer[logBufferPos + offset] = buffer[bufferPos + offset];
            }

            logBufferPos += BLK_SIZE;
        }
        else
        {
            if(!bufferFull)
            {
                bufferFull = true;
                if(DEBUG) std::cerr << "[DEBUG] Writing into overflow buffer" << std::endl;
            }
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
    if(!bufferFull && logBufferPos < SEGMENT_SIZE)
    {
        memcpy(&logBuffer[logBufferPos], inodeStr, sizeof(INodeInfo));
        createdInodeNum = currentIMap.addinode((BLK_SIZE * currentSegment) + (logBufferPos/BLK_SIZE));
        if (DEBUG) std::cerr << "[DEBUG] INode # " << createdInodeNum << " at location " << (BLK_SIZE * currentSegment) + (logBufferPos/BLK_SIZE) << std::endl;
        logBufferPos += BLK_SIZE;
    }
    else
    {
        memcpy(&overflowBuffer[overBufPos], inodeStr, sizeof(INodeInfo));
        createdInodeNum = currentIMap.addinode((BLK_SIZE * (currentSegment + 1)) + overBufPos/BLK_SIZE + 8);
        if (DEBUG) std::cerr << "[DEBUG] INode # " << createdInodeNum << " at location " << (BLK_SIZE * (currentSegment + 1)) + overBufPos/BLK_SIZE + 8<< std::endl;
        overBufPos += BLK_SIZE;
    }

    delete inodeStr;

    // Add file-inode association to filemap
    filemap.addFile(lfsName, createdInodeNum);
    filelist.push_back(std::make_pair(lfsName, size));
    if (DEBUG) std::cerr << "[DEBUG] Import Complete" << std::endl;

    // Close file
    ifs.close();
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
    if (DEBUG) std::cerr << "Current segment file: ./DRIVE/SEGMENT" << currentSegment + 1<< std::endl;

    // Grab contents of current segment
    std::string segmentFile = "./DRIVE/SEGMENT" + std::to_string(currentSegment + 1);

    // Open segment for reading
    std::ifstream ifs(segmentFile, std::ifstream::binary);
    if(!ifs.is_open())
    {
        std::cerr << "[ERROR] Could not open '" << segmentFile << "' file for reading" << std::endl;
        exit(1);
    }
    ifs.read(logBuffer, SEGMENT_SIZE);
    ifs.close();

    // Initialize buffers' positions
    logBufferPos = 8 * BLK_SIZE; // Start buffer after segment summary blocks
    overBufPos = 0;

    // Set up imap array and last used imap piece
    auto imapPieceLocs = chkptregion.getimapArray();
    for (int i = 0; i < IMAP_PIECE_COUNT; i++)
    {
        listIMap[i] = IMap(-1);
        if (DEBUG && imapPieceLocs[i] != 0) std::cerr << "Setting up imap at location: " << imapPieceLocs[i] << std::endl;
        listIMap[i].setUpImap(std::make_pair(imapPieceLocs[i], i), false);
    }
    currentIMap.setUpImap(chkptregion.getLastImapPieceLoc(), true);

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
            if (completedOperations)
            {
                proper_exit();
            }
            else
            {
                exit(0);
            }
        }
        else if (tokens[0] == "list" && tokens.size() == 1)
        {
            list_files();
        }
        else if (tokens[0] == "cat" && tokens.size() == 2)
        {
            cat_file(tokens[1], -1, -1);
        }
        else if (tokens[0] == "display" && tokens.size() == 4)
        {
            cat_file(tokens[1], std::stoi(tokens[2]), std::stoi(tokens[3]));
        }
        else if (tokens[0] == "import" && tokens.size() == 3)
        {
            completedOperations = true;
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
                    ofs.write(logBuffer, SEGMENT_SIZE);
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
            completedOperations = true;
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

    if (completedOperations)
    {
        proper_exit();
    }
    else
    {
        exit(0);
    }

    return 0;
}
