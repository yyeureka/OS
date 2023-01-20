## CPU Scheduler Testing  
While designing your CPU scheduler, there are a few things to consider:  
1. What are the kinds of workloads/situations that the scheduler needs to handle?  
2. What is the average case and what are the edge cases?
3. What statistics would help in making a scheduling decision?  

### What is a stable schedule? 

No PCPU should be under or over utilized.The standard deviation provides a reasonable metric to measure differences in observations from the mean. We expect the standard deviation to be within 5% of the mean utilization for a balanced and stable schedule. 
 
The test cases provided in this directory are by no means exhaustive, but are meant to be a sanity check for your scheduling algorithm. You are encouraged to think about other test cases while designing the scheduler.   
### Test Cases  
We provide three test cases as a starting point. Each test is described below, along with the rationale behind the tests. The point we're trying to illustrate is the expected behavior, so we'll not focus on the actual percentages for usage. The setup for each test is the same- there are 8 VMs running on 4 CPU cores. More detail on setting up and running the tests is covered in [this](HowToDoTest.md) document.   
#### Test Case 1  
**Scenario**: There are 8 VMs with 1 CPU each, running similar workloads. The vCPUs are pinned such that there are 2 vPCPUs pinned to each pCPU.   
**Expected scheduler behavior**: Since pinning is already balanced, your scheduler should not be doing any additional work.  
**Sample output**:
```
Iteration 1:
0 - usage: 100.0 | mapping ['aos_vm3', 'aos_vm2']
1 - usage: 99.0 | mapping ['aos_vm4', 'aos_vm5']
2 - usage: 99.0 | mapping ['aos_vm8', 'aos_vm7']
3 - usage: 100.0 | mapping ['aos_vm1', 'aos_vm6']
Iteration 2:
0 - usage: 100.0 | mapping ['aos_vm3', 'aos_vm2']
1 - usage: 99.0 | mapping ['aos_vm4', 'aos_vm5']
2 - usage: 99.0 | mapping ['aos_vm8', 'aos_vm7']
3 - usage: 99.0 | mapping ['aos_vm1', 'aos_vm6']
Iteration 3:
0 - usage: 100.0 | mapping ['aos_vm3', 'aos_vm2']
1 - usage: 100.0 | mapping ['aos_vm4', 'aos_vm5']
2 - usage: 100.0 | mapping ['aos_vm8', 'aos_vm7']
3 - usage: 100.0 | mapping ['aos_vm1', 'aos_vm6']
```
Note that the pinnings do not change across iterations. In summary, the scheduler does not take any action.  

**What are we testing here?**  
This is the base case. The scheduler should be able to determine that the CPU utilization is balanced.  

**Sample trend** is [here](vcpu_scheduler1_sample.png)

*(The blue line indicates the percentage of pcpu utilization at each of the 25 iterations. The red dots represent pin changes at the corresponding iteration)*


#### Test Case 2  
**Scenario**: There are 8 VMs with 1 CPU each, running similar workloads. All the vCPUs are pinned to pCPU0.   
**Expected scheduler behavior**: Pins should be changed such that there are 2 vCPUs pinned to each available pCPU.  
**Sample output**:
```
Iteration 1:
0 - usage: 100.0 | mapping ['aos_vm3', 'aos_vm2', 'aos_vm1', 'aos_vm4', 'aos_vm6', 'aos_vm5', 'aos_vm7', 'aos_vm8']
1 - usage: 0.0 | mapping []
2 - usage: 0.0 | mapping []
3 - usage: 0.0 | mapping []
Iteration 2:
0 - usage: 150.0 | mapping ['aos_vm3', 'aos_vm2', 'aos_vm7']
1 - usage: 99.0 | mapping ['aos_vm4', 'aos_vm5']
2 - usage: 50.0 | mapping ['aos_vm8']
3 - usage: 99.0 | mapping ['aos_vm1', 'aos_vm6']
Iteration 3:
0 - usage: 100.0 | mapping ['aos_vm3', 'aos_vm2']
1 - usage: 100.0 | mapping ['aos_vm4', 'aos_vm5']
2 - usage: 100.0 | mapping ['aos_vm8', 'aos_vm7']
3 - usage: 100.0 | mapping ['aos_vm1', 'aos_vm6']
Iteration 4:
0 - usage: 100.0 | mapping ['aos_vm3', 'aos_vm2']
1 - usage: 100.0 | mapping ['aos_vm4', 'aos_vm5']
2 - usage: 100.0 | mapping ['aos_vm8', 'aos_vm7']
3 - usage: 100.0 | mapping ['aos_vm1', 'aos_vm6']
```
The scheduler may take a few iterations to arrive at a stable pinning. Once the workloads on each pCPU is balanced, the scheduler should not make unnecessary changes to the pins.  
 
**What are we testing here?**  
This is the second part of the base case. The scheduler should be able to just change pinnings to balance the utilization.    

**Sample trend** is [here](vcpu_scheduler2_sample.png)

*(The blue line indicates the percentage of pcpu utilization at each of the 25 iterations. The red dots represent pin changes at the corresponding iteration)*


#### Test Case 3  
**Scenario**: There are 8 VMs with 1 CPU each but 4 VMs run a heavy workload and 4 VMs run a light load. The vCPUs are equally likely to run on any pCPU. In other words, the vCPUs are randomly pinned initially.   
**Expected scheduler behaviour**: The scheduler should use some statistic to decide the optimal pinning which will balance the workload across the pCPUs.

**Sample output**:
Let's assume that VMs 1,2,3 and 4 run heavy loads, and VMs 5,6,7 and 8 run light loads. We will use 50 to represent a light load and 100 to represent a heavy load. 
```
Iteration 1:
0 - usage: 150.0 | mapping ['aos_vm3', 'aos_vm2', 'aos_vm1']
1 - usage: 100.0 | mapping ['aos_vm5']
2 - usage: 100.0 | mapping ['aos_vm7']
3 - usage: 250.0 | mapping ['aos_vm6', 'aos_vm4', 'aos_vm8']
Iteration 2:
0 - usage: 150.0 | mapping ['aos_vm3', 'aos_vm2', 'aos_vm1']
1 - usage: 200.0 | mapping ['aos_vm8', 'aos_vm5']
2 - usage: 100.0 | mapping ['aos_vm7']
3 - usage: 150.0 | mapping ['aos_vm6', 'aos_vm4']
Iteration 3:
0 - usage: 150.0 | mapping ['aos_vm3', 'aos_vm8']
1 - usage: 150.0 | mapping ['aos_vm4', 'aos_vm5']
2 - usage: 150.0 | mapping ['aos_vm7', 'aos_vm2']
3 - usage: 150.0 | mapping ['aos_vm6', 'aos_vm4']
Iteration 4:
0 - usage: 100.0 | mapping ['aos_vm3', 'aos_vm8']
1 - usage: 100.0 | mapping ['aos_vm4', 'aos_vm5']
2 - usage: 100.0 | mapping ['aos_vm7', 'aos_vm2']
3 - usage: 100.0 | mapping ['aos_vm6', 'aos_vm4']
```
The VMs are randomly pinned initially. The scheduler then pins one VM with heavy load and one VM with load load to each pCPU.  
 
**What are we testing here?**  
This is a rudimentary test for the scheduler which builds on the two previous tests. The scheduler needs to balance the utilization and change the pinnings to do so.      

**Sample trend** is [here](vcpu_scheduler3_sample.png)

*(The blue line indicates the percentage of pcpu utilization at each of the 25 iterations. The red dots represent pin changes at the corresponding iteration)*

**The given sample graphs represent the expected trend generated from the log files**
