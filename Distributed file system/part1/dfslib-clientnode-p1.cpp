#include <regex>
#include <vector>
#include <string>
#include <thread>
#include <cstdio>
#include <chrono>
#include <errno.h>
#include <csignal>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <getopt.h>
#include <unistd.h>
#include <limits.h>
#include <sys/inotify.h>
#include <grpcpp/grpcpp.h>

#include "dfslib-shared-p1.h"
#include "dfslib-clientnode-p1.h"
#include "proto-src/dfs-service.grpc.pb.h"

using namespace std;
using grpc::Status;
using grpc::Channel;
using grpc::StatusCode;
using grpc::ClientWriter;
using grpc::ClientReader;
using grpc::ClientContext;

using dfs_service::Request;
using dfs_service::FileAck;
using dfs_service::FileContent;
using dfs_service::FileList;
using dfs_service::FileStat;

//
// STUDENT INSTRUCTION:
//
// You may want to add aliases to your namespaced service methods here.
// All of the methods will be under the `dfs_service` namespace.
//
// For example, if you have a method named MyMethod, add
// the following:
//
//      using dfs_service::MyMethod
//


DFSClientNodeP1::DFSClientNodeP1() : DFSClientNode() {}

DFSClientNodeP1::~DFSClientNodeP1() noexcept {}

StatusCode DFSClientNodeP1::Store(const string &filename) {
    FileContent file;
    FileAck ack;
    ClientContext context;
    Status status;
    struct stat s;
    ifstream ifile;
    char buffer[BUFFERSIZE];
    long byte_total = 0;
    long read_byte = 0;

    // Check file existence
    if (0 > stat(WrapPath(filename).c_str(), &s)) {
        cout << "File not found:" << filename << endl;
        return StatusCode::NOT_FOUND;
    }

    // Open file
    ifile.open(WrapPath(filename));
    if (!ifile) {
        cout << "Open file failed:" << filename << endl;
        return StatusCode::CANCELLED;
    }
    
    // Set deadline
    chrono::system_clock::time_point deadline = chrono::system_clock::now() 
                                              + chrono::milliseconds(deadline_timeout);
    context.set_deadline(deadline);

    // Send file
    unique_ptr<ClientWriter<FileContent>> writer(service_stub->Store(&context, &ack)); 
    while (byte_total < s.st_size) {
        read_byte = MIN(BUFFERSIZE, s.st_size - byte_total);
        ifile.read(buffer, read_byte);

        file.set_name(filename);
        file.set_foo(buffer, read_byte);
        writer->Write(file);

        byte_total += read_byte;
    }
    ifile.close();
    cout << "Store send:" << s.st_size << endl;

    // Check error code
    writer->WritesDone();  
    status = writer->Finish();
    if (status.ok()) {
        cout << "Store file succeeded:" << filename << endl;
    }
    else {
        cout << "Store file failed:" << filename << " " << status.error_code() 
             << ":" << status.error_message() << endl;
    }
    
    return status.error_code();
}


StatusCode DFSClientNodeP1::Fetch(const string &filename) {
    Request req;
    FileContent file;
    ClientContext context;
    Status status;
    ofstream ofile;

    // Set deadline
    chrono::system_clock::time_point deadline = chrono::system_clock::now() 
                                              + chrono::milliseconds(deadline_timeout);
    context.set_deadline(deadline);

    // Open file
    ofile.open(WrapPath(filename));
    if (!ofile) {
        cout << "Fail to create file:" << filename << endl;
        return StatusCode::CANCELLED;
    }

    // Receive and store file
    req.set_message(filename);
    unique_ptr<ClientReader<FileContent>> reader(service_stub->Fetch(&context, req));
    int total = 0;
    while (reader->Read(&file)) {
        ofile.write(file.foo().c_str(), file.foo().size());
        total += file.foo().size();
    }
    ofile.close();
    cout << "Fetch receive:" << total << endl;
    
    // Check error code
    status = reader->Finish();
    if (status.ok()) {
        cout << "Fetch file succeeded:" << filename << endl;
    }
    else {
        cout << "Fetch file failed:" << filename << " " << status.error_code() 
             << ":" << status.error_message() << endl;
    }
    
    return status.error_code();

    // TODO: try catch?
}

StatusCode DFSClientNodeP1::Delete(const string& filename) {
    Request req;
    FileAck ack;
    ClientContext context;
    Status status;

    // Set deadline
    chrono::system_clock::time_point deadline = chrono::system_clock::now() 
                                              + chrono::milliseconds(deadline_timeout);
    context.set_deadline(deadline);

    // Send request
    req.set_message(filename);
    status = service_stub->Delete(&context, req, &ack);
    if (status.ok()) {
        cout << "Delete file succeeded:" << filename << endl;
    }
    else {
        cout << "Delete file failed:" << filename << " " << status.error_code() 
             << ":" << status.error_message() << endl;
    }

    return status.error_code();
}

StatusCode DFSClientNodeP1::List(map<string,int>* file_map, bool display) {
    Request req;
    FileList list;
    ClientContext context;
    Status status;

    // Set deadline
    chrono::system_clock::time_point deadline = chrono::system_clock::now() 
                                              + chrono::milliseconds(deadline_timeout);
    context.set_deadline(deadline);

    // Send request and read response
    unique_ptr<ClientReader<FileList>> reader(service_stub->List(&context, req));
    while (reader->Read(&list)) {
        if (true == display) {
            cout << "File:" << list.name() << " " << list.mtime() << endl;
        }

        file_map->insert({list.name(), list.mtime()});
    };

    // Check error code
    status = reader->Finish();
    if (status.ok()) {
        cout << "List file succeeded" << endl;
    }
    else {
        cout << "List file failed " << status.error_code() << ":" << status.error_message() << endl;
    }
    
    return status.error_code();
}

StatusCode DFSClientNodeP1::Stat(const string &filename, void* file_status) {
    Request req;
    FileStat stat;
    ClientContext context;
    Status status;

    // Set deadline
    chrono::system_clock::time_point deadline = chrono::system_clock::now() 
                                              + chrono::milliseconds(deadline_timeout);
    context.set_deadline(deadline);

    // Send request and check error code
    req.set_message(filename);
    status = service_stub->Stat(&context, req, &stat);
    if (status.ok()) {
        cout << "Get file status succeeded:" << filename << endl;

        if (NULL == file_status) {
            cout << stat.size() << " " << stat.mtime() << " " << stat.creationtime() << endl;
        }
    }
    else {
        cout << "Get file status failed:" << filename << " " 
             << status.error_code() << ":" << status.error_message() << endl;
    }

    return status.error_code();
}

//
// STUDENT INSTRUCTION:
//
// Add your additional code here, including
// implementations of your client methods
//

