from pathlib import Path
import os
import zipfile

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

def createZippedFile(dirName, requiredFiles):

    zname = dirName + ".zip"

    with zipfile.ZipFile( zname, "w" ) as z:

        z.write( "README.md" )
        print('copied README.md')

        onlyfiles = ['src/'+ f for f in listdir('src') ]

        for f in onlyfiles:
            if f.endswith('.h') or f.endswith('.cc') or f in requiredFiles:
                z.write(f)
                print('copied ' + f)

    print( "wrote " + zname )

if __name__ == '__main__':
    print('checking for required files..')
    requiredFiles = [
                    'README.md',\
                    'src/CMakeLists.txt',\
                    'src/GenerateProtos.cmake',\
                    'src/masterworker.proto',\
                    'src/master.h',\
                    'src/worker.h',\
                    'src/mr_tasks.h',\
                    'src/file_shard.h',\
                    'src/mapreduce_spec.h',\
                    'src/mr_task_factory.cc',\
                    'src/run_worker.cc',\
                    'src/mapreduce_impl.h',\
                    'src/mapreduce_impl.cc',\
                    'src/mapreduce.cc']
    allOk, missingFiles = checkSubmission(requiredFiles)
    if not allOk:
        print('Aborting. Please make sure all required files are present')
        print('The following files were not found:', missingFiles)
    else:
        firstname = input('Enter first name: ')
        lastname = input('Enter last name: ')
       
        dirName = firstname + '_' + lastname + '_p4'
        createZippedFile(dirName, requiredFiles)
