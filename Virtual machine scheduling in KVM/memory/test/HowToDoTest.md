# Memory Coordinator

This directory contains files for evaluating your Memory Coordinator. There are 3 test cases in total, each of which introduces a different workload.

## Prerequisites

Before starting these tests, ensure that you have created at least 4 virtual machines. The names of these VMs **MUST** start with *aos_* (e.g., aos_vm1 for the first VM, aos_vm2 for the second VM, etc.). Note that if you have already created VMs as part of the CPU Scheduler tests, you can re-use 4 those VMs here but will need to delete the other 4. Otherwise, if you are starting with this portion of the assignment you will be creating 4 VMs here and another 4 VMs when you begin the CPU Scheduler portion.

If you need to delete VMs, you can do so with the command:

`uvt-kvm destroy aos_vm1`

(where 'aos_vm1' is the name of the VM you wish to destroy)

If you need to create VMs, you can do so with the command:

`uvt-kvm create aos_vm1 release=bionic --memory 256`

(where 'aos_vm1' is the name of the VM you wish to create)

Ensure the VMs are shutdown before starting. You can do this with the script *~/project1/shutdownallvm.py*

Compile the test programs for the Memory Coordinator evaluations with the following commands:

```
$ cd ~/project1/memory/test/
$ ./makeall.sh
```
 
## Running The Tests

For each test, you will need to follow the procedure outlined below:

(Assuming testcase 1 as an example)
1. Set the maximum allowed memory for VMs by running the script *~/project1/setallmaxmemory.py*
2. Start all of the VMs using the *~/project1/startallvm.py* script
3. Copy the test binaries into each VM by running the *~/project1/memory/test/assignall.sh* script
4. Open a new terminal (e.g., a separate terminal window or a tmux/screen session) and use the *script* command to capture terminal output by running command *script memory_coordinator1.log*
5. In the same terminal, start the provided monitoring tool by running the command *~/project1/memory/test/monitor.py -t runtest1.py*. The *script* command from Step 3 will capture the monitoring tool output to *memory_coordinator1.log* so that you can submit it with your assignment.
6. In a new terminal, start your Memory Coordinator by running the *memory_coordinator* binary
7. Use the output from the monitoring tool to determine if your Memory Coordinator is producing the correct behavior as described under *~/project1/memory/test/README.md*
8. After your test has completed, type "exit" on the command line to stop *script* from logging the console output. A record of your monitoring will be in the file *memory_coordinator1.log*, which you will submit with your assignment deliverables.
9. Copy this log file to *project1/memory/src/* directory
10. Run the script *~/project1/memory/test/plot_graph_memory.py* to plot the trend of your test run. Usage: *python plot_graph_memory.py -i <path to logfile generated at step 8> -o <path to a new file to save the graph>*
*(Note that the graphs generated through this is only for self review and this need not be submitted)*
11. Shut down your test VMs with the command *~/project1/shutdownallvm.py*
11. Repeat these steps for the remaining test cases, substituting the test case number as appropriate.

## Understanding Monitor Output

The *monitor.py* tool will output memory utilization statistics in the following format:

```
Iteration Number : 1
Memory (VM: aos_vm6)  Actual [256.0], Unused: [80.35546875]
Memory (VM: aos_vm7)  Actual [256.0], Unused: [80.24609375]
Memory (VM: aos_vm1)  Actual [256.0], Unused: [83.35546875]
Memory (VM: aos_vm8)  Actual [256.0], Unused: [82.51171875]
```

The first line mentions the iteration number. You should be able to see the expected behaviour in the test case within *50 iterations*. While running *~/project1/memory/test/monitor.py -t runtest1.py*, the test case *runtest1.py* starts after 5 iterations. That is, first 5 iterations just show you the monitor output without running the test case. Then, the script executes the test (in this case *runtest1.py*). 

Where "VM" is the VM for which statistics are printed, "Actual" is VM's total memory allocation in MB, and "Unused" is VM's current unused memory in MB

