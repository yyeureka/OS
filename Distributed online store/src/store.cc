#include "threadpool.h"

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "store.grpc.pb.h"
#include "vendor.grpc.pb.h"
using namespace std;

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::ClientAsyncResponseReader;
using grpc::CompletionQueue;
using grpc::ClientContext;
using grpc::Status;
using store::Store;
using store::ProductQuery;
using store::ProductReply;
using store::ProductInfo;
using vendor::Vendor;
using vendor::BidQuery;
using vendor::BidReply;

class StoreServer final {
	public:
	~StoreServer() {
		server_->Shutdown();
		// Always shutdown the completion queue after the server.
		cq_->Shutdown();
	}

	void Run(vector<string> *vendor_addrs, string addr, int thread_num) {
		ServerBuilder builder;
		// Listen on the given address without any authentication mechanism.
		builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
		// Register asynchronous service through which we'll communicate with clients.
		builder.RegisterService(&service_);
		// Get the completion queue used for the asynchronous communication with the gRPC runtime.
		cq_ = builder.AddCompletionQueue();
		// Assemble the server.
		server_ = builder.BuildAndStart();
		cout << "Server listening on " << addr << endl;

		// Proceed to the server's main loop.
		vendors_ = vendor_addrs;

		thread worker[thread_num];
		for (int i = 0; i < thread_num; i++) {
			worker[i] = thread(&StoreServer::HandleReqs, this);
		}
		for (int i = 0; i < thread_num; i++) {
			worker[i].join();
		}
	}

	private:
	// Class encompasing the state and logic needed to serve a request.
	class CallData {
		public:
		CallData(Store::AsyncService* service, ServerCompletionQueue* cq, vector<string> *vendors) 
		: server_(service), cq_(cq), responder_(&ctx_), status_(CREATE), vendors_(vendors) {
			Proceed();
		}

		void Proceed() {
			if (status_ == CREATE) {
				status_ = PROCESS;
				// start processing SayHello requests. 
				server_->RequestgetProducts(&ctx_, &request_, &responder_, cq_, cq_, this);
			} 
			else if (status_ == PROCESS) {
				// Spawn a new CallData instance to serve new clients.
				new CallData(server_, cq_, vendors_);

				RequestBid();

				status_ = FINISH;
				responder_.Finish(reply_, Status::OK, this);
			}
			else {
				GPR_ASSERT(status_ == FINISH);
				// Once in the FINISH state, deallocate ourselves (CallData).
				delete this;
			}
		}

		private:
		void RequestBid() {
			for (auto iter = vendors_->begin(); iter != vendors_->end(); iter++) {
				unique_ptr<Vendor::Stub> stub_(Vendor::NewStub(grpc::CreateChannel(
					*iter, grpc::InsecureChannelCredentials())));
				
				BidQuery vendor_request;
				vendor_request.set_product_name(request_.product_name());
				BidReply vendor_reply;
				ClientContext vendor_ctx;
				CompletionQueue ventor_cq;
				Status vendor_status;

				// Creates a RPC object
				unique_ptr<ClientAsyncResponseReader<BidReply> > rpc(
					stub_->PrepareAsyncgetProductBid(&vendor_ctx, vendor_request, &ventor_cq));

				// Initiates the RPC call
				rpc->StartCall();

				// Upon completion of the RPC, "reply" be updated with the server's response
				rpc->Finish(&vendor_reply, &vendor_status, (void*)1);

				void* got_tag;
				bool ok = false;

				// Block until the next result is available in the completion queue "cq".
				GPR_ASSERT(ventor_cq.Next(&got_tag, &ok));
				GPR_ASSERT(got_tag == (void*)1);
				GPR_ASSERT(ok);

				if (vendor_status.ok()) {
					if ((vendor_reply.vendor_id().empty()) || (vendor_reply.price() < 0)) {
						cerr << "Invalid vendor reply:" << request_.product_name() 
						<< " " << vendor_reply.price() << endl;
						continue;
					}

					ProductInfo* info = reply_.add_products();
					info->set_price(vendor_reply.price());
					info->set_vendor_id(vendor_reply.vendor_id());
				} 
				else {
					cerr << "RPC failed" << endl;
				}
			}
		}

		Store::AsyncService* server_;
		ServerCompletionQueue* cq_;
		// Context for the rpc
		ServerContext ctx_;
		// What we get from the client.
		ProductQuery request_;
		// What we send back to the client.
		ProductReply reply_;
		// The means to get back to the client.
		ServerAsyncResponseWriter<ProductReply> responder_;

		enum CallStatus { CREATE, PROCESS, FINISH };
		CallStatus status_;  // The current serving state.

		vector<string> *vendors_;
	};

	void HandleReqs() {
		// Spawn a new CallData instance to serve new clients.
		new CallData(&service_, cq_.get(), vendors_);
		void* tag;  // uniquely identifies a request.
		bool ok;
		while (true) {
			// Block waiting to read the next event from the completion queue.
			GPR_ASSERT(cq_->Next(&tag, &ok));
			GPR_ASSERT(ok);
			static_cast<CallData*>(tag)->Proceed();
		}
	}

	unique_ptr<ServerCompletionQueue> cq_;
	Store::AsyncService service_;
	unique_ptr<Server> server_;
	vector<string> *vendors_;
};

int main(int argc, char** argv) {
	string filename;
	string addr;
	int thread_num = 2;

	ifstream file;
	vector<string> vendor_addrs;
	string vendor_addr;

	if (4 == argc) {
		filename = string(argv[1]);
		addr = string(argv[2]);
		thread_num = atoi(argv[3]);
	}
	else {
		cerr << "Correct usage: ./store $file_path_for_vendor_addrresses $address $thread_number" << endl;
		return EXIT_FAILURE;
	}

	file.open(filename);
	if (file.is_open()) {
		while (getline(file, vendor_addr)) {
			vendor_addrs.push_back(vendor_addr);
		}

		if (vendor_addrs.empty()) {
			cerr << "Empty file!" << endl;
			return EXIT_FAILURE;
		}

		file.close();
	}
	else {
		cerr << "Failed to open file " << filename << endl;
		return EXIT_FAILURE;
	}

	StoreServer server;
	server.Run(&vendor_addrs, addr, thread_num);

	return EXIT_SUCCESS;
}

