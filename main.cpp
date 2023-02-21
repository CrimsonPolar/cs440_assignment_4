/*
Skeleton code for linear hash indexing
*/

#include <string>
#include <ios>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include "classes.h"
using namespace std;


int main(int argc, char* const argv[]) {

    // Create the index
    LinearHashIndex emp_index("EmployeeIndex");
    emp_index.createFromFile("Employee.csv");
    
    // Loop to lookup IDs until user is ready to quit
    bool done = false;

    while (!done) {
        int id;
        cout << "Enter an ID to lookup (-1 to quit): ";
        cin >> id;
        if (id == -1) {
            done = true;
        } else {
            Record record = emp_index.findRecordById(id);
            record.print();
        }
    }
    

    return 0;
}
