#include "Descriptors.hpp"

typedef unsigned int blockNumber;
void writeToCheckpointRegion();

std::ostream& bold_on(std::ostream& os){
    return os << "\033[1m";
}


std::ostream& bold_off(std::ostream& os){
    return os << "\033[0m";
}

/***************************************************

STRUCTS

*****************************************************/
struct Data{
    array<char, BLOCK_SIZE> data;
    Data(){
        for(blockNumber i =0; i < data.size(); i++){
            data[i] = 0;
        }
    }
};

//Basic structure of the Inode. Status = 1 = Alive, 0 = dead;
struct Inode{
    char filename[128];
    int filesize;
    array<blockNumber, DIRECT_DATA_BLOCK> DirectDataBlock;
    char dummy[376];
    int status;
};

//Basic structure of the Imap. This will be global!
struct Imap{
    array<int, MAX_NUMBER_OF_FILES> imap;
    int index = 0;
};

//Maps file name to inode number
struct Filemap{
    int inodeNumber;
    string filename;
};

//Checkpoint region. Maps imap to block number and keeps track of segments
struct Checkpoint_Region{
    array<blockNumber, 40> imapLocation;
    char live_bits[64];
};

struct Segement_Entry{
    blockNumber InodeNumber;
    blockNumber blockNumber;
};

//Imap
Imap global_imap;

/***************************************************

MAIN BLOCK STRUCT. USE THIS TO CREATE ALL SEG BLOCKS

*****************************************************/
struct Block{
    blockNumber imapData[BLOCK_SIZE/4];
    int block_counter = 0;
    string blockType;
    Inode inodeDataBlock;
    Data dataBlock;

    /*
    Block Initalization
    */

    Block(string type){
        blockType = type;
    }

    Block(){

    }

    /*
    DATA FUNCTIONS
    */


    //This will copy the data from file onto this array
    void setData(char *aData){
        for(int i =0; i < BLOCK_SIZE; i++){
            if(strcmp(aData, "") != 0){
                dataBlock.data[i] = aData[i];
            }
        }
    }
    Data& getDataBlock(){
        return dataBlock;
    }

    /*
    INODE FUNCTIONS
    */

    void setInodeName(string filename){
        strcpy(inodeDataBlock.filename, filename.c_str());

        //set inode data blocks all to 130;
        for(unsigned i = 0; i < inodeDataBlock.DirectDataBlock.size(); i++){
            inodeDataBlock.DirectDataBlock[i] = 130;
        }

    }
    void setInodeFilesize(int filesize){
        inodeDataBlock.filesize = filesize;
    }
    void setInodeStatus(int status){
        inodeDataBlock.status = status;
    }
    Inode& getInodeBlock(){
        return inodeDataBlock;
    }

    void addDirectDataBlock(int num){
        inodeDataBlock.DirectDataBlock[block_counter] = num;
        block_counter++;
    }

    /*
    IMAP FUNCTIONS
    */
    //Initalizes imapdata block
    void imapDataBlock(){
        for(int i =0; i < BLOCK_SIZE/4; i++){
            imapData[i] = 0;
        }
    }

    //Add inode number
    void addImapData(blockNumber index){
        int inital = index;
        for(unsigned int i = inital; i < index+256; i++){
            if(global_imap.imap[i] == 0){
                cout << "Added " << i << " number of inodes to imap" << endl;
                break;
            }
            cout << "Adding inodes# " <<global_imap.imap[i] << "into imap" << endl;
            imapData[i] = global_imap.imap[i];
        }
    }


    blockNumber* getImapBlockData(){
        return imapData;
    }

    /*
    GENERAL BLOCK FUNCTONS
    */

    blockNumber getBlockCounter(){
        return block_counter;
    }


};
/***************************************************

GLOBAL VARIABLES

*****************************************************/

//File map
// Filemap globalFileMap[MAX_NUMBER_OF_FILES];
array<Filemap, MAX_NUMBER_OF_FILES> globalFileMap;
unsigned int globalFileMap_counter = 0;

//current segment
unsigned int current_segment = 0;


//Global checkpoint region to map the imap
Checkpoint_Region checkpoint_region;
int checkpoint_region_counter = 0;


/***************************************************

WRITER: THIS KEEPS TRACK OF MEMSEGMENT AND WRITES TO DISK

*****************************************************/
struct Writer{
    //Will contain all blocks to be added to disk
    Block *memorySegment[BLOCK_SIZE];
    int numberOfBlocks = 0;
    int numberOfInodes = 0;
    char *writerSegement = new char[SEGMENT_SIZE];
    char empty[BLOCK_SIZE] = {};


