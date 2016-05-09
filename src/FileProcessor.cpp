#include "FileProcessor.h"

// Destructor
FileProcessor::~FileProcessor()
{
    input.close();
}

// Constructor
FileProcessor::FileProcessor(const char *inputIn)
{
    std::ifstream input(inputIn, std::ifstream::ate | std::ifstream::binary);
    filesize = input.tellg();
    position = filesize;
    input.seekg(0, input.beg);
    if (!input)
    {
        std::cerr << "Error: could not open input file for reading" << std::endl;
        exit(1);
    }
}

// Read lines from input until EOF
std::string FileProcessor::readLine()
{
    std::string line;
    if (std::getline(input, line)) return line;
    return "";
}

bool FileProcessor::isDone() { return position == 0; }

// Read into byte array
char* FileProcessor::readBytes()
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
