#!/usr/bin/env python3

import os
import numpy as np
import logging
from enum import Enum


class C_Log_Lv(Enum):
    TRACE = 0  # for pixel level data dump
    DEBUG = 1  # for ip sub function register dump or lut level data dump
    INFO  = 2  # for ip top level register dump, e.g. enable bit
    WARN  = 3  # for the result is not expected, but the data path could keep going
    ERROR = 4  # for the program will crash, highlight the error before crash
    FATAL = 5

c_log_lv_str2int = {
    'trace': C_Log_Lv.TRACE, 'TRACE': C_Log_Lv.TRACE,
    'debug': C_Log_Lv.DEBUG, 'DEBUG': C_Log_Lv.DEBUG,
    'info': C_Log_Lv.INFO, 'INFO': C_Log_Lv.INFO,
    'warn': C_Log_Lv.WARN, 'WARN': C_Log_Lv.WARN,
    'error': C_Log_Lv.ERROR, 'ERROR': C_Log_Lv.ERROR,
    'fatal': C_Log_Lv.FATAL, 'FATAL': C_Log_Lv.FATAL,
    }

py_log_lv_str2log = {
    'debug': logging.DEBUG, 'DEBUG': logging.DEBUG,
    'info': logging.INFO, 'INFO': logging.INFO,
    'warn': logging.WARNING, 'WARN': logging.WARNING,
    'error': logging.ERROR, 'ERROR': logging.ERROR,
    'critical': logging.CRITICAL, 'CRITICAL': logging.CRITICAL,
    }

def img_idx_proc(start, end):
    if end == None:
        end = start
    return start, end

def batch_dst_name_proc(batch_setting):
    for idx, setting in enumerate(batch_setting):
        setting['dst'] = os.path.join(setting['dst'], 'batch_' + f'{idx:02}')

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


def find_index_for_raw(raw_name_list, key_str):
    for i, ele in enumerate(raw_name_list):
        if (key_str in ele):
            return i

def raw_name_parsing(raw_img):
    raw_name_info = {}
    raw_dirname = os.path.dirname(raw_img)
    raw_name = os.path.splitext(os.path.basename(raw_img))[0].lower()  # get raw name without .raw
    raw_name_list = raw_name.split('_')
    new_style = (raw_name[-1] == '_')

    if new_style:
        raw_name_info['time_stamp'] = raw_name_list[3]
    else:
        raw_name_info['time_stamp'] = raw_name_list[-1]

    raw_name_info['raw_dirname'] = raw_dirname
    raw_name_info['raw_name'] = raw_name
    raw_name_info['wdr_flag'] = (raw_name_list[2] == 'wdr')
    raw_name_info['img_width_wdr'] = int(raw_name_list[0].split('x')[0])
    raw_name_info['img_width'] = int(raw_name_info['img_width_wdr'] >> int(raw_name_info['wdr_flag']))
    raw_name_info['img_height'] = int(raw_name_list[0].split('x')[1])

    bit_index = find_index_for_raw(raw_name_list, '-color')
    raw_name_info['bayerid'] = int(raw_name_list[bit_index].split('=')[1])

    bit_index = find_index_for_raw(raw_name_list, '-bits')
    raw_name_info['bit_num'] = int(raw_name_list[bit_index].split('=')[1])

    frame_index = find_index_for_raw(raw_name_list, '-frame')
    raw_name_info['frame_num'] = int(raw_name_list[frame_index].split('=')[1])

    return raw_name_info

def raw2ppm_proc(raw_img):
    raw_name_info = raw_name_parsing(raw_img)
    raw_dirname = raw_name_info['raw_dirname']
    frame_num = raw_name_info['frame_num']
    img_height = raw_name_info['img_height']
    img_width = raw_name_info['img_width']
    img_width_wdr = raw_name_info['img_width_wdr']
    wdr_flag = raw_name_info['wdr_flag']
    bit_num = raw_name_info['bit_num']

    img = np.fromfile(raw_img, dtype=np.uint16)  # raw is [lsb, msb]
    img.byteswap(True)  # ppm is [msb, lsb]
    img = np.reshape(img, (frame_num, img_height, img_width_wdr))

    for frame_idx in range(frame_num):
        src_le_name = os.path.join(raw_dirname, 'src_le_' + f'{frame_idx:03}' + '.ppm')
        src_se_name = os.path.join(raw_dirname, 'src_se_' + f'{frame_idx:03}' + '.ppm')

        with open(src_le_name, 'wb') as le_ppm, open(src_se_name, 'wb') as se_ppm:
            if wdr_flag:
                [img_le, img_se] = np.hsplit(img[frame_idx], 2)
                le_ppm.write(bytearray('P5\n', 'ascii'))
                le_ppm.write(bytearray('# ' + 'LE raw2ppm for run_with_python' + '\n', 'ascii'))
                le_ppm.write(bytearray(f'{img_width} {img_height}\n', 'ascii'))
                le_ppm.write(bytearray(f'{2**bit_num-1}\n', 'ascii'))
                le_ppm.write(img_le.tobytes())

                se_ppm.write(bytearray('P5\n', 'ascii'))
                se_ppm.write(bytearray('# ' + 'SE raw2ppm for run_with_python' + '\n', 'ascii'))
                se_ppm.write(bytearray(f'{img_width} {img_height}\n', 'ascii'))
                se_ppm.write(bytearray(f'{2**bit_num-1}\n', 'ascii'))
                se_ppm.write(img_se.tobytes())
            else:
                img_le = img[frame_idx]
                le_ppm.write(bytearray('P5\n', 'ascii'))
                le_ppm.write(bytearray('# ' + 'LE raw2ppm for run_with_python' + '\n', 'ascii'))
                le_ppm.write(bytearray(f'{img_width} {img_height}\n', 'ascii'))
                le_ppm.write(bytearray(f'{2**bit_num-1}\n', 'ascii'))
                le_ppm.write(img_le.tobytes())

                se_ppm.write(bytearray('P5\n', 'ascii'))
                se_ppm.write(bytearray('# ' + 'SE raw2ppm for run_with_python' + '\n', 'ascii'))
                se_ppm.write(bytearray(f'{img_width} {img_height}\n', 'ascii'))
                se_ppm.write(bytearray(f'{2**bit_num-1}\n', 'ascii'))
                se_ppm.write(np.array([0], dtype=np.uint16).tobytes())
    return frame_num
