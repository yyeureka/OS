# CPU Scheduler

This directory contains files for evaluating your CPU Scheduler. There are 3 test cases in total, each of which introduces a different workload.

## Prerequisites

Before starting these tests, ensure that you have created 8 virtual machines. The names of these VMs **MUST** start with *aos_* (e.g., aos_vm1 for the first VM, aos_vm2 for the second VM, etc.).

If you need to create VMs, you can do so with the command:

`uvt-kvm create aos_vm1 release=bionic --memory 256`

(where 'aos_vm1' is the name of the VM you wish to create)

Note that if you started with the Memory Coordinator portion of this assignment you should already have 4 VMs in place and as such will need to create 4 additional VMs using the above command. Otherwise if you are starting from this portion of the assignment you will be creating 8 VMs.

Ensure the VMs are shutdown before starting. You can do this with the script *~/project1/shutdownallvm.py*

Compile the test programs for the CPU Scheduler evaluations with the following commands:

```
$ cd ~/project1/cpu/test/
$ ./makeall.sh
```

## Running The Tests

For each test, you will need to follow the procedure outlined below:

(Assuming testcase 1 as an example)
1. Start all of the VMs using the *~/project1/startallvm.py* script
2. Copy the test binaries into each VM by running the *~/project1/cpu/test/assignall.sh* script
3. Open a new terminal (e.g., a separate terminal window or a tmux/screen session) and use the *script* command to capture terminal output by running command *script vcpu_scheduler1.log*
4. In the same terminal, start the provided monitoring tool by running the command *~/project1/cpu/test/monitor.py -t runtest1.py*. The *script* command from Step 3 will capture the monitoring tool output to *vcpu_scheduler1.log* so that you can submit it with your assignment. Use `python3 monitor.py --help` for available flags.
5. In a new terminal, start your CPU Scheduler by running the *vcpu_scheduler* binary
6. Use the output from the monitoring tool to determine if your CPU Scheduler is producing the correct behavior as described under *~/project1/cpu/test/README.md*
7. After the test has completed, type "exit" on the command line to stop *script* from logging the console output. A record of your monitoring will be in the file *vcpu_scheduler.log*, which you will submit with your assignment deliverables.
8. Copy this log file to *project1/cpu/src/* directory
9. Run the script *~/project1/cpu/test/plot_graph_cpu.py* to plot the trend of your test run. Usage: *python plot_graph_cpu.py -i <path to logfile generated at step 7> -o <path to a new file to save the graph>*
*(Note that the graphs generated through this is only for self review and this need not be submitted)*
10. Shut down your test VMs with the script *~/project1/shutdownallvm.py*
11. Repeat these steps for the remaining test cases, substituting the test case number as appropriate.

## Understanding Monitor Output

The *monitor.py* tool will output CPU utilization and mapping statistics in the following format:

```
Iteration Number 1:
0 - usage: 24.0 | mapping ['aos_vm3']
1 - usage: 23.0 | mapping ['aos_vm4']
2 - usage: 23.0 | mapping ['aos_vm8']
3 - usage: 23.0 | mapping ['aos_vm1']
4 - usage: 23.0 | mapping ['aos_vm7']
5 - usage: 24.0 | mapping ['aos_vm6']
6 - usage: 23.0 | mapping ['aos_vm5']
7 - usage: 24.0 | mapping ['aos_vm2']
```

The first line mentions the iteration number. Your vcpu_scheduler should be able to balance all the pCPUs in *25 iterations*. While running *~/project1/cpu/test/monitor.py -t runtest1.py*, the test case *runtest1.py* starts after 5 iterations. That is, first 5 iterations just show you the monitor output without running the test case. Then, the script executes the test (in this case *runtest1.py*). 

Where each line is of the form:

`<pCPU #> - usage: <pCPU % utilization> | mapping <VMs mapped to this pCPU>`

The first field indicates the pCPU number.

The second field indicates the % utilization of the given pCPU.

The third field indicates a given pCPU's mapping to different virtual machines (vCPUs).

## Important
Your algorithm should work independent of the number of vcpus and pcpus. The above scenario in the monitor output displays 8 vcpus balanced on 8 pcpus.

Configurations where no. of vcpus > no. of pcpus OR no. of vcpus < no. of pcpus should also be appropriately handled.

Note that you may not need to specifically handle these cases as a generic algorithm that looks to stabilize processor use would apply equally to all cases.

The expectation of the tescases provided operate under the assumption of 8 vcpus and 4 pcpus. But they can be extended to a different count of pcpus. For e.g for an 8 core system (8 pcpus), you should be able to evaulate your algorithm for a setup of 16 VMs (16 vcpus) with similar expectation.
