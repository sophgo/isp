#!/usr/bin/env python3

import os
import sys
import datetime
import time
import shutil
import logging
import configparser
from abc import ABCMeta
from abc import abstractmethod
from multiprocessing import Process

import run_with_python.common.instance_base as isp_ins
from run_with_python.common.utils import C_Log_Lv
from run_with_python.common.utils import bcolors
import pdb
import re

class base(metaclass=ABCMeta):
    linear_pipeline = []
    wdr_pipeline = []
    pipeline = []
    keep_trace = []

    def __init__(self, ins_module, ins_setting, src_path, dst_path, fastMode, cropMode, fs_name='src'):
        #pdb.set_trace()
        self.ins_module = ins_module
        self.ins_setting = ins_setting
        self.src_path = src_path
        self.dst_path = dst_path
        self.log_path = os.path.join(self.dst_path, 'log')
        log_prefix = self.__class__.__name__ + datetime.datetime.now().strftime('_%Y%m%d_%H%M%S')
        self.log_file = os.path.join(self.log_path, log_prefix)  # cmodel log

        self.copy_src_ini_path = os.path.join(dst_path, 'src_ini')
        self.src_ini_file = ''
        self.dump_ini_path_base = os.path.join(dst_path, 'dump_ini')
        self.dump_ini_path = os.path.join(dst_path, 'dump_ini')
        self.result_yuv_path = os.path.join(self.dst_path, 'result_yuv')
        self.dump_ini_all = os.path.join(self.dump_ini_path)
        # self.dump_ini_hw_only = os.path.join(self.dump_ini_path, 'hw_only')
        self.src_ini_to_hw_only_ini = False

        self.fs_name = fs_name
        self.trace = {}
        self.trace_folder_del_list = []
        self.isp_ip = {}
        self.active_pipeline = []
        self.first_stage = 0
        self.last_stage = 0
        self.py_log_lv = logging.DEBUG
        self.c_log_lv = C_Log_Lv.INFO
        self.keyword = ''
        self.timestamp = ''
        self.start = 0
        self.end = 0
        self.leave_no_trace = False
        self.reg_hdr_enable = False
        self.single_frame_sim = False
        self.fastMode = fastMode
        self.cropMode = cropMode
    
    def get_active_pipeline(self):
        active_pipeline = []
        for stage_idx, stage in enumerate(self.pipeline):
            if (stage_idx >= self.first_stage and stage_idx <= self.last_stage):
                active_pipeline.append(stage)
        return active_pipeline

    def set_src_ini_file(self, ini_file):
        self.src_ini_file = ini_file

    def set_reg_hdr_enable(self, isEnable):
        self.reg_hdr_enable = isEnable
        if self.reg_hdr_enable:
            self.pipeline = self.wdr_pipeline
        else:
            self.pipeline = self.linear_pipeline
        self.last_stage = len(self.pipeline) - 1

    def set_src_ini_to_hw_only_ini(self):
        self.src_ini_to_hw_only_ini = True

    def set_single_frame_sim(self):
        self.single_frame_sim = True

    def set_keep_trace(self, keep_trace):
        self.keep_trace = keep_trace

    def set_py_log_lv(self, lv):
        self.py_log_lv = lv

    def set_c_log_lv(self, lv):
        self.c_log_lv = lv

    def set_keyword(self, keyword):
        self.keyword = '_' + keyword
        self.dump_ini_path = self.dump_ini_path_base + '_' + keyword
        self.dump_ini_all = self.dump_ini_path # os.path.join(self.dump_ini_path, 'all')
        # self.dump_ini_hw_only = os.path.join(self.dump_ini_path, 'hw_only')

    def set_timestamp(self, timestamp):
        self.timestamp = '_' + timestamp

    def set_start(self, start):
        self.start = start

    def set_end(self, end):
        self.end = end

    def set_leave_no_trace(self):
        self.leave_no_trace = True

    def set_first_stage(self, ip):
        find_flag = False
        for stage_idx, stage in enumerate(self.pipeline):
            if ip in stage:
                find_flag = True
                self.first_stage = stage_idx
                print(f'{bcolors.HEADER}set first ip: {ip}{bcolors.ENDC}')
                return

        if not find_flag:
            print(f'{bcolors.FAIL}set first ip: {ip} is not in pipeline{bcolors.ENDC}')
            sys.exit(1)


    def set_last_stage(self, ip):
        find_flag = False
        for stage_idx, stage in enumerate(self.pipeline):
            if ip in stage:
                find_flag = True
                self.last_stage = stage_idx
                print(f'{bcolors.HEADER}set last ip: {ip}{bcolors.ENDC}')
                return

        if not find_flag:
            print(f'{bcolors.FAIL}set last ip: {ip} is not in pipeline{bcolors.ENDC}')
            sys.exit(1)

    def dst_folder_gen(self):
        if not os.path.exists(self.dst_path):
            os.makedirs(self.dst_path)


    def log_folder_gen(self):
        if not os.path.exists(self.log_path):
            os.makedirs(self.log_path)


    def log_file_remove(self):
        file_list = os.listdir(self.log_path)
        for item in file_list:
            if item.endswith('.log'):
                os.remove(os.path.join(self.log_path, item))


    def dump_ini_file_remove(self):
        file_list = os.listdir(self.dump_ini_all) if os.path.exists(self.dump_ini_all) else []
        logging.info(f'remove ini files in folder: {self.dump_ini_all}')
        # print(f'{bcolors.WARNING}remove ini files in folder: {self.dump_ini_all}{bcolors.ENDC}')
        for item in file_list:
            if item.endswith('.ini'):
                os.remove(os.path.join(self.dump_ini_all, item))

        # file_list = os.listdir(self.dump_ini_hw_only) if os.path.exists(self.dump_ini_hw_only) else []
        # for item in file_list:
        #     if item.endswith('.ini'):
        #         os.remove(os.path.join(self.dump_ini_hw_only, item))
        # logging.info(f'remove ini files in folder: {self.dump_ini_hw_only}')
        # print(f'{bcolors.WARNING}remove ini files in folder: {self.dump_ini_hw_only}{bcolors.ENDC}')


    def src_ini_folder_gen(self):
        if not os.path.exists(self.copy_src_ini_path):
            os.makedirs(self.copy_src_ini_path)

    def result_yuv_folder_gen(self):
        if not os.path.exists(self.result_yuv_path):
            os.makedirs(self.result_yuv_path)

    def copy_src_ini(self):
        # original src ini
        src_ini_file = self.src_ini_file  #self.ins_setting[self.pipeline[0][0]]['hw_ini']
        src_ini_name = os.path.basename(src_ini_file)
        # copy src ini
        self.src_ini_file = os.path.join(self.copy_src_ini_path, src_ini_name)

        if os.path.isfile(src_ini_file):
            config_all = configparser.ConfigParser()
            config_all.read(src_ini_file)

            if self.src_ini_to_hw_only_ini:
                for section in config_all.sections():
                    if '_sw' in section:
                        config_all.remove_section(section)

            with open(self.src_ini_file, 'w', newline='\n') as configfile:
                config_all.write(configfile)

        else:
            print(f'{bcolors.FAIL}no ini file in specific path{bcolors.ENDC}')


    def dump_ini_folder_gen(self):
        if not os.path.exists(self.dump_ini_path):
            os.makedirs(self.dump_ini_path)

        if not os.path.exists(self.dump_ini_all):
            os.makedirs(self.dump_ini_all)

        # if not os.path.exists(self.dump_ini_hw_only):
        #     os.makedirs(self.dump_ini_hw_only)


    def dst_trace_gen(self):
        for stage in self.pipeline:
            for ip in stage:
                self.trace[ip] = os.path.join(self.dst_path, 'trace', ip)
                if ip not in self.keep_trace:
                    self.trace_folder_del_list.append(self.trace[ip])


    def isp_ip_gen(self):
        if self.reg_hdr_enable:
            isp_ins.base.set_reg_hdr_enable()

        if self.single_frame_sim:
            isp_ins.base.set_single_frame_sim()

        for stage_idx, stage in enumerate(self.pipeline):
            if (stage_idx >= self.first_stage and stage_idx <= self.last_stage):
                self.active_pipeline.append(stage)
            #pdb.set_trace()
            for ip in stage:
                c_log_lv = self.ins_setting[ip].get('log_lv', self.c_log_lv)

                src_path = self.src_path if stage_idx == 0 else self.trace
                dst_path = self.dst_path if stage_idx == (len(self.pipeline) - 1) else self.trace[ip]
                self.isp_ip[ip] = getattr(self.ins_module, ip)(
                    self.ins_setting[ip]['bin'], self.src_ini_file, self.log_file, c_log_lv,
                    src_path, dst_path, self.timestamp, self.keyword)
                #print('##############ip:',ip) # crop0,crop1,...
                #print('##############self.src_ini_file:',self.src_ini_file)
                if stage_idx == 0:  # first stage
                    self.isp_ip[ip].set_src_prefix(self.fs_name)  # input with src__000.ppm

                if stage_idx == (len(self.pipeline) - 1):  # last stage
                    self.isp_ip[ip].set_dbg_top_folder(os.path.join(self.trace[ip], 'dbg'))
                    self.isp_ip[ip].set_bt_top_folder(os.path.join(self.trace[ip], 'bt'))
                    self.isp_ip[ip].set_dump_ini_folder(os.path.join(self.trace[ip], 'dump_ini'))


    def isp_target_folder_gen(self):
        for ip in self.isp_ip.keys():
            self.isp_ip[ip].dst_folder_gen()
            self.isp_ip[ip].dump_ini_folder_gen()
            self.isp_ip[ip].bt_top_folder_gen()
            self.isp_ip[ip].dbg_top_folder_gen()

    def log_config(self):
        py_log_filename = datetime.datetime.now().strftime("run_with_python_%Y%m%d_%H%M%S.log")
        logging.basicConfig(
            filename=os.path.join(self.dst_path, 'log', py_log_filename),
            level=self.py_log_lv,
            format='[%(asctime)s %(levelname)-8s] %(message)s',
            datefmt='%H:%M:%S',
            )

    def setIniFastModeValue(self, filePath):
        file_content = ""
        new_file_content = ""
        with open(filePath, 'r') as f:
            file_content = f.read()
            #rgbcac disable
            pattern = re.compile("reg_rgbcac_enable = \d", re.DOTALL)
            searchStr = pattern.findall(file_content)[0]
            new_file_content = file_content.replace(searchStr, 'reg_rgbcac_enable = 0')
            #lcac disable
            pattern = re.compile("reg_lcac_enable = \d", re.DOTALL)
            searchStr = pattern.findall(new_file_content)[0]
            new_file_content = new_file_content.replace(searchStr, 'reg_lcac_enable = 0')
            #cac disable
            pattern = re.compile("reg_pfc_enable = \d", re.DOTALL)
            searchStr = pattern.findall(new_file_content)[0]
            new_file_content = new_file_content.replace(searchStr, 'reg_pfc_enable = 0')
            #af disable
            pattern = re.compile("reg_af_enable = \d", re.DOTALL)
            searchStr = pattern.findall(new_file_content)[0]
            new_file_content = new_file_content.replace(searchStr, 'reg_af_enable = 0')
            #gms disable
            pattern = re.compile("reg_gms_enable = \d", re.DOTALL)
            searchStr = pattern.findall(new_file_content)[0]
            new_file_content = new_file_content.replace(searchStr, 'reg_gms_enable = 0')
            f.close()
        with open(filePath, 'w') as f:
            f.write(new_file_content)
            f.close()

    def setIniCropModeValue(self, filePath, cropMode):
        x_start = cropMode.split(" ")[0]
        x_end = cropMode.split(" ")[1]
        y_start = cropMode.split(" ")[2]
        y_end = cropMode.split(" ")[3]
        file_content = ""
        new_file_content = ""
        with open(filePath, 'r') as f:
            file_content = f.read()
            #crop enable
            pattern = re.compile("reg_crop_enable = \d", re.DOTALL)
            for searchStr in pattern.findall(file_content):
                new_file_content = file_content.replace(searchStr, 'reg_crop_enable = 1')
            #crop set
            pattern = re.compile("reg_crop_start_y = \d", re.DOTALL)
            for searchStr in pattern.findall(new_file_content):
                new_file_content = new_file_content.replace(searchStr, 'reg_crop_start_y = {}'.format(y_start))
            pattern = re.compile("reg_crop_end_y = \d", re.DOTALL)
            for searchStr in pattern.findall(new_file_content):
                new_file_content = new_file_content.replace(searchStr, 'reg_crop_end_y = {}'.format(y_end))
            pattern = re.compile("reg_crop_start_x = \d", re.DOTALL)
            for searchStr in pattern.findall(new_file_content):
                new_file_content = new_file_content.replace(searchStr, 'reg_crop_start_x = {}'.format(x_start))
            pattern = re.compile("reg_crop_end_x = \d", re.DOTALL)
            for searchStr in pattern.findall(new_file_content):
                new_file_content = new_file_content.replace(searchStr, 'reg_crop_end_x = {}'.format(x_end))
            #disable lsc
            pattern = re.compile("reg_af_enable = \d", re.DOTALL)
            searchStr = pattern.findall(new_file_content)[0]
            new_file_content = new_file_content.replace(searchStr, 'reg_af_enable = 0')
            #disable ldci
            pattern = re.compile("reg_ldci_enable = \d", re.DOTALL)
            searchStr = pattern.findall(new_file_content)[0]
            new_file_content = new_file_content.replace(searchStr, 'reg_ldci_enable = 0')
            #disable dci
            pattern = re.compile("reg_dci_enable = \d", re.DOTALL)
            searchStr = pattern.findall(new_file_content)[0]
            new_file_content = new_file_content.replace(searchStr, 'reg_dci_enable = 0')
            #disable dehaze
            pattern = re.compile("reg_dehaze_enable = \d", re.DOTALL)
            searchStr = pattern.findall(new_file_content)[0]
            new_file_content = new_file_content.replace(searchStr, 'reg_dehaze_enable = 0')
            f.close()
        with open(filePath, 'w') as f:
            f.write(new_file_content)
            f.close()

    def fastMode_handle(self):
        print("fastMode:", self.fastMode)
        if self.fastMode > 1:
            self.setIniFastModeValue(self.src_ini_file)

    def cropMode_handle(self):
        print("crop:", self.cropMode)
        if self.cropMode:
            self.setIniCropModeValue(self.src_ini_file, self.cropMode)

    def pre_proc(self):
        self.dst_folder_gen()
        self.log_folder_gen()
        self.log_file_remove()
        self.log_config()
        self.src_ini_folder_gen()
        self.result_yuv_folder_gen()
        self.copy_src_ini()
        self.dump_ini_folder_gen()
        self.dst_trace_gen()
        self.isp_ip_gen()
        self.isp_target_folder_gen()
        self.fastMode_handle()
        self.cropMode_handle()

    def main_proc(self):  # sp
        if self.reg_hdr_enable is True:
            logging.info(f"===== HDR mode =====")
        else:
            logging.info(f"===== Linear mode =====")

        logging.info(f"active_pipelines = {self.active_pipeline}")

        ts = time.time()
        # processing first image
        isp_ins.base.set_first() # 1
        
        isp_ins.base.update_idx(self.start) # 0
        
        self.single_img_proc_sp(self.start)

        # processing other images
        isp_ins.base.set_not_first()

        for idx in range(self.start+1, self.end+1, 1):
            isp_ins.base.update_idx(idx)
            self.single_img_proc_sp(idx)
        te = time.time()
        logging.info(f"total proc. time: {int(1000*(te-ts))} ms")


    def single_img_proc_sp(self, idx):
        for stage in self.active_pipeline:
            for ip in stage:
                # stage:['crop0', 'crop1', 'crop2']!!!!!ip:crop0

                self.single_ip_send_cmd_proc(ip, idx)          

    def main_proc_mp(self):
        logging.info(f"active_pipelines = {self.active_pipeline}")
        ts = time.time()
        # processing first image
        cmd_queue_first = []  # [stage][ip]

        if (self.start == 0):
            isp_ins.base.set_first()
            isp_ins.base.update_idx(0)
            for stage in self.active_pipeline:
                cmd_queue_first.append([])  # cmd_queue_first = [[]], the next loop be [[p1, p2], []]
                for ip in stage:
                    p = Process(target=self.send_cmd_proc_mp, args=(ip, self.start, True, ))
                    cmd_queue_first[-1].append(p)

            for p_set in cmd_queue_first:
                [x.start() for x in p_set]
                [x.join() for x in p_set]

        # processing other images
        frame_group_num = 5  # 10 is user define

        start = 1 if self.start == 0 else self.start
        frame_idx_cand = list(range(start, self.end+1, frame_group_num))  # end = 10 => 1, 4, 7, 10
        frame_idx_cand.append(self.end+1)  # 1, 4, 7, 11

        isp_ins.base.set_not_first()
        for fbatch in range(len(frame_idx_cand) - 1):  # len -1 => 3, fbatch = 0, 1, 2
            cmd_queue = []  # [frame][stage][ip]
            for idx in range(frame_idx_cand[fbatch], frame_idx_cand[fbatch+1], 1):

                batch_frame = idx - frame_idx_cand[fbatch]
                cmd_queue.append([])  # append frames, process from [[[ip1, ip2], [ip3, ip4]]] to [[[ip1, ip2], [ip3, ip4]], []]

                for _ in range(batch_frame):  # append leading empty stage
                    cmd_queue[batch_frame].append([])

                for ii in range(batch_frame):  # append trailing empty space
                    cmd_queue[ii].append([])

                isp_ins.base.update_idx(idx)
                for stage in self.active_pipeline:
                    cmd_queue[batch_frame].append([])  # append current stage
                    for ip in stage:
                        p = Process(target=self.send_cmd_proc_mp, args=(ip, idx, False, ))
                        cmd_queue[batch_frame][-1].append(p)

            for stage in range(len(cmd_queue[0])):  # len = frame_group_num
                processes = []
                for frame in range(len(cmd_queue)):
                    processes.extend(cmd_queue[frame][stage])

                # group_num = 20
                # proecess_set = [processes[i:i+group_num] for i in range(0, len(processes), group_num)]
                # for p_set in proecess_set:
                #     [x.start() for x in p_set]
                #     [x.join() for x in p_set]

                [x.start() for x in processes]
                [x.join() for x in processes]

        te = time.time()
        logging.info(f"total proc. time: {int(1000*(te-ts))} ms")

    def main_proc_mp2(self):
        logging.info(f"active_pipelines = {self.active_pipeline}")
        ts = time.time()

        # processing images
        frame_group_num = 40  # 10 is user define

        frame_idx_cand = list(range(0, self.end+1, frame_group_num))  # end = 10 => 1, 4, 7, 10
        frame_idx_cand.append(self.end+1)  # 1, 4, 7, 11 [1,6,8]

        for fbatch in range(len(frame_idx_cand) - 1):  # len -1 => 3, fbatch = 0, 1, 2
            cmd_queue = []  # [frame][stage][ip]
            for idx in range(frame_idx_cand[fbatch], frame_idx_cand[fbatch+1], 1):

                batch_frame = idx - frame_idx_cand[fbatch]
                cmd_queue.append([])  # append frames, process from [[[ip1, ip2], [ip3, ip4]]] to [[[ip1, ip2], [ip3, ip4]], []]

                isp_ins.base.update_idx(idx)
                for stage in self.active_pipeline:
                    cmd_queue[batch_frame].append([])  # append current stage
                    for ip in stage:
                        p = Process(target=self.send_cmd_proc_mp, args=(ip, idx, False, ))
                        cmd_queue[batch_frame][-1].append(p)
            
            for stage in range(len(cmd_queue[0])):  # len = frame_group_num
                processes = []
                for frame in range(len(cmd_queue)):
                    processes.extend(cmd_queue[frame][stage])
                print("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
                [x.start() for x in processes]
                [x.join() for x in processes]
                print("--------------------------------------------------------------")

        te = time.time()
        logging.info(f"total proc. time: {int(1000*(te-ts))} ms") 

    def send_cmd_proc_mp(self, ip, idx, first_frame):
        if first_frame is True:
            isp_ins.base.set_first()
        else:
            isp_ins.base.set_not_first()
        isp_ins.base.update_idx(idx)
        self.single_ip_send_cmd_proc(ip, idx)


    def single_ip_send_cmd_proc(self, ip, idx):
        print(f'{bcolors.OKBLUE}[frame {idx:03}] {ip} proc. start{bcolors.ENDC}')
        logging.debug(f'[frame {idx}] {ip} proc. start')

        if idx == 0:
            isp_ins.base.set_first()
        else:
            isp_ins.base.set_not_first()
        self.isp_ip[ip].update_dbg_folder()
        self.isp_ip[ip].update_bt_folder()
        self.isp_ip[ip].send_cmd_proc()
        self.isp_ip[ip].set_hdr_mode(self.reg_hdr_enable)
        self.isp_ip[ip].set_dump_dbg_img(int(not self.fastMode))


        print(f'{bcolors.OKGREEN}[frame {idx:03}] {ip} proc. end{bcolors.ENDC}')
        logging.debug(f'[frame {idx}] {ip} proc. end')


    def update_dump_ini(self, dump_ini_file_name, single_ip_ini):
        config_all = configparser.ConfigParser()
        config_single = configparser.ConfigParser()

        config_all.read(dump_ini_file_name)
        config_single.read(single_ip_ini)

        for section in config_single.sections():
            config_all[section] = config_single[section]

        with open(dump_ini_file_name, 'w', newline='\n') as configfile:
            config_all.write(configfile)


    # def gen_hw_only_ini(self, dump_ini_file_name, dump_ini_hw_only_file_name):
    #     config_all = configparser.ConfigParser()
    #     config_all.read(dump_ini_file_name)

    #     for section in config_all.sections():
    #         if '_sw' in section:
    #             config_all.remove_section(section)

    #     with open(dump_ini_hw_only_file_name, 'w', newline='\n') as configfile:
    #         config_all.write(configfile)


    def dump_ini_process(self):
        self.dump_ini_file_remove()

        for idx in range(self.start, self.end+1, 1):
            isp_ins.base.update_idx(idx)
            src_ini_path = self.src_ini_file
            src_ini_name = os.path.splitext(os.path.basename(src_ini_path))[0]

            dump_ini_file_name = os.path.join(self.dump_ini_all, f'{src_ini_name}_dump_{idx:03}.ini')
            shutil.copy(src_ini_path, dump_ini_file_name)
            logging.info(f'copy src ini:\n{src_ini_path}\nto dump ini:\n{dump_ini_file_name}\n')
            # print(f'{bcolors.WARNING}copy src ini:\n{src_ini_path}\nto dump ini:\n{dump_ini_file_name}\n{bcolors.ENDC}')

            # upate dump ini
            for stage in self.pipeline:
                for ip in stage:
                    single_ip_ini = self.isp_ip[ip].get_dump_ini_file()

                    if os.path.isfile(single_ip_ini):
                        logging.info(f'[frame {idx:03}] update [{ip}] dump ini to pipeline dump ini')
                        print(f'{bcolors.WARNING}[frame {idx:03}] update pipeline dump ini by [{ip}]{bcolors.ENDC}')
                        self.update_dump_ini(dump_ini_file_name, single_ip_ini)

            # dump_ini_hw_only_file_name = os.path.join(self.dump_ini_hw_only, f'{src_ini_name}_dump_hw_only_{idx:03}.ini')
            # self.gen_hw_only_ini(dump_ini_file_name, dump_ini_hw_only_file_name)


    def post_proc(self):
        self.dump_ini_process()

        if self.leave_no_trace:
            self.del_trace_proc()


    def del_trace_proc(self):
        for folder in self.trace_folder_del_list:
            shutil.rmtree(folder)


    def run(self, mp=False):
        self.pre_proc()
                    
        if mp is True:
            print("!!!!!!!!!!!!")
            self.main_proc_mp2()
        else:
            self.main_proc()

        self.post_proc()
