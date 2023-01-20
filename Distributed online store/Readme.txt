Distributed services using gRPC

Overview:
Implemente a multi-threaded online store in a distributed service, which receives requests from multiple clients, querying the prices offered by multiple registered vendors.

Environment setup:
Follow this [https://grpc.io/docs/languages/cpp/quickstart/] for a cmake based setup
Alternatively, try the included `docker-compose.yml` file.

How to build this project:
1. cd /aos/projects
2. mkdir build && cd build
3. cmake ..
4. make all

How to run the test setup:
1. Run the command `./run_vendors vendor_addresses.txt &` to start a process which will run multiple servers on different threads listening to (ip_address:ports) from the file given as command line argument.
2. Run the command `./store vendor_addresses.txt $IP_and_port_on_which_store_is_listening $max_num_threads` to start a process which will serve multiple clients on different threads listening to (ip_address:ports) given as command line argument. This process reads the same address file to know vendors' listening addresses.
3. Run the command `./run_tests $IP_and_port_on_which_store_is_listening $max_num_concurrent_client_requests` to start a process which will simulate real world clients sending requests at the same time. This process reads the queries from the file `product_query_list.txt`

Implementation:
1. Get the <ip address:port> of vendor servers from the provided file given as command line argument.
2. Register asynchronous gRPC service and start listening on the <ip address:port> given as command line argument.
3. Create the number of threads given as command line argument. Each thread:
   a. Block waiting to read the next event from the completion queue.
   b. Upon receiving a client request:
      i. Make asynchronous gRPC calls to all vendors on the list for the queried product.
      ii. Await for all bid replies to come back.
      iii. Collate the results and reply it back to the requesting client.
   c. Go to step a.