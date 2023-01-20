#include <regex>
#include <mutex>
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
#include <utime.h>

#include "src/dfs-utils.h"
#include "src/dfslibx-clientnode-p2.h"
#include "dfslib-shared-p2.h"
#include "dfslib-clientnode-p2.h"
#include "proto-src/dfs-service.grpc.pb.h"

using namespace std;
using grpc::Status;
using grpc::Channel;
using grpc::StatusCode;
using grpc::ClientWriter;
using grpc::ClientReader;
using grpc::ClientContext;


extern dfs_log_level_e DFS_LOG_LEVEL;

//
// STUDENT INSTRUCTION:
//
// Change these "using" aliases to the specific
// message types you are using to indicate
// a file request and a listing of files from the server.
//

using dfs_service::Request;
using dfs_service::WriteAccess;
using dfs_service::FileContent;
using dfs_service::Ack;
using dfs_service::FileList;
using dfs_service::FileStat;
using FileRequestType = Request;
using FileListResponseType = dfs_service::CallbackFileList;

DFSClientNodeP2::DFSClientNodeP2() : DFSClientNode() {}
DFSClientNodeP2::~DFSClientNodeP2() {}

mutex rpc_mutex;

// The StatusCode response should be:
// OK - if all went well
// StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
// StatusCode::RESOURCE_EXHAUSTED - if a write lock cannot be obtained
// StatusCode::CANCELLED otherwise
grpc::StatusCode DFSClientNodeP2::RequestWriteAccess(const string &filename) {
    WriteAccess req;
    Ack ack;
    ClientContext context;
    Status status;

    // Set deadline
    chrono::system_clock::time_point deadline = chrono::system_clock::now() 
                                              + chrono::milliseconds(deadline_timeout);
    context.set_deadline(deadline);

    // Send request
    req.set_filename(filename);
    req.set_clientid(client_id);
    status = service_stub->RequestWriteAccess(&context, req, &ack);

    if (status.ok()) {
        cout << "Get write access:" << filename << "/" << client_id << endl;
    }
    else {
        cout << "Get write access failed:" << filename << "/" << client_id << " " << status.error_message() << endl;
    }

    return status.error_code(); 
}

// The StatusCode response should be:
// StatusCode::OK - if all went well
// StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
// StatusCode::ALREADY_EXISTS - if the local cached file has not changed from the server version
// StatusCode::RESOURCE_EXHAUSTED - if a write lock cannot be obtained
// StatusCode::CANCELLED otherwise
grpc::StatusCode DFSClientNodeP2::Store(const string &filename) {
    FileContent file;
    Ack ack;
    ClientContext context;
    Status status;
    StatusCode status_code;
    FileStatus file_status;
    struct stat s;
    uint crc;
    int mtime;
    ifstream ifile;
    char buffer[BUFFERSIZE];
    long byte_total = 0;
    long read_byte = 0;

    // Check file existence
    if (0 > stat(WrapPath(filename).c_str(), &s)) {
        cout << "Store/File not found:" << filename << endl;
        return StatusCode::CANCELLED;
    }

    crc = dfs_file_checksum(WrapPath(filename), &crc_table);
    mtime = s.st_mtim.tv_sec;

    // Check existence on server
    status_code = DFSClientNodeP2::Stat(filename, &file_status);
    if ((StatusCode::CANCELLED == status_code) || (StatusCode::DEADLINE_EXCEEDED == status_code)) {
        cout << "Store/Get file status failed:" << filename << endl;
        return StatusCode::CANCELLED;
    }
    else if (StatusCode::OK == status_code) {
        if ((file_status.crc == crc) || (file_status.mtime >= mtime)) {
            cout << "Store/File exist:" << filename << endl;
            return StatusCode::ALREADY_EXISTS;
        }
    }

    // Open file
    ifile.open(WrapPath(filename));
    if (!ifile) {
        cout << "Store/Open file failed:" << filename << endl;
        return StatusCode::CANCELLED;
    }


    // Request write lock
    status_code = DFSClientNodeP2::RequestWriteAccess(filename);
    if (StatusCode::OK != status_code) {
        cout << "Store/Lock failed:" << filename << endl;
        ifile.close();
        return StatusCode::RESOURCE_EXHAUSTED;
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
        file.set_clientid(client_id);
        // file.set_crc(crc);
        // file.set_mtime(mtime);
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
        cout << "Store file failed:" << filename << " " << status.error_message() << endl;
    }
    
    return status.error_code();
}

