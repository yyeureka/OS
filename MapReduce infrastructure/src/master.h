#pragma once

#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include "masterworker.grpc.pb.h"
#include "mapreduce_spec.h"
#include "file_shard.h"
using namespace std;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using masterworker::MrTask;
using masterworker::Request;
using masterworker::Reply;
using masterworker::Shard;

#define TIMEOUT 300  // Two minute timeout for each task

/* Handle all the bookkeeping the Master is supposed to do. */
class Master {

	public:
		/* DON'T change the function signature of this constructor */
		Master(const MapReduceSpec&, const std::vector<FileShard>&);

		/* DON'T change this function's signature */
		bool run();

	private:
		MapReduceSpec mr_spec_;
		vector<FileShard> file_shards_;
		vector<unique_ptr<MrTask::Stub>> stubs;
		vector<shared_ptr<Channel>> channels;
		vector<bool> worker_status; // true:valid, false:invalid
		vector<int> map_status;     // 0:not assigned, 1:in-progress, 2:done
		vector<int> reduce_status;  // 0:not assigned, 1:in-progress, 2:done
		map<int, vector<string>> reduce_file_map;

		mutex task_lock;
		mutex file_lock;

		void scheduleMapTask(int worker_id);
		void scheduleReduceTask(int worker_id);
};

Master::Master(const MapReduceSpec& mr_spec, const vector<FileShard>& file_shards)
: mr_spec_(mr_spec), file_shards_(file_shards) {
	for (int i = 0; i < mr_spec_.n_workers; i++) {
		shared_ptr<Channel> channel = grpc::CreateChannel(mr_spec_.worker_ipaddr_ports[i], grpc::InsecureChannelCredentials());
		unique_ptr<MrTask::Stub> stub = MrTask::NewStub(channel);
		stubs.push_back(move(stub));  //unique_ptr cannot be copied, have to be moved
		channels.push_back(channel);
		worker_status.push_back(true);
	}
	for (int i = 0; i < file_shards_.size(); i++) {
		map_status.push_back(0);
	}
	for (int i = 0; i < mr_spec_.n_output_files; i++) {
		reduce_status.push_back(0);
	}
}

/* Handle all map&reduce tasks */
bool Master::run() {
	thread worker[mr_spec_.n_workers];

	for (int i = 0; i < mr_spec_.n_workers; i++) {
		worker[i] = thread(&Master::scheduleMapTask, this, i);
	}
	for (int i = 0; i < mr_spec_.n_workers; i++) {
		worker[i].join();
	}

	// string line;
	// int idx;
	// string key;
	// string val;
	// for (auto it : reduce_file_map) {
	// 	int count = 0;
	// 	for (string f : it.second) {
	// 		ifstream file(f);
	// 		while (getline(file, line)) {
	// 			idx = line.find(" ");
	// 			key = line.substr(0, idx);
	// 			val = line.substr(idx + 1);
	// 			count += stoi(val);
	// 		}
	// 		file.close();
	// 	}
	// 	cout << it.first << "before count:" << count << endl;
	// }

	for (int i = 0; i < mr_spec_.n_workers; i++) {
		worker[i] = thread(&Master::scheduleReduceTask, this, i);
	}
	for (int i = 0; i < mr_spec_.n_workers; i++) {
		worker[i].join();
	}

	for (int i = 0; i < mr_spec_.n_workers; i++) {
		fs::remove_all(mr_spec_.output_dir + "/tmp" + to_string(i));
		cout << "delete" << i << endl;
	}

	return true;
}

