## Setting Up Your Environment
**Note: The course VM is allocated 6 GB of RAM and 4 physical cores by default. These are the recommended specifications for this project.** 

Assuming you don't have it installed already, download [the latest version of Vagrant for your platform](https://www.vagrantup.com/downloads). You'll also need the latest version of VirtualBox, which can be found [here](https://www.virtualbox.org/wiki/Downloads). The latest version of VirtualBox should allow you to run nested virtual machines.

In order to run the project VM, you'll need to make sure that your processor supports virtualization, i.e. Intel VT-x or AMD-V. You can easily check this on a Linux host using `egrep "svm|vmx" /proc/cpuinfo`.

Vagrant allows you to start a virtual machine from any directory that contains a Vagrantfile. You can start the virtual machine using `vagrant up` and connect to it using `vagrant ssh`. Please note that Vagrant mounts the directory containing the Vagrantfile at `/vagrant`. Once you log into the VM, you can access the project files by changing your current directory via `cd /vagrant`. For more information, use `vagrant --help`.

The provided Vagrantfile does not install a GUI by default. If you'd like to install a GUI, set `vb.gui = true` on line 40 of the Vagrantfile and install the desktop environment of your choosing (i.e. using `sudo apt update && sudo apt install ubuntu-desktop-minimal`). Restart the VM to log into the GUI.

You can also use something like VS Code to edit your code locally but compile and run it on the VM. Google around to find some online guides for your IDE of choice.

## Creating Test VMs for Project 1

The most convenient way to create test VMs for this project is via Ubuntu's `uvtool` utility. This tool provides pre-built Ubuntu VM images for use within a libvirt/KVM environment. The provided Vagrantfile has already installed all the necessary packages to create, run, and connect to your virtual machines.

We will be running Ubuntu 18.04 LTS "Bionic Beaver'" VMs for testing purposes. The Vagrantfile has already synchronized the 18.04 Ubuntu image to your environment.

My steps:
Create VMs
1. Run createallvm.sh
2. Run guestSSHsetup.sh
Delete VMs: Run deleteallvm.sh
Start VMs: Run startallvm.sh
Shutdown VMs: Run shudownallvm.sh

 1. Create a new virtual machine, where *aos_vm1* is the name of the VM you want to create: 
    ``` shell
    uvt-kvm create aos_vm1 release=bionic --memory=256
    ```
    **Note: Your VM names must start with *aos_* for testing purposes!**
 
 2. Wait for the virtual machine to boot up and start the SSH daemon:
    ``` shell
    uvt-kvm wait aos_vm1
    ```

 3. (Optional) Connect to the running VM:
    ``` shell
    uvt-kvm ssh aos_vm1 --insecure
    ```
## Other Useful Commands

 1. Delete a VM, where *aos_vm1* is the name of the VM you want to delete: 

    ``` shell
    uvt-kvm destroy aos_vm1
    ```
 
 2. List all your VMs: 
 
    ``` shell
    virsh list --all
    ```
    
 3. Track resource usage across VMs: 
 
    ``` shell
    virt-top
    ```
 ... And plenty more. These are just a few commands to get you started.
 

## References:
[https://help.ubuntu.com/lts/serverguide/virtualization.html](https://help.ubuntu.com/lts/serverguide/virtualization.html)
