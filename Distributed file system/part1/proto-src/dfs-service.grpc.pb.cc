// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: dfs-service.proto

#include "dfs-service.pb.h"
#include "dfs-service.grpc.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/method_handler_impl.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace dfs_service {

static const char* DFSService_method_names[] = {
  "/dfs_service.DFSService/Store",
  "/dfs_service.DFSService/Fetch",
  "/dfs_service.DFSService/Delete",
  "/dfs_service.DFSService/List",
  "/dfs_service.DFSService/Stat",
};

std::unique_ptr< DFSService::Stub> DFSService::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< DFSService::Stub> stub(new DFSService::Stub(channel));
  return stub;
}

DFSService::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_Store_(DFSService_method_names[0], ::grpc::internal::RpcMethod::CLIENT_STREAMING, channel)
  , rpcmethod_Fetch_(DFSService_method_names[1], ::grpc::internal::RpcMethod::SERVER_STREAMING, channel)
  , rpcmethod_Delete_(DFSService_method_names[2], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_List_(DFSService_method_names[3], ::grpc::internal::RpcMethod::SERVER_STREAMING, channel)
  , rpcmethod_Stat_(DFSService_method_names[4], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::ClientWriter< ::dfs_service::FileContent>* DFSService::Stub::StoreRaw(::grpc::ClientContext* context, ::dfs_service::FileAck* response) {
  return ::grpc::internal::ClientWriterFactory< ::dfs_service::FileContent>::Create(channel_.get(), rpcmethod_Store_, context, response);
}

void DFSService::Stub::experimental_async::Store(::grpc::ClientContext* context, ::dfs_service::FileAck* response, ::grpc::experimental::ClientWriteReactor< ::dfs_service::FileContent>* reactor) {
  ::grpc::internal::ClientCallbackWriterFactory< ::dfs_service::FileContent>::Create(stub_->channel_.get(), stub_->rpcmethod_Store_, context, response, reactor);
}

::grpc::ClientAsyncWriter< ::dfs_service::FileContent>* DFSService::Stub::AsyncStoreRaw(::grpc::ClientContext* context, ::dfs_service::FileAck* response, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::internal::ClientAsyncWriterFactory< ::dfs_service::FileContent>::Create(channel_.get(), cq, rpcmethod_Store_, context, response, true, tag);
}

::grpc::ClientAsyncWriter< ::dfs_service::FileContent>* DFSService::Stub::PrepareAsyncStoreRaw(::grpc::ClientContext* context, ::dfs_service::FileAck* response, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncWriterFactory< ::dfs_service::FileContent>::Create(channel_.get(), cq, rpcmethod_Store_, context, response, false, nullptr);
}

::grpc::ClientReader< ::dfs_service::FileContent>* DFSService::Stub::FetchRaw(::grpc::ClientContext* context, const ::dfs_service::Request& request) {
  return ::grpc::internal::ClientReaderFactory< ::dfs_service::FileContent>::Create(channel_.get(), rpcmethod_Fetch_, context, request);
}

void DFSService::Stub::experimental_async::Fetch(::grpc::ClientContext* context, ::dfs_service::Request* request, ::grpc::experimental::ClientReadReactor< ::dfs_service::FileContent>* reactor) {
  ::grpc::internal::ClientCallbackReaderFactory< ::dfs_service::FileContent>::Create(stub_->channel_.get(), stub_->rpcmethod_Fetch_, context, request, reactor);
}

::grpc::ClientAsyncReader< ::dfs_service::FileContent>* DFSService::Stub::AsyncFetchRaw(::grpc::ClientContext* context, const ::dfs_service::Request& request, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::internal::ClientAsyncReaderFactory< ::dfs_service::FileContent>::Create(channel_.get(), cq, rpcmethod_Fetch_, context, request, true, tag);
}

::grpc::ClientAsyncReader< ::dfs_service::FileContent>* DFSService::Stub::PrepareAsyncFetchRaw(::grpc::ClientContext* context, const ::dfs_service::Request& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncReaderFactory< ::dfs_service::FileContent>::Create(channel_.get(), cq, rpcmethod_Fetch_, context, request, false, nullptr);
}

::grpc::Status DFSService::Stub::Delete(::grpc::ClientContext* context, const ::dfs_service::Request& request, ::dfs_service::FileAck* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_Delete_, context, request, response);
}

void DFSService::Stub::experimental_async::Delete(::grpc::ClientContext* context, const ::dfs_service::Request* request, ::dfs_service::FileAck* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_Delete_, context, request, response, std::move(f));
}

void DFSService::Stub::experimental_async::Delete(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::dfs_service::FileAck* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_Delete_, context, request, response, std::move(f));
}

void DFSService::Stub::experimental_async::Delete(::grpc::ClientContext* context, const ::dfs_service::Request* request, ::dfs_service::FileAck* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_Delete_, context, request, response, reactor);
}

