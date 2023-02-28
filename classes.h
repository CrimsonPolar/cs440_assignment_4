#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
using namespace std;

class Record {
public:
    // ids are always 8 digits
    unsigned int id, manager_id;
    std::string bio, name;

    Record(vector<std::string> fields) {
        id = stoul(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoul(fields[3]);
    }

    void print() {
        cout << "\n" << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n" << "\n";
    }

};


class LinearHashIndex {

private:
    const int BLOCK_SIZE = 4096;
    const int RECORD_SIZE = 716;
    const float EXPAND_THRESH = .7;

    vector<int> blockDirectory; // Map the least-significant-bits of h(id) to a bucket location in EmployeeIndex (e.g., the jth bucket)
                                // can scan to correct bucket using j*BLOCK_SIZE as offset (using seek function)
								// can initialize to a size of 256 (assume that we will never have more than 256 regular (i.e., non-overflow) buckets)
    int n;  // The number of indexes in blockDirectory currently being used
    int i;	// The number of least-significant-bits of h(id) to check. Will need to increase i once n > 2^i
    int numRecords;    // Records currently in index. Used to test whether to increase n
    int nextFreeBlock; // Next place to write a bucket. Should increment it by BLOCK_SIZE whenever a bucket is written to EmployeeIndex
    string fName;      // Name of index file


    // BLOCK STRUCTURE
    // 1st byte is num records
    // 2nd byte is offset to overflow block
    //  - e.g. offset of 1 flags next block as overflow
    //  - offset of 0 indicates no overflow
    // Records start at 3rd byte
    //  - 5 records per block
    //  - 716 bytes each

    inline int hash(const int id) {
        return id % 256;
    }

    int getBlock(const int id) {
        int hashed = hash(id);
        int LSBs = hashed & ((1 << i) - 1);

        if (LSBs >= n)
        {
            LSBs -= (1 << (i - 1));
        }

        return blockDirectory.at(LSBs);
    }

    void resetBlockMem(char * block){
        fill_n(block, BLOCK_SIZE, 0);
    }

    // Insert new record into index
    void insertRecord(Record record) {

        // Add record to the index in the correct block, creating a overflow block if necessary
        int block = getBlock(record.id);
        char blockData[BLOCK_SIZE];
        FILE * index = fopen(fName.c_str(), "r+b");

        fseek(index, BLOCK_SIZE * block, SEEK_SET);
        resetBlockMem(blockData);
        fread(blockData, sizeof(char), BLOCK_SIZE, index);

        // If the block is full, add overflow block
        if(blockData[0] == 5){
            // Create overflow block and update blockData
            blockData[1] = (nextFreeBlock / BLOCK_SIZE) - block;
            fseek(index, BLOCK_SIZE * block + 1, SEEK_SET);
            fputc(blockData[1], index);
            nextFreeBlock += BLOCK_SIZE;

            int newBlock = block + blockData[1];

            writeRecord(record, newBlock, 0, index);
            
        } else {
            writeRecord(record, block, blockData[0], index);
        }
        numRecords++;


        // Check capacity
        float total = 0;
        for(int j = 0; j < n; j++){
            fseek(index, BLOCK_SIZE * blockDirectory.at(j), SEEK_SET);
            total += fgetc(index);
            
            /* Don't want to check overflows

            // Check for overflow block
            int overflow = fgetc(index);
            while (overflow) {
                // Seek to next overflow block's num records
                fseek(index, BLOCK_SIZE * overflow - 2, SEEK_CUR);
                total += fgetc(index);
                
                overflow = fgetc(index);
            }
            */
        }
        float average = total / (n * 5);
        // Take neccessary steps if capacity is reached:
		// increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip
        if(average >= EXPAND_THRESH){
            expandIndex(index, blockData);
        }

        fclose(index);
    }

    void printBlock(char * r){
        cout << endl << r[0] << " " << r[1] << endl;
        for(int k = 0; k < r[0]; k++){
            cout << endl;
            Record(
                std::vector<std::string>{
                    to_string(getIntFromBlockData(r, 2 + k * RECORD_SIZE)),
                    std::string(r + 2 + 16 + k * RECORD_SIZE, 200),
                    std::string(r + 2 + 216 + k * RECORD_SIZE, 500),
                    to_string(getIntFromBlockData(r, 2 + 8 + k *RECORD_SIZE))
                }
            ).print();
        }
    }