    /*
    WRITER FUNCTIONS
    */

    void addBlockToSegment(Block *blk){
        memorySegment[numberOfBlocks % BLOCK_SIZE] = blk;
        numberOfBlocks++;

        //Write to disk when the segment is full
        if(numberOfBlocks % 1023 == 0){
            //Function for writing to disk
            cout << "The Segment#" << current_segment << " is full. Writing to segment. Please hold on" << endl;
            writeSegmentToDisk();
        }
    }
    // long size = (sizeof(memorySegment)/BLOCK_SIZE);
    // cout << "Helo"
    //Memory segment is full. Need to write to disk
    void writeSegmentToDisk(){
        // unsigned int tempCurrSegement = current_segment;
        // ofstream writing("DRIVE/SEGMENT" + to_string(tempCurrSegement), ios::out | ios::binary);

        //adding all the stuff in memorySegment to char segement
        for(int i =0; i <numberOfBlocks; i++){
            if(memorySegment[i]->blockType == "data"){
                memcpy((writerSegement+(i*BLOCK_SIZE)), &memorySegment[i]->getDataBlock().data, BLOCK_SIZE);
                // writing.write((char*)&memorySegment[i]->getDataBlock().data, BLOCK_SIZE);
            }else if(memorySegment[i]->blockType == "inode"){
                memcpy((writerSegement+(i*BLOCK_SIZE)), (const char*)&memorySegment[i]->getInodeBlock(), BLOCK_SIZE);
                // writing.write((char*)&memorySegment[i]->getInodeBlock(), BLOCK_SIZE);
            }else if(memorySegment[i]->blockType == "imap"){
                memcpy((writerSegement+(i*BLOCK_SIZE)), memorySegment[i]->getImapBlockData(), BLOCK_SIZE);
                // writing.write((char*)&memorySegment[i]->imapData, BLOCK_SIZE);

            }else{
                memcpy((writerSegement+(i*BLOCK_SIZE)), empty, BLOCK_SIZE);
                // writing.write((char*)&empty, BLOCK_SIZE);
            }
        }

        cout << "The Segment#" << current_segment << " is being written to. Please hold on" << endl;


        unsigned int tempCurrSegement = current_segment;
        ofstream writing("DRIVE/SEGMENT" + to_string(tempCurrSegement), ios::out | ios::binary);
        writing.write(writerSegement, SEGMENT_SIZE);

        writing.close();
        //current segment has live data
        // checkpoint_region.live_bits[tempCurrSegement] = 1;

        current_segment++;

        cout << "Segment writing finished. Segement#" << tempCurrSegement << " now has live data."<< endl;
        cout <<"New segment we are working on when we restart will be: Segement#" << current_segment << endl;

        delete[] writerSegement;
    }

    void addInode(){
        numberOfInodes++;
        //cout << numberOfInodes << endl;
        if(numberOfInodes % ((BLOCK_SIZE/4)-1) == 0)
        writeToCheckpointRegion();
    }


Block* getMemorySegmentBlock(int index){
    return memorySegment[index];
}


} writer;




/***************************************************

HELPER FUNCTIONS (INODES)

*****************************************************/

//Used for finding the inode given the inode number
Inode& findInode(int index){
    blockNumber inodeBlock = global_imap.imap[index];

    blockNumber segmentBlock = inodeBlock % BLOCK_SIZE;

    blockNumber segment = (inodeBlock/BLOCK_SIZE);
    Inode *tempInode= nullptr;
    //The inode is in the current segment. get it from memory
    if(segment == current_segment){
        Block *temp;
        temp = writer.getMemorySegmentBlock(segmentBlock);
        Inode *node = &temp->getInodeBlock();
        // delete temp;
        return *node;
    }else{
        //Not in current segement. So looks for it in other segments

        ifstream segmentLook("DRIVE/SEGMENT"+ to_string(segment), ios::binary);

        //go to the segment where the inode block
        segmentLook.seekg(segmentBlock*BLOCK_SIZE);


        Block *inode = new Block("inode");
        segmentLook.read((char*)&inode->inodeDataBlock, BLOCK_SIZE);

        tempInode = &inode->getInodeBlock();
        // delete inode;
        segmentLook.close();

        return *tempInode;


    }
    cout <<"Hmmmm...thats not right. Couldnt find the inode" << endl;
    Inode *fault;
    fault->filesize = -9999999;
    return *fault;
}


