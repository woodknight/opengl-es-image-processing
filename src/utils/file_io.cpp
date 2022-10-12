#include <fstream>
#include <iostream>
#include <string>

#include "file_io.h"

using namespace std;

void read_file(string path, void *data, size_t size)
{
    std::ifstream file(path, ios::binary | ios::in);
    if (!file.good()) {
        cout << "File open failed: " << path << endl;
        return;
    }
    file.read((char *)data, size);
    file.close();
}

void write_file(string path, void *data, size_t size)
{
    ofstream file(path, ios::binary | ios::out);
    if (!file) {
        cout << "Cannot open file to write (file_io)!" << endl;
        return;
    }
    file.write((char *)data, size);
    file.close();
}