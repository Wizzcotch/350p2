#include "SegmentSummary.h"


SegmentSummary::SegmentSummary(){
  for(int i = 0; i < 1016; i++){
    sumBlock.dataBlockInfo[i].inode = -4;
    sumBlock.dataBlockInfo[i].block = i + 8;
  }
  
}

SegmentSummary::~SegmentSummary(){} 

void SegmentSummary::UpdateBlockInfo(int index, int inode){
  sumBlock.dataBlockInfo[index].inode = inode;
  sumBlock.dataBlockInfo[index-8].block = index;
}

std::pair<int,int> SegmentSummary::getBlockInfo(int index){
return std::make_pair(sumBlock.dataBlockInfo[index].inode,sumBlock.dataBlockInfo[index].block);
}


char* SegmentSummary::toString(){
  char * retChar = new char[sizeof(sumBlock)];
  memcpy(retChar, &sumBlock, sizeof(sumBlock));
  return retChar;
}
