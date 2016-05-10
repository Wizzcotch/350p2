#include <iostream>
#include <string>

#include "buffer.h"

int main(int argc, char* argv[]) {
    if(argc == 1)
        exit(0);
    else {
        Buffer buffer(argv[1]);
        while(!buffer.isDone()) {
            char* str = buffer.readBytes();
            std::cout << str;
        }
        //std::cout << "\n====END OF FILE====" << std::endl;
        //std::cout << "Size of file: " << buffer.filesize << std::endl;
    }
    return 0;
}
