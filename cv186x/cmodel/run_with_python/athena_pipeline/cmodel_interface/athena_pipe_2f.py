#!/usr/bin/env python3

import os
from abc import ABCMeta
from abc import abstractmethod

from run_with_python.common.instance_base import base as base
from run_with_python.common.pipeline_base import base as pipe_base


class crop0(base):
    def send_cmd(self):
        ins_id = 0
        cmd = [

            self.bin_file, 
			self.ini, 
			base.log_name_gen(self.log_file), 
			self.log_level.value,
            ins_id,
            base.img_name_gen(self.src_path, self.src_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
            ]
        return cmd

class crop1(base):
    def send_cmd(self):
        ins_id = 1
        cmd = [
            self.bin_file, 
			self.ini, 
			base.log_name_gen(self.log_file), 
			self.log_level.value,
            ins_id,
            base.img_name_gen(self.src_path, self.src_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
            ]
        return cmd

class blc0(base):
    def send_cmd(self):
        ins_id = 0
        cmd = [
            self.bin_file, 
			self.ini, 
			base.log_name_gen(self.log_file), 
			self.log_level.value,
            ins_id, 
			self.ini,
            base.img_name_gen(self.src_path['crop0'], self.src_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
            ]
        return cmd

class blc1(base):
    def send_cmd(self):
        ins_id = 1
        cmd = [
            self.bin_file, 
			self.ini, 
			base.log_name_gen(self.log_file), 
			self.log_level.value,
            ins_id, 
			self.ini,
            base.img_name_gen(self.src_path['crop1'], self.src_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
            ]
        return cmd

class blc2(base):
    def send_cmd(self):
        ins_id = 2
        cmd = [
            self.bin_file, 
			self.ini, 
			base.log_name_gen(self.log_file), 
			self.log_level.value,
            ins_id, 
			self.ini,
            base.img_name_gen(self.src_path['crop0'], self.src_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
            ]
        return cmd

class blc3(base):
    def send_cmd(self):
        ins_id = 3
        cmd = [
            self.bin_file, 
			self.ini, 
			base.log_name_gen(self.log_file), 
			self.log_level.value,
            ins_id, 
			self.ini,
            base.img_name_gen(self.src_path['crop1'], self.src_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
            ]
        return cmd

class rgbir0(base):
    def send_cmd(self):
        ins_id = 0
        cmd = [
            self.bin_file, self.ini, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_id,
            self.dbg_path, self.dump_dbg_img,
            base.img_name_gen(self.src_path['blc0'], self.src_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le_rggb'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le_ir'),
            # base.img_name_gen(cls, path, prefix, specific, ext='.ppm')
            base.img_name_gen(self.dst_path, self.dst_prefix, 'ir_8b_golden_le', '.dat'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'ir_12b_golden_le', '.dat'),
            ]
        return cmd

class rgbir1(base):
    def send_cmd(self):
        ins_id = 1
        cmd = [
            self.bin_file, self.ini, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_id,
            self.dbg_path, self.dump_dbg_img,
            base.img_name_gen(self.src_path['blc1'], self.src_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se_rggb'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se_ir'),
            # base.img_name_gen(cls, path, prefix, specific, ext='.ppm')
            base.img_name_gen(self.dst_path, self.dst_prefix, 'ir_8b_golden_se', '.dat'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'ir_12b_golden_se', '.dat'),
            ]
        return cmd

class dpc0(base):
    def send_cmd(self):
        ins_id = 0
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_id, self.ini,
            base.img_name_gen(self.src_path['rgbir0'], self.src_prefix, 'le_rggb'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
            ]
        return cmd

class dpc1(base):
    def send_cmd(self):
        ins_id = 1
        cmd = [
            self.bin_file, 
			self.ini, 
			base.log_name_gen(self.log_file), 
			self.log_level.value,
            ins_id, 
			self.ini,
            base.img_name_gen(self.src_path['rgbir1'], self.src_prefix, 'se_rggb'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
            ]
        return cmd

class rgbmap0(base):
    def send_cmd(self):
        ins_id = 0
        sw_enable = 1
        cmd = [
            self.bin_file, 
			self.ini, 
			base.log_name_gen(self.log_file), 
			self.log_level.value,
            base.get_idx(), ins_id, self.dbg_path, self.dump_dbg_img, self.ini, sw_enable,
            base.img_name_gen(self.src_path['blc0'], self.src_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'rgbmap_le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'irmap_le'),
            ]
        return cmd

class rgbmap1(base):
    def send_cmd(self):
        ins_id = 1
        sw_enable = 1
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.get_idx(), ins_id, self.dbg_path, self.dump_dbg_img, self.ini, sw_enable,
            base.img_name_gen(self.src_path['blc1'], self.src_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'rgbmap_se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'irmap_se'),
            ]
        return cmd

class af0(base):
    def send_cmd(self):
        ip_pos = 0
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ip_pos, self.ini,
            base.img_name_gen(self.src_path['rgbir0'], self.src_prefix, 'le_rggb'),
            self.dst_path,
            ]
        return cmd

class af1(base):
    def send_cmd(self):
        ip_pos = 0
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ip_pos, self.ini,
            base.img_name_gen(self.src_path['rgbir1'], self.src_prefix, 'se_rggb'),
            self.dst_path,
            ]
        return cmd

class bnr(base):
    def send_cmd(self):
        if base.get_reg_hdr_enable() == True:
            print("===========bnr on se===========")
            cmd = [
                self.bin_file,
                self.ini,
                base.log_name_gen(self.log_file),
                self.log_level.value,
                self.ini,
                base.img_name_gen(self.src_path['dpc1'], self.src_prefix, 'se'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
                base.img_name_gen(self.src_path['mmap'], self.src_prefix, 'sc0_pix'),
                base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'se'),
                base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'sc0_pix'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'sc0_pix'),
                base.get_idx()
            ]
        else:
            print("===========bnr on le===========")
            cmd = [
                self.bin_file,
                self.ini,
                base.log_name_gen(self.log_file),
                self.log_level.value,
                self.ini,
                base.img_name_gen(self.src_path['dpc0'], self.src_prefix, 'le'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
                base.img_name_gen(self.src_path['mmap'], self.src_prefix, 'sc0_pix'),
                base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'le'),
                base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'sc0_pix'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'sc0_pix'),
                base.get_idx()
            ]
        return cmd

class wbg2(base):
    def send_cmd(self):
        ins_id = 2
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.get_idx(), ins_id, self.dbg_path, self.dump_dbg_img, self.ini,
            base.img_name_gen(self.src_path['rgbmap0'], self.src_prefix, 'rgbmap_le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
            ]
        return cmd

class wbg3(base):
    def send_cmd(self):
        ins_id = 3
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.get_idx(), ins_id, self.dbg_path, self.dump_dbg_img, self.ini,
            base.img_name_gen(self.src_path['rgbmap1'], self.src_prefix, 'rgbmap_se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
            ]
        return cmd

class lsc0(base):
    def send_cmd(self):
        if base.get_reg_hdr_enable():
            cmd = [
                self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
                self.dbg_path, self.dump_dbg_img, self.ini,
                base.img_name_gen(self.src_path['dpc0'], self.src_prefix, 'le'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
                ]
        else:
            cmd = [
                self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
                self.dbg_path, self.dump_dbg_img, self.ini,
                base.img_name_gen(self.src_path['bnr'], self.src_prefix, 'le'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
                ]
        return cmd

class lsc1(base):
    def send_cmd(self):
        if base.get_reg_hdr_enable():
            cmd = [
                self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
                self.dbg_path, self.dump_dbg_img, self.ini,
                base.img_name_gen(self.src_path['bnr'], self.src_prefix, 'se'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
                ]
        else:
            cmd = [
                self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
                self.dbg_path, self.dump_dbg_img, self.ini,
                base.img_name_gen(self.src_path['dpc1'], self.src_prefix, 'se'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
                ]
        return cmd
"""
class mmap_2f(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value, base.get_idx(),
            self.ini,
            base.img_name_gen(self.src_path['wbg3'], self.src_prefix, 'le'),
            base.pre_img_name_gen(self.src_path['wbg3'], self.src_prefix, 'le'),
            base.img_name_gen(self.src_path['wbg4'], self.src_prefix, 'se'),
            base.pre_img_name_gen(self.src_path['wbg4'], self.src_prefix, 'se'),
            base.img_name_gen(self.src_path['rgbmap0'], self.src_prefix, 'irmap_le'),
            base.pre_img_name_gen(self.src_path['rgbmap0'], self.src_prefix, 'irmap_le'),
            base.img_name_gen(self.src_path['rgbmap1'], self.src_prefix, 'irmap_se'),
            base.pre_img_name_gen(self.src_path['rgbmap1'], self.src_prefix, 'irmap_se'),

            base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'tnr'),
            base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'ref'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'tnr'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'ref'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'sc0_pix'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'sc1_pix'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'sc2_pix'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'sc3_pix'),
	        self.dbg_path,
            ]
        return cmd
"""

class mmap(base):
    def send_cmd(self):
        cmd = []
        # if self.hdr_mode:
        if base.get_reg_hdr_enable() == True:
            cmd = [
                self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value, base.get_idx(),
                self.ini,
                base.img_name_gen(self.src_path['wbg2'], self.src_prefix, 'le'),
                base.pre_img_name_gen(self.src_path['wbg2'], self.src_prefix, 'le'),
                base.img_name_gen(self.src_path['wbg3'], self.src_prefix, 'se'),
                base.pre_img_name_gen(self.src_path['wbg3'], self.src_prefix, 'se'),
                base.img_name_gen(self.src_path['rgbmap0'], self.src_prefix, 'irmap_le'),
                base.pre_img_name_gen(self.src_path['rgbmap0'], self.src_prefix, 'irmap_le'),
                base.img_name_gen(self.src_path['rgbmap1'], self.src_prefix, 'irmap_se'),
                base.pre_img_name_gen(self.src_path['rgbmap1'], self.src_prefix, 'irmap_se'),

                base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'tnr'),
                base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'ref'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'tnr'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'ref'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'sc0_pix'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'sc1_pix'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'sc2_pix'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'sc3_pix'),
                self.dbg_path,
                ]
        else:
            cmd = [
                self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value, base.get_idx(),
                self.ini,
                base.img_name_gen(self.src_path['wbg2'], self.src_prefix, 'le'),
                base.pre_img_name_gen(self.src_path['wbg2'], self.src_prefix, 'le'),
                base.img_name_gen(self.src_path['wbg2'], self.src_prefix, 'le'),
                base.pre_img_name_gen(self.src_path['wbg2'], self.src_prefix, 'le'),
                base.img_name_gen(self.src_path['rgbmap0'], self.src_prefix, 'irmap_le'),
                base.pre_img_name_gen(self.src_path['rgbmap0'], self.src_prefix, 'irmap_le'),
                base.img_name_gen(self.src_path['rgbmap0'], self.src_prefix, 'irmap_le'),
                base.pre_img_name_gen(self.src_path['rgbmap0'], self.src_prefix, 'irmap_le'),

                base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'tnr'),
                base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'ref'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'tnr'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'ref'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'sc0_pix'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'sc1_pix'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'sc2_pix'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'sc3_pix'),
                self.dbg_path,
                ]

        return cmd

class wbg0(base):
    def send_cmd(self):
        ins_id = 0
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.get_idx(), ins_id, self.dbg_path, self.dump_dbg_img, self.ini,
            base.img_name_gen(self.src_path['lsc0'], self.src_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
            ]
        return cmd

class wbg1(base):
    def send_cmd(self):
        ins_id = 1
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.get_idx(), ins_id, self.dbg_path, self.dump_dbg_img, self.ini,
            base.img_name_gen(self.src_path['lsc1'], self.src_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
            ]
        return cmd

class ae0(base):
    def send_cmd(self):
        ins_id = 0
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_id, self.ini,
            base.img_name_gen(self.src_path['lsc0'], self.src_prefix, 'le'),
            self.dst_path,
            ]
        return cmd

class ae1(base):
    def send_cmd(self):
        ins_id = 1
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_id, self.ini,
            base.img_name_gen(self.src_path['lsc1'], self.src_prefix, 'se'),
            self.dst_path,
            ]
        return cmd

class gms0(base):
    def send_cmd(self):
        ins_id = 'NULL'
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_id, self.ini,
            base.img_name_gen(self.src_path['lsc0'], self.src_prefix, 'le'),
            self.dst_path,
            ]
        return cmd

class gms1(base):
    def send_cmd(self):
        ins_id = 'NULL'
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_id, self.ini,
            base.img_name_gen(self.src_path['lsc1'], self.src_prefix, 'se'),
            self.dst_path,
            ]
        return cmd

