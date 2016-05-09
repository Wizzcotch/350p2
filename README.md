## CS350 Operating Systems (Spring 2016)

### Metadata

- LAB 7 README FILE
- Author(s): Miguel Lumapat, Denis Plotkin, Alan Plotko
- Team Name: Team M.A.D.

### Purpose

Simulate a log-structured filesystem within a folder labeled "DRIVE" that persists between multiple runs of the program.

### Compilation Instructions

Just extract the files and then run the following command: `make`

The makefile will generate the DRIVE folder, 1MB SEGMENT files, 192 byte CHECKPOINT_REGION file, and empty FILEMAP, since we are instructed to not worry about directories and indirect pointers, and just work with complex filenames in that file.
