#!/usr/bin/env python3

import argparse
import os
import sys
import getpass
import glob
import logging
import shutil
from multiprocessing import Process
import configparser
import pdb
from PIL import Image

os.chdir(os.path.dirname(__file__))
sys.path.append("../..")
import run_with_python.common.utils as utils
from run_with_python.common.utils import img_idx_proc
from run_with_python.common.utils import C_Log_Lv
from run_with_python.common.utils import bcolors
from run_with_python.common.utils import raw_name_parsing
from run_with_python.common.pre_process.preProcess import preProcess

PROJECT = 'Athena'
def command_proc():
    parser = argparse.ArgumentParser()
    parser.add_argument('img_folder', help='test img folder name', type=str)
    parser.add_argument('--folder_name', help='choose single folder', type=str, default=None) # new added by huichong
    parser.add_argument('--ini_file', help='test ini file', type=str, default=None) # new added by huichong
    parser.add_argument('--get_ini_mode', help='from folder of chosen by user', type=int, default=0) # new added by huichong
    parser.add_argument('--src', help='specific src img path', type=str, default=None)
    parser.add_argument('--dst', help='specific dst img path', type=str, default=None)
    parser.add_argument('--end', help='img end num', type=int, default=100)
    parser.add_argument('--leave_no_trace', '-n', help='leave no trace', action='store_true', default=False)
    parser.add_argument('--delete_dst', '-d', help='delete dst folder', action='store_true', default=False)
    parser.add_argument('--single_frame_sim', '-s', help='for single frame simulation', action='store_true', default=False)
    parser.add_argument('--first_stage', help='specific first stage with ip', type=str, default=None)
    parser.add_argument('--last_stage', help='specific last stage with ip', type=str, default=None)
    parser.add_argument('--py_log_lv', help='set python log level', type=str, default=None)
    parser.add_argument('--c_log_lv', help='set log log level', type=str, default=None)
    parser.add_argument('--keyword', help='set keyword for result', type=str, default=None)
    parser.add_argument('--sp', help='single process', action='store_true', default=False)
    parser.add_argument('--hw_only_ini', help='auto convert ini to hw_only_ini', action='store_true', default=False)
    parser.add_argument('--not_using_real_raw', help='using real raw', action='store_true',default=False)
    parser.add_argument('--fastMode', '-f', help='run fast', type=int, default=0)
    parser.add_argument('--crop', help='x_start x_end y_start y_end', type=str, default=None)
    args = parser.parse_args()
    return args


def log_setting(ins_setting):
    pass
    # ins_setting['blc0']['log_lv'] = C_Log_Lv.INFO