/***************************************************

HELPER FUNCTIONS (SHUTDOWN)

*****************************************************/
void writeToFileMap(){
    cout << "Writing to file map file" << endl;
    cout <<endl;
    ofstream fileMapNames;
    fileMapNames.open("DRIVE/FILE_MAP");
    int i = 0;
    while(i < writer.numberOfInodes){
        fileMapNames << globalFileMap[i].inodeNumber << " " << globalFileMap[i].filename << endl;
        cout << "Writing filename: " << globalFileMap[i].filename << " with Inode Number: " << globalFileMap[i].inodeNumber << " to File Map" << endl;
        i++;
    }
    fileMapNames.close();
}

void writeToCheckpointRegion(){
    // for(int i =0; i < 2; i++){
    //     cout << checkpoint_region.imapLocation[i] << endl;
    // }
    cout << "Writing Checkpoint to checkpoint region" << endl;
    cout << endl;
    checkpoint_region.live_bits[current_segment] = 1;
    cout << "Setting segement#" << current_segment << " as live in checkpoint region" << endl;
    ofstream checkpoint("DRIVE/CHECKPOINT_REGION",ios::binary);
    checkpoint.write((char*)&checkpoint_region, sizeof(checkpoint_region));
    checkpoint.close();

    writeToFileMap();
}

void initalizeShutdown(){
    //Go ever 256 bytes to add imap block
    cout << "Starting shutdown. Wont be too long!" << endl;
    cout << endl;
    for(int i =0 ; i<BLOCK_SIZE; i+= 256){
        if(global_imap.imap[i] == 0){
            cout << "Global imap at position " << i << " is 0. No inode is there" << endl;
            break;
        }
        //create the imap block and add every inode from 0-255 in global imap
        Block *imapBlock = new Block("imap");
        imapBlock->addImapData(i);
        //Add it to the memeory segment
        writer.addBlockToSegment(imapBlock);

        //add imap data to the global checkpoint
        int blockNumber = (BLOCK_SIZE * current_segment + writer.numberOfBlocks) -1;
        cout << "Adding imap block #" << blockNumber <<  " to checkpoint region." << endl;

        if(checkpoint_region_counter >= 40){
            cout << "Checkpoint regions imap location array is filled. You need to do some cleaning" << endl;
            return;
        }

        checkpoint_region.imapLocation[checkpoint_region_counter] = blockNumber;
        checkpoint_region_counter++;
    }

    //Now that everythings done, now write checkpoint to the actual checkpoint region
    writeToCheckpointRegion();
}




/***************************************************

MAIN FUNCTIONS

*****************************************************/


void import(string filename, string lfsFilename){
    ifstream input_file(filename, ios::ate| ios::binary);
    array<blockNumber, DIRECT_DATA_BLOCK> tempDataBlock = {0};

    if(input_file.is_open()){

        //1. Get filesize
        unsigned int filesize = input_file.tellg();
        if(filesize > MAX_FILE_SIZE){
            perror("File size wayyy too large. Import a smaller file");
            exit(EXIT_FAILURE);
        }

        //2. Create Inode blocks. It will map the data blocks
        Block *inodeBlock = new Block("inode");
        inodeBlock->setInodeName(lfsFilename);
        inodeBlock->setInodeFilesize(filesize);
        inodeBlock->setInodeStatus(1);
        cout << "Created Inode with Filename: " << inodeBlock->inodeDataBlock.filename << " and size: " << inodeBlock->inodeDataBlock.filesize << endl;

        //3. Iterate through file and store data in Data file
        for(blockNumber i =0; i < filesize; i+=BLOCK_SIZE){
            input_file.seekg(i);
            char data[BLOCK_SIZE] = {0};
            input_file.read(data, BLOCK_SIZE);


            //Store the data in the DATA Block
            Block *dataBlock = new Block("data");
            dataBlock->setData(data);

            //Add to memory segment
            cout <<"Adding data block to memory segement" << endl;
            writer.addBlockToSegment(dataBlock);

            //Get block number
            int blockNumber = (BLOCK_SIZE * current_segment + writer.numberOfBlocks) -1;

            cout <<"Inode with filename: " << inodeBlock->inodeDataBlock.filename << " adding to direct data block directory. Block number: " <<blockNumber <<endl;
            inodeBlock->addDirectDataBlock(blockNumber);

            //This will be used for segment summary
            tempDataBlock[i/BLOCK_SIZE]= blockNumber;
        }




        //4. Add inode to the memory segment
        cout << "Adding Inode with Filename: " << inodeBlock->inodeDataBlock.filename << " and size: " << inodeBlock->inodeDataBlock.filesize << " to memory segment!" << endl;
        writer.addBlockToSegment(inodeBlock);
        writer.addInode();

        cout <<"Memeory segment size currently: " << writer.numberOfBlocks << endl;

        //Get inode number
        //unsigned int inodeNumber = BLOCK_SIZE * current_segment + writer.numberOfBlocks;
        unsigned int inodeNumber = writer.numberOfInodes - 1;
        int blockNumber = (BLOCK_SIZE * current_segment + writer.numberOfBlocks) -1;
        //cout << inodeNumber << ", " << writer.numberOfInodes;

        //5.Update imap
        cout << "Adding Inode #" << inodeNumber << " with a blocknumber of "<< blockNumber << " to imap and file map!" << endl;
        global_imap.imap[inodeNumber] = blockNumber;
        global_imap.index++;


        //6. Update file map
        globalFileMap[globalFileMap_counter].filename = lfsFilename;
        globalFileMap[globalFileMap_counter].inodeNumber = inodeNumber;
        globalFileMap_counter++;




        cout << "Size of imap is: " << global_imap.index << endl;
        cout << "Size file map is: " << globalFileMap_counter <<endl;


    }else{
        cout<< "File doesnt exist! Check the inputs please" << endl;
        return;
    }
}



