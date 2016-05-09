#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

class FileProcessor
{
    public:
        std::ifstream input;
        int filesize, position;
        ~FileProcessor();
        FileProcessor(const char *inputIn);
        std::string readLine();
        bool isDone();
        char* readBytes();

};

#endif
