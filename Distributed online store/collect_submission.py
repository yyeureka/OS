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
            print('ERROR:required file {} not found! '.format(r) )
    return allOk, missingFiles

def createZippedFile(dirName):
    subprocess.call(['rm', dirName + '.zip'])
    os.system('zip -r ' + dirName + '.zip ' + 'Readme.txt ' + 'src/*.cc ' + 'src/*.h ' + 'src/CMakeLists.txt')
    print('done')

if __name__ == '__main__':
    print('checking for required files..')
    requiredFiles = ['Readme.txt', 'src/CMakeLists.txt', 'src/store.cc', 'src/threadpool.h']
    allOk, missingFiles = checkSubmission(requiredFiles)
    if not allOk:
        print('Aborting. Please make sure all required files are present')
    else:
        firstname = input('Enter first name: ')
        lastname = input('Enter last name: ')
        dirName = firstname + '_' + lastname + '_p3'
        createZippedFile(dirName)
