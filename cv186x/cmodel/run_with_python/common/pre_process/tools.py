def modify_base_addr(base_add):
    new_add = "0x" + base_add[1:].upper()
#     new_base = new_add[:3] + "0" + new_add[4:]
    return new_add


def parse_txt(txt_path):
    iso = None
    lv = None
    with open(txt_path, "r") as F:
        for line in F.readlines():
            tmp = line.strip().split(" = ")
            if line.startswith("ISO"):
                iso = float(tmp[-1])
            if line.startswith("Light Value"):
                lv = float(tmp[-1])*100
            continue
        if not iso:
            print("lack of ISO Value!!!")
            exit()
        if not lv:
            print("lack of Light Value!!!")
            exit()
        return iso, lv


def check_sub_add(sub_add):
    '''
    修改地址格式
    :param sub_add: 地址偏移
    :return:
    '''
    if len(sub_add)==4:
        if sub_add[1]=="0":
            sub_add = "h" + sub_add[2:]
    return sub_add


# def write_ini(path, reg_data):
#     with open(path, "w") as f:
#         for (reg_name, data) in reg_data:
#             f.write("{} = {}\n".format(reg_name, data))
#     f.close()

# added by huichong
def check_gamma(test_ini):
    if not (test_ini["[extra lut]"]["reg_ygamma_lut"] == test_ini["[extra lut]"]["reg_y_gamma_g"] == test_ini["[extra lut]"]["reg_y_gamma_b"]):
        print("YGAMMA not same LUT VALUE!!!")
    if not (test_ini["[extra lut]"]["reg_gamma_lut"] == test_ini["[extra lut]"]["reg_rgb_gamma_g"] == test_ini["[extra lut]"]["reg_rgb_gamma_b"]):
        print("RGBGAMMA not same LUT VALUE!!!")
    if not (test_ini["[extra lut]"]["reg_dci_map_lut"] == test_ini["[extra lut]"]["reg_dci_gamma_g"] == test_ini["[extra lut]"]["reg_dci_gamma_b"]):
        print("DCIGAMMA not same LUT VALUE!!!")


def upddate_sp(test_ini):
    sp_reg = [("[rgbmap0]", "reg_rgbmap_w_bit"), ("[rgbmap0]", "reg_rgbmap_h_bit"),
              ("[rgbmap0]", "reg_rgbmap_w_grid_num"), ("[rgbmap0]", "reg_rgbmap_h_grid_num"),
              ("[rgbmap1]", "reg_rgbmap_w_bit"), ("[rgbmap1]", "reg_rgbmap_h_bit"),
              ("[rgbmap1]", "reg_rgbmap_w_grid_num"), ("[rgbmap1]", "reg_rgbmap_h_grid_num"),
              ("[lmap0]", "reg_lmap_w_bit"), ("[lmap0]", "reg_lmap_h_bit"),
              ("[lmap1]", "reg_lmap_w_bit"), ("[lmap1]", "reg_lmap_h_bit"),
              ("[ynr]", "reg_ynr_weight_lut"), 
              ("[lsc]", "reg_lsc_rtbl0"), ("[lsc]", "reg_lsc_gtbl0"), 
              ("[lsc]", "reg_lsc_btbl0"), ("[lsc]", "reg_lsc_rtbl1"), 
              ("[lsc]", "reg_lsc_gtbl1"), ("[lsc]", "reg_lsc_btbl1"), 
              ("[dpc0]", "reg_static_defect"), ("[dpc1]", "reg_static_defect")]

    sp_reg_sp = [("[pre_raw_fe0]", "reg_le_rgbmp_h_grid_size"), ("[pre_raw_fe0]", "reg_le_rgbmp_v_grid_size"),
                ("[pre_raw_fe0]", "reg_le_rgbmp_h_grid_numm1"), ("[pre_raw_fe0]", "reg_le_rgbmp_v_grid_numm1"),
                ("[pre_raw_fe1]", "reg_le_rgbmp_h_grid_size"), ("[pre_raw_fe1]", "reg_le_rgbmp_v_grid_size"),
                ("[pre_raw_fe1]", "reg_le_rgbmp_h_grid_numm1"), ("[pre_raw_fe1]", "reg_le_rgbmp_v_grid_numm1"),
                ("[ltm]", "reg_lmap_w_bit"), ("[ltm]", "reg_lmap_h_bit"),
                ("[ltm]", "reg_lmap_w_bit"), ("[ltm]", "reg_lmap_h_bit"),
                ("[extra lut]", "reg_ynr_weight_lut"), 
                ("[extra lut]", "reg_mlsc_gain_r"), ("[extra lut]", "reg_mlsc_gain_g"), 
                ("[extra lut]", "reg_mlsc_gain_b"), ("[extra lut]", "reg_mlsc_gain_r"), 
                ("[extra lut]", "reg_mlsc_gain_g"), ("[extra lut]", "reg_mlsc_gain_b"), 
                ("[extra lut]", "reg_dpc_le_bp_tbl"), ("[extra lut]", "reg_dpc_se_bp_tbl")]
    length = len(sp_reg)
    # dehaze sp
    name_list = []
    for i in range(129):
        zero_len = 3 - len(str(i))
        name_list.append("reg_dehaze_tmap_gain_lut"+"0"*zero_len+str(i))
    # print(test_ini["[dehaze]"])
    test_ini["[dehaze]"]["reg_dehaze_tmap_gain_lut"] = ", ".join([test_ini["[dehaze]"][name] for name in name_list])
    for idx in range(length):
        old_mode = sp_reg[idx][0]
        old_reg = sp_reg[idx][1]

        new_mode = sp_reg_sp[idx][0]
        new_reg = sp_reg_sp[idx][1]
        if new_reg in test_ini[new_mode]:
            # print(new_mode, new_reg)
            test_ini[old_mode][old_reg] = test_ini[new_mode][new_reg]
    check_gamma(test_ini)
    # add pre_img_bayerid
    # if "reg_pre_img_bayerid" not in test_ini["[cmodel_top]"].keys():
    #     test_ini["[cmodel_top]"]["reg_pre_img_bayerid"] = test_ini["[cmodel_top]"]["reg_img_bayerid"]
    return test_ini


