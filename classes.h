#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
using namespace std;

class Record {
public:
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
    // Records start at 3rd byte
    //  - 5 records per block
    //  - 716 bytes each


    // Insert new record into index
    void insertRecord(Record record) {

        // Hash function
        int hashed = record.id % 256;
        // Get i least significant bits
        int LSBs = hashed & ((1 << i) - 1);
        // Adjusting MSB if i least significant bits >= n
        if(LSBs >= n){
            LSBs -= (1 << (i - 1));
        }

        // No records written to index yet
        if (numRecords == 0) {
            // Initialize index with first blocks (start with 4)

        }

        // Add record to the index in the correct block, creating a overflow block if necessary
        char block1[4096];

        // Take neccessary steps if capacity is reached:
		// increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip


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
        
    }
};
