#!/usr/bin/env python3

import os
import sys
from abc import ABCMeta
from abc import abstractmethod
import time
import logging
import configparser
import signal
from run_with_python.common.utils import bcolors


class base(metaclass=ABCMeta):
    IMG_IDX = 0
    FIRST_FLAG = '1'
    REG_HDR_ENABLE = False
    SINGLE_FRAME_SIM = '0'
    REG_AINR_ENABLE = False
    def __init__(self, bin_file, src_ini, log_file, log_level, src_path, dst_path, timestamp='', keyword=''):
        self.bin_file = bin_file
        self.log_file = log_file
        self.log_level = log_level
        self.log_folder = os.path.dirname(log_file)

        # stc_path => string for singel ip unittest, dict for pipeline
        self.src_path = src_path
        self.dst_path = dst_path

        self.copy_src_ini_path = os.path.join(dst_path, 'src_ini')
        self.ini = src_ini
        self.src_ini_to_hw_only_ini = False

        self.ini2 = ''
        self.set_ini2_flag = False

        # optional parameter
        self.src_prefix = 'out'
        self.dst_prefix = 'out'
        self.dbg_top_folder = os.path.join(dst_path, 'dbg')
        self.dbg_path_base = os.path.join(self.dbg_top_folder, 'dbg')
        self.dbg_path = os.path.join(dst_path, 'dbg')
        self.bt_top_folder = os.path.join(dst_path, 'bt')
        self.bt_path_base = os.path.join(self.bt_top_folder, 'bt')
        self.bt_path = os.path.join(dst_path, 'bt')

        self.dump_ini_folder = os.path.join(dst_path, 'dump_ini')
        self.standalone_ini = False

        self.timestamp = timestamp
        self.keyword = keyword
        self.ins_id = 'NULL'
        self.dump_dbg_img = 1
        self.hdr_mode = False

        ################# yana.chen modified start ###############
        cmodel_top_ini = configparser.ConfigParser()
        cmodel_top_ini.read(self.ini)
        self.reg_AINR_enable = bool(int(cmodel_top_ini.get('cmodel_top', 'reg_AINR_enable', fallback='0')))
        ################# yana.chen modified end ###############

    @classmethod
    def set_reg_hdr_enable(cls):
        cls.REG_HDR_ENABLE = True

    @classmethod
    def get_reg_hdr_enable(cls):
        return cls.REG_HDR_ENABLE 

    @classmethod
    def set_single_frame_sim(cls):
        cls.SINGLE_FRAME_SIM = '1'
    
    @classmethod
    def get_single_frame_sim(cls):
        return cls.SINGLE_FRAME_SIM

    @classmethod
    def set_first(cls):
        cls.FIRST_FLAG = '1'

    @classmethod
    def set_not_first(cls):
        cls.FIRST_FLAG = '0'

    @classmethod
    def get_first_flag(cls):
            return cls.FIRST_FLAG

    @classmethod
    def update_idx(cls, idx):
        cls.IMG_IDX = idx

    @classmethod
    def get_idx(cls):
        return cls.IMG_IDX

    @classmethod
    def img_name_gen(cls, path, prefix, specific, ext='.ppm'):
        img_name = prefix + '_' + specific + '_' + f'{cls.IMG_IDX:03}' + ext
        return os.path.join(path, img_name)

    @classmethod
    def pre_img_name_gen(cls, path, prefix, specific, ext='.ppm'):
        # pre_idx = cls.IMG_IDX if int(cls.FIRST_FLAG) else (cls.IMG_IDX-1)
        pre_idx = max((cls.IMG_IDX - 1), 0)
        img_name = prefix + '_' + specific + '_' + f'{pre_idx:03}' + ext
        return os.path.join(path, img_name)

    @classmethod
    def log_name_gen(cls, string):
        log_name = string + '_' + f'{cls.IMG_IDX:03}' + '.log'
        return log_name

    @classmethod
    def log_config(cls):
        logging.basicConfig(
            level=logging.DEBUG,  # python log level
            format='[%(asctime)s %(levelname)-8s] %(message)s',
            datefmt='%H:%M:%S',
        )
    def set_dump_dbg_img(self, isEnable):
        self.dump_dbg_img = isEnable
    
    def set_hdr_mode(self, isEnable):
        self.hdr_mode = isEnable
    ################# yana.chen modified start############# 
    @classmethod
    def set_reg_AINR_enable(cls):
        cls.REG_AINR_ENABLE = self.reg_AINR_enable
    @classmethod
    def get_reg_AINR_enable(cls):
        return cls.REG_AINR_ENABLE 
    ################# yana.chen modified start#############  
    def get_dump_ini_file(self):
        return os.path.join(self.dump_ini_folder, self.__class__.__name__ + f'_{base.get_idx():03}' + '.ini')

    def set_src_ini_to_hw_only_ini(self):
        self.src_ini_to_hw_only_ini = True

    def set_ini2(self, ini2):
        self.ini2 = ini2
        self.set_ini2_flag = True

    def set_src_prefix(self, src_prefix):
        self.src_prefix = src_prefix

    def set_dst_prefix(self, dst_prefix):
        self.dst_prefix = dst_prefix

    def set_dbg_top_folder(self, dbg_top_folder):
        self.dbg_top_folder = dbg_top_folder
        self.dbg_path_base = os.path.join(self.dbg_top_folder, 'dbg')

    def set_bt_top_folder(self, bt_top_folder):
        self.bt_top_folder = bt_top_folder
        self.bt_path_base = os.path.join(self.bt_top_folder, 'bt')

    def set_dump_ini_folder(self, dump_ini_folder):
        self.dump_ini_folder = dump_ini_folder

    def set_standalone_ini(self):
        self.standalone_ini = True

    def pre_proc(self):
        self.dst_folder_gen()
        self.log_folder_gen()
        self.log_file_remove()
        self.log_config()
        self.src_ini_folder_gen()
        self.copy_src_ini()
        self.dump_ini_folder_gen()
        self.dbg_top_folder_gen()
        self.bt_top_folder_gen()

    def dst_folder_gen(self):
        if not os.path.exists(self.dst_path):
            os.makedirs(self.dst_path)

    def log_folder_gen(self):
        if not os.path.exists(self.log_folder):
            os.makedirs(self.log_folder)

    def src_ini_folder_gen(self):
        if not os.path.exists(self.copy_src_ini_path):
            os.makedirs(self.copy_src_ini_path)

    def copy_src_ini(self):
        # original src ini
        src_ini_file = self.ini
        src_ini_name = os.path.basename(src_ini_file)
        # copy src ini
        self.ini = os.path.join(self.copy_src_ini_path, src_ini_name)

        if os.path.isfile(src_ini_file):
            config_all = configparser.ConfigParser()
            config_all.read(src_ini_file)

            if self.src_ini_to_hw_only_ini:
                for section in config_all.sections():
                    if '_sw' in section:
                        config_all.remove_section(section)

            if self.set_ini2_flag:
                if os.path.isfile(self.ini2):
                    config_all2 = configparser.ConfigParser()
                    config_all2.read(self.ini2)

                    if self.src_ini_to_hw_only_ini:
                        for section in config_all2.sections():
                            if '_sw' not in section:
                                config_all[section] = config_all2[section]
                    else:
                        for section in config_all2.sections():
                            
                            config_all[section] = config_all2[section]

                    self.ini = os.path.splitext(self.ini)[0] + '_' \
                            + os.path.splitext(os.path.basename(self.ini2))[0] \
                            + os.path.splitext(self.ini)[1]
                else:
                    print(f'{bcolors.FAIL}no ini2 file in specific path{bcolors.ENDC}')

            with open(self.ini, 'w', newline='\n') as configfile:
                config_all.write(configfile)

        else:
            print(f'{bcolors.FAIL}no ini file in specific path{bcolors.ENDC}')

    def dbg_top_folder_gen(self):
        if not os.path.exists(self.dbg_top_folder):
            os.makedirs(self.dbg_top_folder)

    def bt_top_folder_gen(self):
        if not os.path.exists(self.bt_top_folder):
            os.makedirs(self.bt_top_folder)

    def dbg_folder_gen(self):
        if not os.path.exists(self.dbg_path):
            os.makedirs(self.dbg_path)

    def bt_folder_gen(self):
        if not os.path.exists(self.bt_path):
            os.makedirs(self.bt_path)

    def dump_ini_folder_gen(self):
        if not os.path.exists(self.dump_ini_folder):
            os.makedirs(self.dump_ini_folder)

    def log_file_remove(self):
        file_list = os.listdir(self.log_folder)
        for item in file_list:
            if item.endswith('.log'):
                os.remove(os.path.join(self.log_folder, item))

    def update_ini(self):
        if self.standalone_ini:
            cur_ini = self.ini
            x = cur_ini.rsplit('_', 1)
            self.ini = x[0] + '_' + f'{base.get_idx():03}' + '.ini'

    def update_dbg_folder(self):
        self.dbg_path = self.dbg_path_base + '_' + f'{base.IMG_IDX:03}'
        self.dbg_folder_gen()

    def update_bt_folder(self):
        self.bt_path = self.bt_path_base + '_' + f'{base.IMG_IDX:03}'
        self.bt_folder_gen()

    def main_proc(self, start, end):
        # processing first image
        base.set_first()
        base.update_idx(start)
        self.update_ini()
        self.update_dbg_folder()
        self.update_bt_folder()
        self.send_cmd_proc()

        # processing other images
        base.set_not_first()
        for idx in range(start+1, end+1, 1):
            base.update_idx(idx)
            self.update_ini()
            self.update_dbg_folder()
            self.update_bt_folder()
            self.send_cmd_proc()

    def send_cmd_proc(self):
        cmd = self.send_cmd()
        ########################## yana.chen modified start ##################################
        if self.reg_AINR_enable:        
            if cmd[0].split("/")[-1] == "rgb2yuv422":
                print("=======================AI NR start===========================")
                rgb_path = cmd[7]
                rgb_AINR_path = cmd[8]
                python_AI_exe = "../../tool/NAFNet/basicsr/demo.py"
                #config = "../../tool/NAFNet/options/test/SIDD/NAFNet-width32.yml"
                config = "../../tool/NAFNet/config/NAFNet-width32.yml"
                AI_cmd = "python3.9 " + python_AI_exe + " -opt " + config + " --input_path " + rgb_path + " --output_path " + rgb_AINR_path
                os.system(AI_cmd)
                print("=======================AI NR end===========================")
        
        parsing_cmd = self.parse_cmd(cmd)
        logging.debug(f'[run_with_python] execute command: {parsing_cmd}\n')
        
        self.exec_cmd(parsing_cmd)

    @abstractmethod
    def send_cmd(self):
        pass

    def parse_cmd(self, cmd_list):
        parsing_cmd = " ".join(map(str, cmd_list))
        return parsing_cmd

    def exec_cmd(self, cmd):    
        ts = time.time()
        returnValue = os.system(cmd)
        print(cmd)

        if returnValue:
            logging.error(f'{self.__class__.__name__} proc. failed')
            print(f'{bcolors.FAIL}{self.__class__.__name__} proc. failed{bcolors.ENDC}')
            os.kill(os.getppid(), signal.SIGTERM)  # kill parent process
            sys.exit(1)

        te = time.time()
        # logging.info(f"{self.__class__.__name__} proc. time:".rjust(24) +
        #              f"{int(1000*(te-ts))}".rjust(6) + f" ms\n")
        logging.info(f"{self.__class__.__name__:>12} proc. time: {int(1000*(te-ts))} ms")


    def post_proc(self):
        pass

    def run(self, start, end):
        self.pre_proc()
        self.main_proc(start, end)
        self.post_proc()

    def gen_common_args(self):
        common_args = [self.bin_file, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.bt_path,
            base.get_first_flag(),  # first frame flag, [frame 0] true, else false
            base.get_single_frame_sim(),
            self.ini,  # cmodel_top
            self.ins_id, self.ini, self.get_dump_ini_file()]
        return common_args