def upddate_sp_a2(test_ini):
    sp_reg = [("[rgbmap0]", "reg_rgbmap_w_bit"), ("[rgbmap0]", "reg_rgbmap_h_bit"),
              ("[rgbmap0]", "reg_rgbmap_w_grid_num"), ("[rgbmap0]", "reg_rgbmap_h_grid_num"),
              ("[rgbmap1]", "reg_rgbmap_w_bit"), ("[rgbmap1]", "reg_rgbmap_h_bit"),
              ("[rgbmap1]", "reg_rgbmap_w_grid_num"), ("[rgbmap1]", "reg_rgbmap_h_grid_num"),
              ("[lmap0]", "reg_lmap_w_bit"), ("[lmap0]", "reg_lmap_h_bit"),
              ("[lmap1]", "reg_lmap_w_bit"), ("[lmap1]", "reg_lmap_h_bit"),
              ("[ynr]", "reg_ynr_weight_lut"), 
              ("[lsc0]", "reg_lsc_rtbl0"), ("[lsc0]", "reg_lsc_gtbl0"), 
              ("[lsc0]", "reg_lsc_btbl0"), ("[lsc0]", "reg_lsc_rtbl1"), 
              ("[lsc0]", "reg_lsc_gtbl1"), ("[lsc0]", "reg_lsc_btbl1"), 
              ("[lsc1]", "reg_lsc_rtbl0"), ("[lsc1]", "reg_lsc_gtbl0"), 
              ("[lsc1]", "reg_lsc_btbl0"), ("[lsc1]", "reg_lsc_rtbl1"), 
              ("[lsc1]", "reg_lsc_gtbl1"), ("[lsc1]", "reg_lsc_btbl1"), 
              ("[dpc0]", "reg_static_defect"), ("[dpc1]", "reg_static_defect")]

    sp_reg_sp = [("[pre_raw_fe0]", "reg_le_rgbmp_h_grid_size"), ("[pre_raw_fe0]", "reg_le_rgbmp_v_grid_size"),
                ("[pre_raw_fe0]", "reg_le_rgbmp_h_grid_numm1"), ("[pre_raw_fe0]", "reg_le_rgbmp_v_grid_numm1"),
                ("[pre_raw_fe1]", "reg_le_rgbmp_h_grid_size"), ("[pre_raw_fe1]", "reg_le_rgbmp_v_grid_size"),
                ("[pre_raw_fe1]", "reg_le_rgbmp_h_grid_numm1"), ("[pre_raw_fe1]", "reg_le_rgbmp_v_grid_numm1"),
                ("[ltm]", "reg_lmap_w_bit"), ("[ltm]", "reg_lmap_h_bit"),
                ("[ltm]", "reg_lmap_w_bit"), ("[ltm]", "reg_lmap_h_bit"),
                ("[extra lut]", "reg_ynr_weight_lut"), 
                ("[extra lut]", "reg_mlsc_gain_r"), ("[extra lut]", "reg_mlsc_gain_g"), 
                ("[extra lut]", "reg_mlsc_gain_b"), ("[extra lut]", "reg_mlsc_gain_r"), 
                ("[extra lut]", "reg_mlsc_gain_g"), ("[extra lut]", "reg_mlsc_gain_b"), 
                ("[extra lut]", "reg_mlsc_gain_r"), ("[extra lut]", "reg_mlsc_gain_g"), 
                ("[extra lut]", "reg_mlsc_gain_b"), ("[extra lut]", "reg_mlsc_gain_r"), 
                ("[extra lut]", "reg_mlsc_gain_g"), ("[extra lut]", "reg_mlsc_gain_b"), 
                ("[extra lut]", "reg_dpc_le_bp_tbl"), ("[extra lut]", "reg_dpc_se_bp_tbl")]
    length = len(sp_reg)
    # dehaze sp
    name_list = []
    for i in range(129):
        zero_len = 3 - len(str(i))
        name_list.append("reg_dehaze_tmap_gain_lut"+"0"*zero_len+str(i))
    # print(test_ini["[dehaze]"])
    test_ini["[dehaze]"]["reg_dehaze_tmap_gain_lut"] = ", ".join([test_ini["[dehaze]"][name] for name in name_list])
    for idx in range(length):
        old_mode = sp_reg[idx][0]
        old_reg = sp_reg[idx][1]

        new_mode = sp_reg_sp[idx][0]
        new_reg = sp_reg_sp[idx][1]
        if new_reg in test_ini[new_mode]:
            # print(new_mode, new_reg)
            test_ini[old_mode][old_reg] = test_ini[new_mode][new_reg]
    check_gamma(test_ini)
    # add pre_img_bayerid
    # if "reg_pre_img_bayerid" not in test_ini["[cmodel_top]"].keys():
    #     test_ini["[cmodel_top]"]["reg_pre_img_bayerid"] = test_ini["[cmodel_top]"]["reg_img_bayerid"]
    return test_ini


