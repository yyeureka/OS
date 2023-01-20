#pragma once

#include <map>
#include <grpcpp/grpcpp.h>
#include "masterworker.grpc.pb.h"
#include <mr_task_factory.h>
#include "mr_tasks.h"
using namespace std;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using masterworker::MrTask;
using masterworker::Request;
using masterworker::Reply;
using masterworker::Shard;

extern std::shared_ptr<BaseMapper> get_mapper_from_task_factory(const std::string& user_id);
extern std::shared_ptr<BaseReducer> get_reducer_from_task_factory(const std::string& user_id);

/* Handle all the task a map/reduce Worker is supposed to do. */
class Worker : public MrTask::Service {

	public:
		/* DON'T change the function signature of this constructor */
		Worker(std::string ip_addr_port);

		/* DON'T change this function's signature */
		bool run();
		
		Status AssignTask(ServerContext* ctx, const Request* request, Reply* reply) override {
			cout << ip_addr_ << " get " << request->task_type() << request->index() << endl;

			if ("map" == request->task_type()) {
				vector<string> filenames = doMap(request);
				if (filenames.size() < request->n_output_files()) {
					cerr << "Invalid intermediate file number:" << filenames.size() << endl; 
				}
				
				for (string filename : filenames) {
					reply->add_output_files(filename);
				}
				reply->set_status(true);			
			} 
			else if ("reduce" == request->task_type()) {
				string filename = doReduce(request);
				reply->add_output_files(filename);
				reply->set_status(true);
			}
			else {
				cerr << "Invalid task type:" << request->task_type() << endl; 
				reply->set_status(false);
			}

    		return Status::OK;
  		}

	private:
		string ip_addr_;

		vector<string> doMap(const Request* request) {
			string line;

			auto mapper = get_mapper_from_task_factory(request->user_id());
			mapper->impl_->output_dir = request->output_dir() + "/intermediate" + to_string(request->index()) + "_";
			mapper->impl_->n_output_files = request->n_output_files();

			for (Shard shard : request->shards()) {
				// cout << "shard" << request->index() << ":" << shard.filename() << " from:" << shard.start() << "to:" << shard.end() << endl;

				ifstream file(shard.filename());
				while (!file.is_open()) {
					cerr << "worker fail to open shard file." << shard.filename() << endl;
					file.open(shard.filename());
				}

				file.seekg(shard.start());
				int line_count = shard.line_count();
				while (line_count > 0) {
					getline(file, line);
					mapper->map(line);
					line_count--;
				}
				file.close();
			}

			cout << ip_addr_ << " finish map" << request->index() << endl;
			cout << ip_addr_ << " after map:" << mapper->impl_->word << ":" << mapper->impl_->count << endl;
			
			mapper->impl_->flush();
			return mapper->impl_->get_filenames();
		}

		string doReduce(const Request* request) {
			string filename;
			string line;
			string key;
			string val;
			int idx;

			auto reducer = get_reducer_from_task_factory(request->user_id());
			filename = request->output_dir() + "/final" + to_string(request->worker_id()) + to_string(request->index()) + ".txt";
			reducer->impl_->filename = filename;

			map<string, vector<string>> reduce_map;
			int count = 0;
			for (Shard shard : request->shards()) {
				ifstream file(shard.filename());
				while (!file.is_open()) {
					cerr << "worker fail to open intermediate file." << shard.filename() << endl;
					file.open(shard.filename());
				}

				while (getline(file, line)) {
					idx = line.find(" ");
					key = line.substr(0, idx);
					val = line.substr(idx + 1);
					reduce_map[key].push_back(val);

					count += stoi(val);
				}
				file.close();

				// while (remove(shard.filename().c_str()) < 0) {
				// 	cerr << "worker fail to delete " << shard.filename() << endl;
				// }
			}

			// cout << "medium count:" << count << endl;

			for (auto iter : reduce_map) {
				reducer->reduce(iter.first, iter.second);
    		}	

			cout << ip_addr_ << " finish reduce" << request->index() << endl;
			cout << ip_addr_ << " after reduce:" << reducer->impl_->word << ":" << reducer->impl_->count << endl;

			reducer->impl_->flush();
			return filename;
		}

};

Worker::Worker(string ip_addr_port) {
	ip_addr_ = ip_addr_port;
}

/* Wait for task requests from Master */
bool Worker::run() {
	string server_addr(ip_addr_);
	ServerBuilder builder;
	builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
	builder.RegisterService(this);
	unique_ptr<Server> server(builder.BuildAndStart());
	server->Wait();
	return true;
}
