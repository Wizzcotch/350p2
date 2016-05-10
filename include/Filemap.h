#ifndef FILEMAP_H
#define FILEMAP_H
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

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
        void removeFile(std::string filename);
        int getinodeNumber(std::string filename);
        std::unordered_map<std::string, int> getFilemap() { return filemap; }
        std::vector<std::pair<std::string, int> > populate();


};

#endif