    void expandIndex(FILE * index, char * blockData){
        blockDirectory.push_back(nextFreeBlock/BLOCK_SIZE);
        // Write blank block to end of index
        fseek(index, BLOCK_SIZE * nextFreeBlock + BLOCK_SIZE - 1, SEEK_SET);
        fputc(0, index);
        // Update vars
        n++;
        nextFreeBlock += BLOCK_SIZE;
        // if n > 2^i
        if(n > (1 << i)){
            i++;
        }
        // for every block
        for(int j = 0; j < n; j++){
            int overflow = 0;
            // loop for overflow checking, normally only do this once but supports sequential overflow blocks
            do {
                fseek(index, BLOCK_SIZE * (blockDirectory.at(j) + overflow), SEEK_SET);
                resetBlockMem(blockData);
                fread(blockData, sizeof(char), BLOCK_SIZE, index);
                // printBlock(blockData);
                // for every record
                for(int k = 0; k < blockData[0]; k++){
                    int destBlock = getBlock(getIntFromBlockData(blockData, 2 + k * RECORD_SIZE));
                    // if the block the current record should be hashed to is different to the one it is currently in
                    if(blockDirectory.at(j) != destBlock){
                        // Replace record

                        char destBlockData[4096]; // 2nd block in memory
                        fseek(index, BLOCK_SIZE * destBlock, SEEK_SET);
                        resetBlockMem(destBlockData);
                        fread(destBlockData, sizeof(char), BLOCK_SIZE, index);

                        Record temp = Record(
                            std::vector<std::string>{
                                to_string(getIntFromBlockData(blockData, 2 + k * RECORD_SIZE)),
                                std::string(blockData + 2 + 16 + k * RECORD_SIZE, 200),
                                std::string(blockData + 2 + 216 + k * RECORD_SIZE, 500),
                                to_string(getIntFromBlockData(blockData, 2 + 8 + k *RECORD_SIZE))
                            }
                        );
                        // if an overflow block exists
                        while(destBlockData[0] == 5 && destBlockData[1] != 0){
                            // read in overflow instead
                            destBlock += destBlockData[1];
                            fseek(index, BLOCK_SIZE * destBlock, SEEK_SET);
                            resetBlockMem(destBlockData);
                            fread(destBlockData, sizeof(char), BLOCK_SIZE, index);
                        }
                        // if the block is full with no overflow
                        if(destBlockData[0] == 5){
                             // Create overflow block and update blockData
                            destBlockData[1] = (nextFreeBlock / BLOCK_SIZE) - destBlock;
                            fseek(index, BLOCK_SIZE * destBlock + 1, SEEK_SET);
                            fputc(destBlockData[1], index);
                            nextFreeBlock += BLOCK_SIZE;

                            int newBlock = destBlock + destBlockData[1];

                            writeRecord(temp, newBlock, 0, index);
                        } else {
                            writeRecord(temp, destBlock, destBlockData[0], index);
                        }
                        fseek(index, BLOCK_SIZE * destBlock, SEEK_SET);
                        resetBlockMem(destBlockData);
                        fread(destBlockData, sizeof(char), BLOCK_SIZE, index);


                        

                        
                        // if record wasn't last
                        if(k < blockData[0] - 1 && k < 5){
                            // move others down to preserve sanity
                            for(int l = k + 1; l < blockData[0] && l <= 5; l++){
                                temp = Record(
                                    std::vector<std::string>{
                                        to_string(getIntFromBlockData(blockData, 2 + l * RECORD_SIZE)),
                                        std::string(blockData + 2 + 16 + l * RECORD_SIZE, 200),
                                        std::string(blockData + 2 + 216 + l * RECORD_SIZE, 500),
                                        to_string(getIntFromBlockData(blockData, 2 + 8 + l * RECORD_SIZE))
                                    }
                                );
                                writeRecord(temp, blockDirectory.at(j) + overflow, l - 1, index);
                                
                            }
                            // ensure loop doesn't break
                            k--;

                            // delete records at end just in case
                            // usually can't be accessed, but erase data anyway
                            fseek(index, BLOCK_SIZE * (blockDirectory.at(j) + overflow) + 2 + RECORD_SIZE * blockData[0], SEEK_SET);
                            for(int l = 0; l < RECORD_SIZE; l++){
                                fputc(0, index);
                            }
                            
                        }
                        
                        
                        
                        // update count in index
                        fseek(index, BLOCK_SIZE * (blockDirectory.at(j) + overflow), SEEK_SET);
                        fputc(blockData[0] - 1, index);
                        blockData[0]--;

                        fseek(index, BLOCK_SIZE * (blockDirectory.at(j) + overflow), SEEK_SET);
                        resetBlockMem(blockData);
                        fread(blockData, sizeof(char), BLOCK_SIZE, index);

                    }
                }
                overflow += blockData[1];
            } while(blockData[0] == 5 && blockData[1] != 0);
        }
    }

