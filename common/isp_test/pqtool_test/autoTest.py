import pytesseract
import pyautogui
import numpy as np
import os
import io
import logging
import sys
import time
from random import *
import tkinter as tk
import subprocess
import json
from tkinter import messagebox
from tkinter import filedialog
from PIL import Image
import yaml
import shutil

sys.stdout = io.TextIOWrapper(sys.stdout.detach(), encoding='utf-8')
sys.stderr = io.TextIOWrapper(sys.stderr.detach(), encoding='utf-8')

os.environ['isp_tool_autotest'] = 'true'
# print(f"set the env var isp_tool_autotest: {os.environ.get('isp_tool_autotest')}")

class TEST_APP:
    pqtool_log_position = 0

    def __init__(self, apps_info_dict, test_cases, skip_value_ls, pqtool_log_path, logger):
        self.apps_info_dict = apps_info_dict
        self.test_cases = test_cases
        self.logger = logger
        self.pqtool_log_path = pqtool_log_path
        self.skip_value_ls = skip_value_ls

    def run_app_test(self):
        # get app information
        for app_name, app_info in self.apps_info_dict.items():
            app_path = app_info["path"]
            self._start_app(app_name)
            time.sleep(2)
            # case
            cases = self.test_cases[app_name]
            for case, steps in cases.items():
                # step
                for op_step, vals in steps.items():
                    input_str = ""
                    img_name = ""
                    input_str = ""
                    sleep_secs = ""

                    # op_step: sleep
                    if "sleep" in op_step:
                        try:
                            time.sleep(int(vals))
                        except:
                            time.sleep(1)
                        continue

                    vals_ls = vals.split(",")
                    img_name = vals_ls[0].strip()

                    if len(vals_ls) >= 2:
                        input_str = vals_ls[1].strip()
                    img_path_ls = self._get_img_path(app_name, case, img_name)
                    if len(img_path_ls) == 0:
                        pass

                    for img_path in img_path_ls:
                        self.logger.debug(img_path)
                        # input: low confidence is ok
                        if "input" in op_step:
                            point = self._locate_img(img_path, 0.6, 0, 0)
                        else:
                            point = self._locate_img(img_path, 0.8, 0, 0)
                        if point == None:
                            if img_path == img_path_ls[-1] and "check" not in op_step:
                                self.logger.debug(f"locate {app_name}-{case}-{op_step}-{img_name} fail!")
                                self._test_fail_handler(app_name, case, is_exit=True)
                            else:
                                continue
                        time.sleep(1)
                        self._test_app_item(op_step, point, input_str)
                        break
                if app_name == "pqtool":
                    self._verify_pqtool_log(app_name, case)
                self._test_success_handler(app_name, case, is_exit=False)
        # stop the process
        self.logger.info("ISP_PQTOOL test PASS")
        time.sleep(10)
        for app_name, _ in self.apps_info_dict.items():
            self._stop_app(app_name)

    def _verify_pqtool_log(self, app_name, case):
        with open(self.pqtool_log_path) as f:
            f.seek(TEST_APP.pqtool_log_position)
            for line in f:
                if "ERROR" in line:
                    found_skip = False
                    for skip_value in self.skip_value_ls:
                        if skip_value in line:
                            found_skip = True
                            break
                    if not found_skip:
                        self.logger.error("fail!")
                        self._test_fail_handler(app_name, case, is_exit=True)
            TEST_APP.pqtool_log_position = f.tell()

    def _get_img_path(self, app_name, case, img):
        case_dir = os.path.join("image", app_name, case)
        img_name_all_ls = os.listdir(case_dir)
        img_name_ls = [img_name for img_name in img_name_all_ls if img in img_name]

        if len(img_name_ls) == 0:
            return []
        else:
            return [os.path.join(case_dir, img_name) for img_name in img_name_ls]

    def _test_app_item(self, op_type, point, input_str):
        if "click" in op_type:
            self._op_click(point)
        elif "input" in op_type:
            self._op_input(point, input_str)
            time.sleep(1)
        elif "check" in op_type:
            self._op_check(point)
        else:
            self.logger.debug(f"{op_type} is not supported!")

    def _get_app_test_case(self, test_case_path):
        test_cases = dict()

        if not os.path.exists(test_case_path):
            self.logger.debug(f"{test_case_path} not exists!")
        else:
            with open(test_case_path, 'r') as file:
                test_cases = yaml.safe_load(file)
        return test_cases

    # locate
    def _locate_img(self, img_path, conf_thres, x_offset = 0, y_offset = 0):
        point = None
        try:
            point_ls = list(pyautogui.locateAllOnScreen(img_path, confidence = conf_thres, grayscale=True))
            if len(point_ls) > 0:
                point = point_ls[-1]
                point = (point.left + x_offset, point.top + y_offset, point.width, point.height)
        except Exception as e:
            self.logger.debug(f"{img_path} locate failed: {e}!")
            if "vlc" in img_path:
                print(f"debug: {img_path} locate failed: {e}!")

        return point

    # operation
    def _op_click(self, point, is_double_click=False):
        for i in range(2):
            pyautogui.moveTo(point[0] + point[2] / 2, point[1] + point[3] / 2)
            pyautogui.click()
            if not is_double_click:
                break

    def _op_input(self, point, input_content):
        # deal the input_content
        self._op_click(point)
        pyautogui.hotkey('ctrl', 'a')
        pyautogui.write(input_content)

    def _op_check(self, point):
        if point == None:
            # no error
            pass
        else:
            # error
            self.logger.error("check fail!")
            self._test_fail_handler(is_exit=True)
        time.sleep(2)

    # handler
    def _test_fail_handler(self, app_name, case, is_exit=True):
        self.logger.error(f"{app_name}-{case}: FAIL!")
        if is_exit:
            self.logger.error("ISP_PQTOOL test Fail")
            for key, val in self.apps_info_dict.items():
                self._stop_app(key)
            exit(1)

    def _test_success_handler(self, app_name, case, is_exit=True):
        self.logger.info(f"{app_name}-{case}: PASS!")
        if is_exit:
            for key, val in self.apps_info_dict.items():
                self._stop_app(key)
            exit(1)

    def _start_app(self, app_name):
        for key, val in self.apps_info_dict.items():
            if key == app_name:
                exe_path = val["path"]
                if not os.path.exists(exe_path) or not exe_path.endswith(".exe"):
                    break
                if "process" in val and val["process"] != None:
                    break
                # change dir
                #current_dir = os.getcwd()
                #os.chdir(os.path.dirname(exe_path))
                app_process = subprocess.Popen(exe_path)
                #os.chdir(current_dir)
                if app_name == "vlc":
                    #time.sleep(100000)
                    pass
                val["process"] = app_process
                break

    def _stop_app(self, app_name):
        for key, val in self.apps_info_dict.items():
            if key == app_name:
                if "process" in val and val["process"] != None:
                    val["process"].kill()
                    val["process"] = None
        # delete the file directroy
        #trash_dir_ls = ["dump_", "yuv_", "raw_"]
        #app_dir = os.path.dirname(self.apps_info_dict[app_name]["path"])
        #current_dir = os.getcwd()
        #os.chdir(app_dir)
        #dir_file_ls = os.listdir(app_dir)
        #try:
        #    for dir_file in dir_file_ls:
        #        for trash_dir in trash_dir_ls:
        #            if trash_dir in dir_file and os.path.isdir(dir_file):
        #                shutil.rmtree(os.path.join(app_dir, dir_file))
        #except:
        #    pass
        #os.chdir(current_dir)

