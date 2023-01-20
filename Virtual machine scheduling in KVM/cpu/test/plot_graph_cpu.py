import argparse
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys, os
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from vm import VMManager

NUM_PCPUS = 16

def get_file_content(file_path):
    ls = list()
    with open(file_path) as fh:
        ls = fh.readlines()
    
    for index, line in enumerate(ls):
        ls[index] = line.strip()
    return ls

def are_same(list1, list2):
    for i in list1:
        if i in list2:
            continue
        else:
            return False
    return True
 
def parse_content(data_list, num_pcpu=NUM_PCPUS):
    processor_usage = []
    pinning = []
    prev_mapping = []

    for i in range(num_pcpu):
        processor_usage.append([])
        prev_mapping.append([])
        pinning.append([])

    dashed_line = "--------------------------------------------------"
    index = 0
    while index < len(data_list):
        if data_list[index] == dashed_line:
            index += 2
            while index < len(data_list) and "mapping" in data_list[index]:
                '''2 - usage: 19.0 | mapping ['aos_vm8', 'aos_vm1']'''
                data = data_list[index].split("mapping")
                
                data1 = data[0].split(" ")
                pid = int(data1[0])
                usage = float(data1[3])
                processor_usage[pid].append(usage)
                
                data2 = eval(data[1])
                if are_same(data2, prev_mapping[pid]):
                    pinning[pid].append(np.nan)
                else:
                    pinning[pid].append(usage)
                prev_mapping[pid] = data2
                index += 1
        else:
            index += 1
    return processor_usage, pinning


def create_dataframe(data, column_names=None):    
    df = pd.DataFrame(data)
    df = df.transpose()

    if column_names is not None:
        df.columns = column_names
    return df

def plotting_function(plotting_list_usage, plotting_list_pinning, plot_title,
                      x_label, y_label_list, figure_name):
    figure, axes = plt.subplots(len(plotting_list_usage), figsize=(25, 18))

    for index, plot_list in enumerate(plotting_list_usage):
        x_list, y_list = plot_list
        axes[index].plot(x_list, y_list, label='processor_usage')
        
    for index, plot_list in enumerate(plotting_list_pinning):
        x_list, y_list = plot_list
        axes[index].scatter(x_list, y_list,
                            color='mediumvioletred',
                            label='pin_changes')

        axes[index].set_xlabel(x_label)
        axes[index].set_ylabel(y_label_list[index])
    
    figure.tight_layout(pad=4)
    figure.suptitle(figure_name, fontweight='bold', fontsize=14)
    plt.legend()
    figure.savefig(figure_name)


def workflow(file_path, figure_name):
    manager=VMManager()
    NUM_PCPUS = manager.getPhysicalCpus()
    
    data_list = get_file_content(file_path)
    processor_usage, pinning = parse_content(data_list, NUM_PCPUS)

    column_names = []
    for i in range(NUM_PCPUS):
        column_names.append(i)

    df_usage = create_dataframe(processor_usage[:NUM_PCPUS], column_names)
    df_pinning = create_dataframe(pinning[:NUM_PCPUS], column_names)

    x_label = "Iteration"

    y_label = "usage (%)"
    y_label_list = []
    for i in range(NUM_PCPUS):
        y_label_list.append("pCPU{} {}".format(i, y_label))

    plot_title = "Processor Usage"

    plotting_list_usage = list()
    plotting_list_pinning = list()
    for column_name in column_names:
        x_list = [index for index in range(df_usage.shape[0])]
        y_list = list(df_usage[column_name])
        x_list_pins = [index for index in range(df_pinning.shape[0])]
        y_list_pins = list(df_pinning[column_name])
        plotting_list_usage.append([x_list, y_list])
        plotting_list_pinning.append([x_list_pins, y_list_pins])

    plotting_function(plotting_list_usage, plotting_list_pinning, plot_title,
                      x_label, y_label_list[:NUM_PCPUS], figure_name)

def main(file_path, figure_name):
    workflow(file_path, figure_name)

    
if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--infile", help="Path of the file to parse")
    parser.add_argument("-o","--outfile", help="filename to save the graph to")
    
    args = parser.parse_args()

    if args.infile == None or args.outfile == None:
        sys.exit("Usage: python plot_graph_cpu.py -i <input log file> -o <path to save graph>")

    main(args.infile, args.outfile)
