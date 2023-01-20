#include <map>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <getopt.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grpcpp/grpcpp.h>

#include "src/dfs-utils.h"
#include "dfslib-shared-p1.h"
#include "dfslib-servernode-p1.h"
#include "proto-src/dfs-service.grpc.pb.h"

using namespace std;
using grpc::Status;
using grpc::Server;
using grpc::StatusCode;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::ServerContext;
using grpc::ServerBuilder;

using dfs_service::DFSService;
using dfs_service::Request;
using dfs_service::FileAck;
using dfs_service::FileContent;
using dfs_service::FileList;
using dfs_service::FileStat;

//
// STUDENT INSTRUCTION:
//
// DFSServiceImpl is the implementation service for the rpc methods
// and message types you defined in the `dfs-service.proto` file.
//
// You should add your definition overrides here for the specific
// methods that you created for your GRPC service protocol. The
// gRPC tutorial described in the readme is a good place to get started
// when trying to understand how to implement this class.
//
// The method signatures generated can be found in `proto-src/dfs-service.grpc.pb.h` file.
//
// Look for the following section:
//
//      class Service : public ::grpc::Service {
//
// The methods returning grpc::Status are the methods you'll want to override.
//
// In C++, you'll want to use the `override` directive as well. For example,
// if you have a service method named MyMethod that takes a MyMessageType
// and a ServerWriter, you'll want to override it similar to the following:
//
//      Status MyMethod(ServerContext* context,
//                      const MyMessageType* request,
//                      ServerWriter<MySegmentType> *writer) override {
//
//          /** code implementation here **/
//      }
//
class DFSServiceImpl final : public DFSService::Service {

private:

    /** The mount path for the server **/
    string mount_path;

    /**
     * Prepend the mount path to the filename.
     *
     * @param filepath
     * @return
     */
    const string WrapPath(const string &filepath) {
        return this->mount_path + filepath;
    }


public:

    DFSServiceImpl(const string &mount_path): mount_path(mount_path) {
    }

    ~DFSServiceImpl() {}

    Status Store(ServerContext* context, ServerReader<FileContent>* reader, FileAck* response) {
        FileContent file;
        ofstream ofile;
        struct stat s;
        string tmp;

        // Check deadline
        if (context->IsCancelled()) {
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded");
        }      

        // Receive file
        while (reader->Read(&file)) {
            tmp += file.foo();
        }
        cout << "Store receive:" << tmp.size() << endl;

        // Store file
        ofile.open(WrapPath(file.name()));
        if (!ofile) {
            cout << "Create file failed:" << file.name() << endl;
            return Status(StatusCode::CANCELLED, "Create file failed:" + file.name());
        }
        ofile.write(tmp.c_str(), tmp.size());
        ofile.close();
        
        // Send response
        if (0 > stat(WrapPath(file.name()).c_str(), &s)) {
            cout << "Get file status failed:" << file.name() << endl;
            return Status(StatusCode::CANCELLED, "Get file status failed:" + file.name());
        }
        response->set_name(file.name());
        response->set_mtime(s.st_mtim.tv_sec);

        return Status::OK;
    }

    Status Fetch(ServerContext* context, const Request* request, ServerWriter<FileContent>* writer) {
        FileContent file;
        struct stat s;
        ifstream ifile;
        char buffer[BUFFERSIZE];
        long byte_total = 0;
        long read_byte = 0;

        // Check deadline
        if (context->IsCancelled()) {
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded");
        }

        // Check file existence
        if (0 > stat(WrapPath(request->message()).c_str(), &s)) {
            cout << "File not found:" << request->message() << endl;
            return Status(StatusCode::NOT_FOUND, "File not found:" + request->message());
        }

        // Open file
        ifile.open(WrapPath(request->message()));
        if (!ifile) {
            cout << "Open file failed:" << request->message() << endl;
            return Status(StatusCode::CANCELLED, "Open file failed:" + request->message());
        }

        while (byte_total < s.st_size) {
            read_byte = MIN(BUFFERSIZE, s.st_size - byte_total);
            ifile.read(buffer, read_byte);

            // send file
            file.set_name(request->message());
            file.set_mtime(s.st_mtim.tv_sec);
            file.set_foo(buffer, read_byte);
            writer->Write(file);

            byte_total += read_byte;
        }
        cout << "Fetch send:" << s.st_size << endl;
        ifile.close();
        
        return Status::OK;
    }