class LOGGER:
    def __init__(self, logger_name, logger_level):
        self.logger = logging.getLogger(logger_name)
        self.logger.setLevel(logger_level)

    def set_console_handler(self, logger_level, logger_format):
        formatter = logging.Formatter(logger_format)
        console_handler = logging.StreamHandler()
        console_handler.setLevel(logger_level)
        console_handler.setFormatter(formatter)
        self.logger.addHandler(console_handler)

    def set_file_handler(self, log_file, logger_level, logger_format):
        with open(log_file, "w") as f:
            pass
        formatter = logging.Formatter(logger_format)
        file_handler = logging.FileHandler(log_file, encoding = 'utf-8')
        file_handler.setLevel(logger_level)
        file_handler.setFormatter(formatter)
        self.logger.addHandler(file_handler)

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print("Too few arguments!")
        print("Usage: python autoTest_v1.py <pqtool_exe_path> <pqtool_log_pqth> <board_ip> <vlc_exe_path>")
        print("example: python .\autoTest_v1.py C:\\data\\CviPQtool_20240401\\CviPQTool.exe C:\\data\\CviPQtool_20240401\\log.txt 192.168.1.101 C:\\software\\VLC\\vlc.exe")
        exit(1)

    pqtool_exe_path = sys.argv[1]
    pqtool_log_path = sys.argv[2]
    #board_ip = "192.168.1.101"
    board_ip = sys.argv[3].strip()
    vlc_exe_path = sys.argv[4]

    vlc_url = f"rtsp://{board_ip}:8554/stream0"

    app_list_info = {
        "pqtool": {"path": "C:\\data\\CviPQtool_20240401\\CviPQTool.exe"},
        "vlc": {"path": "C:\\software\\VLC\\vlc.exe"}
    }

    app_list_info["pqtool"]["path"] = pqtool_exe_path
    app_list_info["vlc"]["path"] = vlc_exe_path

    # init logger
    logger = LOGGER("LOG", logging.INFO)
    logger_formatter = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
    logger.set_console_handler(logging.DEBUG, logger_formatter)
    logger.set_file_handler("my_log.log", logging.INFO, logger_formatter)

    # repace the input str
    test_case_path = "./ToTest/test_case.yaml"
    test_cases = dict()
    if not os.path.exists(test_case_path):
        print(f"{test_case_path} not exists!")
    else:
        with open(test_case_path, 'r') as file:
            test_cases = yaml.safe_load(file)
    # replace the input
    replace_str = ""
    for app_name, app_info in test_cases.items():
        cases = test_cases[app_name]
        for case, steps in cases.items():
            for op_step, vals in steps.items():
                if "input" in op_step:
                    vals_ls = [i.strip() for i in vals.split(",")]
                    if len(vals_ls) >= 2 and vals_ls[1].startswith("{") and vals_ls[1].endswith("}"):
                        var = vals_ls[1][1:-1]
                        exec(f"replace_str = vals.replace(vals_ls[1], {var})")
                        test_cases[app_name][case][op_step] = replace_str

    # get the skip value ls
    skip_value_ls = []
    skip_value_json_path = "./ErrorKey.json"
    try:
        with open(skip_value_json_path, "r") as f:
            data = json.load(f)
            for value in data.values():
                skip_value_ls.append(value)
    except:
        pass

    # create the test case object and then run
    test_case = TEST_APP(app_list_info, test_cases, skip_value_ls, pqtool_log_path, logger.logger)
    test_case.run_app_test()
