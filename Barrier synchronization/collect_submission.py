from pathlib import Path
import os
import subprocess

from os import listdir
from os.path import isfile, join

def checkSubmission(requiredFiles):
    allOk = True
    missingFiles = []
    for r in requiredFiles:
        if not Path(r).is_file():
            allOk = False
            missingFiles.append(r)
            
            print('WARNING: file {} not found! '.format(r))
    return allOk, missingFiles

def createZippedFile(dirName):
    subprocess.call(['rm', dirName+'.zip'])
    subprocess.call(['rm', '-rf', dirName])
    subprocess.call(['mkdir', '-p', dirName+'/omp'])
    subprocess.call(['mkdir', '-p', dirName+'/mpi'])
    subprocess.call(['mkdir', '-p', dirName+'/combined'])

    print('created directory', dirName)
    # subprocess.call(['cp', 'Readme.txt', dirName])
    print('copying omp/ to omp')
    onlyfiles = ['omp/'+ f for f in listdir('omp/') if isfile(join('omp/', f))]
    print(onlyfiles) 

    for f in onlyfiles:
        print('copying ' + f)
        subprocess.call(['cp',  f , dirName + '/omp/'])

    print('copying mpi/ to mpi')
    onlyfiles = ['mpi/'+ f for f in listdir('mpi/') if isfile(join('mpi/', f))]
    print(onlyfiles) 

    for f in onlyfiles:
        print('copying ' + f)
        subprocess.call(['cp',  f , dirName + '/mpi/'])

    print('copying combined/ to combined')
    onlyfiles = ['combined/'+ f for f in listdir('combined/') if isfile(join('combined/', f))]
    print(onlyfiles) 

    for f in onlyfiles:
        print('copying ' + f)
        subprocess.call(['cp',  f , dirName + '/combined/'])

    print('creating zip file')
    subprocess.call(['zip', '-r', dirName + '.zip', dirName])
    print('done')

if __name__ == '__main__':
    print('checking for required files..')
    omp_requiredFiles = ['omp/Makefile', 'omp/gtmp.h', 'omp/gtmp1.c', 'omp/gtmp2.c', 'omp/harness.c']
    mpi_requiredFiles = ['mpi/Makefile', 'mpi/gtmpi.h', 'mpi/gtmpi1.c', 'mpi/gtmpi2.c', 'mpi/harness.c']
    combined_requiredFiles = ['combined/Makefile']
    omp_allOk, missingFiles = checkSubmission(omp_requiredFiles)
    mpi_allOk, missingFiles = checkSubmission(mpi_requiredFiles)
    combined_allOk, missingFiles = checkSubmission(combined_requiredFiles)

    check = 'y'
    if not omp_allOk or not mpi_allOk or not combined_allOk:
        print("WARNING... SOME FILES ARE MISSING")

        check = input('Do you want to continue? (y/n) - ')
        
    if check == 'y':
        dirName = 'project2'
        createZippedFile(dirName)