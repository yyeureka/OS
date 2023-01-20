#!/bin/bash

set -e

if [[ $# -ne 1 ]]; then
    echo "Usage: start_workers n_workers"
    exit 1
fi

N_WORKERS="$1"
BIN_DIR=../_build/bin

BASE_PORT="50050"

# make
cd test
for i in $(seq $N_WORKERS); do
    PORT=$(($BASE_PORT+$i))
    $BIN_DIR/mr_worker "localhost:$PORT" &
done