class cfa0(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dump_dbg_img, self.dbg_path, self.ini,
            base.img_name_gen(self.src_path['wbg0'], self.src_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'dc_le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'sg_le'),
            ]
        return cmd

class cfa1(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dump_dbg_img, self.dbg_path, self.ini,
            base.img_name_gen(self.src_path['wbg1'], self.src_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'dc_se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'sg_se'),
            ]
        return cmd

class lmap0(base):
    def send_cmd(self):
        ins_id = 0
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_id, self.dbg_path, self.dump_dbg_img, base.get_idx(), self.ini,
            base.img_name_gen(self.src_path['wbg0'], self.src_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
            ]
        return cmd

class lmap1(base):
    def send_cmd(self):
        ins_id = 1
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_id, self.dbg_path, self.dump_dbg_img, base.get_idx(), self.ini,
            base.img_name_gen(self.src_path['wbg1'], self.src_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
            ]
        return cmd

class rgbcac0(base):
    def send_cmd(self):
        ins_id = 0
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img, ins_id, self.ini,
            base.img_name_gen(self.src_path['wbg0'], self.src_prefix, 'le'),
            base.img_name_gen(self.src_path['cfa0'], self.src_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
            ]
        return cmd

class rgbcac1(base):
    def send_cmd(self):
        ins_id = 1
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img, ins_id, self.ini,
            base.img_name_gen(self.src_path['wbg1'], self.src_prefix, 'se'),
            base.img_name_gen(self.src_path['cfa1'], self.src_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
            ]
        return cmd

class lcac0(base):
    def send_cmd(self):
        ins_id = 0
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img, ins_id,
            base.img_name_gen(self.src_path['rgbcac0'], self.src_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
            ]
        return cmd

class lcac1(base):
    def send_cmd(self):
        ins_id = 1
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img, ins_id,
            base.img_name_gen(self.src_path['rgbcac1'], self.src_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
            ]
        return cmd

"""
class bfs_igamma(base):
    def send_cmd(self):
        cmd = self.gen_common_args() + [
            base.img_name_gen(self.src_path['lcac1'], self.src_prefix, 'se'),
            #base.img_name_gen(self.src_path['bfs_gamma'], self.src_prefix, 'sw_info'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'sw_info'),
            ]
        return cmd
"""
class ccm0(base):
    def send_cmd(self):
        ins_id = 0
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_id,
            base.img_name_gen(self.src_path['lcac0'], self.src_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'le'),
            ]
        return cmd

class ccm1(base):
    def send_cmd(self):
        ins_id = 1
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_id,
            base.img_name_gen(self.src_path['lcac1'], self.src_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'se'),
            ]
        return cmd
