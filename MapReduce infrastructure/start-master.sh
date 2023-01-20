#!/bin/bash

if [[ $# -ne 3 ]]; then
    echo "Usage: run_master n_workers num_reducers shard_size_kb"
    exit 1
fi
set -e

N_WORKERS="$1"
N_REDUCERS="$2"
SHARD_SIZE="$3"
BIN_DIR=../_build/bin

# make

# Run from test/ as this is where input files live
cd test

START_PORT="50052"
END_PORT=$(($START_PORT + $N_WORKERS - 2))

IPADDR_STR="localhost:50051"

for PORT in $(seq $START_PORT $END_PORT); do
    IPADDR_STR="$IPADDR_STR,localhost:$PORT"
done

CONFIGFILE="config.ini"

echo "n_workers=$N_WORKERS" > "$CONFIGFILE"
echo "worker_ipaddr_ports=$IPADDR_STR" >> "$CONFIGFILE"
echo "input_files=input/testdata_1.txt,input/testdata_2.txt,input/testdata_3.txt" >> "$CONFIGFILE"
echo "output_dir=output" >> "$CONFIGFILE"
echo "n_output_files=$N_REDUCERS" >> "$CONFIGFILE"
echo "map_kilobytes=$SHARD_SIZE" >> "$CONFIGFILE"
echo "user_id=cs6210" >> "$CONFIGFILE"

echo "starting master"
$BIN_DIR/mrdemo