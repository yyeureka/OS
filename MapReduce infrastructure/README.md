MapReduce Infrastructure

## Overview
Implement a simplified version of Map Reduce infrastructure.
MapReduce is a programming model and an associated implementation for processing and generating large data sets. Users specify a map function that processes a key/value pair to generate a set of intermediate key/value pairs, and a reduce function that merges all intermediate values associated with the same intermediate key.

## Environment Setup
Follow the steps [here](https://grpc.io/docs/languages/cpp/quickstart/) to setup gRPC.
Alternatively, try the included `docker-compose.yml` file.

## How to build this project
1. cd /aos/projects
2. cmake ..
3. make

## How to run the tests
Two binaries would be created under the `bin` directory: `mrdemo` and `mr_worker`.
Symbolic link to `project4/test/config.ini` is installed as `bin/config.ini`.
The input directory is under `project4/test/`, you can copy paste it from the `test` folder to the `bin` folder.

1. Start all the worker processes (e.g. for 2 workers): `./mr_worker localhost:50051 & ./mr_worker localhost:50052;`
2. Start main map reduce process: `./mrdemo config.ini`
3. Once the `./mrdemo` finishes, kill all the worker proccesses started.

## Implementation

### Loading specification
1. Load the user specifications from `config.ini`
2. Set the parameters: number of worker threads, worker addresses ip:port, input files addresses, output directory, number of output files, the size of each shard in Kb, user ID
3. Validate the parameters, try open each input files and create the output directory. If output directory already exist, delete it first.

### File sharding
Generate file shards given the split size.
I terminates each shard on line endings, so the new shard start on newlines.
In this case, the sizes may slightly overshoot the given size.

### Master thread
Schedule the map and reduce tasks to worker threads.
Keep track of the status of all workers and all intermediate or output files.
Whenever there's a task, the Master pick one of the idle worker and communicate with it via sync RPC.
Exception handling:
Worker failure: The master will label the worker as invalid and reschedule the unfinished task of that worker.
Slow worker: Set a time limit of 300s for each worker. If timeout, reschedule the unfinished tasks.

### Mapper and Reducer
Start reduce after all map tasks are done.
Say M shards and N outputs:
Each mapper handle 1 shard and return N intermediate files. When map function gets a <key, val> pair, it calulates an index i using hash(key) % N, and write this <key, val> into the i intermediate file.
Each reducer j handle j group of intermediate files and return 1 file.
Upon complete, each mapper or reducer returns the file addresses to the Master.

## Testcases
1. Run under bin, copy input folder from test to bin:
./mr_worker localhost:50051
./mrdemo config.ini
2. Run under test:
../bin/mr_worker localhost:50051
../bin/mrdemo ../bin/config.ini
3. Kill a mapper while it servicing
4. Kill a reducer while it servicing
5. Change parameters in config.ini:
Original config.ini (6 workers, 3 input files, 8 output files, 500KB).
Small files: 0.3kb, 0.7kb, 2.2kb
Large files: 4mb, 6mb as https://piazza.com/class/ksibwlavcyd5h4?cid=559_f2
Input files 1+2+3:
    In one long line
    Small shard size: 1kb
    Large shard size: 500 kb
    Large output files: 32
...
And all kinds of combinations.

I can pass local test using word_count_test.py: https://piazza.com/class/ksibwlavcyd5h4?cid=561. 
However, I tried everything I can come up with and made a lot of modifications, but still, I cannot pass a single hidden test on Gradescope.
Please see this post for more information:
https://piazza.com/class/ksibwlavcyd5h4?cid=654