void list(){
    cout << "I see you wanna see all the files in this directory"<< endl;
    cout <<"\t\tJust a moment....." << endl;
    cout <<"-----------------FILES----------------------" << endl;
    if(globalFileMap_counter != 0){

        for(blockNumber i =0; i < globalFileMap_counter;i++){
            //cout << globalFileMap[i].inodeNumber << endl;

            Inode *temp = &findInode(globalFileMap[i].inodeNumber);
            if(temp->status != 0){
                cout << globalFileMap[i].filename;
                cout << "\t\t";
                cout << temp->filesize << " bytes" << endl;
            }else{
                // cout << "Directory is empty. Needs clean up though" << endl;

            }
        }
    }else{
        cout << "Your directory is empty dude" << endl;
        return;
    }
}

bool removeHelper(Inode *node, int inodeIndex){
    blockNumber segment = inodeIndex/BLOCK_SIZE;
    blockNumber segmentBlock = inodeIndex % BLOCK_SIZE;
    if(segment != current_segment && node != nullptr){
        //
        cout << "Indoe not in current segment, need to change status in another segment" << endl;
        ofstream inodeStatus("DRIVE/SEGMENT" + to_string(segment), ios::binary);
        inodeStatus.seekp(BLOCK_SIZE*segmentBlock);
        node->status = 0;
        inodeStatus.write((char*)&node, BLOCK_SIZE);
        inodeStatus.close();
        return true;
    }else{
        node->status = 0;
        return true;
    }

    return false;
}

void remove(string lfsFilename){
    //1. Find the file in the global file map
    if(globalFileMap_counter !=0){
        for(unsigned i =0; i < globalFileMap_counter; i++){
            if(globalFileMap[i].filename == lfsFilename){
                Inode *temp = &(findInode(globalFileMap[i].inodeNumber));
                bool removed = removeHelper(temp, globalFileMap[i].inodeNumber);
                if(removed){
                    cout << "Removed file: " << lfsFilename << endl;
                    return;
                }else{
                    cout <<"Something went wrong while removing. Needs debuggin" << endl;
                    return;
                }
            }else{
                cout << "Invalid file name" << endl;
            }
        }
    }else{
        cout << "Your directory is empty! Can't delete nothing right?" << endl;
        return;
    }
}

Block* catDisplayHelper(int dataBlockIndex){
    blockNumber dataBlock = dataBlockIndex;
    blockNumber dataBlockNumber = dataBlock % BLOCK_SIZE;
    blockNumber segment = (dataBlock/BLOCK_SIZE);

    if(segment != current_segment){
        ifstream data("DRIVE/SEGMENT" + to_string(segment), ios::binary);
        data.seekg(BLOCK_SIZE*dataBlockNumber);
        Block *retData = new Block("data");
        data.read((char*)&retData->dataBlock, BLOCK_SIZE);
        data.close();
        return retData;
    }
    return writer.getMemorySegmentBlock(dataBlockIndex);
}