def ins_and_run(args, ip = 'athena_pipe_3f'):

    isp_pipe_config = __import__('config.' + 'athena_pipe' + '_config', fromlist=[None])
                  
    # src, dst
    Data = os.path.join('/ic/cmodel_dpu/cmodel_image/raw_ppm/', getpass.getuser(), ip, args.img_folder)
    results = os.path.join('/home', getpass.getuser(), 'Desktop', 'results_' + PROJECT + '_algo_run', args.img_folder)

    preProcessDir = "../../run_with_python/common/pre_process"
    regJsonFile = None
    templateIniFile = None
    chip = None
    for file in os.listdir(preProcessDir):
        if file.endswith("{}.json".format(PROJECT)):
            regJsonFile = os.path.join(preProcessDir, file)
        if file.endswith("{}.ini".format(PROJECT)):
            templateIniFile = os.path.join(preProcessDir, file)
            chip = PROJECT.lower()

    # args proc
    if args.src:
        Data = args.src

    if args.dst:
        results = args.dst
    
    print('Data:',Data)
    print('results:', results)
    if args.delete_dst:
        if os.path.exists(results):
            for root, dirs, files in os.walk(results, topdown=False):
                for name in files:
                    os.remove(os.path.join(root, name))
                for name in dirs:
                    os.rmdir(os.path.join(root, name))

    processes = []
    tasks = []

    for raw_folder in os.listdir(Data):
        # added by huichong
        print(raw_folder)
        if args.get_ini_mode == 1:
            if not args.folder_name:
                print("Folder name is empty!!")
                exit()
            if raw_folder != args.folder_name:
                print("This is not the target folder!!")
                continue
        if not os.path.isdir(os.path.join(Data, raw_folder)):
            continue
        # end
        src_path = os.path.join(Data, raw_folder)
        dst_path = os.path.join(results, raw_folder)

        if not args.not_using_real_raw:
            raw_list = glob.glob(os.path.join(Data, raw_folder, '*.raw'))

            if len(raw_list) < 1:
                print(f"{bcolors.FAIL}there is no raw file in [{os.path.join(Data, raw_folder)}], pass the folder{bcolors.ENDC}")
                continue

            if len(raw_list) > 1:
                print(f"{bcolors.FAIL}raw image numbers > 1 in the folder: [{raw_folder}], pass the folder{bcolors.ENDC}")
                continue

            preProcess(regJsonFile, templateIniFile, chip, src_path, src_path)
            print(f"{bcolors.OKGREEN}regJson: {bcolors.ENDC}", regJsonFile)
            print(f"{bcolors.OKGREEN}templateIni: {bcolors.ENDC}", templateIniFile)

            raw_name_info = raw_name_parsing(raw_list[0])
 
        # search ini candidates         
        ini_list_all = glob.glob(os.path.join(Data, raw_folder, '*.ini'))
        print("-------", Data, raw_folder, ini_list_all, PROJECT)
        ini_list = []
        for ini_name in ini_list_all:
            ini_basename = os.path.basename(ini_name)
            if PROJECT in ini_basename:
                ini_list.append(ini_name)       
       
        print('ini_list:',ini_list)
        if len(ini_list) > 1:
            print(f"{bcolors.OKGREEN} Run single raw with multi-ini.\n {bcolors.ENDC}{ini_basename}")

            
        for single_ini in ini_list:
            # added by huichong
            if args.get_ini_mode == 1:
                print("choose single ini file manually!!!")
                print("src_path: ", os.listdir(src_path))
                if not os.path.exists(args.ini_file):
                    print("cannot find target ini file!!!!")
                    exit()
                single_ini = args.ini_file
            # end
            ini_basename = os.path.basename(single_ini)
            if not args.not_using_real_raw:
                print(f"{bcolors.OKGREEN}RAW path: {bcolors.ENDC}{raw_list[0]}")
            print(f"{bcolors.OKGREEN}ini: {bcolors.ENDC}{ini_basename}")
            # set reg_hdr_enable
            print("single_ini: ", single_ini)
            # exit()
            cmodel_top_ini = configparser.ConfigParser()
            cmodel_top_ini.read(single_ini)
            #print('single_ini:', single_ini)

            reg_3exp_hdr_enable = bool(int(cmodel_top_ini.get('cmodel_top', 'reg_3exp_hdr_enable', fallback='0')))
            print('!!!!!!!!!!reg_3exp_hdr_enable:',reg_3exp_hdr_enable)  
            if reg_3exp_hdr_enable:
                ip = 'athena_pipe_3f'
                # load module
                isp_pipe = __import__('cmodel_interface.' + ip, fromlist=[None])
                # get ins setting
                ins_setting = isp_pipe_config.get_ins_setting_3f()
                log_setting(ins_setting)  # log setting
            else:
                ip = 'athena_pipe_2f'
                # load module
                isp_pipe = __import__('cmodel_interface.' + ip, fromlist=[None])
                # get ins setting
                # pdb.set_trace()
                ins_setting = isp_pipe_config.get_ins_setting_2f()
                log_setting(ins_setting)  # log setting
                 
