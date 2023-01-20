from pathlib import Path
import os
import subprocess

from os import listdir
from os.path import isfile, join

def checkSubmission(requiredFiles, required=True):
    allOk = True
    missingFiles = []
    for r in requiredFiles:
        if not Path(r).is_file():
            allOk = False
            missingFiles.append(r)
            if required:
                print('ERROR:required file {} not found! '.format(r) )
            else:
                print('WARNING: file {} not found! '.format(r))
    return allOk, missingFiles

def createZippedFile(dirName):
    subprocess.call(['rm', '-rf', dirName])
    subprocess.call(['mkdir', '-p', dirName+'/cpu'])
    subprocess.call(['mkdir', '-p', dirName+'/memory'])
    print('created directory', dirName)
    # subprocess.call(['cp', 'Readme.txt', dirName])
    print('copying cpu/src to cpu')
    onlyfiles = ['cpu/src/'+ f for f in listdir('cpu/src/') if isfile(join('cpu/src', f))]
    print(onlyfiles) 

    for f in onlyfiles:
        print('copying ' + f)
        subprocess.call(['cp',  f , dirName + '/cpu/'])

    print('copying memory/src to memory')
    onlyfiles = ['memory/src/'+ f for f in listdir('memory/src/') if isfile(join('memory/src', f))]
    print(onlyfiles) 

    for f in onlyfiles:
        print('copying ' + f)
        subprocess.call(['cp',  f , dirName + '/memory/'])

    print('creating zip file')
    subprocess.call(['zip', '-r', dirName + '.zip', dirName])
    print('done')

if __name__ == '__main__':
    print('checking for required files..')
    requiredFiles = ['cpu/src/vcpu_scheduler.c', 'cpu/src/Makefile', 'cpu/src/Readme.md', 'memory/src/memory_coordinator.c', 'memory/src/Makefile', 'memory/src/Readme.md']
    allOk, missingFiles = checkSubmission(requiredFiles)

    logFiles = ['/cpu/src/vcpu_scheduler1.log',
                '/cpu/src/vcpu_scheduler2.log',
                '/cpu/src/vcpu_scheduler3.log',
                '/memory/src/memory_coordinator1.log',
                '/memory/src/memory_coordinator2.log',
                '/memory/src/memory_coordinator3.log']
    logAllOk, missingFiles = checkSubmission(logFiles, required=False)
    if not logAllOk:
        print("Warning. Some log files are mising")
    else:
        print("All log files present")
    if not allOk:
        print('Aborting. Please make sure all required files are present')
    else:
        print('All required files present')
        firstname = input('Enter first name: ')
        lastname = input('Enter last name: ')
        dirName = firstname + '_' + lastname + '_p1'
        createZippedFile(dirName)