void cat(string lfsFilename){
    //Step 1: Find the file name in the file map and get the inode number
    int inodeNumber = -1;
    for(unsigned int i =0; i < globalFileMap.size(); i++){
        if(globalFileMap[i].filename == lfsFilename){
            inodeNumber = globalFileMap[i].inodeNumber;
        }
    }
    if(inodeNumber == -1){
        cout << "File was not found in the File Map! Check to see if you actually imported the file!" << endl;
        return;
    }

    //Step 2. Find the inode
    Inode *temp = &findInode(inodeNumber);
    // cout << temp->filesize << endl;

    //Step 3. Go through the inode data blocks and print out all the stuff contained in it
    int j = 0;
    while(temp->DirectDataBlock[j] != 130){
        int blockNumber =  (temp->DirectDataBlock[j] );
        Block *readBlock = catDisplayHelper(blockNumber);
        Data readData = readBlock->getDataBlock();
        for(unsigned int i =0; i < readData.data.size(); i++){
            cout << readData.data[i];
        }
        j++;
    }
    cout << endl;
}


void display(string lfsFilename, int howmany, int start){
    int inodeNumber = -1;
    for(unsigned int i =0; i < globalFileMap.size(); i++){
        if(globalFileMap[i].filename == lfsFilename){
            inodeNumber = globalFileMap[i].inodeNumber;
        }
    }
    if(inodeNumber == -1){
        cout << "File was not found in the File Map! Check to see if you actually imported the file!" << endl;
        return;
    }

    //Step 2. Find the inode
    Inode *temp = &findInode(inodeNumber);

    //Checking to see if the user asked to display more bytes than there are in the file
    unsigned int startNumber = start;
    unsigned int endNumber = howmany;
    int total = startNumber + endNumber;

    if(startNumber > endNumber){
        cout << "Starting number of bytes can't be more than 'howmany' bytes! Try again"<< endl;
        return;
    }


    if((temp->filesize < BLOCK_SIZE) && (endNumber > BLOCK_SIZE)){
        endNumber = temp->filesize - 1;

    }

    if(total > temp->filesize){
        cout << "You're asking to display way more bytes than the filesize. I will display the content to the end of the file" << endl;
    }

    cout << "Displaying file " << lfsFilename << " starting from byte: " << startNumber << " to: " << endNumber << endl;

    int tracker = (endNumber - startNumber)+1;
    int tracker2 = 0;

    //if its in 1 data block
    if(temp->filesize < BLOCK_SIZE){
        cout << "File content is in one block!" << endl;
        cout << "Displaying....." << endl;
        cout << endl;

        //Get blockNumber
        int blockNumber =  (temp->DirectDataBlock[0] );
        Block *readBlock = catDisplayHelper(blockNumber);
        Data readData = readBlock->getDataBlock();
        for(unsigned int i =startNumber; i <= endNumber; i++){
            cout << readData.data[i];
        }
    }else if(temp->filesize > BLOCK_SIZE){
        int j = 0;

        while(temp->DirectDataBlock[j] != 130 && (startNumber != endNumber)){
            int blockNumber =  (temp->DirectDataBlock[j]);
            Block *readBlock = catDisplayHelper(blockNumber);
            Data readData = readBlock->getDataBlock();
            for(; startNumber<=endNumber ; startNumber++){
                if(startNumber == readData.data.size()-1){
                    int remaining = endNumber - startNumber;
                    startNumber = 0;
                    endNumber = remaining;
                    break;


                }
                cout << readData.data[startNumber];
                tracker2++;
            }
            j++;

        }
    }
    cout << endl;

    cout << "Total bytes to display: " << tracker << endl;
    cout <<"Actually displayed: " << tracker2 << endl;
}