    // write record into index file
    void writeRecord(Record record, int blockNum, int recordNum, FILE * index){
        if(recordNum > 4){
            printf("awesome");
            throw invalid_argument("This didn't work");
        }

        fseek(index, BLOCK_SIZE * blockNum, SEEK_SET);
        fputc(1 + recordNum, index);
        // "1 +" to adjust for overflow offset var at second byte of block
        fseek(index, 1 + recordNum * RECORD_SIZE, SEEK_CUR);
        writeIntToIndex(index, record.id);
        writeIntToIndex(index, record.manager_id);
        // Write name and adjust file pointer based on length 
        fputs(record.name.c_str(), index);
        fseek(index, BLOCK_SIZE * blockNum + 2 + RECORD_SIZE * recordNum + 216, SEEK_SET);
        fputs(record.bio.c_str(), index);
    }

    //writes int to index at current position as 8 byte
    void writeIntToIndex(FILE * index, unsigned int w){
        for(int i = 0; i < 4; i++){
            fputc(w & 0xFF, index);
            w = w >> 8;
        }
        fseek(index, 4, SEEK_CUR);
    }

    unsigned int getIntFromBlockData(char * block, int ind){
        unsigned int res = 0;
        for(int i = 0; i < 4; i++){
            res += ((unsigned char)block[ind + i]) << (8 * i);
        }
        return res;
    }

    unsigned int getIntFromIndex(FILE * index){
        unsigned int res = 0;
        for(int i = 0; i < 4; i++){
            res += ((unsigned char)fgetc(index)) << (8 * i);
        }
        fseek(index, -4, SEEK_CUR);
        return res;
    }

public:
    LinearHashIndex(string indexFileName) {
        n = 4; // Start with 4 buckets in index
        i = 2; // Need 2 bits to address 4 buckets
        numRecords = 0;
        nextFreeBlock = 0;
        fName = indexFileName;

        // Create your EmployeeIndex file and write out the initial 4 buckets
        // make sure to account for the created buckets by incrementing nextFreeBlock appropriately
        FILE * iFile = fopen(fName.c_str(), "w+b");
        fseek(iFile, 4096 * n - 1, SEEK_SET);
        fputc(0, iFile);
        fclose(iFile);

        nextFreeBlock = 4096*n;

        for(int j = 0; j < n; j++){
            blockDirectory.push_back(j);
        }
    }

    // Read csv file and add records to the index
    void createFromFile(string csvFName) {
        // Open the csv file
        ifstream csvFile(csvFName);
        if (!csvFile.is_open()) {
            throw invalid_argument("Could not open file");
        }

        // Read the csv file line by line
        string line;
        while (getline(csvFile, line)) {
            // Parse the line
            stringstream ss(line);
            string field;
            vector<std::string> fields;
            while (getline(ss, field, ',')) {
                fields.push_back(field);
            }

            // Create a record from the line
            Record record(fields);

            // Add the record to the index
            insertRecord(record);
        }
        
    }
    // TODO: Still deleting some records
        // ie. 11432159 can be found, but 11432121 cannot
        // all records should be in the index
    // Given an ID, find the relevant record and print it
    Record findRecordById(int id) {
        
        // Block to search
        int block = getBlock(id);

        // Load data from block
        char blockData[BLOCK_SIZE];
        FILE * index = fopen(fName.c_str(), "rb");
        fseek(index, BLOCK_SIZE * block, SEEK_SET);
        fread(blockData, sizeof(char), BLOCK_SIZE, index);
        fclose(index);
        printBlock(blockData);

        // Linear search through block for record with matching id
        for(int j = 0; j < blockData[0]; j++) {
            if (getIntFromBlockData(blockData, 2 + j * RECORD_SIZE) == id) {
                Record record = Record(
                    std::vector<std::string>{
                        to_string(getIntFromBlockData(blockData, 2 + j * RECORD_SIZE)),
                        std::string(blockData + 2 + 16 + j * RECORD_SIZE, 200),
                        std::string(blockData + 2 + 216 + j * RECORD_SIZE, 500),
                        to_string(getIntFromBlockData(blockData, 2 + 8 + j *RECORD_SIZE))
                    }
                );
                
                // record.print();
                return record;
            }

            // If this is the last record in the block, check for overflow block
            if(j == blockData[0] - 1 && blockData[1] != 0) {
                // Update blockData to overflow block and reset i
                fseek(index, BLOCK_SIZE * blockData[1], SEEK_CUR);
                fread(blockData, sizeof(char), BLOCK_SIZE, index);
                j = -1; // i will be incremented to 0 at the end of the loop
            }
        }

        // If no record found, throw error
        throw invalid_argument("Record not found");
        // cout << "Record not found" << endl;
    }
};