"""
class fusion(base):
    def send_cmd(self):
        cmd = self.gen_common_args() + [
            base.img_name_gen(self.src_path['ccm0'], self.src_prefix, 'le'),
            base.img_name_gen(self.src_path['ccm1'], self.src_prefix, 'se'),
            base.img_name_gen(self.src_path['cfa0'], self.src_prefix, 'dc_le'),
            base.img_name_gen(self.src_path['cfa0'], self.src_prefix, 'sg_le'),
            base.img_name_gen(self.src_path['cfa1'], self.src_prefix, 'sg_se'),
            base.img_name_gen(self.src_path['mmap'], self.dst_prefix, 'sc2_pix'),
            base.img_name_gen(self.src_path['mmap'], self.src_prefix, 'sc2_pix'),
            base.img_name_gen(self.dst_path, self.dst_prefix, ''),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'sg'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'lmap_dark_wgt'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'lmap_brit_wgt'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'calib_rgbsum'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'sw_info'),
            ]
        return cmd
"""
class fusion(base):
    def send_cmd(self):
        sw_enable = 1
        cmd = []
        # if self.hdr_mode:
        if base.get_reg_hdr_enable() == True:
            cmd = [
                self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
                self.dbg_path, self.dump_dbg_img, self.ini, sw_enable,
                base.img_name_gen(self.src_path['ccm0'], self.src_prefix, 'le'),
                base.img_name_gen(self.src_path['ccm1'], self.src_prefix, 'se'),
                base.img_name_gen(self.src_path['cfa0'], self.src_prefix, 'dc_le'),
                base.img_name_gen(self.src_path['cfa0'], self.src_prefix, 'sg_le'),
                base.img_name_gen(self.src_path['cfa1'], self.src_prefix, 'sg_se'),
                base.img_name_gen(self.src_path['mmap'], self.dst_prefix, 'sc2_pix'),
                base.img_name_gen(self.dst_path, self.dst_prefix, ''),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'sg'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'lmap_dark_wgt'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'lmap_brit_wgt'),
                ]
        else:
            cmd = [
                self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
                self.dbg_path, self.dump_dbg_img, self.ini, sw_enable,
                base.img_name_gen(self.src_path['ccm0'], self.src_prefix, 'le'),
                base.img_name_gen(self.src_path['ccm0'], self.src_prefix, 'le'),
                base.img_name_gen(self.src_path['cfa0'], self.src_prefix, 'dc_le'),
                base.img_name_gen(self.src_path['cfa0'], self.src_prefix, 'sg_le'),
                base.img_name_gen(self.src_path['cfa0'], self.src_prefix, 'sg_le'),
                base.img_name_gen(self.src_path['mmap'], self.dst_prefix, 'sc2_pix'),
                base.img_name_gen(self.dst_path, self.dst_prefix, ''),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'sg'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'lmap_dark_wgt'),
                base.img_name_gen(self.dst_path, self.dst_prefix, 'lmap_brit_wgt'),
                ]
        return cmd

