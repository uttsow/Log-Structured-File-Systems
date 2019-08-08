#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <array>
#include <vector>





using namespace std;




/*
    This file defines all the information about this file system such as the block size, segment sizes, max file size, max data blocks...etc

    I thought this was easiest to keep track of stuff
*/

#define BLOCK_SIZE 1024
#define SEGMENT_SIZE 1024*1024
#define DIRECT_DATA_BLOCK 128
#define INODE_SIZE 1024
#define MAX_FILE_SIZE BLOCK_SIZE*DIRECT_DATA_BLOCK //131072 bytes
#define MAX_NUMBER_OF_FILES 10240
#define CHECKPOINT_REGION_SIZE 224




#endif