def combine(test_res, draft_path, frame):
    '''
    将test json得到的ini与cmodel的模板结合，更新模板中的数据，并补充新的数据，得到最终可以跑cmodel的ini
    :param test_content: test json得到的ini内容
    :param draft_path: cmodel的ini模板路径
    :return:
    '''
    test_ini = {}
    # 修改json文件中与ini中模块名不对应的部分
    for i, line in enumerate(test_res):
        if line.strip().startswith("["):
            module_name = line.strip()
            # print(module_name)
            if module_name == "[hdrfusion]":
                module_name = "[fusion]"
            if module_name == "[hdrltm]":
                module_name = "[ltm]"
            if module_name == "[hist_v]":
                module_name = "[hist_edge_v]"
            if module_name == "[rgbdither]":
                module_name = "[rgb_dither]"
            if module_name == "[yuvdither]":
                module_name = "[yuv_dither]"
            if module_name == "[pre_ee]":
                module_name = "[ee_pre]"
            if module_name == "[dhz]":
                module_name = "[dehaze]"
            if module_name == "[ee]":
                module_name = "[ee_post]"
            if module_name == "[ycurve]":
                module_name = "[ycur]"
            if module_name == "[aehist0]":
                module_name = "[ae0]"
                # print(module_name)
            if module_name == "[aehist1]":
                module_name = "[ae1]"
            j = i + 1
            module = {}
            while j < len(test_res) and (test_res[j].strip().startswith("r") or test_res[j].strip().startswith("f")):
                equ = test_res[j].find("=")
                # 修改参数名，json与ini中一致
                reg_name = test_res[j][:equ - 1].strip()
                if reg_name == "reg_y_gamma_r":
                    reg_name = "reg_ygamma_lut"
                if reg_name == "reg_rgb_gamma_r":
                    reg_name = "reg_gamma_lut"
                if reg_name == "reg_clut_r":
                    reg_name = "reg_clut_r_lut"
                if reg_name == "reg_clut_g":
                    reg_name = "reg_clut_g_lut"
                if reg_name == "reg_clut_b":
                    reg_name = "reg_clut_b_lut"
                if reg_name == "reg_ltm_dtone_curve":
                    reg_name = "reg_ltm_dark_tone_lut"
                if reg_name == "reg_ltm_btone_curve":
                    reg_name = "reg_ltm_bright_tone_lut"
                if reg_name == "reg_ltm_global_curve":
                    reg_name = "reg_ltm_global_tone_lut"
                if reg_name == "reg_ycurve":
                    reg_name = "reg_ycur_lut"
                if reg_name == "reg_ca_y_ratio":
                    reg_name = "reg_ca_ca_y_ratio_lut"
                if reg_name == "reg_ca_cp_y":
                    reg_name = "reg_ca_cp_y_lut"
                if reg_name == "reg_ca_cp_u":
                    reg_name = "reg_ca_cp_u_lut"
                if reg_name == "reg_ca_cp_v":
                    reg_name = "reg_ca_cp_v_lut"
                if reg_name == "reg_dci_gamma_r":
                    reg_name = "reg_dci_map_lut"
                value = test_res[j][equ + 2:]
                if reg_name == "reg_se_in_sel" and value.strip() == "1":
                    # print("reg_se_in_sel = 1")
                    module["reg_fs_enable"] = "0"
                module[reg_name] = value.strip()
                j += 1
            test_ini[module_name] = module
    # added by huichong
    test_ini = upddate_sp_a2(test_ini)
    # end

    ini = []
    not_in_json = []
    # path = "D:\\Users\\huichong.li\\Documents\\ISP\\para\\mw2cmodel_transfer\\cmodel_ini\\20211119183132_Mars.ini"
    # path = "D:\\Users\\huichong.li\\Documents\\ISP\\para\\mw2cmodel_transfer\\cmodel_ini\\cfa.ini"
    with open(draft_path, "r") as f:
        ini_dra = f.readlines()
    for i, line in enumerate(ini_dra):
        if line.strip().startswith("["):
            written = []
            module_name = line.strip()
            ini.append(module_name + "\n")
            j = i + 2
            while j < len(ini_dra) and (ini_dra[j].strip().startswith("r") or ini_dra[j].strip().startswith("sw")):
                tmp = ini_dra[j].strip()
                
                while j + 1 < len(ini_dra) and ini_dra[j + 1].startswith("\t"):
                    tmp = tmp + " " + ini_dra[j + 1].strip()
                    j += 1
                j += 1
                equ = tmp.find("=")
                reg_name = tmp[:equ - 1].strip()
                value = tmp[equ + 2:]
                if reg_name == "reg_img_bayerid":
                    value = frame["bayerid"]
                if reg_name == "reg_img_width":
                    value = frame["img_width"]
                if reg_name == "reg_img_height":
                    value = frame["img_height"]
                if reg_name == "reg_hdr_enable":
                    value = int(frame["wdr_flag"])

                # if reg_name == "reg_wbg_enable":
                #     print(module_name, test_ini[module_name][reg_name], value)
                if module_name not in test_ini:
                    # print(module_name)
                    # print(tmp)
                    ini.append("{} = {}\n".format(reg_name, value))
                    written.append(reg_name)
                    not_in_json.append((module_name, reg_name, value))
                    continue
                else:
                    test_content = test_ini[module_name]
                    if reg_name not in test_content and reg_name not in test_ini["[extra lut]"]:
                       #  print(module_name, reg_name, value)
                        ini.append("{} = {}\n".format(reg_name, value))
                        not_in_json.append((module_name, reg_name, value))
                    else:
                        if reg_name in test_content:
                            ini.append("{} = {}\n".format(reg_name, test_content[reg_name]))
                        elif reg_name in test_ini["[extra lut]"]:
                            #                         print(reg_name, test_ini["[extra lut]"][reg_name])
                            ini.append("{} = {}\n".format(reg_name, test_ini["[extra lut]"][reg_name]))
                    written.append(reg_name)
            #                 if module_name == "[cfa]":
            #                     print(written)
            if module_name in test_ini:
                test_content = test_ini[module_name]
                for key in test_content.keys():
                    if key.strip() not in written:
                        ini.append("{} = {}\n".format(key.strip(), test_content[key]))

            # add debug part
            if module_name == "[rgbgamma]" and "reg_rgb_gamma" in test_ini["[extra lut]"].keys():
                ini.append("{} = {}\n".format("reg_rgb_gamma", test_ini["[extra lut]"]["reg_rgb_gamma"]))
            if module_name == "[ygamma]" and "reg_y_gamma" in test_ini["[extra lut]"].keys():
                ini.append("{} = {}\n".format("reg_y_gamma", test_ini["[extra lut]"]["reg_y_gamma"]))
            if module_name == "[dci]" and "reg_dci_gamma" in test_ini["[extra lut]"].keys():
                ini.append("{} = {}\n".format("reg_dci_gamma", test_ini["[extra lut]"]["reg_dci_gamma"]))
            if module_name == "[ca]" and "reg_ca_cp_y_debug" in test_ini["[extra lut]"].keys():
                ini.append("{} = {}\n".format("reg_ca_cp_y", test_ini["[extra lut]"]["reg_ca_cp_y_debug"]))
            if module_name == "[ca]" and "reg_ca_y_ratio_debug" in test_ini["[extra lut]"].keys():
                ini.append("{} = {}\n".format("reg_ca_y_ratio", test_ini["[extra lut]"]["reg_ca_y_ratio_debug"]))
    return ini


