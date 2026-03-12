import pytesseract
import pyautogui
import cv2
import numpy as np
import os
import io
import logging
import sys
from time import *
from random import *
import tkinter as tk
import subprocess
import json
from time import sleep
from tkinter import messagebox
from tkinter import filedialog
from PIL import Image
sys.stdout = io.TextIOWrapper(sys.stdout.detach(), encoding='utf-8')
sys.stderr = io.TextIOWrapper(sys.stderr.detach(), encoding='utf-8')

os.environ['isp_tool_autotest'] = 'true'
print(f"set the env var isp_tool_autotest: {os.environ.get('isp_tool_autotest')}")

#Warn Message
def warn(text):
    pyautogui.alert(text, title="auto Warnning", button="OK")
    pass
#warn("开始自动测试\n请注意如下几点:\n一.截图的屏幕跟运行自动化的屏幕要是同一个,避免定位图标失败")

if len(sys.argv) < 5:
    print("Usage: python autoTest.py <path_to_file>")
    sys.exit(1)

pqToolExePath = sys.argv[1]
PqToolLogPath = sys.argv[2]
boardIP = sys.argv[3]
VLCExePath = sys.argv[4]

global_process = None
all_TestNum = 0
compelte_TestNum = 0

rstpIP = f"rtsp://{boardIP}:8554/stream0"
#print(PqToolLogPath)

#Read json file to get the keyword
ErrorKeyJson_path = 'ErrorKey.json'
needToSkipValue = []
try:
    if not os.path.isfile(pqToolExePath):
     print("No Json file in this Path")

    with open('ErrorKey.json', 'r') as file:
        data = json.load(file)
        #Traverse the dictionary
        for value in data.values():
            needToSkipValue.append(value)
            print(value)

except FileNotFoundError as e:
    print(e)

except json.JSONDecodeError as e:
    print(f"Error decoding JSON: {e}")

except Exception as e:
    print(f"An unexpected error occurred: {e}")




def start_process(pqToolExePath):
    global global_process
    try:
        global_process = subprocess.Popen([pqToolExePath])
        logger.info(f"{pqToolExePath} start!")
        print(f" {pqToolExePath} start!")
        sleep(3)
       # process.wait()
    except Exception as e:
        print(f"Error in start CviPQTOOL:{e}")

def close_process(pqToolExePath):
    global global_process
    if global_process:
        global_process.kill()
        global_process = None
        logger.info(f"{pqToolExePath} close!")
#Test日志设置
LogLastPosition = 0 #记录对比的日志行数
#PqToolLogPath = "D:\\Users\\xingyu.feng\\Desktop\\isp_tool\\isp-tool\\log\\log.txt"





logger = logging.getLogger('my_logger')
logger.setLevel(logging.INFO)

console_handler = logging.StreamHandler()
console_handler.setLevel(logging.INFO)

file_handler = logging.FileHandler('my_log.log', encoding= 'utf-8')
file_handler.setLevel(logging.INFO)

formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')

console_handler.setFormatter(formatter)
file_handler.setFormatter(formatter)

logger.addHandler(console_handler)
logger.addHandler(file_handler)


def initmylog(logName):
    with open(logName, 'w') as file:
        pass

initmylog("my_log.log")

#获取图片所在的坐标位置
def findLocate(imageName, x_offset = 0, y_offset = 0, idx = 0, wait = False):
      imagePath = "common/" + imageName + ".png"
      locate =list(pyautogui.locateAllOnScreen(imagePath, confidence=0.85, grayscale=True))
      if(wait):
          # print("WaitMode", len(locate))
           while(len(locate) <= 0):
                 locate =list(pyautogui.locateAllOnScreen(imagePath, confidence=0.9, grayscale=True))
           #如果出现多个相同图标
           if len(locate) != 1 and len(locate) != 0:
             if (idx >= len(locate)):
                warn("屏幕有相同的图片，你想点击的第{}个超出找到的总个数{}".format(idx, len(locate)))
             return locate[idx]
           #
           else:
                locate = locate[0]
                if locate:
                     if x_offset != 0 or y_offset != 0:
                          locate = (locate.left + x_offset, locate.top + y_offset, locate.width, locate.height)
                     return locate
                else:
                     warn("locate {}fail".format(imageName))
                     return locate
      else:
        if len(locate) != 1 and len(locate) != 0:
           if (idx >= len(locate)):
               warn("屏幕有相同的图片，你想点击的第{}个超出找到的总个数{}".format(idx, len(locate)))
           return locate[idx]
        else:
            locate = locate[0]
            if locate:
                if x_offset != 0 or y_offset != 0:
                    locate = (locate.left + x_offset, locate.top + y_offset, locate.width, locate.height)
                return locate
            else:
                warn("locate {} fail".format(imagePath))
                return locate

#执行点击操作
def clickButton(point, Num, t):
       # print(point)
        logger.info("this part location is" + f'{point}')
        if(Num == 1):
            pyautogui.click(point)
        else:
             pyautogui.doubleClick(point)
        sleep(t)

#获取位置并点击
def locateAndClick(imageName, x_offset = 0, y_offset = 0, idx = 0, wait = False, clickThrough = 1, t = 5):
    lock = findLocate(imageName, x_offset, y_offset, idx, wait)
    clickButton(lock, clickThrough,t)
    #compare( isPass = 1)

