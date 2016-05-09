#include "filemap.h"

#include <sstream>

Filemap::Filemap(std::string filename): filename(filename)
{
    std::ifstream ifs(filename);
    if(ifs.is_open())
    {
        std::string inputBuffer;
        while(std::getline(ifs, inputBuffer))
        {
            std::stringstream ss(inputBuffer);
            std::string fname, inodeNumber; //File names contained in filemap file
            ss >> fname;
            ss >> inodeNumber;
            if(ss.fail())
            {
                std::cerr << "[ERROR] Wrong format detected in filemap" << std::endl;
                ifs.close();
                exit(1);
            }
            addFile(fname, std::stoi(inodeNumber));
        }

        ifs.close();
    }
    else
    {
        std::cerr << "[ERROR] Could not open filemap file" << std::endl;
        exit(1);
    }
}
Filemap::~Filemap()
{
    std::ofstream ofs(filename);
    if(ofs.is_open())
    {
        for(auto iter = filemap.begin(); iter != filemap.end(); ++iter)
        {
            //std::cerr << iter->first << ": " << iter->second << std::endl;
            ofs << iter->first << " " << std::to_string(iter->second) << std::endl;
        }
        ofs.close();
    }
    else
    {
        std::cerr << "[ERROR] Could not open filemap file" << std::endl;
        exit(1);
    }
}
void Filemap::addFile(std::string filename, int inode)
{
    //std::cerr << filename << ": " << inode << std::endl;
    filemap[filename] = inode;
}

void Filemap::removeFile(std::string filename)
{
    //std::cerr << filename << ": " << inode << std::endl;
    filemap.erase(filename);
}

int Filemap::getinodeNumber(std::string filename)
{
    return filemap[filename];
}

std::vector<std::pair<std::string, int> > Filemap::populate()
{
    std::vector<std::pair<std::string, int> > list;
    for (auto file : filemap)
    {
        list.push_back(std::make_pair(file.first, 0));
    }
    return list;
}