def write_ini(data_reg, write, draft_path, path, frame):
    '''

    :param data_reg: （module data, extra lut）test json parse得到的数据
    :param write: 是否写入ini文件
    :param draft_path: cmodel ini模板路径
    :param path: 写入ini的路径
    :return:
    '''
    data_result = []
    written = []
    last = ""
    for data, reg, mode_name in data_reg[0]:
        if mode_name not in written:
            data_result.append("[{}]\n".format(mode_name))
            written.append(mode_name)
            last = mode_name
        else:
            if mode_name != last:
                data_result.append("[{}]\n".format(mode_name))
                last = mode_name
        for name, lsb, msb, signed in reg:
            lsb = int(lsb)
            msb = int(msb)
            bit = msb-lsb+1
            # print(name)
            # if "lut_slp" in name:
            #     print(name)
            data_dec = (data>>lsb) & (2**bit-1)
            # if name == "reg_wbg_enable":
            #     print(mode_name, name, lsb, msb, bit, data)
            if name == "reg_img_bayerid":
                data_dec = frame["bayerid"]
            data_result.append("{} = {}\n".format(name, data_dec))
    # exit()
            # if name == "reg_basel":
            #     print(data, lsb, msb)
    data_result.append("[extra lut]")
    for line in data_reg[1]:
        data_result.append("{} = {}\n".format("reg_"+line[0], line[1]))
    # print(data_result)
    ini = combine(data_result, draft_path, frame)
    if write:
        with open(path, "w") as f:
            for i, line in enumerate(ini):
                if ini[i-1] == "[cmodel_top]\n":
                    f.write("reg_pre_img_bayerid = {}\n".format(frame["bayerid"]))
                    if "iso" in frame.keys() and "lv" in frame.keys():
                        f.write("reg_img_iso = {}\n".format(frame["iso"]))
                        f.write("reg_img_lv = {}\n".format(frame["lv"]))
                f.write(line)
        f.close()