    Status Delete(ServerContext* context, const Request* request, FileAck* response) {
        struct stat s;

        // Check deadline
        if (context->IsCancelled()) {
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded");
        }

        // Check file existence
        if (0 > stat(WrapPath(request->message()).c_str(), &s)) {
            cout << "File not found:" << request->message() << endl;
            return Status(StatusCode::NOT_FOUND, "File not found:" + request->message());
        }

        // Send response
        response->set_name(request->message());
        response->set_mtime(s.st_mtim.tv_sec);

        // Delete file
        if (0 > remove(WrapPath(request->message()).c_str())) {
            cout << "Delete failed:" << request->message() << endl;
            return Status(StatusCode::CANCELLED, "Delete failed:" + request->message());
        }

        return Status::OK;
    }

    Status List(ServerContext* context, const Request* request, ServerWriter<FileList>* writer) {
        FileList list;
        struct dirent *ptr;
        DIR *dir;
        struct stat s;

        // Check deadline
        if (context->IsCancelled()) {
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded");
        }

        // List all file
        dir = opendir(mount_path.c_str());
        while (NULL != (ptr = readdir(dir))) {
            // Skip '.' and '..'
            if ('.' == ptr->d_name[0]) {
                continue;
            }

            if (0 > stat(WrapPath(ptr->d_name).c_str(), &s)) {
                cout << "Get file status failed:" << ptr->d_name << endl;
                continue;
            }

            // Send response
            list.set_name(ptr->d_name);
            list.set_mtime(s.st_mtim.tv_sec);
            writer->Write(list);
        }
        closedir(dir);

        return Status::OK;
    }

    Status Stat(ServerContext* context, const Request* request, FileStat* response) {
        struct stat s;

        // Check deadline
        if (context->IsCancelled()) {
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded");
        }

        // Check file existence
        if (0 > stat(WrapPath(request->message()).c_str(), &s)) {
            cout << "File not found:" << request->message() << endl;
            return Status(StatusCode::NOT_FOUND, "File not found:" + request->message());
        }

        // Send response
        response->set_name(request->message());
        response->set_size(s.st_size);
        response->set_mtime(s.st_mtim.tv_sec);
        response->set_creationtime(s.st_mtim.tv_sec);
        
        return Status::OK;
    }

};

//
// STUDENT INSTRUCTION:
//
// The following three methods are part of the basic DFSServerNode
// structure. You may add additional methods or change these slightly,
// but be aware that the testing environment is expecting these three
// methods as-is.
//
/**
 * The main server node constructor
 *
 * @param server_address
 * @param mount_path
 */
DFSServerNode::DFSServerNode(const string &server_address,
        const string &mount_path,
        function<void()> callback) :
    server_address(server_address), mount_path(mount_path), grader_callback(callback) {}

/**
 * Server shutdown
 */
DFSServerNode::~DFSServerNode() noexcept {
    dfs_log(LL_SYSINFO) << "DFSServerNode shutting down";
    this->server->Shutdown();
}

/** Server start **/
void DFSServerNode::Start() {
    DFSServiceImpl service(this->mount_path);
    ServerBuilder builder;
    builder.AddListeningPort(this->server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    this->server = builder.BuildAndStart();
    dfs_log(LL_SYSINFO) << "DFSServerNode server listening on " << this->server_address;
    this->server->Wait();
}

//
// STUDENT INSTRUCTION:
//
// Add your additional DFSServerNode definitions here
//

