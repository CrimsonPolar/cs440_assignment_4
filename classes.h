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
    int id, manager_id;
    std::string bio, name;

    Record(vector<std::string> fields) {
        id = stoi(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoi(fields[3]);
    }

    void print() {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
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

    // Insert new record into index
    void insertRecord(Record record) {

        // Add record to the index in the correct block, creating a overflow block if necessary
        int block = getBlock(record.id);
        char blockData[BLOCK_SIZE];
        FILE * index = fopen(fName.c_str(), "r+b");

        fseek(index, BLOCK_SIZE * block, SEEK_SET);
        fread(blockData, sizeof(char), BLOCK_SIZE, index);

        // If the block is full, add overflow block
        if(blockData[0] == 5){
            // Create overflow block and update blockData
            blockData[1] = block - (nextFreeBlock / BLOCK_SIZE);
            nextFreeBlock += BLOCK_SIZE;

            int newBlock = getBlock(block + blockData[1]);

            writeRecord(record, newBlock, blockData[0], index);
            
        } else {
            writeRecord(record, block, blockData[0], index);
        }
        numRecords++;


        // Check capacity
        float total = 0;
        for(int i = 0; i < n; i++){
            fseek(index, BLOCK_SIZE * blockDirectory.at(i), SEEK_SET);
            total += fgetc(index);
            
            // Check for overflow block
            fseek(index, 1, SEEK_CUR); // Next byte is overflow offset
            int overflow = fgetc(index);
            while (overflow) {
                // Seek to next overflow block's num records
                fseek(index, BLOCK_SIZE * overflow - 1, SEEK_CUR);
                total += fgetc(index);
                
                fseek(index, 1, SEEK_CUR); // Next byte is bucket's overflow offset
                overflow = fgetc(index);
            }
        }
        float average = total / (n * 5);
        // Take neccessary steps if capacity is reached:
		// increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip
        if(average >= EXPAND_THRESH){
            blockDirectory.push_back(nextFreeBlock/BLOCK_SIZE);
            // Write blank block to end of index
            fseek(index, BLOCK_SIZE * nextFreeBlock + BLOCK_SIZE - 1, SEEK_SET);
            fputc(0, index);

            //TODO: check entries for if they're misplaced as per slides 61, 62 of indexing.pdf


            // Update vars
            n++;
            nextFreeBlock += BLOCK_SIZE;

            // if n > 2^i
            //  add new bit
            if(n > (1 << i)){
                // TODO: implement
            }
            
        }

        fclose(index);
    }

    // write record into index file
    void writeRecord(Record record, int blockNum, int recordNum, FILE * index){

        fseek(index, BLOCK_SIZE * blockNum, SEEK_SET);
        fputc(recordNum + 1, index);
        // "1 +" to adjust for overflow offset var at second byte of block
        fseek(index, 1 + recordNum * RECORD_SIZE, SEEK_CUR);
        fputc(record.id, index);
        fputc(record.manager_id, index);
        // Write name and adjust file pointer based on length 
        fputs(record.name.c_str(), index);
        fseek(index, 200 - record.name.length(), SEEK_CUR);
        fputs(record.bio.c_str(), index);
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

        // Linear search through block for record with matching id
        for(int i = 0; i < blockData[0]; i++) {
            if (blockData[3 + i * RECORD_SIZE] == id) {
                Record record = Record(
                    std::vector<std::string> {
                        std::to_string(id),
                        std::to_string(blockData[4 + i * RECORD_SIZE]),
                        std::string(blockData + 5 + i * RECORD_SIZE, 200),
                        std::string(blockData + 205 + i * RECORD_SIZE, 500)
                    }
                );
                
                record.print();
                return record;
            }

            // If this is the last record in the block, check for overflow block
            if(i == blockData[0] - 1 && blockData[1] != 0) {
                // Update blockData to overflow block and reset i
                fseek(index, BLOCK_SIZE * blockData[1], SEEK_CUR);
                fread(blockData, sizeof(char), BLOCK_SIZE, index);
                i = -1; // i will be incremented to 0 at the end of the loop
            }
        }

        // If no record found, throw error
        throw invalid_argument("Record not found");
    }
};