def get_field_add(name):
    '''
    处理field
    :param name:
    :return:
    '''
    field_path = "D:\\Users\\huichong.li\\Documents\\ISP\\para\\mw2cmodel_transfer\\regStruct\\vi_reg_fields.h"
    with open(field_path, "r") as f:
        content = f.readlines()
        field_dic = {}
        for idx, line in enumerate(content):
            if line.startswith("union REG_"+name):
                block_name = line.split(" ")[1]
                i = idx+3

                lsb = 0
                msb = 0
                reg_bit_dic = {}
                total_bit = 0
                while content[i].strip().startswith("uint32_t"):
                    tmp = content[i].split(" ")
                    reg_name = "reg_" + tmp[1].lower()
                    if reg_name.startswith("reg__rsv"):
                        i += 1
                        continue
                    bit = int(tmp[-1][:-2])
                    total_bit += bit
                    msb = lsb + bit - 1
                    reg_bit_dic[reg_name] = (lsb, msb)
    #                 print(lsb, msb)
                    lsb = msb + 1
    #                 print(tmp[1], tmp[-1][:-2])
                    i += 1
    #             reg_bit_dic["bit"] = total_bit
                field_dic[block_name] = reg_bit_dic
    return field_dic


def get_block_add(module_name):
    block_path = "D://Users//huichong.li//Documents//ISP//para//mw2cmodel_transfer//regStruct/vi_reg_blocks.h"

    with open(block_path, "r") as f:
        block_content = f.readlines()
        block_add_dic = {}
        for idx, line in enumerate(block_content):
            if line.startswith("struct REG_"+module_name):
                i = idx+1
                start = 0
                while block_content[i].strip().startswith("u"):
                    if block_content[i].strip().startswith("union"):
                        block_name = block_content[i].strip().split(" ")[1]

                    if block_content[i].strip().startswith("uint32_t"):
                        block_name = block_content[i].strip().split(" ")[-1][:-1]
    #                     print(block_name)

                    add_offset = str(hex(start))[2:]
                    if len(add_offset) == 1:
                        add_offset = "0"+add_offset
                    add_offset = "h"+add_offset
    #                 print(add_offset)
                    block_add_dic[block_name] = add_offset
                    i += 1
                    start += 4
    return block_add_dic