// The StatusCode response should be:
// OK - if all went well
// DEADLINE_EXCEEDED - if the deadline timeout occurs
// NOT_FOUND - if the file cannot be found on the server
// ALREADY_EXISTS - if the local cached file has not changed from the server version
// CANCELLED otherwise
grpc::StatusCode DFSClientNodeP2::Fetch(const string &filename) {
    Request req;
    FileContent file;
    ClientContext context;
    Status status;
    StatusCode status_code;
    FileStatus file_status;
    struct stat s;
    uint crc;
    int mtime;
    ofstream ofile;

    // Check existence on server
    status_code = DFSClientNodeP2::Stat(filename, &file_status);
    if (StatusCode::OK != status_code) {
        cout << "Fetch/Fail to get file status:" << filename << endl;
        return StatusCode::CANCELLED;
    }

    // Check file existence
    if (0 == stat(WrapPath(filename).c_str(), &s)) {
        crc = dfs_file_checksum(WrapPath(filename), &crc_table);
        mtime = s.st_mtim.tv_sec;

        if ((crc == file_status.crc) || (mtime >= file_status.mtime)) {
            cout << "Fetch/File exist:" << filename << endl;
            return StatusCode::ALREADY_EXISTS;
        }
    }

    // Set deadline
    chrono::system_clock::time_point deadline = chrono::system_clock::now() 
                                              + chrono::milliseconds(deadline_timeout);
    context.set_deadline(deadline);

    // Open file
    ofile.open(WrapPath(filename));
    if (!ofile) {
        cout << "Fetch: Fail to open file:" << filename << endl;
        return StatusCode::CANCELLED;
    }

    // Receive and store file
    req.set_name(filename);
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
        cout << "Fetch file failed:" << filename << " " << status.error_message() << endl;
    }
    
    return status.error_code();
}

// The StatusCode response should be:
// StatusCode::OK - if all went well
// StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
// StatusCode::RESOURCE_EXHAUSTED - if a write lock cannot be obtained
// StatusCode::CANCELLED otherwise
grpc::StatusCode DFSClientNodeP2::Delete(const string &filename) {
    WriteAccess req;
    Ack ack;
    ClientContext context;
    Status status;
    StatusCode status_code;
    FileStatus file_status;

    // Check file status on server
    status_code = DFSClientNodeP2::Stat(filename, &file_status);
    if (StatusCode::OK != status_code) {
        cout << "Delete/Fail to get file status:" << filename << endl;
        return StatusCode::CANCELLED;
    }

    // Request write lock
    status_code = DFSClientNodeP2::RequestWriteAccess(filename);
    if (StatusCode::OK != status_code) {
        cout << "Delete/Lock failed::" << filename << endl;
        return StatusCode::RESOURCE_EXHAUSTED;
    }

    // Set deadline
    chrono::system_clock::time_point deadline = chrono::system_clock::now() 
                                            + chrono::milliseconds(deadline_timeout);
    context.set_deadline(deadline);

    // Send request
    req.set_filename(filename);
    req.set_clientid(client_id);
    status = service_stub->Delete(&context, req, &ack);
    if (status.ok()) {
        cout << "Delete file succeeded:" << filename << endl;
    }
    else {
        cout << "Delete file failed:" << filename << " " << status.error_message() << endl;
    }

    return status.error_code();
}

// The StatusCode response should be:
// StatusCode::OK - if all went well
// StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
// StatusCode::CANCELLED otherwise
grpc::StatusCode DFSClientNodeP2::List(map<string,int>* file_map, bool display) {
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
        cout << "List file failed " << status.error_message() << endl;
    }
    
    return status.error_code();
}

// The StatusCode response should be:
// StatusCode::OK - if all went well
// StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
// StatusCode::NOT_FOUND - if the file cannot be found on the server
// StatusCode::CANCELLED otherwise
grpc::StatusCode DFSClientNodeP2::Stat(const string &filename, void* file_status) {
    Request req;
    FileStat stat;
    ClientContext context;
    Status status;

    // Set deadline
    chrono::system_clock::time_point deadline = chrono::system_clock::now() 
                                              + chrono::milliseconds(deadline_timeout);
    context.set_deadline(deadline);

    // Send request and check error code
    req.set_name(filename);
    status = service_stub->Stat(&context, req, &stat);
    if (status.ok()) {
        cout << "Get file status succeeded:" << filename << endl;
        cout << stat.size() << " " << stat.mtime() << " " << stat.creationtime() << endl;

        if (file_status) {
            ((FileStatus *)file_status)->crc = stat.crc();
            ((FileStatus *)file_status)->mtime = stat.mtime();
            ((FileStatus *)file_status)->size = stat.size();
            ((FileStatus *)file_status)->creationTime = stat.creationtime();
        }
    }
    else {
        cout << "Get file status failed:" << filename << " " << status.error_message() << endl;
    }

    return status.error_code();
}

