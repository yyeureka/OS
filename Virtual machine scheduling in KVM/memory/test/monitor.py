#!/usr/bin/env python3

from __future__ import print_function
import argparse
import os, sys
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from vm import VMManager
from testLibrary import TestLib
import sched, time
import subprocess

VM_PREFIX="aos"
def print_line(x, iterations):
    print("")
    for i in range(x):
        print('-', end = "")
    print("")
    print("Iteration number : ", iterations)
    
def run(sc,vmobjlist,machineParseable, iterations, test):
    
    if iterations == 5:
        os.system('python3 {0}'.format(test))
    iterations += 1
    i = 0
    print_line(50, iterations)
    for vm in vmobjlist:
        stats = vm.memoryStats()
        if machineParseable:
            print("memory,{},{},{}"
                    .format(vm.name(), 
                        stats['actual'] / 1024.0,
                        stats['unused'] / 1024.0))
        else:
            print("Memory (VM: {})  Actual [{}], Unused: [{}]"
                    .format(vm.name(), 
                        stats['actual'] / 1024.0,
                        stats['unused'] / 1024.0))

        i+=1

    if iterations == 50:
        return
    sc.enter(2, 1, run, (sc,vmobjlist,machineParseable,iterations, test))

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument("-m","--machine",action="store_true",help="outputs a machine parseable format")
    parser.add_argument("-t","--test",type=str,help="test case file")
    args = parser.parse_args()
    machineParseable = args.machine
    test = args.test

    
    s = sched.scheduler(time.time, time.sleep)
    manager = VMManager()
    vmlist = manager.getRunningVMNames(VM_PREFIX)
    vmobjlist = [manager.getVmObject(name) for name in vmlist]   
    
    for vm in vmobjlist:
        vm.setMemoryStatsPeriod(1)   
    
    iterations = 0
    s.enter(2, 1, run, (s,vmobjlist,machineParseable,iterations, test))
    s.run()

    os.system('python3 killall.py')
