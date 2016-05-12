#ifndef SEGMENTSUMMARY_H
#define SEGMENTSUMMARY_H

#include <iostream>                                                                 
#include <string>                                                                   
#include <vector>
#include <string>                                                                   
#include <cstring>
#include <array>
#include <utility>

typedef struct {
  int inode;
  int block;
} Node;

typedef struct {
  // pair<INode #, dataBlock Number>
  Node dataBlockInfo[1016];
} SummaryBlock;

class SegmentSummary{
 private:
  
 public:
  ~SegmentSummary();
  SummaryBlock sumBlock;
  SegmentSummary();
  void UpdateBlockInfo(int index, int inode);
  std::pair<int,int> getBlockInfo(int index);
  char * toString();
  
};


#endif
