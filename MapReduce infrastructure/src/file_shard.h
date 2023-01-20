#pragma once

#include "mapreduce_spec.h"
using namespace std;

/* File shard information that the master use for bookkeeping and to convey the tasks to the mappers */
struct File {
     string filename;
     int start;
     int line_count = 0;
};

struct FileShard {
     vector<File> files;
};

/* Create fileshards from the list of input files */ 
inline bool shard_files(const MapReduceSpec& mr_spec, vector<FileShard>& fileShards) {
     int shard_size = mr_spec.map_kilobytes * 1024;
     int remain = shard_size;
     int start = 0;
     int line_count = 0;
     bool new_shard = true;
     string line;
     FileShard *shard;

     for (string filename : mr_spec.input_files) {
          ifstream fd(filename);
          while (!fd.is_open()) {
               cerr << "shard fail to open input file:" << filename << endl;
               fd.open(filename);
          }

          while (-1 != fd.tellg()) {
               if (remain == shard_size) {
                    new_shard = true;
               }
               else {
                    new_shard = false;
               }

               File file;
               file.filename = filename;
               file.start = start;

               fd.seekg(start);
               line_count = 0;
               while (getline(fd, line)) {
                    line_count++;
                    start = fd.tellg();
                    if (-1 == start) {
                         start = 0;
                    }
                    remain -= line.size();
                    if (remain <= 0) {
                         remain = shard_size;
                         break;
                    }
               }
               file.line_count = line_count;

               if (new_shard) {
                    FileShard shard;
                    shard.files.push_back(file);
                    fileShards.push_back(shard);
               }
               else {
                    fileShards.back().files.push_back(file);
               }
          }

          fd.close();
     }

     cout << "shard number:" << fileShards.size() << " " << shard_size << endl;

	return true;
}
