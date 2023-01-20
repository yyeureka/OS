## Memory coordinator Testing  
While designing your memory coordinator, there are a few things to consider:  
1. What are the kinds of memory consumption the coordinator needs to handle?  
2. What is the average case and what are the edge cases?
3. What statistics would help in making a decision to give/take memory?
4. When should you stop reclaiming memory?  
  
The test cases provided in this directory are by no means exhaustive, but are meant to be a sanity check for your memory coordinator. You are encouraged to think about other test cases while designing the coordinator.   
### Test Cases  
We provide three test cases as a starting point. Each test is described below, along with the rationale behind the tests. The setup for each test is the same- there are 4 VMs. All VMs start with 512 MB memory. We assume that the memory for each VM cannot fall below 200 MB. The max memory on each VM can grow up to 2048 MB. More detail on setting up and running the tests is covered in [this](HowToDoTest.md) document.   
#### Test Case 1  
**Scenario**: There are 4 VMs, each with 512 MB. Only VM1 is running a program that consumes memory. Other VMs are idling.   
**Expected coordinator behavior**: VM1 keeps consuming memory until it hits the max limit. The memory is being supplied from the other VMs. Once VM1 hits the max limit, it starts freeing memory.  
**Sample output**:
```
Iteration 1:
Memory (VM: aos_vm1)  Actual [512.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [512.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [512.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [512.0], Unused: [82.51171875]
Iteration 2:
Memory (VM: aos_vm1)  Actual [556.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [452.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [512.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [512.0], Unused: [82.51171875]
...
Iteration 34:
Memory (VM: aos_vm1)  Actual [2008.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [200.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [200.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [200.0], Unused: [82.51171875]
Iteration 35:
Memory (VM: aos_vm1)  Actual [2048.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [200.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [200.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [200.0], Unused: [82.51171875]
...
Iteration 40:
Memory (VM: aos_vm1)  Actual [1056.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [200.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [200.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [200.0], Unused: [82.51171875]
...
Iteration 45:
Memory (VM: aos_vm1)  Actual [512.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [200.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [200.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [200.0], Unused: [82.51171875]
...
```
- The first VM gains memory, which can be eithr supplied by other VMs(e.g iteration 2) or from the host (at iteration 34).   
- Once it reaches the max limit (iteration 35), the program that is consuming memory on the VM stops running, and so the coordinator begins reclaiming memory (iteration 40, 45 for example).  

**What are we testing here?**  
This is the base case. The coordinator should be able to allocate/reclaim memory from a single VM.  

**Sample trend** is [here](memory_coordinator1_sample.png)

*(The blue line and red line indicate the actual memory and the unused memory of the VM used at each iteration.)*


#### Test Case 2  
**Scenario**: There are 4 VMs with 512 MB each. All VMs begin consuming memory. They all have similar balloon sizes.  
**Expected coordinator behavior**: The coordinator decides if it can afford to supply more memory to the VMs, or if it should stop doing so. Once the VMs can no longer sustain the memory demands of the program running in them, the program stops running. Now the coordinator should begin reclaiming memory.   
**Sample output**:
```
Iteration 1:
Memory (VM: aos_vm1)  Actual [512.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [512.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [512.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [512.0], Unused: [82.51171875]
Iteration 2:
Memory (VM: aos_vm1)  Actual [556.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [550.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [556.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [556.0], Unused: [82.51171875]
...
Iteration 34:
Memory (VM: aos_vm1)  Actual [1508.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [1500.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [1500.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [1500.0], Unused: [82.51171875]
...
Iteration 40:
Memory (VM: aos_vm1)  Actual [1056.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [1056.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [1050.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [1050.0], Unused: [82.51171875]
...
Iteration 45:
Memory (VM: aos_vm1)  Actual [512.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [512.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [512.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [512.0], Unused: [82.51171875]
...
```
- All VMs start consuming memory, supplied by the host.   
- Their consumption is about the same. Once the coordinator determines that the host can no longer afford to give more memory (iteration 34), it stops supplying more memory.  
- The program on the VMs stops running so the coordinator then starts reclaiming memory from the VMs (iterations 40, 45).  

**What are we testing here?**  
When all VMs are consuming memory, we are forcing the coordinator to allocate memory from the host instead.   

**Sample trend** is [here](memory_coordinator2_sample.png)

*(The blue line and red line indicate the actual memory and the unused memory of the VM used at each iteration.)*

#### Test Case 3  
**Scenario**: There are 4 VMs with 512 MB each. VM1 and VM2 initially start consuming memory. After some time, VM1 stops consuming memory but VM2 continues consuming memory. The other 2 VMs are inactive.   
**Expected coordinator behaviour**: The coordinator needs to decide how to supply memory to the VMs with growing demand.  

**Sample output**:  
```
Iteration 1:
Memory (VM: aos_vm1)  Actual [512.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [512.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [512.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [512.0], Unused: [82.51171875]
Iteration 2:
Memory (VM: aos_vm1)  Actual [556.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [550.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [456.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [456.0], Unused: [82.51171875]
...
Iteration 25:
Memory (VM: aos_vm1)  Actual [1508.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [1500.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [200.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [200.0], Unused: [82.51171875]
...
Iteration 35:
Memory (VM: aos_vm1)  Actual [1056.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [1848.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [200.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [200.0], Unused: [82.51171875]
...
Iteration 40:
Memory (VM: aos_vm1)  Actual [800.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [2048.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [200.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [200.0], Unused: [82.51171875]
...
Iteration 45:
Memory (VM: aos_vm1)  Actual [600.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [1548.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [200.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [200.0], Unused: [82.51171875]
...
Iteration 50:
Memory (VM: aos_vm1)  Actual [200.0], Unused: [80.35546875]
Memory (VM: aos_vm2)  Actual [1048.0], Unused: [80.24609375]
Memory (VM: aos_vm3)  Actual [200.0], Unused: [83.35546875]
Memory (VM: aos_vm4)  Actual [200.0], Unused: [82.51171875]
...
```
- VM1 and VM2 are consuming memory initially (iteration2, iteration 25).  
- The coordinator supplies them more memory either by taking memory away from VM3 and VM4, or from the host.   
- Then VM1 stops consuming memory, but VM2's demand is still growing (iteration 35).  
- The coordinator can now redirect the memory from VM1 to VM2.  
- Then VM2 reaches its max limit (iteration 40).   
- Now the coordinator starts reclaiming memory from VM2 as well (iteration 45, 50).  
 
**What are we testing here?**  
We have a scenario where the memory demands are met by a combination of host memory and using the memory from idling VMs.

**Sample trend** is [here](memory_coordinator3_sample.png)

*(The blue line and red line indicate the actual memory and the unused memory of the VM used at each iteration.)*

**The given sample graphs represent the expected trend generated from the log files**