class hist_edge_v0(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img,
            base.img_name_gen(self.src_path['ccm0'], self.src_prefix, 'le'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'hist_le'),
            ]
        return cmd

class hist_edge_v1(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img,
            base.img_name_gen(self.src_path['ccm1'], self.src_prefix, 'se'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'hist_se'),
            ]
        return cmd

class ltm(base):
    def send_cmd(self):
        hist_en = 1
        sw_enable = 0
        first_frame_flag = '0' if base.get_single_frame_sim()=='1' else base.get_first_flag()
        cmd = []
        # if self.hdr_mode:
        if base.get_reg_hdr_enable() == True:
            cmd = [
                self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value, first_frame_flag,
                self.dbg_path, self.dump_dbg_img, self.ini, sw_enable, hist_en,
                base.img_name_gen(self.src_path['hist_edge_v0'], self.src_prefix, 'hist_le'),
                base.img_name_gen(self.src_path['hist_edge_v1'], self.src_prefix, 'hist_se'),
                base.img_name_gen(self.src_path['fusion'], self.src_prefix, ''),
                base.img_name_gen(self.src_path['fusion'], self.src_prefix, 'sg'),
                base.img_name_gen(self.src_path['fusion'], self.src_prefix, 'lmap_dark_wgt'),
                base.img_name_gen(self.src_path['fusion'], self.src_prefix, 'lmap_brit_wgt'),
                base.pre_img_name_gen(self.src_path['lmap0'], self.src_prefix, 'le'),
                base.pre_img_name_gen(self.src_path['lmap1'], self.src_prefix, 'se'),
                base.img_name_gen(self.src_path['mmap'], self.dst_prefix, 'sc2_pix'),
                base.img_name_gen(self.dst_path, self.dst_prefix, ''),
                ]
        else:
            cmd = [
                self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value, first_frame_flag,
                self.dbg_path, self.dump_dbg_img, self.ini, sw_enable, hist_en,
                base.img_name_gen(self.src_path['hist_edge_v0'], self.src_prefix, 'hist_le'),
                base.img_name_gen(self.src_path['hist_edge_v0'], self.src_prefix, 'hist_le'),
                base.img_name_gen(self.src_path['fusion'], self.src_prefix, ''),
                base.img_name_gen(self.src_path['fusion'], self.src_prefix, 'sg'),
                base.img_name_gen(self.src_path['fusion'], self.src_prefix, 'lmap_dark_wgt'),
                base.img_name_gen(self.src_path['fusion'], self.src_prefix, 'lmap_brit_wgt'),
                base.pre_img_name_gen(self.src_path['lmap0'], self.src_prefix, 'le'),
                base.pre_img_name_gen(self.src_path['lmap0'], self.src_prefix, 'le'),
                base.img_name_gen(self.src_path['mmap'], self.dst_prefix, 'sc2_pix'),
                base.img_name_gen(self.dst_path, self.dst_prefix, ''),
                ]

        return cmd

