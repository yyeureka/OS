### external directory (Do not make any changes in this directory)
When you provide a package(you can relate with grpc) to the outside world, you expose mainly two things: `libraries and header files`.

1. **external/include** - This directory consists of header files containing function declarations, that the user will make use of to run MapReduce.
  - `mapreduce.h` - This contains the interface that enables the user to use a MapReduce job.
  - `mr_task_factory.h` - This contains the interfaces through which the user can register their own mapper and reducer algorithm with MapReduce.

### test directory (Do not make any changes in this directory)
This gives you an insight of how a user will use your MapReduce package.

1. **`config.ini`** - Through this file the user gives input to your MapReduce framework as to how the user would like it to run. `cmake` creates a make rule to install a symbolic link to this file into the build/bin folder. Edit fields accordingly.

2. **`user_tasks.cc`** - This is where the user has implemented his/her own algorithm for mapper and reducer. Then he/she registers it with the mr_task_factory, so that it can be picked up by your framework.

3. **`main.cc`** - This is just the entry point in the user's binary to run the MapReduce job.

### src directory (Read the comments in each source file that indicates what you are supposed to do with it)
Now you own the development of MapReduce and this is where you implement the ideas.
We are describing here only the files which you are supposed to work on. For the other files, just have a quick glance through them. It should be straightforward to understand their purpose.

1. **`masterworker.proto`** - containing the grpc protocol between master and worker. **The choice of communication mode (synchronous/asynchronous) is up to you, as long as the parallel nature of MapReduce is maintained (the workers must be able perform their task in parallel). We will accept both methods.**

2. **`file_shard.h`** - This file contains the data structure for how you would maintain info about the file splits, that master passes to workers for mapper task. Also it contains functions, that framework will call to generate file shards given the split size.

3. **`mapreduce_spec.h`** - This structure will be populated by the framework. There is a helper function which takes the config file name and populates this structure. Framework will also validate the structure(valid input file paths, apt amount of ip_addr_ports etc.). This structure is finally what is passed to the master, so think what all you would need in this.

4. **`master.h`** - This is where you have to think a lot about, what all master does in map reduce. And then construct the required data structure and book keeping. How it will communicate using grpc with the workers etc.

5. **`mr_tasks.h`** - This is where you implement the crux of how you handle a map/reduce task. It also includes emit function's implementation, which user's map/reduce function will use to write intermediate key/value papers.

6. **`worker.h`** - This is where you code how the workers will run map and reduce tasks on receiving a request from master, meanwhile simply waiting for a task the other times.

## Setup Instructions.
  - Follow the steps [here](https://grpc.io/docs/languages/cpp/quickstart/) to setup gRPC.
  - You might have different version of global and local cmake as described in the link above. To resolve that follow the following steps:
    - run `which cmake` this should give you `/usr/bin/cmake`.
    - run `sudo rm /usr/bin/cmake` to remove the global cmake.
    - Now create a symlink between your local and global cmake which you just installed in `.local` directory by `ln -s $MY_INSTALL_DIR/bin/cmake /usr/bin/cmake`. Checkout this [link](https://stackoverflow.com/questions/1951742/how-can-i-symlink-a-file-in-linux) for more information.
    - Now if you run cmake --version the global version should match the local version.
## Build Instructions
  - Make sure you're using cmake 3.10+.  
  - Go to project4 directory.
  - Run `cmake` as follows:   `cmake .`
  - Run `make`
  - Copy paste the `input` directory from the `test` folder to your `bin` folder.
  - Two binaries would be created under the `bin` directory: `mrdemo` and `mr_worker`.
  - Symbolic link to `project4/test/config.ini` is installed as `bin/config.ini`

## Run Instructions
  - Clear the files if any in the `output`/, `temp/` and/or `intermediate/` directory.
  - Go to `bin` directory.
  - Start all the worker processes (e.g. for 2 workers): `./mr_worker localhost:50051 & ./mr_worker localhost:50052;`
  - Then start your main map reduce process: `./mrdemo config.ini`
  - Once the `./mrdemo` finishes, kill all the worker proccesses you started.
  - Check output directory to see if you have the correct results(obviously once you have done the proper implementation of your library).
