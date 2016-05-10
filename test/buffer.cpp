#include "buffer.h"

// Destructor
Buffer::~Buffer()
{
    input.close();
}

// Constructor
Buffer::Buffer(const char *inputIn)
{
    std::ifstream input(inputIn, std::ifstream::ate | std::ifstream::binary);
    filesize = input.tellg();
    position = filesize;
    input.seekg(0, std::ios::beg);
    if (!input)
    {
        std::cerr << "Error: could not open input file for reading" << std::endl;
        exit(1);
    }
}

// Read lines from input until EOF
std::string Buffer::readLine()
{
    std::string line;
    if (std::getline(input, line)) return line;
    return "";
}

bool Buffer::isDone() { return position == 0; }

// Read into byte array
char* Buffer::readBytes()
{
    //std::string dataBlock;
    char *memory;
    if (position - 1024 >= 0)
    {
        position -= 1024;
        memory = new char[1024];
        input.read(memory, 1024);
    }
    else
    {
        position = 0;
        memory = new char[position];
        input.read(memory, position);
    }

    return memory;
}