void DFSClientNodeP2::InotifyWatcherCallback(function<void()> callback) {

    //
    // STUDENT INSTRUCTION:
    //
    // This method gets called each time inotify signals a change
    // to a file on the file system. That is every time a file is
    // modified or created.
    //
    // You may want to consider how this section will affect
    // concurrent actions between the inotify watcher and the
    // asynchronous callbacks associated with the server.
    //
    // The callback method shown must be called here, but you may surround it with
    // whatever structures you feel are necessary to ensure proper coordination
    // between the async and watcher threads.
    //
    // Hint: how can you prevent race conditions between this thread and
    // the async thread when a file event has been signaled?
    //

    lock_guard<mutex> lock(rpc_mutex);
    cout << "inotify" << endl;
    callback();

}

//
// STUDENT INSTRUCTION:
//
// This method handles the gRPC asynchronous callbacks from the server.
// We've provided the base structure for you, but you should review
// the hints provided in the STUDENT INSTRUCTION sections below
// in order to complete this method.
//
void DFSClientNodeP2::HandleCallbackList() {
    void* tag;
    bool ok = false;
    struct stat s;

    //
    // STUDENT INSTRUCTION:
    //
    // Add your file list synchronization code here.
    //
    // When the server responds to an asynchronous request for the CallbackList,
    // this method is called. You should then synchronize the
    // files between the server and the client based on the goals
    // described in the readme.
    //
    // In addition to synchronizing the files, you'll also need to ensure
    // that the async thread and the file watcher thread are cooperating. These
    // two threads could easily get into a race condition where both are trying
    // to write or fetch over top of each other. So, you'll need to determine
    // what type of locking/guarding is necessary to ensure the threads are
    // properly coordinated.
    //

    // Block until the next result is available in the completion queue.
    while (completion_queue.Next(&tag, &ok)) {
        {
            //
            // STUDENT INSTRUCTION:
            //
            // Consider adding a critical section or RAII style lock here
            //
            lock_guard<mutex> lock(rpc_mutex);

            // The tag is the memory location of the call_data object
            AsyncClientData<FileListResponseType> *call_data = static_cast<AsyncClientData<FileListResponseType> *>(tag);

            dfs_log(LL_DEBUG2) << "Received completion queue callback";

            // Verify that the request was completed successfully. Note that "ok"
            // corresponds solely to the request for updates introduced by Finish().
            // GPR_ASSERT(ok);
            if (!ok) {
                dfs_log(LL_ERROR) << "Completion queue callback not ok.";
            }

            if (ok && call_data->status.ok()) {

                dfs_log(LL_DEBUG3) << "Handling async callback ";

                //
                // STUDENT INSTRUCTION:
                //
                // Add your handling of the asynchronous event calls here.
                // For example, based on the file listing returned from the server,
                // how should the client respond to this updated information?
                // Should it retrieve an updated version of the file?
                // Send an update to the server?
                // Do nothing?
                //

                for (FileStat i : call_data->reply.stat()) {
                    if (0 > stat(WrapPath(i.name()).c_str(), &s)) {
                        cout << "File not found:" << i.name() << endl;
                        DFSClientNodeP2::Fetch(i.name());
                    }
                    else {
                        if (i.crc() == dfs_file_checksum(WrapPath(i.name()), &crc_table)) {
                            cout << "Same file:" << i.name() << endl;
                            continue;
                        }

                        if (s.st_mtim.tv_sec > i.mtime()) {
                            cout << "File up-to-data:" << i.name() << endl;
                            DFSClientNodeP2::Store(i.name());
                        }
                        else {
                            cout << "File out-of-data:" << i.name() << endl;
                            DFSClientNodeP2::Fetch(i.name());
                        }
                    }
                }
                // TODO: Store other files only on client


            } else {
                dfs_log(LL_ERROR) << "Status was not ok. Will try again in " << DFS_RESET_TIMEOUT << " milliseconds.";
                dfs_log(LL_ERROR) << call_data->status.error_message();
                this_thread::sleep_for(chrono::milliseconds(DFS_RESET_TIMEOUT));
            }

            // Once we're complete, deallocate the call_data object.
            delete call_data;

            //
            // STUDENT INSTRUCTION:
            //
            // Add any additional syncing/locking mechanisms you may need here

        }


        // Start the process over and wait for the next callback response
        dfs_log(LL_DEBUG3) << "Calling InitCallbackList";
        InitCallbackList();

    }
}

/**
 * This method will start the callback request to the server, requesting
 * an update whenever the server sees that files have been modified.
 *
 * We're making use of a template function here, so that we can keep some
 * of the more intricate workings of the async process out of the way, and
 * give you a chance to focus more on the project's requirements.
 */
void DFSClientNodeP2::InitCallbackList() {
    CallbackList<FileRequestType, FileListResponseType>();
}

//
// STUDENT INSTRUCTION:
//
// Add any additional code you need to here
//


