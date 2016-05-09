## CS350 Operating Systems

Spring 2016
LAB 7 README FILE

Author(s): Miguel Lumapat, Denis Plotkin, Alan Plotko
Team Name: Team M.A.D.

### PURPOSE:

Lab 7 begins the part of Program 2 to simulate LFS.

### FILES:

Included with this project are 13 files:

1) FileProcessor.cpp, for file operations
2) FileProcessor.h, the header file for a file processor
3) main.cpp, the main file
4) makefile, the file to build the lab 7 program and required DRIVE files
5) README.txt, the text file you are presently reading.
6) IMap.cpp, for representing an imap
7) IMap.h, the header file for an imap
8) INode.cpp, for representing an inode
9) INode.h, the header file for an inode
10) chkptregion.cpp, for processing to and from the checkpoint region file
11) chkptregion.h, the header file for the checkpoint region file
12) filemap.cpp, the file to store the associations, as per the instructions to not work with directories and indirect pointers
13) filemap.h, the header file for creating the filemap

### TO COMPILE:

Just extract the files and then run the following command: make

### GENERATING DRIVE FILES:

The makefile will generate the DRIVE folder, 1MB SEGMENT files, 192 byte CHECKPOINT_REGION file, and empty FILEMAP, since we are instructed to not worry about directories and indirect pointers, and just work with complex filenames in that file.
