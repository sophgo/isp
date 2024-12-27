#!/usr/bin/env python3

import argparse
import os
import sys
import logging
import datetime
from multiprocessing import Process
from pathlib import Path
import shutil
import getpass
import pdb

import athena_pipe_config as isp_pipe_config

os.chdir(os.path.dirname(__file__))
sys.path.append("../../..")
from run_with_python.common.utils import bcolors


def command_proc():
    parser = argparse.ArgumentParser()
    parser.add_argument('--sp', help='using single process or not', action='store_true', default=False)
    parser.add_argument('--rel', help='debug mode or release mode', action='store_true')
    args = parser.parse_args()
    return args

def log_config():
    log_folder = os.path.join(os.path.dirname(__file__), '..', 'log')
    if not os.path.exists(log_folder):
        os.makedirs(log_folder)

    logging.basicConfig(
        filename=os.path.join(log_folder, datetime.datetime.now().strftime("build_all_%Y%m%d_%H%M%S.log")),
        level=logging.DEBUG,  # logging.INFO logging.DEBUG
        format='[%(asctime)s %(levelname)-8s] %(message)s',
        datefmt='%H:%M:%S',
        )


def bulid_cmd_wrap(build):
    os.chdir(os.path.dirname(build))
    os.system(build)


def bulid_cmd_wrap_dbg(build):
    os.chdir(os.path.dirname(build))
    os.system("echo compile debug mode")
    os.system("./clean.sh")
    os.system("cmake -DCMAKE_BUILD_TYPE=Debug .") 
    os.system("make")
    os.system("./clean.sh")


def bulid_cmd_wrap_rel(build):
    os.chdir(os.path.dirname(build))
    os.system("echo compile release mode")
    os.system("./clean.sh")
    os.system("cmake -DCMAKE_BUILD_TYPE=Release .") 
    os.system("make")
    os.system("./clean.sh")



def remove_existing_bin():
    ins_setting = isp_pipe_config.get_ins_setting()
    bin_list = []

    for single_module_info in ins_setting.values():
        bin_path = single_module_info['bin']
        if bin_path not in bin_list:
            bin_list.append(bin_path)

    for bin_file in bin_list:
        if os.path.isfile(bin_file):
            os.remove(bin_file)


def build_process(args):
    # get ins setting
    ins_setting = isp_pipe_config.get_ins_setting()
    build_list = []

    for single_module_info in ins_setting.values():
        bin_path = single_module_info['bin']
        build_path = os.path.join(Path(bin_path).parent.parent, 'src', 'build.sh')
        #build_path = os.path.join(Path(bin_path).parent, 'src', 'build.sh')
        if build_path not in build_list:
            build_list.append(build_path)

    print(f'{bcolors.HEADER}build list:{bcolors.ENDC}')
    logging.info(f'build list:')
    for build in build_list:
        print(f'{bcolors.OKGREEN}  {build}{bcolors.ENDC}')
        logging.info(f'  {build}')

    processes = []
    for build in build_list:
        if args.sp:
            bulid_cmd_wrap(build)
        else:
            if args.rel:
                p = Process(target=bulid_cmd_wrap_rel, args=(build,))
                processes.append(p)
            else:  
                p = Process(target=bulid_cmd_wrap_dbg, args=(build,))
                processes.append(p)
    
    [x.start() for x in processes]
    [x.join() for x in processes]


def check_build_result(args):
    ins_setting = isp_pipe_config.get_ins_setting()
    bin_list = []

    for single_module_info in ins_setting.values():
        bin_path = single_module_info['bin']
        if bin_path not in bin_list:
            bin_list.append(bin_path)

    pass_flag = True
    for bin_file in bin_list:
        if not os.path.isfile(bin_file):
            pass_flag = False
            logging.error(f'build {os.path.basename(bin_file)} is failed')

    if pass_flag:
        logging.info(f'Success to bulid all modules.')



def main():
    #pdb.set_trace()
    args = command_proc()
    log_config()

    remove_existing_bin()
    build_process(args)
    check_build_result(args)


if __name__ == '__main__':
    main()
