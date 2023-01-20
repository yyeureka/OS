#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/stat.h>
#include <cstdio>
#include <dirent.h>
#include <experimental/filesystem>
using namespace std;

namespace fs = std::experimental::filesystem;

/* Store specification from the config file */
struct MapReduceSpec {
	int n_workers;
	int n_output_files;
	int map_kilobytes;
	string output_dir;
	string user_id;
	vector<string> worker_ipaddr_ports;
	vector<string> input_files;
	vector<bool> worker_busy;
};

inline bool initialize_mr_spec(MapReduceSpec& mr_spec) {
	mr_spec.n_workers = 0;
	mr_spec.n_output_files = 0;
	mr_spec.map_kilobytes = 0;
	mr_spec.output_dir = "";
	mr_spec.user_id = "";

	return true;
}

/* Populate MapReduceSpec data structure with the specification from the config file */
inline bool read_mr_spec_from_config_file(const string& config_filename, MapReduceSpec& mr_spec) {
	ifstream file(config_filename);
	string line;
	int idx;
	string key;
	string val;

	initialize_mr_spec(mr_spec);

	if (!file.is_open()) {
		cerr << "Failed to open config file." << config_filename << endl;
		return false;
	}

	while (getline(file, line)) {
		idx = line.find("=");
		if (idx == line.npos) {
			cerr << "Invalid config file." << config_filename << endl;
			return false;
		}

		key = line.substr(0, idx);
		val = line.substr(idx + 1);

		if ("n_workers" == key) {
			mr_spec.n_workers = stoi(val);
		}
		else if ("n_output_files" == key) {
			mr_spec.n_output_files = stoi(val);
		}
		else if ("map_kilobytes" == key) {
			mr_spec.map_kilobytes = stoi(val);
		}
		else if ("output_dir" == key) {
			mr_spec.output_dir = val;
		}
		else if ("user_id" == key) {
			mr_spec.user_id = val;
		}
		else if ("worker_ipaddr_ports" == key) {
			while (val.find(",") != val.npos) {
				idx = val.find(",");
				mr_spec.worker_ipaddr_ports.push_back(val.substr(0, idx));
				mr_spec.worker_busy.push_back(false);
				val = val.substr(idx + 1);
			}
			mr_spec.worker_ipaddr_ports.push_back(val);
			mr_spec.worker_busy.push_back(false);
		}
		else if ("input_files" == key) {
			while (val.find(",") != val.npos) {
				idx = val.find(",");
				mr_spec.input_files.push_back(val.substr(0, idx));
				val = val.substr(idx + 1);
			}
			mr_spec.input_files.push_back(val);
		}
		else {
			cerr << "Invalid key." << key << endl;
			return false;
		}

	}

	file.close();

	return true;
}

/* Validate the specification read from the config file */
inline bool validate_mr_spec(const MapReduceSpec& mr_spec) {
	if (mr_spec.n_workers <= 0) {
		cerr << "Invalid n_workers." << endl;
		return false;
	}
	if (mr_spec.n_output_files <= 0) {
		cerr << "Invalid n_output_files." << endl;
		return false;
	}
	if (mr_spec.map_kilobytes <= 0) {
		cerr << "Invalid map_kilobytes." << endl;
		return false;
	}
	if ("" == mr_spec.output_dir) {
		cerr << "Invalid output_dir." << endl;
		return false;
	}
	if ("" == mr_spec.user_id) {
		cerr << "Invalid user_id." << endl;
		return false;
	}
	if (mr_spec.n_workers != mr_spec.worker_ipaddr_ports.size()) {
		cerr << "Invalid worker_ipaddr_ports." << endl;
		return false;
	}
	if (0 == mr_spec.input_files.size()) {
		cerr << "Invalid input_files." << endl;
		return false;
	}

	cerr << "input file:" << endl;
	for (string filename : mr_spec.input_files) {
		cerr << filename << endl;
		ifstream file(filename);
		if (!file.is_open()) {
			cerr << "Failed to open input file." << filename << endl;
			return false;
		}
		file.close();
	}

	DIR *dir = opendir(mr_spec.output_dir.c_str());
	if (dir) {
		fs::remove_all(mr_spec.output_dir);
	}
	fs::create_directories(mr_spec.output_dir);

	// string path = "tmp";
	// dir = opendir(path.c_str());
	// if (dir) {
	// 	fs::remove_all(path);
	// }

	return true;
}