void overwrite(string lfsFilename, int howmany, int start, char c){
	if(start < 0 || start > MAX_FILE_SIZE){
		cerr << "Cant overwrite past the MAX_FILE_SIZE dickhead." << endl;
		return;
	}
	//Find the desired file to be overwritten
	int inodeNum = -1;
	for(unsigned int i = 0; i < globalFileMap.size(); i++){
		if(globalFileMap[i].filename == lfsFilename){
			inodeNum = globalFileMap[i].inodeNumber;
		}
	}
	//Inode *currentInode = &findInode(inodeNum);
	Inode *file = &findInode(inodeNum);
	Block *inodeBlock = new Block("inode");
	std::memcpy(inodeBlock, file, BLOCK_SIZE);
	//Block *currentInodeBlock
	//TODO:MAKE A COPY OF THE INODE NOT JUST A POINTER TO IT (DAN AND JOHN SAID THIS IS THE RIGHT WAY TO DO IT??)
	if(inodeNum == -1){
		cerr << "File not found. Please import the file and try again." << endl;
		return;
	}
	if(start > file->filesize){
		cerr << "Can't start there, dickhead" << endl;
		return;
	}
	unsigned int blockNum = (start/BLOCK_SIZE);
	unsigned int bytesToWrite = min(howmany, (MAX_FILE_SIZE - start));
	int case2_1 = (start + bytesToWrite)/BLOCK_SIZE;
	int case2_2 = (file->filesize)/BLOCK_SIZE;
	//SETTING UP CASES TO OVERWRITE
	if((start%1024)+bytesToWrite <= BLOCK_SIZE){//CASE ONE: When data being overwritten is within one datablock of the file
		Block *overwriteBlock = writer.getMemorySegmentBlock(blockNum);
		Block *newBlock = new Block();
		Data overwriteData = overwriteBlock->getDataBlock();
		char newData[BLOCK_SIZE] = {0};
		int newDataTracker = 0;
		for(unsigned int i = (blockNum*BLOCK_SIZE); i < ((blockNum+1)*BLOCK_SIZE)-1; i++){
			newData[newDataTracker] = overwriteData.data[newDataTracker];
			newDataTracker++;
		}
		for(unsigned int i = (start%BLOCK_SIZE); i < (start%BLOCK_SIZE)+bytesToWrite; i++){
			newData[i] = c;
		}
		newBlock->setData(newData);
		writer.addBlockToSegment(newBlock);
		inodeBlock->addDirectDataBlock(blockNum);
		writer.addBlockToSegment(inodeBlock);
		writer.addInode();
		//unsigned int inodeNumber = writer.numberOfInodes - 1;
		//global_imap.imap[inodeNumber] = blockNum


	} else if(case2_1 > case2_2){//CASE TWO: When Filesize needs to be increased
		//int oldFileSize = file->filesize;
		//file->filesize = start + bytesToWrite;
		//int newFileSize = file->filesize;
		unsigned int bytesToFill_c1, bytesToFill_c2, bytesToFill_c3;
		bytesToFill_c1 = BLOCK_SIZE - (start % BLOCK_SIZE);
		bytesToWrite -= bytesToFill_c1;
		bytesToFill_c2 = (bytesToWrite/BLOCK_SIZE);
		bytesToWrite -= (bytesToFill_c2*BLOCK_SIZE);
		bytesToFill_c3 = bytesToWrite%BLOCK_SIZE;
		while(bytesToWrite != 0){
			Block *overwriteFirstBlock = writer.getMemorySegmentBlock(blockNum);
			Block *firstBlock = new Block("data");
			Data overwriteData = overwriteFirstBlock->getDataBlock();
			char dataTransfer[BLOCK_SIZE] = {0};
			int tracker = 0;
			for(unsigned int i = (blockNum*BLOCK_SIZE); i < ((blockNum+1)*BLOCK_SIZE)-1; i++){
				dataTransfer[tracker] = overwriteData.data[tracker];
				tracker++;
			}
			for(unsigned int i = (start%BLOCK_SIZE); i < BLOCK_SIZE; i++){
				dataTransfer[i] = c;
			}
			firstBlock->setData(dataTransfer);
			for(int i = 0; i < BLOCK_SIZE; i++){
				dataTransfer[i] = 0;
			}
			writer.addBlockToSegment(firstBlock);
			inodeBlock->addDirectDataBlock(blockNum);
			blockNum++;
			for(unsigned int i = 0; i < bytesToFill_c2; i++){
				Block *nextBlock = new Block("data");
				char nextBlockData[BLOCK_SIZE] = {c};
				nextBlock->setData(nextBlockData);
				writer.addBlockToSegment(nextBlock);
				inodeBlock->addDirectDataBlock(blockNum);
				blockNum++;
			}
			Block *lastBlock = new Block("data");
			//int lastBlockStart = (blockNum + bytesToFill_c2 + 1)*BLOCK_SIZE;
			for(unsigned int i = 0; i < bytesToFill_c3; i++){
				dataTransfer[i] = c;
			}
			lastBlock->setData(dataTransfer);
			writer.addBlockToSegment(lastBlock);
			inodeBlock->addDirectDataBlock(blockNum);
			bytesToWrite -= bytesToFill_c3;
		}
		writer.addBlockToSegment(inodeBlock);
		writer.addInode();

	} else{//CASE T: When data being overwritten is within Filesize and over at least TWO datablocks
		//if((start+bytesToWrite) > file->filesize){
		//	file->filesize = start + bytesToWrite;//If total bytes being written is greater than filesize, set new filesize
		//}
		unsigned int bytesToFill_c1, bytesToFill_c2, bytesToFill_c3;
		bytesToFill_c1 = BLOCK_SIZE - (start % BLOCK_SIZE);
		bytesToWrite -= bytesToFill_c1;
		bytesToFill_c2 = bytesToWrite/BLOCK_SIZE;//THIS IS NUMBER OF FULL BLOCKS BEING CREATED
		bytesToWrite -= (bytesToFill_c2*BLOCK_SIZE);
		bytesToFill_c3 = bytesToWrite%BLOCK_SIZE;
		while(bytesToWrite != 0){
			Block *overwriteFirstBlock = writer.getMemorySegmentBlock(blockNum);
			Block *firstBlock = new Block("data");
			Data overwriteData = overwriteFirstBlock->getDataBlock();
			char dataTransfer[BLOCK_SIZE] = {0};
			int tracker = 0;
			for(unsigned int i = (blockNum*BLOCK_SIZE); i < ((blockNum+1)*BLOCK_SIZE)-1; i++){
				dataTransfer[tracker] = overwriteData.data[tracker];
				tracker++;
			}
			for(unsigned int i = (start%BLOCK_SIZE); i < BLOCK_SIZE; i++){
				dataTransfer[i] = c;
			}
			firstBlock->setData(dataTransfer);
			writer.addBlockToSegment(firstBlock);
			inodeBlock->addDirectDataBlock(blockNum);
			blockNum++;
			for(int i = 0; i < BLOCK_SIZE; i++){
				dataTransfer[i] = 0;
			}
			for(unsigned int i = 0; i < bytesToFill_c2; i++){
				Block *nextBlock = new Block("data");
				char nextBlockData[BLOCK_SIZE] = {c};
				nextBlock->setData(nextBlockData);
				writer.addBlockToSegment(nextBlock);
				inodeBlock->addDirectDataBlock(blockNum);
				blockNum++;

			}
			//blockNum += bytesToFill_c2 + 1;
			Block *overwriteLastBlock = writer.getMemorySegmentBlock(blockNum);
			Block *lastBlock = new Block("data");
			overwriteData = overwriteLastBlock->getDataBlock();
			tracker = 0;
			for(unsigned int i = (blockNum*BLOCK_SIZE); i < ((blockNum+1)*BLOCK_SIZE)-1; i++){
				dataTransfer[tracker] = overwriteData.data[tracker];
				tracker++;
			}
			for(unsigned int i = 0; i < bytesToFill_c3; i++){
				dataTransfer[i] = c;
			}
			lastBlock->setData(dataTransfer);
			writer.addBlockToSegment(lastBlock);
			inodeBlock->addDirectDataBlock(blockNum);
			//TODO:Inode Imap shit
			bytesToWrite -= bytesToFill_c3;
		}
		writer.addBlockToSegment(inodeBlock);
		writer.addInode();
	}
}