class ygamma(base):
    def send_cmd(self):
        sw_enable = 0
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            sw_enable,
            base.img_name_gen(self.src_path['ltm'], self.src_prefix, ''),
            base.img_name_gen(self.dst_path, self.dst_prefix, ''),
            ]
        return cmd

class rgbgamma(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.img_name_gen(self.src_path['ygamma'], self.src_prefix, ''),
            base.img_name_gen(self.dst_path, self.dst_prefix, ''),
            ]
        return cmd

class dehaze(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img, base.get_idx(),
            base.img_name_gen(self.src_path['rgbgamma'], self.src_prefix, ''),
            base.img_name_gen(self.dst_path, self.dst_prefix, ''),
            base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'A_global'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'A_global'),
            ]
        return cmd

class rgb_dither(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.img_name_gen(self.src_path['dehaze'], self.src_prefix, ''),
            base.img_name_gen(self.dst_path, self.dst_prefix, ''),
            ]
        return cmd

class clut(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.img_name_gen(self.src_path['rgb_dither'], self.src_prefix, ''),
            base.img_name_gen(self.dst_path, self.dst_prefix, ''),
            ]
        return cmd

class csc(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.img_name_gen(self.src_path['clut'], self.src_prefix, ''),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            ]
        return cmd

class dci_hist0(base):
    def send_cmd(self):
        dci_mode = 0 # dci_hist
        dci_id = 0   # 10-bit domain
        cmd = [
            self.bin_file, self.ini, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img,
            dci_mode,
            dci_id,
            base.get_idx(),
            base.img_name_gen(self.src_path['csc'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['csc'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['csc'], self.src_prefix, 'v'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'dci_hist'),
            ]
        return cmd

class dci_map0(base):
    def send_cmd(self):
        dci_mode = 1 # dci_map
        dci_id = 0   # 10-bit domain
        cmd = [
            self.bin_file, self.ini, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img,
            dci_mode,
            dci_id,
            base.get_idx() + int(base.get_single_frame_sim()),
            base.img_name_gen(self.src_path['dci_hist0'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['dci_hist0'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['dci_hist0'], self.src_prefix, 'v'),
            base.pre_img_name_gen(self.src_path['dci_hist0'], self.src_prefix,'dci_hist'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            ]
        return cmd

class ldci_stats0(base):
    def send_cmd(self):
        ldci_mode = 0 # ldci_stats
        ldci_id = 0   # 10-bit domain
        cmd = [
            self.bin_file, self.ini, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img,
            ldci_mode,
            ldci_id,
            base.get_idx(),
            base.img_name_gen(self.src_path['dci_map0'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['dci_map0'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['dci_map0'], self.src_prefix, 'v'),
            base.pre_img_name_gen(self.src_path['ldci_stats0'], self.src_prefix, 'ldci_idx_map'),
            base.pre_img_name_gen(self.src_path['ldci_stats0'], self.src_prefix, 'ldci_var_map'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'ldci_idx_map'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'ldci_var_map'),
            ]
        print(base.get_idx())
        return cmd

class ldci_map0(base):
    def send_cmd(self):
        ldci_mode = 1 # ldci_map
        ldci_id = 0   # 10-bit domain
        cmd = [
            self.bin_file, self.ini, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img,
            ldci_mode,
            ldci_id,
            base.get_idx() + int(base.get_single_frame_sim()),
            base.img_name_gen(self.src_path['ldci_stats0'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['ldci_stats0'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['ldci_stats0'], self.src_prefix, 'v'),
            base.pre_img_name_gen(self.src_path['ldci_stats0'], self.src_prefix, 'ldci_idx_map'),
            base.pre_img_name_gen(self.src_path['ldci_stats0'], self.src_prefix, 'ldci_var_map'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            ]
        return cmd

class yuv_dither(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.img_name_gen(self.src_path['ldci_map0'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['ldci_map0'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['ldci_map0'], self.src_prefix, 'v'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            ]
        return cmd

class yuv444to422(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini,
            base.img_name_gen(self.src_path['yuv_dither'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['yuv_dither'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['yuv_dither'], self.src_prefix, 'v'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            ]
        return cmd


# -------------------------------------------------------------------------------------------------
# modified by bairong.li start
# -------------------------------------------------------------------------------------------------
# class pre_ee(base):
#     def send_cmd(self):
#         ins_name = '_pre'
#         input_mode = 1  # 0: RGB input, 1: 422 YCbCr input
#         cmd = [
#             self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
#             ins_name, input_mode, self.dbg_path, self.dump_dbg_img,
#             base.img_name_gen(self.src_path['yuv444to422'], self.src_prefix, 'y'),
#             base.img_name_gen(self.src_path['yuv444to422'], self.src_prefix, 'u'),
#             base.img_name_gen(self.src_path['yuv444to422'], self.src_prefix, 'v'),
#             base.img_name_gen(self.src_path['yuv444to422'], self.src_prefix, 'y'),
#             base.img_name_gen(self.src_path['mmap'], self.src_prefix, 'sc3_pix'),
#             base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
#             ]
#         return cmd

class pre_ee(base):
    def send_cmd(self):
        ins_name = '_pre'
        input_mode = 1  # 0: RGB input, 1: 422 YCbCr input
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_name, input_mode, self.dbg_path, self.dump_dbg_img,
            base.img_name_gen(self.src_path['yuv444to422'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['yuv444to422'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['yuv444to422'], self.src_prefix, 'v'),
            base.img_name_gen(self.src_path['yuv444to422'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['mmap'], self.src_prefix, 'sc3_pix'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.src_path['fusion'], self.src_prefix, '422_Y'),
            ]
        return cmd
        
# -------------------------------------------------------------------------------------------------
# modified by bairong.li end
# -------------------------------------------------------------------------------------------------   

class tnr(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value, base.get_idx(),
            self.ini,
            base.img_name_gen(self.src_path['pre_ee'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['yuv444to422'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['yuv444to422'], self.src_prefix, 'v'),
            base.pre_img_name_gen(self.dst_path, self.dst_prefix, '420_y'),
            base.pre_img_name_gen(self.dst_path, self.dst_prefix, '420_u'),
            base.pre_img_name_gen(self.dst_path, self.dst_prefix, '420_v'),
            base.img_name_gen(self.src_path['mmap'], self.src_prefix, 'sc0_pix'),
            base.img_name_gen(self.dst_path, self.dst_prefix, '420_y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, '420_u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, '420_v'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'motion_out'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'rgb'),
            base.img_name_gen(self.dst_path, self.dst_prefix, '422_y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, '422_u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, '422_v'),
            base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'motion_out'),
   	    ]
        return cmd

class ynr(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value, base.get_idx(),
            base.img_name_gen(self.src_path['tnr'], self.src_prefix, '422_y'),
            base.img_name_gen(self.src_path['mmap'], self.src_prefix, 'sc1_pix'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            ]
        return cmd

class cnr(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.img_name_gen(self.src_path['tnr'], self.src_prefix, '422_y'),
            base.img_name_gen(self.src_path['tnr'], self.src_prefix, '422_u'),
            base.img_name_gen(self.src_path['tnr'], self.src_prefix, '422_v'),
            base.img_name_gen(self.src_path['mmap'], self.src_prefix, 'sc1_pix'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            ]
        return cmd

class af2(base):
    def send_cmd(self):
        ip_pos = 1
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ip_pos, self.ini,
            base.img_name_gen(self.src_path['tnr'], self.src_prefix, '422_y'),
            self.dst_path,
            ]
        return cmd


class ee(base):
    def send_cmd(self):
        ins_name = '_post'
        input_mode = 1  # 0: RGB input, 1: 422 YCbCr input
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            ins_name, input_mode, self.dbg_path, self.dump_dbg_img,
            base.img_name_gen(self.src_path['tnr'], self.src_prefix, '422_y'),
            base.img_name_gen(self.src_path['tnr'], self.src_prefix, '422_u'),
            base.img_name_gen(self.src_path['tnr'], self.src_prefix, '422_v'),
            base.img_name_gen(self.src_path['ynr'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['mmap'], self.src_prefix, 'sc1_pix'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            ]
        return cmd


class dci_hist1(base):
    def send_cmd(self):
        dci_mode = 0 # dci_hist
        dci_id = 1   # 8-bit domain
        cmd = [
            self.bin_file, self.ini, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img,
            dci_mode,
            dci_id,
            base.get_idx(),
            base.img_name_gen(self.src_path['ee'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['cnr'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['cnr'], self.src_prefix, 'v'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'dci_hist'),
            ]
        return cmd

class dci_map1(base):
    def send_cmd(self):
        dci_mode = 1 # dci_map
        dci_id = 1   # 8-bit domain
        cmd = [
            self.bin_file, self.ini, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img,
            dci_mode,
            dci_id,
            base.get_idx() + int(base.get_single_frame_sim()),
            base.img_name_gen(self.src_path['dci_hist1'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['dci_hist1'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['dci_hist1'], self.src_prefix, 'v'),
            # base.pre_img_name_gen(self.dst_path, self.dst_prefix, 'dci_hist'),
            base.pre_img_name_gen(self.src_path['dci_hist1'], self.src_prefix, 'dci_hist'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            ]
        return cmd

class ldci_stats1(base):
    def send_cmd(self):
        ldci_mode = 0 # dci_hist
        ldci_id = 1   # 8-bit domain
        cmd = [
            self.bin_file, self.ini, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img,
            ldci_mode,
            ldci_id,
            base.get_idx(),
            base.img_name_gen(self.src_path['dci_map1'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['dci_map1'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['dci_map1'], self.src_prefix, 'v'),
            base.pre_img_name_gen(self.src_path['ldci_stats1'], self.src_prefix, 'ldci_idx_map'),
            base.pre_img_name_gen(self.src_path['ldci_stats1'], self.src_prefix, 'ldci_var_map'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'ldci_idx_map'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'ldci_var_map'),
        ]
        return cmd

class ldci_map1(base):
    def send_cmd(self):
        ldci_mode = 1 # dci_map
        ldci_id = 1   # 8-bit domain
        cmd = [
            self.bin_file, self.ini, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img,
            ldci_mode,
            ldci_id,
            base.get_idx() + int(base.get_single_frame_sim()),
            base.img_name_gen(self.src_path['ldci_stats1'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['ldci_stats1'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['ldci_stats1'], self.src_prefix, 'v'),
            # base.pre_img_name_gen(self.src_path['ldci_stats1'], self.src_prefix, 'ldci_idx_map'),
            # base.pre_img_name_gen(self.src_path['ldci_stats1'], self.src_prefix, 'ldci_var_map'),
            base.pre_img_name_gen(self.src_path['ldci_stats1'], self.src_prefix, 'ldci_idx_map'),
            base.pre_img_name_gen(self.src_path['ldci_stats1'], self.src_prefix, 'ldci_var_map'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            ]
        return cmd

class ca(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.img_name_gen(self.src_path['ldci_map1'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['ldci_map1'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['ldci_map1'], self.src_prefix, 'v'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            ]
        return cmd

class ycur(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.img_name_gen(self.src_path['ca'], self.src_prefix, 'y'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'y'),
            ]
        return cmd

class ca_lite(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            self.dbg_path, self.dump_dbg_img,
            base.img_name_gen(self.src_path['ca'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['ca'], self.src_prefix, 'v'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'u'),
            base.img_name_gen(self.dst_path, self.dst_prefix, 'v'),
            ]
        return cmd

class post_proc(base):
    def send_cmd(self):
        cmd = [
            self.bin_file, self.ini, base.log_name_gen(self.log_file), self.log_level.value,
            base.img_name_gen(self.src_path['ycur'], self.src_prefix, 'y'),
            base.img_name_gen(self.src_path['ca_lite'], self.src_prefix, 'u'),
            base.img_name_gen(self.src_path['ca_lite'], self.src_prefix, 'v'),
            base.img_name_gen(self.dst_path, 'result', 'y'),
            base.img_name_gen(self.dst_path, 'result', 'u'),
            base.img_name_gen(self.dst_path, 'result', 'v'),
            base.img_name_gen(self.dst_path, 'result' + self.timestamp + self.keyword, 'rgb'),
            ]
        return cmd


class athena_pipe_2f(pipe_base):
        linear_pipeline = [
            ['crop0'         ]        ,
            ['blc0'          ]    ,

	        ['rgbir0'        ]        ,##

            ['dpc0'          , 'rgbmap0'        , 'af0']   ,
            ['wbg2'          ]          ,
			['mmap'],
			['bnr'],
            ['lsc0'          ]          ,
            ['wbg0'          , 'ae0'    ,        'gms0']  ,
            ['cfa0'          ]   ,
            ['lmap0'         ]   ,
            ['rgbcac0'       ]      ,
            ['lcac0'         ]        ,
           # ['bfs_igamma']   ,
            ['ccm0'          ]         ,
            ['fusion'        , 'hist_edge_v0']  ,
            ['ltm']          ,
            ['ygamma']       ,
            ['rgbgamma']     ,
            ['dehaze']       ,
            ['rgb_dither']   ,
            ['clut']         ,
            ['csc']          ,
            ['dci_hist0']    ,
            ['dci_map0']     ,
            ['ldci_stats0']  ,
            ['ldci_map0']    ,
            ['yuv_dither']   ,
            ['yuv444to422']  ,
            ['pre_ee']       ,
            ['tnr']          ,
            ['ynr'           , 'cnr'           , 'af2']           ,
            ['ee']           ,
            ['dci_hist1']    ,
            ['dci_map1']     ,
            ['ldci_stats1']  ,
            ['ldci_map1']    ,
            ['ca']           ,
            ['ycur'          , 'ca_lite']      ,
            ['post_proc']    ,
        ]
        wdr_pipeline = [
            ['crop0'         , 'crop1']        ,
            ['blc0'          , 'blc1'          , 'blc2'           , 'blc3']    ,

	        ['rgbir0'        , 'rgbir1']        ,##

            ['dpc0'          , 'dpc1'          , 'rgbmap0'        , 'rgbmap1'  , 'af0'   , 'af1']   ,
            ['wbg2'          , 'wbg3']          ,
			['mmap'],
			['bnr'],
            ['lsc0'          , 'lsc1']          ,
            ['wbg0'          , 'wbg1'          , 'ae0'            , 'ae1'      , 'gms0'  , 'gms1']  ,
            ['cfa0'          , 'cfa1']   ,
            ['lmap0'         , 'lmap1']   ,
            ['rgbcac0'       , 'rgbcac1']      ,
            ['lcac0'         , 'lcac1']        ,
           # ['bfs_igamma']   ,
            ['ccm0'          , 'ccm1']         ,
            ['fusion'        , 'hist_edge_v0'  , 'hist_edge_v1']  ,
            ['ltm']          ,
            ['ygamma']       ,
            ['rgbgamma']     ,
            ['dehaze']       ,
            ['rgb_dither']   ,
            ['clut']         ,
            ['csc']          ,
            ['dci_hist0']    ,
            ['dci_map0']     ,
            ['ldci_stats0']  ,
            ['ldci_map0']    ,
            ['yuv_dither']   ,
            ['yuv444to422']  ,
            ['pre_ee']       ,
            ['tnr']          ,
            ['ynr'           , 'cnr'           , 'af2']           ,
            ['ee']           ,
            ['dci_hist1']    ,
            ['dci_map1']     ,
            ['ldci_stats1']  ,
            ['ldci_map1']    ,
            ['ca']           ,
            ['ycur'          , 'ca_lite']      ,
            ['post_proc']    ,
        ]
