#ifndef FILEMAP_H
#define FILEMAP_H
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

class Filemap
{
    private:
        std::string filename;
        std::unordered_map<std::string, int> filemap;

    public:
        Filemap() = delete;
        Filemap(std::string filename);
        ~Filemap();
        void addFile(std::string filename, int inode);
        int getinodeNumber(std::string filename);


};

#endif
