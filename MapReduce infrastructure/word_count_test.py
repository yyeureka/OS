#!/usr/bin/env python3

import argparse
import pickle
import re
import subprocess
from collections import Counter
from pathlib import Path


class bcolors:
    OK = '\033[92m'  #GREEN
    FAIL = '\033[91m'  #RED
    RESET = '\033[0m'  #RESET COLOR


def CreateSolution(outputFile, inputDir):
    linesList = []

    for p in Path(inputDir).glob('*.txt'):
        with open(p.absolute(), 'r') as f:
            linesList.append(f.readlines())

    counts = {}
    for lines in linesList:
        words = []
        for line in lines:
            words = words + re.split('\s|,|\"|\'',
                                     line.strip().replace('.', ''))

        count = {item: words.count(item) for item in words}
        counts = Counter(counts) + Counter(count)

    result = [(k, counts[k]) for k in sorted(counts)]
    with open(outputFile, 'wb') as f:
        pickle.dump(result, f)


def Test(solutionFile, inputDir, outputFile):
    texts = []

    for p in Path(inputDir).glob('*'):
        with open(p.absolute(), 'r') as f:
            texts.append(f.readlines())

    linesList = []
    for text in texts:
        linesList = linesList + text

    result = list(map(lambda x: tuple(x.split()), linesList))
    result = list(map(lambda x: (x[0], int(x[1])), result))
    result.sort(key=lambda x: x[0])
    with open(outputFile, 'wb') as f:
        pickle.dump(result, f)

    resultSha = subprocess.check_output(['sha1sum', outputFile])
    solutionSha = subprocess.check_output(['sha1sum', solutionFile])
    resultSha = resultSha.split()[0]
    solutionSha = solutionSha.split()[0]

    passed = resultSha == solutionSha
    if passed:
        print(f'Test {bcolors.OK}Passed{bcolors.RESET}')
        return True

    print(f'Test {bcolors.FAIL}Failed{bcolors.RESET}\n')
    correct = []
    with open(solutionFile, 'rb') as f:
        correct = pickle.load(f)

    if len(correct) != len(result):
        print(f'Result length {len(result)} != {len(correct)} (correct)')

    missing = [item for item in correct if item not in result]

    correctDict = dict((x, y) for x, y in correct)
    resultDict = dict((x, y) for x, y in result)
    for word, _ in missing:
        print(f'{word}: {resultDict.get(word)} vs {correctDict.get(word)}')
    
    return False


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Solves word count or test MR output')
    subparser = parser.add_subparsers()

    solve = subparser.add_parser('solve')
    solve.set_defaults(command="solve")
    solve.add_argument('solution_file',
                       type=str,
                       help="Output file for solution")
    solve.add_argument('mr_input_dir',
                       type=str,
                       help="Directory containing files with words to count")

    test = subparser.add_parser('test')
    test.set_defaults(command='test')
    test.add_argument('solution_file',
                      type=str,
                      help="Solution file to check against")
    test.add_argument('mr_output_dir',
                      type=str,
                      help="MR output directory with reducer output files")
    test.add_argument(
        '-r',
        type=str,
        default='result.out',
        help="Result file for solution comparison, default=result.out")

    args = parser.parse_args()
    if 'command' not in args:
        parser.print_help()
        exit(1)

    if args.command == 'solve':
        CreateSolution(args.solution_file, args.mr_input_dir)
    elif args.command == 'test':
       result = Test(args.solution_file, args.mr_output_dir, args.r)
       if not result:
           exit(1)