void Master::scheduleMapTask(int worker_id) {
	int shard_index;
	bool done;
	int start;
	int end;
	int key;
	
	while (worker_status[worker_id]) {
		shard_index = -1;
		done = true;

		task_lock.lock();
			for (int i = 0; i < file_shards_.size(); i++) {
				if (0 == map_status[i]) {
					shard_index = i;
					map_status[i] = 1;
					done = false;
					break;
				}
				else if (1 == map_status[i]) {
					done = false;
				}
			}
		task_lock.unlock();

		if (done) {
			break;
		}
		else if (-1 == shard_index) {
			this_thread::yield();
			continue;
		}

		Request request;
		request.set_user_id(mr_spec_.user_id);
		request.set_task_type("map");
		string path = mr_spec_.output_dir + "/tmp" + to_string(worker_id);
		mkdir(path.c_str(), 0777);
		request.set_output_dir(path);
		request.set_n_output_files(mr_spec_.n_output_files);
		request.set_worker_id(worker_id);
		request.set_index(shard_index);
		for (File file : file_shards_[shard_index].files) {
			Shard* shard = request.add_shards();
			shard->set_filename(file.filename);
			shard->set_start(file.start);
			shard->set_line_count(file.line_count);
			// cout << "shard" << shard_index << ":" << file.filename << " from:" << file.start << "to:" << file.end << endl;
		}
		// cout << "worker" << worker_id << ":shard" << shard_index << " " << file_shards_[shard_index].files.size() << "files" << endl;

		Reply reply;	
		ClientContext ctx;
		chrono::system_clock::time_point deadline = chrono::system_clock::now() + chrono::seconds(TIMEOUT);
		ctx.set_deadline(deadline);
		Status status = stubs[request.worker_id()]->AssignTask(&ctx, request, &reply);

		if (status.ok()) {
			for (string filename : reply.output_files()) {
				start = filename.find('_') + 1;
				end = filename.find('.');
				key = stoi(filename.substr(start, end));

				file_lock.lock();
					Master::reduce_file_map[key].push_back(filename);
				file_lock.unlock();
			}

			task_lock.lock();
				map_status[request.index()] = 2;
				cout << "worker" << request.worker_id() << " finish map" << request.index() << endl;
			task_lock.unlock();
		}
		else {
			cout << "map" << request.worker_id() << request.index() << "failed" << endl;
			worker_status[request.worker_id()] = false;
			task_lock.lock();
				map_status[request.index()] = 0;
			task_lock.unlock();
		}
	}
}

void Master::scheduleReduceTask(int worker_id) {
	int out_index;
	bool done;

	while (worker_status[worker_id]) {
		out_index = -1;
		done = true;

		task_lock.lock();
			for (int i = 0; i < mr_spec_.n_output_files; i++) {
				if (0 == reduce_status[i]) {
					out_index = i;
					reduce_status[i] = 1;
					done = false;
					break;
				}
				else if (1 == reduce_status[i]) {
					done = false;
				}
			}
		task_lock.unlock();

		if (done) {
			break;
		}
		else if (-1 == out_index) {
			this_thread::yield();
			continue;
		}

		Request request;
		request.set_user_id(mr_spec_.user_id);
		request.set_task_type("reduce");
		request.set_output_dir(mr_spec_.output_dir);
		request.set_n_output_files(mr_spec_.n_output_files);
		request.set_worker_id(worker_id);
		request.set_index(out_index);
		for (string filename : reduce_file_map[out_index]) {
			Shard* shard = request.add_shards();
			shard->set_filename(filename);
			shard->set_start(0);
			shard->set_line_count(0);
			// cout << "intermediate" << out_index << ":" << filename << endl;
		}
		// cout << "worker" << worker_id << ":intermediate" << out_index << " " << reduce_file_map[out_index].size() << "files" << endl;

		Reply reply;	
		ClientContext ctx;
		chrono::system_clock::time_point deadline = chrono::system_clock::now() + chrono::seconds(TIMEOUT);
		ctx.set_deadline(deadline);
		Status status = stubs[request.worker_id()]->AssignTask(&ctx, request, &reply);

		if (status.ok()) {
			task_lock.lock();
				reduce_status[request.index()] = 2;
				cout << "worker" << request.worker_id() << " finish reduce" << request.index() << endl;
			task_lock.unlock();
		}
		else {
			cout << "reduce" << request.worker_id() << request.index() << "failed" << endl;
			worker_status[request.worker_id()] = false;
			task_lock.lock();
				reduce_status[request.index()] = 0;
			task_lock.unlock();
		}
	}
}