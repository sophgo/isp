import json
import argparse
import os
from . import utilty
from . import tools

def parserArgs():
    parser = argparse.ArgumentParser(description='GEN_INI')
    parser.add_argument('--json_folder_path', type=str, default='D:\\Users\\huichong.li\\Documents\\ISP\\para\\mw2cmodel_transfer\\dump_20220628163858\\',
                    help='json folder path')
    parser.add_argument('--ppl_path', type=str ,default='D:\\Users\\huichong.li\\Documents\\ISP\\para\\mw2cmodel_transfer\\test_json\\1920X1080_RGGB_Linear_20220628164140_00_-color=3_-bits=12_-frame=1_-hdr=1_ISO=0_.json',
                    help='pipeline address')
    parser.add_argument('--add_reg_path', type=str, default='..\\add_reg_20230322.json',
                    help='address reg file address')
    parser.add_argument('--out_ini_path', type=str, default='..\\genini\\',
                    help='output ini file address')
    parser.add_argument('--draft_ini_path', type=str, default='..\\cmodel_ini\\20211119183132_Mars.ini',
                    help='output ini file address')
    parser.add_argument('--write', type=bool, default=True, help='write in ini file or not')
    parser.add_argument('--chip', type=str, default='mars', help='input chip name')

    return parser.parse_args()


def read_json(ppl_path, add_reg_path):
    '''
    读取json文件，拿到寄存器中的地址以及extra lut中的数据
    :param ppl_path:
    :param add_reg_path:
    :return:
    '''
    dma_add = []
    # a2 need
    print("---------------------",os.getcwd())
    with open("../common/pre_process/dma_add.txt", "r") as F:
        for line in F.readlines():
            dma_add.append(line.strip())
    # end

    print(add_reg_path)
    with open(add_reg_path) as f:
        add_reg = json.load(f)  # add_reg

    json_data = []
    print(ppl_path)
    for jpath in ppl_path:
        with open(jpath) as F:
            add_data = json.load(F)  # add_data
            json_data.append(add_data)
    j1 = None
    j2 = None
    for jdata in json_data:
        if "rgb_gamma" in jdata.keys():
            j2 = jdata
        if "rgb_gamma_r" in jdata.keys():
            j1 = jdata

    res = []
    lut = []
    sub_add_not_used = []
    for base_add in j1.keys():
        if not base_add.startswith("0"):
            if base_add != "end":
                lut.append((base_add, ",  ".join([str(x) for x in j1[base_add]["lut"]])))
            continue
        sub_data_info = j1[base_add]
        if base_add in dma_add:
            continue
        sub_reg_info = add_reg[base_add]
        for sub_key in sub_data_info.keys():
            # print(sub_data_info[sub_key])
            if sub_key == "size":  # size不考虑
                continue
            #             print(sub_key in sub_reg_info.keys())
            if sub_key not in sub_reg_info.keys():  # 不在reg文件中的地址且数据等于0的暂不考虑
                if sub_data_info[sub_key] != 0:
                    sub_add_not_used.append((base_add, sub_key))
                    continue
                continue
            data = sub_data_info[sub_key]
            reg_field = sub_reg_info[sub_key]
            mode_name = sub_reg_info["module"]
            # if "wbg" in mode_name and sub_key == "h08":
            #     print(mode_name, data)
            #             print((data, reg_field, mode_name))
            res.append((data, reg_field, mode_name))
    #     print(res)

    if j2:
        for base_add in j2.keys():
            if base_add in ["rgb_gamma", "y_gamma", "dci_gamma"]:
                lut.append((base_add, ",  ".join([str(x) for x in j2[base_add]["lut"]])))
            if base_add in ["ca_cp_y", "ca_y_ratio"]:
                lut.append((base_add + "_debug", ",  ".join([str(x) for x in j2[base_add]["lut"]])))
    return res, lut


def preProcess(regJsonPath, templateIniPath, chip, inputDir, outputDir):
    #prepare
    json_path = []
    raw_img = None
    ini_img = None
    ini_exist = False
    ppm_exist = False
    txt_name = None
    for file in os.listdir(inputDir):
        if file.endswith('.json'):
            ini_img = file
            frame = tools.frame_config(file)
            json_path.append(os.path.join(inputDir, file))
        if file.endswith('.raw'):
            raw_img = file
        if file.endswith('.ppm'):
            ppm_exist = True
        if file.endswith('.ini'):
            ini_exist = True
        if file.endswith('.txt') and "log" not in file and "Info" not in file:
            print(file)
            txt_name = file
    #parse raw info
    if txt_name:
        txt_path = os.path.join(inputDir, txt_name)
        iso, lv = tools.parse_txt(txt_path)
    parse_info = None
    if raw_img:
        parse_info = raw_img
    elif ini_img:
        parse_info = ini_img
    else:
        print("There is no json file or raw file!!!")
        exit()
    if txt_name:
        raw_name_info = utilty.raw_name_parsing(parse_info, iso, lv)
    else:
        raw_name_info = utilty.raw_name_parsing(parse_info)
    print(raw_name_info)
    #raw to ppm
    if not ppm_exist and raw_img != None:
        raw_img = os.path.join(outputDir, raw_img)
        utilty.raw2ppm_np(raw_img, raw_name_info, outputDir)
        print("------generating ppm finish!------")
    #json to cmodel ini
    if not ini_exist:
        iniFile = outputDir+"/"+raw_name_info["time_stamp"]+"_{}".format(chip.capitalize())+".ini"
        content = read_json(json_path, regJsonPath)
        tools.write_ini(content, 1, templateIniPath, iniFile, raw_name_info)
        print("------generating ini finish!------")

if __name__=='__main__':
    #read args
    args = parserArgs()
    #run
    regJsonPath = args.add_reg_path
    templateIniPath = args.draft_ini_path
    inputDir = args.json_folder_path
    outputDir = args.out_ini_path
    chip = args.chip
    preProcess(regJsonPath, templateIniPath, chip, inputDir, outputDir)