#顺序执行的逻辑，如果出现前置连接出错，则报错并结束检测
# network not connect情况
allTestFlag = True
#对比日志，检测功能是否正常
def verifyLog(caseName = " "):
    global LogLastPosition
    global compelte_TestNum
    try:
        with open(PqToolLogPath, 'r') as f:
           # print("open file Success")
            f.seek(LogLastPosition)
            for line in f:
                if "ERROR" in line:
                   found_skip = False
                   for Skipvalue in needToSkipValue:
                        if Skipvalue in line:
                            found_skip = True
                            break
                   if not found_skip:
                        logger.error(caseName + " Test Fail")
                        sys.exit()
            LogLastPosition = f.tell()
            compelte_TestNum += 1
            print(compelte_TestNum)
            logger.info("verifyLog" + caseName + "- " + "compelte_TestNum :" + compelte_TestNum)
    except FileNotFoundError:
        print("Error: No Log")
    except PermissionError:
        print("Error: No privilege to read Log")
    except Exception as e:
        print(f"Error in read file:{e}")
    with open('PqToolLogPath', 'w') as f:
         f.write(str( LogLastPosition))

#对比图片弹窗，出现错误信息，需要清除掉warning警告窗口，继续其他的测试
#如果没有warning弹窗，则跳出该函数
#TODO 配合verifylog日志双层验证再跳出



def getIp():
        pyautogui.hotkey('ctrl', 'a')
        pyautogui.write(boardIP)

def getRstp():
    pyautogui.hotkey('ctrl', 'a')
    sleep(2)
    pyautogui.write(rstpIP)
    print(rstpIP)

def verifyVLC():
    VLCErrorPath = "common/pqtool/VLCerror.png"
    global compelte_TestNum
    try:
        VLCError = next(pyautogui.locateAllOnScreen(VLCErrorPath,confidence=0.9, grayscale=True))
    except:
         logger.info("VLC Test PASS")
         compelte_TestNum += 1
         print(compelte_TestNum)
    else:
        logger.error("VLC Test Fail")


def PQTooltest(caseName = ""):
    ImagePath = "pqtool/" + caseName

    #TODO 后续if else过多，采用简单工厂模式封装 or 枚举switch替换
    if(caseName == "connect"):
        locateAndClick("pqtool/IPaddress", clickThrough = 1, t = 2, wait=True)
        getIp()
        sleep(10)
        locateAndClick("pqtool/get", clickThrough = 1, t = 2)
        locateAndClick(ImagePath, clickThrough = 1, t = 10)
        sleep(50) #等待连接

    elif(caseName == "preview"):
        locateAndClick("pqtool/preview", clickThrough = 1, t = 2)
        locateAndClick("pqtool/raw", clickThrough = 1, t = 2)

    elif(caseName == "Capture"):
        locateAndClick("pqtool/Capture", clickThrough = 1, t = 2)
        clickCaptures("pqtool/YUVCapture", clickThrough = 1, t = 2)
        close_process(pqToolExePath)

    elif(caseName == "VLC"):
        start_process(VLCExePath)
        locateAndClick("pqtool/start", clickThrough = 1, t = 2)
        locateAndClick("pqtool/openStream", clickThrough = 1, t = 2)
        print(rstpIP)
        locateAndClick("pqtool/URL", clickThrough = 1, t = 5)
        getRstp()
        locateAndClick("pqtool/P", clickThrough = 1, t = 2)
        sleep(15)
        close_process(VLCExePath)


    else:
        locateAndClick(ImagePath, clickThrough = 1, t = 2)

    if(caseName == "VLC"):
        verifyVLC()
    else:
        verifyLog(caseName)
        logger.info(caseName + " Test PASS")

def exec_error_handler(case_name):
    pqtool_case_ls = ["connect", "preview", "Capture"]
    vlc_case_ls = ["VLC"]
    if case_name in pqtool_case_ls:
        close_process(pqToolExePath)
    elif case_name in vlc_case_ls:
        close_process(VLCExePath)
    else:
        logger.error(f"try to close the case: {case_name}, this case not exist!")
    logger.info(f"exec the case test: {case_name} fail! Exit...")
    sys.exit(1)

def readToTest():
    todoPath = "TOTest/Todo.txt"
    global all_TestNum

    test_tasks_ls = []
    lines = ""

    try:
        with open(todoPath, 'r') as toDoFile:
            lines = toDoFile.readlines()
    except FileNotFoundError:
        logger.error("Error: No Log")

    for line in lines:
        if line.startswith("#"):
            continue
        line = line.strip()
        if len(line) > 0:
            test_tasks_ls.append(line)

    logger.info("---------- test case for isp pqtool ---------------")
    logger.info(f"{', '.join(test_tasks_ls)}")
    logger.info("---------- test case for isp pqtool ---------------")

    all_TestNum = len(test_tasks_ls)

    for Testcase in test_tasks_ls:
        try:
            logger.info("start: " + Testcase)
            PQTooltest(Testcase)
            sleep(8)
        except PermissionError:
            logger.error("Error: No privilege to read Log")
        except Exception as e:
            logger.error(f"Error in read file: {e}")
            exec_error_handler(Testcase)

def clickCaptures(imageName, x_offset = 0, y_offset = 0, idx = 0, clickThrough = 1,t = 15):
     imagePath = "common/" + imageName + ".png"
     matches = list(pyautogui.locateAllOnScreen(imagePath, confidence = 0.9, grayscale = True))
     matches.sort(key = lambda m: m.top)
     for match in matches:
         clickButton(match, 1 , 10)
         locateAndClick("pqtool/ok", clickThrough = 1, t = 2)


start_process( pqToolExePath)
readToTest()

if(compelte_TestNum == all_TestNum):
    logger.info("ISP_PQTOOL test PASS")
else:
    logger.error("ISP_PQTOOL test Fail")