def get_base_add():
    '''
    得到基地址
    :return:
    '''
    base_path = "D://Users//huichong.li//Documents//ISP//para//mw2cmodel_transfer//regStruct//isp_reg.h"

    module_add_dic = {}
    with open(base_path, "r") as f:
        base_add_content = f.readlines()
        for line in base_add_content:
            if line.startswith("#define ISP_BLK"):
                base_add = line[36:46]
                module_name = line.split(" ")[1][11:]
                module_add_dic[module_name] = base_add
    return module_add_dic


def combine_sub_reg(field_dic, block_add_dic, sub_add_reg_dic):
    for key in field_dic.keys():
#         print(field_dic[key].keys())
        sub_add_reg_dic[block_add_dic[key]] = [[x, field_dic[key][x][0], field_dic[key][x][1]] for x in field_dic[key].keys()]
    return sub_add_reg_dic


def precess_single_module(name):
    '''
    处理单独模块
    :param name: 模块名
    :return:
    '''
    module_dic = {}
    base_add = get_base_add()
    base_list = []
    for key in base_add.keys():
        if name.upper() in key:
            base_list.append(key)
    sub_add = get_block_add(name)
    field_add = get_field_add(name)
    sub_reg_combined = {"module": "reg_" + name.lower()}
    sub_reg_combined = combine_sub_reg(field_add, sub_add, sub_reg_combined)
    #     print(sub_reg_combined)

    for base in base_list:
        base_add_new = base_add[base][:3] + "A" + base_add[base][4:]
        module_dic[base_add_new] = sub_reg_combined
    return module_dic


def frame_config(name):
    res = []
    X_idx = name.find("X")
    width = name[:X_idx]
    height = name[X_idx+1:X_idx+5]
    res.append(width)
    res.append(height)

    if "Linear" in name:
        res.append(1)
    elif "WDR" in name:
        res.append(2)
    else:
        res.append(0)

    bayer_idx = name.find("color")
    res.append(name[bayer_idx+6])
    bits_idx = name.find("bits")
    if "8" in name[bits_idx:]:
        res.append(8)
    else:
        res.append(name[bits_idx+5:bits_idx+7])

    frame_idx = name.find("frame")
    res.append(name[frame_idx+6])

    hdr_idx = name.find("hdr")
    res.append(name[hdr_idx+4])

    iso_idx = name.find("ISO")
    res.append(name[iso_idx+4])

    return res


if __name__ == "__main__":
    name = "1920X1080_RGGB_Linear_20220628164140_00_-color=3_-bits=12_-frame=1_-hdr=1_ISO=0_.json"
    # print(frame_config(name))
