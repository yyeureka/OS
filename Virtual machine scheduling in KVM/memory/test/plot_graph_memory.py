import argparse
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import re

MAX_VMS = 8
VM_PREFIX = "aos_vm"

def get_file_content(file_path):
    ls = list()
    with open(file_path) as fh:
        ls = fh.readlines()
    
    for index, line in enumerate(ls):
        ls[index] = line.strip()
    return ls
 
def parse_content(data_list):
    
    actual_memory = {}
    unused_memory = {}

    dashed_line = "--------------------------------------------------"
    index = 0
    while index < len(data_list):
        if dashed_line in data_list[index]:
            index += 2
            while index < len(data_list) and "Memory" in data_list[index]:
                '''Memory (VM: aos_vm1)  Actual [757.2890625], Unused: [161.76171875]'''
                data = data_list[index].split(" ")
                vm_name = re.match("{}\d+".format(VM_PREFIX), data[2])
                data5 = data[5].split(',')

                if vm_name.group() not in actual_memory:
                    actual_memory[vm_name.group()] = []
                    unused_memory[vm_name.group()] = []

                actual_memory[vm_name.group()].append(eval(data5[0])[0])
                unused_memory[vm_name.group()].append(eval(data[7])[0])
                index += 1
        else:
            index += 1

    return actual_memory, unused_memory


def create_dataframe(data, column_names=None):    
    df = pd.DataFrame(data)
    df = df.transpose()

    if column_names is not None:
        df.columns = column_names
    return df

def plotting_function(plotting_list_actual, plotting_list_unused,
                      plot_title_list, x_label, y_label, figure_name):
    figure, axes = plt.subplots(len(plotting_list_actual), figsize=(25, 18))

    for index, plot_list in enumerate(plotting_list_actual):
        x_list, y_list = plot_list
        axes[index].plot(x_list, y_list, label='actual_memory')

    for index, plot_list in enumerate(plotting_list_unused):
        x_list, y_list = plot_list
        axes[index].plot(x_list, y_list, color='mediumvioletred', label='unused_memory')

        axes[index].set_title(plot_title_list[index], fontweight='bold')
        axes[index].set_xlabel(x_label)
        axes[index].set_ylabel(y_label)
    figure.tight_layout(pad=2.0)
    plt.legend()
    figure.savefig(figure_name)


def workflow(file_path, figure_name):
    data_list = get_file_content(file_path)
    actual_memory, unused_memory = parse_content(data_list)

    global MAX_VMS
    MAX_VMS = len(actual_memory)

    column_names = []
    for i in range(MAX_VMS):
        column_names.append(i)
    
    df_actual = create_dataframe(list(actual_memory.values())[:MAX_VMS], column_names)
    df_unused = create_dataframe(list(unused_memory.values())[:MAX_VMS], column_names)

    x_label = "Iteration"
    y_label = "Memory (MB)"
    plot_title_list =  list(actual_memory.keys())

    plotting_list_actual = list()
    plotting_list_unused = list()
    for column_name in column_names:
        x_list = [index for index in range(df_actual.shape[0])]
        y_list = list(df_actual[column_name])
        y_list2 = list(df_unused[column_name])
        
        plotting_list_actual.append([x_list, y_list])
        plotting_list_unused.append([x_list, y_list2])
    
    plotting_function(plotting_list_actual, plotting_list_unused,
                      plot_title_list, x_label, y_label, figure_name)


def main(file_path, figure_name):
    workflow(file_path, figure_name)

    
if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--infile", help="path of the file to parse")
    parser.add_argument("-o","--outfile",help="filename to save the graph to")
    
    args = parser.parse_args()

    if args.infile == None or args.outfile == None:
        sys.exit("Usage: python plot_graph_memory.py -i <input log file> -o <path to save graph>")

    main(args.infile, args.outfile)
