#include <map>
#include <mutex>
#include <shared_mutex>
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

#include "proto-src/dfs-service.grpc.pb.h"
#include "src/dfslibx-call-data.h"
#include "src/dfslibx-service-runner.h"
#include "dfslib-shared-p2.h"
#include "dfslib-servernode-p2.h"

using namespace std;
using grpc::Status;
using grpc::Server;
using grpc::StatusCode;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::ServerContext;
using grpc::ServerBuilder;

using dfs_service::DFSService;

//
// STUDENT INSTRUCTION:
//
// Change these "using" aliases to the specific
// message types you are using in your `dfs-service.proto` file
// to indicate a file request and a listing of files from the server
//
using dfs_service::Request;
using dfs_service::WriteAccess;
using dfs_service::FileContent;
using dfs_service::Ack;
using dfs_service::FileList;
using dfs_service::FileStat;
using FileRequestType = Request;
using FileListResponseType = dfs_service::CallbackFileList;

extern dfs_log_level_e DFS_LOG_LEVEL;

//
// STUDENT INSTRUCTION:
//
// As with Part 1, the DFSServiceImpl is the implementation service for the rpc methods
// and message types you defined in your `dfs-service.proto` file.
//
// You may start with your Part 1 implementations of each service method.
//
// Elements to consider for Part 2:
//
// - How will you implement the write lock at the server level?
// - How will you keep track of which client has a write lock for a file?
//      - Note that we've provided a preset client_id in DFSClientNode that generates
//        a client id for you. You can pass that to the server to identify the current client.
// - How will you release the write lock?
// - How will you handle a store request for a client that doesn't have a write lock?
// - When matching files to determine similarity, you should use the `file_checksum` method we've provided.
//      - Both the client and server have a pre-made `crc_table` variable to speed things up.
//      - Use the `file_checksum` method to compare two files, similar to the following:
//
//          std::uint32_t server_crc = dfs_file_checksum(filepath, &this->crc_table);
//
//      - Hint: as the crc checksum is a simple integer, you can pass it around inside your message types.
//
class DFSServiceImpl final :
    public DFSService::WithAsyncMethod_CallbackList<DFSService::Service>,
        public DFSCallDataManager<FileRequestType , FileListResponseType> {

private:

    /** The runner service used to start the service and manage asynchronicity **/
    DFSServiceRunner<FileRequestType, FileListResponseType> runner;

    /** The mount path for the server **/
    string mount_path;

    /** Mutex for managing the queue requests **/
    mutex queue_mutex;

    /** The vector of queued tags used to manage asynchronous requests **/
    vector<QueueRequest<FileRequestType, FileListResponseType>> queued_tags;

    /**
     * Prepend the mount path to the filename.
     *
     * @param filepath
     * @return
     */
    const string WrapPath(const string &filepath) {
        return this->mount_path + filepath;
    }

    /** CRC Table kept in memory for faster calculations **/
    CRC::Table<uint32_t, 32> crc_table;

    mutex file_mutex;
    map<string, string> write_lock;

public:

    DFSServiceImpl(const string& mount_path, const string& server_address, int num_async_threads):
        mount_path(mount_path), crc_table(CRC::CRC_32()) {

        this->runner.SetService(this);
        this->runner.SetAddress(server_address);
        this->runner.SetNumThreads(num_async_threads);
        this->runner.SetQueuedRequestsCallback([&]{ this->ProcessQueuedRequests(); });
    }

    ~DFSServiceImpl() {
        this->runner.Shutdown();
    }

    void Run() {
        this->runner.Run();
    }

    /**
     * Request callback for asynchronous requests
     *
     * This method is called by the DFSCallData class during
     * an asynchronous request call from the client.
     *
     * Students should not need to adjust this.
     *
     * @param context
     * @param request
     * @param response
     * @param cq
     * @param tag
     */
    void RequestCallback(grpc::ServerContext* context,
                         FileRequestType* request,
                         grpc::ServerAsyncResponseWriter<FileListResponseType>* response,
                         grpc::ServerCompletionQueue* cq,
                         void* tag) {

        lock_guard<mutex> lock(queue_mutex);
        this->queued_tags.emplace_back(context, request, response, cq, tag);

    }

    /**
     * Process a callback request
     *
     * This method is called by the DFSCallData class when
     * a requested callback can be processed. You should use this method
     * to manage the CallbackList RPC call and respond as needed.
     *
     * See the STUDENT INSTRUCTION for more details.
     *
     * @param context
     * @param request
     * @param response
     */
    void ProcessCallback(ServerContext* context, FileRequestType* request, FileListResponseType* response) {

        //
        // STUDENT INSTRUCTION:
        //
        // You should add your code here to respond to any CallbackList requests from a client.
        // This function is called each time an asynchronous request is made from the client.
        //
        // The client should receive a list of files or modifications that represent the changes this service
        // is aware of. The client will then need to make the appropriate calls based on those changes.
        //

        FileStat *status;
        struct dirent *ptr;
        DIR *dir;
        struct stat s;

        // List all file
        dir = opendir(mount_path.c_str());
        while (NULL != (ptr = readdir(dir))) {
            // Skip '.' and '..'
            if ('.' == ptr->d_name[0]) {
                continue;
            }

            if (0 > stat(WrapPath(ptr->d_name).c_str(), &s)) {
                cout << "ProcessCallback/Get file status failed:" << ptr->d_name << endl;
                continue;
            }

            status = response->add_stat();
            status->set_name(ptr->d_name);
            status->set_crc(dfs_file_checksum(WrapPath(ptr->d_name), &crc_table));
            status->set_mtime(s.st_mtim.tv_sec);
            status->set_size(s.st_size);
            status->set_creationtime(s.st_mtim.tv_sec);
            cout << status->name() << " " << status->crc() << " " << status->mtime() << endl;
        }
        closedir(dir);
    }

    /**
     * Processes the queued requests in the queue thread
     */
    void ProcessQueuedRequests() {
        while(true) {

            //
            // STUDENT INSTRUCTION:
            //
            // You should add any synchronization mechanisms you may need here in
            // addition to the queue management. For example, modified files checks.
            //
            // Note: you will need to leave the basic queue structure as-is, but you
            // may add any additional code you feel is necessary.
            //

            lock_guard<mutex> lock(file_mutex);

            // Guarded section for queue
            {
                dfs_log(LL_DEBUG2) << "Waiting for queue guard";
                lock_guard<mutex> lock(queue_mutex);


                for(QueueRequest<FileRequestType, FileListResponseType>& queue_request : this->queued_tags) {
                    this->RequestCallbackList(queue_request.context, queue_request.request,
                        queue_request.response, queue_request.cq, queue_request.cq, queue_request.tag);
                    queue_request.finished = true;
                }

                // any finished tags first
                this->queued_tags.erase(remove_if(
                    this->queued_tags.begin(),
                    this->queued_tags.end(),
                    [](QueueRequest<FileRequestType, FileListResponseType>& queue_request) { return queue_request.finished; }
                ), this->queued_tags.end());

            }
        }
    }

    //
    // STUDENT INSTRUCTION:
    //
    // Add your additional code here, including
    // the implementations of your rpc protocol methods.
    //
    Status RequestWriteAccess(ServerContext* context, const WriteAccess* request, Ack* response) {
        // Check deadline
        if (context->IsCancelled()) {
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded");
        }

        // Send response
        response->set_message("");

        if (request->clientid() == write_lock[request->filename()]) {
            cout << "Double lock:" << request->filename() << "/" << request->clientid() << endl;
            return Status(StatusCode::OK, "Double lock");
        }
        if ("" != write_lock[request->filename()]) {
            cout << "Lock failed:" << request->filename() << "/" << request->clientid() << endl;
            return Status(StatusCode::RESOURCE_EXHAUSTED, "Lock failed");
        }

        cout << "Lock:" << request->filename() << "/" << request->clientid() << endl;
        write_lock[request->filename()] = request->clientid(); // Lock

        return Status::OK;
    }

    Status Store(ServerContext* context, ServerReader<FileContent>* reader, Ack* response) {
        FileContent file;
        ofstream ofile;
        // struct stat s;
        // uint crc;
        // int mtime;
        string tmp;

        // Receive file
        while (reader->Read(&file)) {
            tmp += file.foo();
        }
        cout << "Store receive:" << tmp.size() << endl;

        // Check lock
        if (file.clientid() != write_lock[file.name()]) {
            cout << "Store/No lock:" << file.name() << endl;
            return Status(StatusCode::RESOURCE_EXHAUSTED, "Store/No lock:" + file.name());
        }

        // Check deadline
        if (context->IsCancelled()) {
            // Release lock
            cout << "Unlock:" << file.name()  << endl;
            write_lock[file.name()] = "";

            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded");
        }

        // // Check crc and mtime
        // if (0 == stat(WrapPath(file.name()).c_str(), &s)) {
        //     crc = dfs_file_checksum(WrapPath(file.name()), &crc_table);
        //     mtime = s.st_mtim.tv_sec;

        //     if ((file.crc() == crc) || (mtime >= file.mtime())) {
        //         cout << "Store/File exist:" << file.name() << endl;
        //         return Status(StatusCode::ALREADY_EXISTS, "Store/File exist:" + file.name());
        //     }
        // }

        // Store file
        {
            lock_guard<mutex> lock(file_mutex);

            ofile.open(WrapPath(file.name()));
            if (!ofile) {
                cout << "Create file failed:" << file.name() << endl;

                // Release lock
                cout << "Unlock:" << file.name()  << endl;
                write_lock[file.name()] = "";

                return Status(StatusCode::CANCELLED, "Create file failed:" + file.name());
            }
            ofile.write(tmp.c_str(), tmp.size());
            ofile.close();
        }

        // Send response
        response->set_message("");

        // Release lock
        cout << "Unlock:" << file.name()  << endl;
        write_lock[file.name()] = "";

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
        if (0 > stat(WrapPath(request->name()).c_str(), &s)) {
            cout << "File not found:" << request->name() << endl;
            return Status(StatusCode::NOT_FOUND, "File not found:" + request->name());
        }

        // Open file
        ifile.open(WrapPath(request->name()));
        if (!ifile) {
            cout << "Open file failed:" << request->name() << endl;
            return Status(StatusCode::CANCELLED, "Open file failed:" + request->name());
        }

        while (byte_total < s.st_size) {
            read_byte = MIN(BUFFERSIZE, s.st_size - byte_total);
            ifile.read(buffer, read_byte);

            // send file
            file.set_name(request->name());
            // file.set_mtime(s.st_mtim.tv_sec);
            file.set_foo(buffer, read_byte);
            writer->Write(file);

            byte_total += read_byte;
        }
        cout << "Fetch send:" << s.st_size << endl;
        ifile.close();
        
        return Status::OK;
    }

    Status Delete(ServerContext* context, const WriteAccess* request, Ack* response) {
        struct stat s;

        // Check lock
        if (request->clientid() != write_lock[request->filename()]) {
            cout << "Delete/No lock:" << request->filename() << endl;
            return Status(StatusCode::RESOURCE_EXHAUSTED, "Delete/No lock:" + request->filename());
        }

        // Check deadline
        if (context->IsCancelled()) {
            // Release lock
            cout << "Unlock:" << request->filename()  << endl;
            write_lock[request->filename()] = "";

            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded");
        }

        // Check file existence
        if (0 > stat(WrapPath(request->filename()).c_str(), &s)) {
            cout << "File not found:" << request->filename() << endl;

            // Release lock
            cout << "Unlock:" << request->filename()  << endl;
            write_lock[request->filename()] = "";

            return Status(StatusCode::NOT_FOUND, "File not found:" + request->filename());
        }

        // Send response
        response->set_message("");

        // Delete file
        {
            lock_guard<mutex> lock(file_mutex);

            if (0 > remove(WrapPath(request->filename()).c_str())) {
                cout << "Delete failed:" << request->filename() << endl;

                // Release lock
                cout << "Unlock:" << request->filename()  << endl;
                write_lock[request->filename()] = "";

                return Status(StatusCode::CANCELLED, "Delete failed:" + request->filename());
            }
        }

        // TODO: notify all clients that the file has been deleted
        
        // Release lock
        cout << "Unlock:" << request->filename()  << endl;
        write_lock[request->filename()] = "";

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
                cout << "List/Get file status failed:" << ptr->d_name << endl;
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
        if (0 > stat(WrapPath(request->name()).c_str(), &s)) {
            cout << "File not found:" << request->name() << endl;
            return Status(StatusCode::NOT_FOUND, "File not found:" + request->name());
        }

        // Send response
        response->set_name(request->name());
        response->set_crc(dfs_file_checksum(WrapPath(request->name()), &crc_table));
        response->set_mtime(s.st_mtim.tv_sec);
        response->set_size(s.st_size);
        response->set_creationtime(s.st_mtim.tv_sec);
        
        return Status::OK;
    }
};

//
// STUDENT INSTRUCTION:
//
// The following three methods are part of the basic DFSServerNode
// structure. You may add additional methods or change these slightly
// to add additional startup/shutdown routines inside, but be aware that
// the basic structure should stay the same as the testing environment
// will be expected this structure.
//
/**
 * The main server node constructor
 *
 * @param mount_path
 */
DFSServerNode::DFSServerNode(const string &server_address,
        const string &mount_path,
        int num_async_threads,
        function<void()> callback) :
        server_address(server_address),
        mount_path(mount_path),
        num_async_threads(num_async_threads),
        grader_callback(callback) {}
/**
 * Server shutdown
 */
DFSServerNode::~DFSServerNode() noexcept {
    dfs_log(LL_SYSINFO) << "DFSServerNode shutting down";
}

/**
 * Start the DFSServerNode server
 */
void DFSServerNode::Start() {
    DFSServiceImpl service(this->mount_path, this->server_address, this->num_async_threads);


    dfs_log(LL_SYSINFO) << "DFSServerNode server listening on " << this->server_address;
    service.Run();
}

//
// STUDENT INSTRUCTION:
//
// Add your additional definitions here
//
