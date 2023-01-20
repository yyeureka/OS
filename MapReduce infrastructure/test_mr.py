#!/usr/bin/env python3

import subprocess
import pytest
import word_count_test

TEST_CONFIG = [[1, 1], [1, 4], [1, 16], [4, 4], [4, 16], [16, 1], [16, 4],
               [16, 16]]


def pytest_generate_tests(metafunc):
    metafunc.parametrize("shardSizeKb", [2, 50, 100, 200, 500, 600])


@pytest.mark.parametrize("numWorkers, numOutputs", TEST_CONFIG)
def test_answer(numWorkers, numOutputs, shardSizeKb):
    workersStarted = False
    masterStarted = False
    try:
        print("start")
        subprocess.Popen(['./start-worker.sh', str(numWorkers)])
        print("done")
        workersStarted = True
    except subprocess.CalledProcessError:
        workersStarted = False

    try:
        subprocess.check_output([
            './start-master.sh',
            str(numWorkers),
            str(numOutputs),
            str(shardSizeKb)
        ])
        masterStarted = True
    except subprocess.CalledProcessError:
        masterStarted = False

    assert workersStarted
    assert masterStarted
    assert word_count_test.Test('solution.out', 'test/output', 'result.out')