/***************************************************

HELPER FUNCTIONS (RESTARTING THE PROGRAM)

*****************************************************/
void populateMemorySegment(){

}



void populateImap(){
    cout << "Populating Global Imap now" << endl;
    cout << endl;
    int j =0;
    int imapBlockTracker = 0;
    while(checkpoint_region.imapLocation[j] != 0){
        blockNumber imapBlock = checkpoint_region.imapLocation[j];

        blockNumber segmentBlock = imapBlock % BLOCK_SIZE;

        blockNumber segment = (imapBlock/BLOCK_SIZE);
        cout << "Opening Segment#" << segment << endl;
        ifstream imap("DRIVE/SEGMENT" + to_string(segment), ios::binary);

        imap.seekg(BLOCK_SIZE*segmentBlock);

        Block *imapDataBlock = new Block("imap");


        imap.read((char*)imapDataBlock->imapData, BLOCK_SIZE);



        while(imapDataBlock->imapData[imapBlockTracker] != 0){

            cout << bold_on << "Adding Inode Block #" << imapDataBlock->imapData[imapBlockTracker] << " into global imap" <<bold_off << endl;
            global_imap.imap[global_imap.index] = imapDataBlock->imapData[imapBlockTracker];
            global_imap.index++;
            imapBlockTracker++;

        }
        // delete imapDataBlock;
        j++;
    }

    cout << "Working in Segment#" <<current_segment << endl;
}



