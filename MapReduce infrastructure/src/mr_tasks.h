#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
using namespace std;

/* Map task */
struct BaseMapperInternal {

	/* DON'T change this function's signature */
	BaseMapperInternal();

	/* DON'T change this function's signature */
	void emit(const std::string& key, const std::string& val);
	string get_filename(string key);
	void flush();
	vector<string> get_filenames();

	string output_dir;
	int n_output_files;
	map<string, unordered_map<string, string>> buffer;
	set<string> files;
	int key_count = 0;

	int word = 0;
	int count = 0;
};

inline BaseMapperInternal::BaseMapperInternal() {}

inline void BaseMapperInternal::emit(const std::string& key, const std::string& val) {
	word++;
	count += stoi(val);

	string filename = get_filename(key);

	auto iter1 = buffer.find(filename);
	if (iter1 != buffer.end()) {
		auto iter2 = iter1->second.find(key);
		if (iter2 != iter1->second.end()) {
			int count = stoi(iter2->second);
			count += stoi(val);
			iter2->second = to_string(count);
		}
		else {
			iter1->second.insert({key, val});
			key_count++;
		}
	}
	else {
		unordered_map<string, string> tmp;
		tmp.insert({key, val});
		buffer.insert({filename, tmp});
		key_count++;
	}

	// if(key_count >= 100){
	// 	flush();
	// }
}

inline string BaseMapperInternal::get_filename(string key){
	return output_dir + to_string(hash<string>{}(key)%n_output_files) + ".txt";
}

inline vector<string> BaseMapperInternal::get_filenames() {
	vector<string> filenames;
	for (string filename : files) {
		filenames.push_back(filename);
	}

	return filenames;
}

inline void BaseMapperInternal::flush() {
	for (auto iter1 : buffer) {
		ofstream file;
		file.open(iter1.first, ofstream::out | ios_base::trunc);  // TODO: 
		while (!file.is_open()) {
			cerr << "mapper fail to open intermediate file:" << iter1.first << endl;
			file.open(iter1.first, ofstream::out | ios_base::trunc);
		}

		for (auto iter2 : iter1.second) {
			file << iter2.first << " " << iter2.second << "\n";
			file.flush();  // TODO:
		}
		file.close();
		files.insert(iter1.first);
	}

	buffer.clear();
	key_count = 0;
}

/*-----------------------------------------------------------------------------------------------*/

/* Reduce task */
struct BaseReducerInternal {

	/* DON'T change this function's signature */
	BaseReducerInternal();

	/* DON'T change this function's signature */
	void emit(const std::string& key, const std::string& val);
	void flush();

	string filename;
	ofstream fd;
	vector<string> buffer;
	int key_count = 0;

	int word = 0;
	int count = 0;
};

inline BaseReducerInternal::BaseReducerInternal() {
	count = 0;
}

inline void BaseReducerInternal::emit(const std::string& key, const std::string& val) {
	word++;
	count += stoi(val);

	string line = key + " " + val + "\n";
	buffer.push_back(line);
	key_count++;

	// if(key_count >= 100){
	// 	flush();
	// }
}

inline void BaseReducerInternal::flush() {
	ofstream file;
	file.open(filename, ofstream::out | ios_base::trunc);  // TODO: 
	while (!file.is_open()) {
		cerr << "reducer fail to open output file:" << filename << endl;
		file.open(filename, ofstream::out | ios_base::trunc);
	}

	for (string i : buffer) {
		file << i;
		file.flush();  // TODO:
	}
	file.close();

	buffer.clear();
	key_count = 0;
}