void DFSService::Stub::experimental_async::Delete(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::dfs_service::FileAck* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_Delete_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::dfs_service::FileAck>* DFSService::Stub::AsyncDeleteRaw(::grpc::ClientContext* context, const ::dfs_service::Request& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::dfs_service::FileAck>::Create(channel_.get(), cq, rpcmethod_Delete_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::dfs_service::FileAck>* DFSService::Stub::PrepareAsyncDeleteRaw(::grpc::ClientContext* context, const ::dfs_service::Request& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::dfs_service::FileAck>::Create(channel_.get(), cq, rpcmethod_Delete_, context, request, false);
}

::grpc::ClientReader< ::dfs_service::FileList>* DFSService::Stub::ListRaw(::grpc::ClientContext* context, const ::dfs_service::Request& request) {
  return ::grpc::internal::ClientReaderFactory< ::dfs_service::FileList>::Create(channel_.get(), rpcmethod_List_, context, request);
}

void DFSService::Stub::experimental_async::List(::grpc::ClientContext* context, ::dfs_service::Request* request, ::grpc::experimental::ClientReadReactor< ::dfs_service::FileList>* reactor) {
  ::grpc::internal::ClientCallbackReaderFactory< ::dfs_service::FileList>::Create(stub_->channel_.get(), stub_->rpcmethod_List_, context, request, reactor);
}

::grpc::ClientAsyncReader< ::dfs_service::FileList>* DFSService::Stub::AsyncListRaw(::grpc::ClientContext* context, const ::dfs_service::Request& request, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::internal::ClientAsyncReaderFactory< ::dfs_service::FileList>::Create(channel_.get(), cq, rpcmethod_List_, context, request, true, tag);
}

::grpc::ClientAsyncReader< ::dfs_service::FileList>* DFSService::Stub::PrepareAsyncListRaw(::grpc::ClientContext* context, const ::dfs_service::Request& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncReaderFactory< ::dfs_service::FileList>::Create(channel_.get(), cq, rpcmethod_List_, context, request, false, nullptr);
}

::grpc::Status DFSService::Stub::Stat(::grpc::ClientContext* context, const ::dfs_service::Request& request, ::dfs_service::FileStat* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_Stat_, context, request, response);
}

void DFSService::Stub::experimental_async::Stat(::grpc::ClientContext* context, const ::dfs_service::Request* request, ::dfs_service::FileStat* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_Stat_, context, request, response, std::move(f));
}

void DFSService::Stub::experimental_async::Stat(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::dfs_service::FileStat* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_Stat_, context, request, response, std::move(f));
}

void DFSService::Stub::experimental_async::Stat(::grpc::ClientContext* context, const ::dfs_service::Request* request, ::dfs_service::FileStat* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_Stat_, context, request, response, reactor);
}

void DFSService::Stub::experimental_async::Stat(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::dfs_service::FileStat* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_Stat_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::dfs_service::FileStat>* DFSService::Stub::AsyncStatRaw(::grpc::ClientContext* context, const ::dfs_service::Request& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::dfs_service::FileStat>::Create(channel_.get(), cq, rpcmethod_Stat_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::dfs_service::FileStat>* DFSService::Stub::PrepareAsyncStatRaw(::grpc::ClientContext* context, const ::dfs_service::Request& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::dfs_service::FileStat>::Create(channel_.get(), cq, rpcmethod_Stat_, context, request, false);
}

DFSService::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      DFSService_method_names[0],
      ::grpc::internal::RpcMethod::CLIENT_STREAMING,
      new ::grpc::internal::ClientStreamingHandler< DFSService::Service, ::dfs_service::FileContent, ::dfs_service::FileAck>(
          std::mem_fn(&DFSService::Service::Store), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      DFSService_method_names[1],
      ::grpc::internal::RpcMethod::SERVER_STREAMING,
      new ::grpc::internal::ServerStreamingHandler< DFSService::Service, ::dfs_service::Request, ::dfs_service::FileContent>(
          std::mem_fn(&DFSService::Service::Fetch), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      DFSService_method_names[2],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< DFSService::Service, ::dfs_service::Request, ::dfs_service::FileAck>(
          std::mem_fn(&DFSService::Service::Delete), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      DFSService_method_names[3],
      ::grpc::internal::RpcMethod::SERVER_STREAMING,
      new ::grpc::internal::ServerStreamingHandler< DFSService::Service, ::dfs_service::Request, ::dfs_service::FileList>(
          std::mem_fn(&DFSService::Service::List), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      DFSService_method_names[4],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< DFSService::Service, ::dfs_service::Request, ::dfs_service::FileStat>(
          std::mem_fn(&DFSService::Service::Stat), this)));
}

DFSService::Service::~Service() {
}

::grpc::Status DFSService::Service::Store(::grpc::ServerContext* context, ::grpc::ServerReader< ::dfs_service::FileContent>* reader, ::dfs_service::FileAck* response) {
  (void) context;
  (void) reader;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status DFSService::Service::Fetch(::grpc::ServerContext* context, const ::dfs_service::Request* request, ::grpc::ServerWriter< ::dfs_service::FileContent>* writer) {
  (void) context;
  (void) request;
  (void) writer;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status DFSService::Service::Delete(::grpc::ServerContext* context, const ::dfs_service::Request* request, ::dfs_service::FileAck* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status DFSService::Service::List(::grpc::ServerContext* context, const ::dfs_service::Request* request, ::grpc::ServerWriter< ::dfs_service::FileList>* writer) {
  (void) context;
  (void) request;
  (void) writer;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status DFSService::Service::Stat(::grpc::ServerContext* context, const ::dfs_service::Request* request, ::dfs_service::FileStat* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace dfs_service