void loadCheckpoint(){
    cout << "Loading from checkpoint region. Just a sec" << endl;
    cout << endl;

    ifstream checkpoint;
    checkpoint.open("DRIVE/CHECKPOINT_REGION", ios::binary);
    checkpoint.seekg(0, ios::end);
    if(checkpoint.tellg() ==0 ){
        perror("CHECKPOINT REGION IS EMPTY!");
        exit(EXIT_FAILURE);
    }
    checkpoint.seekg(0);

    /*
        Read in the file into to the global checkpoint struct
    */
    checkpoint.read((char*)&checkpoint_region.imapLocation, sizeof(checkpoint_region.imapLocation));
    checkpoint.read((char*)&checkpoint_region.live_bits, sizeof(checkpoint_region.live_bits));

    // for(int i =0; i < 2; i++){
    //     int num = checkpoint_region.imapLocation[i];
    //     cout << num << endl;
    // }

    //increment segement to a new clean one.
    int j =0;
    while(checkpoint_region.imapLocation[j] != 0){
        cout << "Checkpoint region contains imap block# " << bold_on << checkpoint_region.imapLocation[j] << endl;
        checkpoint_region_counter++;
        j++;
    }

    j = 0;
    int temp = current_segment;
    while(checkpoint_region.live_bits[j] != 0){
        cout << "Checkpoint region indicates that segment#" << bold_on << temp << bold_off << " has live data" << endl;
        // populateImap(current_segment);

        current_segment++;
        temp++;
        j++;
    }
    checkpoint.close();
    populateImap();



    //Reading data
    // ifstream segmentRead("DRIVE/SEGMENT0", ios::binary);
    // Block *block = new Block("data");
    // segmentRead.read((char*)&block->dataBlock.data, BLOCK_SIZE);
    // segmentRead.read((char*)&block->dataBlock.data, BLOCK_SIZE);
    // Block *inode = new Block("inode");
    // segmentRead.read((char*)&inode->inodeDataBlock, BLOCK_SIZE);
    // cout << inode->inodeDataBlock.filesize << endl;
    // int i = 0;
    // while(inode->inodeDataBlock.DirectDataBlock[i] != 130){
    //     cout << inode->inodeDataBlock.DirectDataBlock[i] << endl;
    //     i++;
    // }

}

/*
        READING FROM FILE MAP
*/

void readFromFileMap(){
    cout << "Reading from file map" << endl;
    cout << endl;
    string line;

    //redundant check just to be sure
    globalFileMap_counter = 0;
    string::size_type sz;
    ifstream filemap("DRIVE/FILE_MAP", ios::in);
    if(filemap.good()){
        while(getline(filemap, line)){
            string inodeNumber = line.substr(0, line.find(" "));
            string filename = line.substr(line.find(" ")+1);

            blockNumber iNumber = stoi(inodeNumber,&sz);
            globalFileMap[globalFileMap_counter].inodeNumber = iNumber;
            globalFileMap[globalFileMap_counter].filename = filename;
            cout <<"Adding file name: " << bold_on << filename << bold_off << " with Inode number: " << bold_on << iNumber << bold_off << " to global file map" << endl;
            globalFileMap_counter++;
            writer.numberOfInodes++;
        }
    }

    cout << "DONE READING INTO FILE MAP!" << endl;
    cout << endl;


}

//Inital initalizer of the drive
void createDrive(){

    //Checks if DRIVE exits or not.
    string pathName = "DRIVE";
    ifstream inFile;
    inFile.open(pathName);
    if(inFile.good()){
        cout << "I see a /DRIVE already exists. Loading data from it" << endl;
        readFromFileMap();
        loadCheckpoint();
        return;
    }

    cout << "Initializing disk drive" << endl;
    int errorCheck;
    errorCheck = mkdir(pathName.c_str(), 0777);
    if(errorCheck!= 0){
        perror("Could not make DRIVE");
        exit(EXIT_FAILURE);
    }
    cout <<"Disk Drive successfully created" << endl;

    string fileName = "DRIVE/SEGMENT";
    char* buf = new char[SEGMENT_SIZE];
    memset(buf, 0, SEGMENT_SIZE);

    for(int i = 0; i < 64; i++){
        cout << "Creating Segment File: " << i << endl;
        ofstream file;
        file.open(fileName + to_string(i), ios::binary);
        file.write(buf, SEGMENT_SIZE);
        file.close();
    }

    //Create file name map
    cout << "Creating FILE_MAP" << endl;
    string filePath = "/FILE_MAP";
    ofstream filemap(pathName + filePath);
    filemap.close();

    //Create CHECKPOINT_REGION. 224 Bytes
    cout <<"Creating CHECKPOINT_REGION!" << endl;
    ofstream checkpoint("DRIVE/CHECKPOINT_REGION", ios::binary);
    checkpoint.write(buf, CHECKPOINT_REGION_SIZE);
    checkpoint.close();


    cout << "DISK DRIVE COMPLETED! YAY!" << endl;

    //Initalize the globalimpa index to 0 since its empty.
    global_imap.index = 0;

}