##########################################

            # inital isp pipeline
            if len(ini_list) > 1:
                dst_path_multi_ini = os.path.join(dst_path, os.path.splitext(ini_basename)[0])
                tasks.append(getattr(isp_pipe, ip)(isp_pipe, ins_setting, src_path, dst_path_multi_ini, args.fastMode, args.crop))
                tasks[-1].set_timestamp(os.path.splitext(ini_basename)[0])
                #print(f"{bcolors.OKGREEN}dst_folder: {bcolors.ENDC}{dst_path_multi_ini}\n") 
            else:
                tasks.append(getattr(isp_pipe, ip)(isp_pipe, ins_setting, src_path, dst_path, args.fastMode, args.crop))
                if not args.not_using_real_raw:      
                    tasks[-1].set_timestamp(raw_name_info['time_stamp'])
               # print(f"{bcolors.OKGREEN}dst folder: {bcolors.ENDC}{dst_path}\n") _20211119183132_Mars

            tasks[-1].set_start(0)
            if not args.not_using_real_raw: 
                tasks[-1].set_end(min(raw_name_info['frame_num'] - 1, args.end))
            else:
                tasks[-1].set_end(args.end)

            tasks[-1].set_src_ini_file(single_ini)
            if args.hw_only_ini:
                 tasks[-1].set_src_ini_to_hw_only_ini()
            # added by huichong
            if args.get_ini_mode == 1:
                break
            # end
                    
    print('tasks:',tasks)
    for isp_pipe_ins in tasks:
         # set reg_hdr_enable
        reg_hdr_enable = bool(int(cmodel_top_ini.get('cmodel_top', 'reg_hdr_enable', fallback='0')))
        if reg_hdr_enable:
            isp_pipe_ins.set_reg_hdr_enable(True)
            print(f'{bcolors.OKGREEN}Run pipeline in HDR mode\n {bcolors.ENDC}')
        else:
            isp_pipe_ins.set_reg_hdr_enable(False)
            print(f'{bcolors.OKGREEN}Run pipeline in Linear mode\n {bcolors.ENDC}')
    
        if args.py_log_lv is not None:
            isp_pipe_ins.set_py_log_lv(utils.py_log_lv_str2log[args.py_log_lv])

        if args.c_log_lv is not None:
            isp_pipe_ins.set_c_log_lv(utils.c_log_lv_str2int[args.c_log_lv])

        if args.leave_no_trace:
            isp_pipe_ins.set_keep_trace([])
            isp_pipe_ins.set_leave_no_trace()

        if args.first_stage is not None:
            isp_pipe_ins.set_first_stage(args.first_stage)

        if args.last_stage is not None:
            isp_pipe_ins.set_last_stage(args.last_stage)

        if args.keyword is not None:
            isp_pipe_ins.set_keyword(args.keyword)

        if args.single_frame_sim:
            print(f"{bcolors.WARNING}[single frame simulation, only process frame_idx 0]{bcolors.ENDC}\n")
            isp_pipe_ins.set_single_frame_sim()
            isp_pipe_ins.set_end(0)

        print('args.sp:',args.sp) 
        if args.sp:
            isp_pipe_ins.run()
        else:
            #mp should rm pre build trace result
            dst_trace_path = dst_path + "/trace/"
            one_d_list = [item for sublist in isp_pipe_ins.get_active_pipeline() for item in sublist]
            for module in one_d_list:
                os.system('rm -rf ' + dst_trace_path + module)
            stage_mp = True
            p = Process(target=isp_pipe_ins.run, args=(stage_mp,))
            processes.append(p)

    [x.start() for x in processes]
    [x.join() for x in processes]


    # dump multi image to collection folder
    if args.keyword is not None:
        # define target folder
        dst_collection_path = os.path.join(results, os.path.basename(results) + '_collection')
        if not os.path.exists(dst_collection_path):
            os.makedirs(dst_collection_path)

        # find all result folder
        for single_raw_result in os.listdir(results):
            single_result_folder = os.path.join(results, single_raw_result)
            if not os.path.isdir(single_result_folder):
                continue

            # get all result ppm
            single_target = glob.glob(os.path.join(single_result_folder, '*' + args.keyword + '_rgb_000.ppm'))
            for img in single_target:
                im = Image.open(img)
                target_name = os.path.splitext(os.path.basename(img))[0] + '.png'
                im.save(os.path.join(dst_collection_path, target_name))
                print(f'{bcolors.OKCYAN}copy\n"{img}"\nto\n"{os.path.join(dst_collection_path, target_name)}"{bcolors.ENDC}')
                # shutil.copy(img, os.path.join(dst_collection_path, os.path.basename(img)))


def main():
    args = command_proc()
    #pdb.set_trace()
    ins_and_run(args)


if __name__ == '__main__':
    main()
