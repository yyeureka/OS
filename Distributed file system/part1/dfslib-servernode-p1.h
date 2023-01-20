#ifndef _DFSLIB_SERVERNODE_H
#define _DFSLIB_SERVERNODE_H

#include <string>
#include <iostream>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "proto-src/dfs-service.grpc.pb.h"

class DFSServerNode {

private:
    /** The server address information **/
    std::string server_address;

    /** The mount path for the server **/
    std::string mount_path;

    /** The pointer to the grpc server instance **/
    std::unique_ptr<grpc::Server> server;

    /** Server callback **/
    std::function<void()> grader_callback;

public:
    DFSServerNode(const std::string& server_address, const std::string& mount_path, std::function<void()> callback);
    ~DFSServerNode();
    void Shutdown();
    void Start();

    grpc::Status Store(
        ::grpc::ServerContext* context, 
        ::grpc::ServerReader< ::dfs_service::FileContent>* reader, 
        ::dfs_service::FileAck* response);

    grpc::Status Fetch(
        ::grpc::ServerContext* context, 
        const ::dfs_service::Request* request, 
        ::grpc::ServerWriter< ::dfs_service::FileContent>* writer);

    grpc::Status Delete(
        ::grpc::ServerContext* context, 
        const ::dfs_service::Request* request, 
        ::dfs_service::FileAck* response);

    grpc::Status List(
        ::grpc::ServerContext* context, 
        const ::dfs_service::Request* request, 
        ::grpc::ServerWriter< ::dfs_service::FileList>* writer);

    grpc::Status Stat(
        ::grpc::ServerContext* context, 
        const ::dfs_service::Request* request, 
        ::dfs_service::FileStat* response);
};

#endif
