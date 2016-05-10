#ifndef BUFFER_H
#define BUFFER_H

#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

class Buffer
{
    public:
        std::ifstream input;
        int filesize, position;
        ~Buffer();
        Buffer(const char *inputIn);
        std::string readLine();
        bool isDone();
        char* readBytes();

};

#endif
