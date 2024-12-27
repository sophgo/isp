#!/usr/bin/env python3

import os


def get_ins_setting():
    dpu_folder = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..'))
    cmodel_folder = os.path.join(dpu_folder, 'cmodel')
    
    ins_setting = {
        # ------------------------------------------ raw domain ------------------------------------------ #
        'crop0':        {'bin': os.path.join(cmodel_folder, 'crop',        'bin', 'crop')},
        'crop1':        {'bin': os.path.join(cmodel_folder, 'crop',        'bin', 'crop')},
        'crop2':        {'bin': os.path.join(cmodel_folder, 'crop',        'bin', 'crop')},
        'blc0':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc1':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc2':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc3':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc4':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc5':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},

	    'rgbir0':  {'bin': os.path.join(cmodel_folder, 'rgbir',  'bin', 'rgbir')}, ##
        'rgbir1':  {'bin': os.path.join(cmodel_folder, 'rgbir',  'bin', 'rgbir')}, ##
	    'rgbir2':  {'bin': os.path.join(cmodel_folder, 'rgbir',  'bin', 'rgbir')}, ##

        'dpc0':         {'bin': os.path.join(cmodel_folder, 'dpc',         'bin', 'dpc')},
        'dpc1':         {'bin': os.path.join(cmodel_folder, 'dpc',         'bin', 'dpc')},
        'dpc2':         {'bin': os.path.join(cmodel_folder, 'dpc',         'bin', 'dpc')},
        'rgbmap0':      {'bin': os.path.join(cmodel_folder, 'rgbmap',      'bin', 'rgbmap')},
        'rgbmap1':      {'bin': os.path.join(cmodel_folder, 'rgbmap',      'bin', 'rgbmap')},
        'rgbmap2':      {'bin': os.path.join(cmodel_folder, 'rgbmap',      'bin', 'rgbmap')},

        'af0':          {'bin': os.path.join(cmodel_folder, 'af',          'bin', 'af')},
        'af1':          {'bin': os.path.join(cmodel_folder, 'af',          'bin', 'af')},
        'af2':          {'bin': os.path.join(cmodel_folder, 'af',          'bin', 'af')},
        'bnr':          {'bin': os.path.join(cmodel_folder, 'bnr',         'bin', 'bnr')},
        'wbg3':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'wbg4':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'wbg5':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},

        'lsc0':         {'bin': os.path.join(cmodel_folder, 'lsc',         'bin', 'lsc')},
        'lsc1':         {'bin': os.path.join(cmodel_folder, 'lsc',         'bin', 'lsc')},
        'lsc2':         {'bin': os.path.join(cmodel_folder, 'lsc',         'bin', 'lsc')},
        'mmap':         {'bin': os.path.join(cmodel_folder, 'mmap',        'bin', 'mmap')},
        'wbg0':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'wbg1':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'wbg2':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'ae0':          {'bin': os.path.join(cmodel_folder, 'ae',          'bin', 'ae')},
        'ae1':          {'bin': os.path.join(cmodel_folder, 'ae',          'bin', 'ae')},
        'ae2':          {'bin': os.path.join(cmodel_folder, 'ae',          'bin', 'ae')},
        'gms0':         {'bin': os.path.join(cmodel_folder, 'gms',         'bin', 'gms')},
        'gms1':         {'bin': os.path.join(cmodel_folder, 'gms',         'bin', 'gms')},
        'gms2':         {'bin': os.path.join(cmodel_folder, 'gms',         'bin', 'gms')},
        'bfusion':      {'bin': os.path.join(cmodel_folder, 'bfusion',     'bin', 'bfusion')},
        'bfs_gamma':    {'bin': os.path.join(cmodel_folder, 'bfs_gamma',   'bin', 'bfs_gamma')},
        'cfa0':         {'bin': os.path.join(cmodel_folder, 'cfa',         'bin', 'cfa')},
        'cfa1':         {'bin': os.path.join(cmodel_folder, 'cfa',         'bin', 'cfa')},
        'lmap0':        {'bin': os.path.join(cmodel_folder, 'lmap',        'bin', 'lmap')},
        'lmap1':        {'bin': os.path.join(cmodel_folder, 'lmap',        'bin', 'lmap')},

        # ------------------------------------------ rgb domain ------------------------------------------ #
        'rgbcac0':      {'bin': os.path.join(cmodel_folder, 'rgbcac',      'bin', 'rgbcac')},
        'rgbcac1':      {'bin': os.path.join(cmodel_folder, 'rgbcac',      'bin', 'rgbcac')},
        'lcac0':        {'bin': os.path.join(cmodel_folder, 'lcac',        'bin', 'lcac')},
        'lcac1':        {'bin': os.path.join(cmodel_folder, 'lcac',        'bin', 'lcac')},
        'bfs_igamma':   {'bin': os.path.join(cmodel_folder, 'bfs_igamma',  'bin', 'bfs_igamma')},
        'ccm0':         {'bin': os.path.join(cmodel_folder, 'ccm',         'bin', 'ccm')},
        'ccm1':         {'bin': os.path.join(cmodel_folder, 'ccm',         'bin', 'ccm')},
        'fusion':       {'bin': os.path.join(cmodel_folder, 'fusion',      'bin', 'fusion')},
        'hist_edge_v0': {'bin': os.path.join(cmodel_folder, 'hist_edge_v', 'bin', 'hist_edge_v')},
        'hist_edge_v1': {'bin': os.path.join(cmodel_folder, 'hist_edge_v', 'bin', 'hist_edge_v')},
        'ltm':          {'bin': os.path.join(cmodel_folder, 'ltm',         'bin', 'ltm')},
        'ygamma':       {'bin': os.path.join(cmodel_folder, 'ygamma',      'bin', 'ygamma')},
        'rgbgamma':     {'bin': os.path.join(cmodel_folder, 'rgbgamma',    'bin', 'rgbgamma'),},
        'dehaze':       {'bin': os.path.join(cmodel_folder, 'dehaze',      'bin', 'dehaze')},
        'rgb_dither':   {'bin': os.path.join(cmodel_folder, 'rgb_dither',  'bin', 'rgb_dither')},
        'clut':         {'bin': os.path.join(cmodel_folder, 'clut',        'bin', 'clut')},
        'csc':          {'bin': os.path.join(cmodel_folder, 'csc',         'bin', 'csc')},

        # ------------------------------------------ yuv domain ------------------------------------------ #
        'dci_hist0':    {'bin': os.path.join(cmodel_folder, 'dci',         'bin', 'dci')},
        'dci_map0':     {'bin': os.path.join(cmodel_folder, 'dci',         'bin', 'dci')},
        'ldci_stats0':  {'bin': os.path.join(cmodel_folder, 'ldci',        'bin', 'ldci')},
        'ldci_map0':    {'bin': os.path.join(cmodel_folder, 'ldci',        'bin', 'ldci')},
        'yuv_dither':   {'bin': os.path.join(cmodel_folder, 'yuv_dither',  'bin', 'yuv_dither')},
        'yuv444to422':  {'bin': os.path.join(cmodel_folder, 'yuv444to422', 'bin', 'yuv444to422')},
        'pre_ee':       {'bin': os.path.join(cmodel_folder, 'pre_ee',      'bin', 'pre_ee')},
        'tnr':          {'bin': os.path.join(cmodel_folder, 'tnr',         'bin', 'tnr')},
        'ynr':          {'bin': os.path.join(cmodel_folder, 'ynr',         'bin', 'ynr')},
        'cnr':          {'bin': os.path.join(cmodel_folder, 'cnr',         'bin', 'cnr')},
        'af2':          {'bin': os.path.join(cmodel_folder, 'af',          'bin', 'af')},
        'ee':           {'bin': os.path.join(cmodel_folder, 'ee' ,         'bin', 'ee')},
        'dci_hist1':    {'bin': os.path.join(cmodel_folder, 'dci',         'bin', 'dci')},
        'dci_map1':     {'bin': os.path.join(cmodel_folder, 'dci',         'bin', 'dci')},
        'ldci_stats1':  {'bin': os.path.join(cmodel_folder, 'ldci',        'bin', 'ldci')},
        'ldci_map1':    {'bin': os.path.join(cmodel_folder, 'ldci',        'bin', 'ldci')},
        'ca':           {'bin': os.path.join(cmodel_folder, 'ca',          'bin', 'ca')},
        'ycur':         {'bin': os.path.join(cmodel_folder, 'ycur',        'bin', 'ycur')},
        'ca_lite':      {'bin': os.path.join(cmodel_folder, 'ca_lite',     'bin', 'ca_lite')},
        'post_proc':    {'bin': os.path.join(cmodel_folder, 'post_proc',   'bin', 'post_proc')},
    }

    return ins_setting

def get_ins_setting_3f():
    dpu_folder = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..'))
    cmodel_folder = os.path.join(dpu_folder, 'cmodel')
    
    ins_setting = {
        # ------------------------------------------ raw domain ------------------------------------------ #
        'crop0':        {'bin': os.path.join(cmodel_folder, 'crop',        'bin', 'crop')},
        'crop1':        {'bin': os.path.join(cmodel_folder, 'crop',        'bin', 'crop')},
        'crop2':        {'bin': os.path.join(cmodel_folder, 'crop',        'bin', 'crop')},
        'blc0':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc1':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc2':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc3':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc4':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc5':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'dpc0':         {'bin': os.path.join(cmodel_folder, 'dpc',         'bin', 'dpc')},
        'dpc1':         {'bin': os.path.join(cmodel_folder, 'dpc',         'bin', 'dpc')},
        'dpc2':         {'bin': os.path.join(cmodel_folder, 'dpc',         'bin', 'dpc')},
        'rgbmap0':      {'bin': os.path.join(cmodel_folder, 'rgbmap',      'bin', 'rgbmap')},
        'rgbmap1':      {'bin': os.path.join(cmodel_folder, 'rgbmap',      'bin', 'rgbmap')},
        'rgbmap2':      {'bin': os.path.join(cmodel_folder, 'rgbmap',      'bin', 'rgbmap')},

        'af0':          {'bin': os.path.join(cmodel_folder, 'af',          'bin', 'af')},
        'af1':          {'bin': os.path.join(cmodel_folder, 'af',          'bin', 'af')},
        'af2':          {'bin': os.path.join(cmodel_folder, 'af',          'bin', 'af')},
        'bnr':          {'bin': os.path.join(cmodel_folder, 'bnr',         'bin', 'bnr')},
        'wbg3':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'wbg4':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'wbg5':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'lsc0':         {'bin': os.path.join(cmodel_folder, 'lsc',         'bin', 'lsc')},
        'lsc1':         {'bin': os.path.join(cmodel_folder, 'lsc',         'bin', 'lsc')},
        'lsc2':         {'bin': os.path.join(cmodel_folder, 'lsc',         'bin', 'lsc')},
        'mmap':      {'bin': os.path.join(cmodel_folder, 'mmap',     'bin', 'mmap')},
        'wbg0':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'wbg1':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'wbg2':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'ae0':          {'bin': os.path.join(cmodel_folder, 'ae',          'bin', 'ae')},
        'ae1':          {'bin': os.path.join(cmodel_folder, 'ae',          'bin', 'ae')},
        'ae2':          {'bin': os.path.join(cmodel_folder, 'ae',          'bin', 'ae')},
        'gms0':         {'bin': os.path.join(cmodel_folder, 'gms',         'bin', 'gms')},
        'gms1':         {'bin': os.path.join(cmodel_folder, 'gms',         'bin', 'gms')},
        'gms2':         {'bin': os.path.join(cmodel_folder, 'gms',         'bin', 'gms')},
        'bfusion':      {'bin': os.path.join(cmodel_folder, 'bfusion',     'bin', 'bfusion')},
        'bfs_gamma':    {'bin': os.path.join(cmodel_folder, 'bfs_gamma',   'bin', 'bfs_gamma')},
        'cfa0':         {'bin': os.path.join(cmodel_folder, 'cfa',         'bin', 'cfa')},
        'cfa1':         {'bin': os.path.join(cmodel_folder, 'cfa',         'bin', 'cfa')},
        'lmap0':        {'bin': os.path.join(cmodel_folder, 'lmap',        'bin', 'lmap')},
        'lmap1':        {'bin': os.path.join(cmodel_folder, 'lmap',        'bin', 'lmap')},

        # ------------------------------------------ rgb domain ------------------------------------------ #
        'rgbcac0':      {'bin': os.path.join(cmodel_folder, 'rgbcac',      'bin', 'rgbcac')},
        'rgbcac1':      {'bin': os.path.join(cmodel_folder, 'rgbcac',      'bin', 'rgbcac')},
        'lcac0':        {'bin': os.path.join(cmodel_folder, 'lcac',        'bin', 'lcac')},
        'lcac1':        {'bin': os.path.join(cmodel_folder, 'lcac',        'bin', 'lcac')},
        'bfs_igamma':   {'bin': os.path.join(cmodel_folder, 'bfs_igamma',   'bin', 'bfs_igamma')},
        'ccm0':         {'bin': os.path.join(cmodel_folder, 'ccm',         'bin', 'ccm')},
        'ccm1':         {'bin': os.path.join(cmodel_folder, 'ccm',         'bin', 'ccm')},
        'fusion':       {'bin': os.path.join(cmodel_folder, 'fusion',      'bin', 'fusion')},
        'hist_edge_v0': {'bin': os.path.join(cmodel_folder, 'hist_edge_v', 'bin', 'hist_edge_v')},
        'hist_edge_v1': {'bin': os.path.join(cmodel_folder, 'hist_edge_v', 'bin', 'hist_edge_v')},
        'ltm':          {'bin': os.path.join(cmodel_folder, 'ltm',         'bin', 'ltm')},
        'ygamma':       {'bin': os.path.join(cmodel_folder, 'ygamma',      'bin', 'ygamma')},
        'rgbgamma':     {'bin': os.path.join(cmodel_folder, 'rgbgamma',    'bin', 'rgbgamma'),},
        'dehaze':       {'bin': os.path.join(cmodel_folder, 'dehaze',      'bin', 'dehaze')},
        'rgb_dither':   {'bin': os.path.join(cmodel_folder, 'rgb_dither',  'bin', 'rgb_dither')},
        'clut':         {'bin': os.path.join(cmodel_folder, 'clut',        'bin', 'clut')},
        'csc':          {'bin': os.path.join(cmodel_folder, 'csc',         'bin', 'csc')},

        # ------------------------------------------ yuv domain ------------------------------------------ #
        'dci_hist0':    {'bin': os.path.join(cmodel_folder, 'dci',         'bin', 'dci')},
        'dci_map0':     {'bin': os.path.join(cmodel_folder, 'dci',         'bin', 'dci')},
        'ldci_stats0':  {'bin': os.path.join(cmodel_folder, 'ldci',        'bin', 'ldci')},
        'ldci_map0':    {'bin': os.path.join(cmodel_folder, 'ldci',        'bin', 'ldci')},
        'yuv_dither':   {'bin': os.path.join(cmodel_folder, 'yuv_dither',  'bin', 'yuv_dither')},
        'yuv444to422':  {'bin': os.path.join(cmodel_folder, 'yuv444to422', 'bin', 'yuv444to422')},
        'pre_ee':       {'bin': os.path.join(cmodel_folder, 'pre_ee',      'bin', 'pre_ee')},
        'tnr':          {'bin': os.path.join(cmodel_folder, 'tnr',         'bin', 'tnr')},
        'ynr':          {'bin': os.path.join(cmodel_folder, 'ynr',         'bin', 'ynr')},
        'cnr':          {'bin': os.path.join(cmodel_folder, 'cnr',         'bin', 'cnr')},
        'af2':          {'bin': os.path.join(cmodel_folder, 'af',          'bin', 'af')},
        'ee':           {'bin': os.path.join(cmodel_folder, 'ee' ,         'bin', 'ee')},
        'dci_hist1':    {'bin': os.path.join(cmodel_folder, 'dci',         'bin', 'dci')},
        'dci_map1':     {'bin': os.path.join(cmodel_folder, 'dci',         'bin', 'dci')},
        'ldci_stats1':  {'bin': os.path.join(cmodel_folder, 'ldci',        'bin', 'ldci')},
        'ldci_map1':    {'bin': os.path.join(cmodel_folder, 'ldci',        'bin', 'ldci')},
        'ca':           {'bin': os.path.join(cmodel_folder, 'ca',          'bin', 'ca')},
        'ycur':         {'bin': os.path.join(cmodel_folder, 'ycur',        'bin', 'ycur')},
        'ca_lite':      {'bin': os.path.join(cmodel_folder, 'ca_lite',     'bin', 'ca_lite')},
        'post_proc':    {'bin': os.path.join(cmodel_folder, 'post_proc',   'bin', 'post_proc')},
    }

    return ins_setting


def get_ins_setting_2f():
    dpu_folder = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..'))
    cmodel_folder = os.path.join(dpu_folder, 'cmodel')

    ins_setting = {
        # ------------------------------------------ raw domain ------------------------------------------ #
        'crop0':        {'bin': os.path.join(cmodel_folder, 'crop',        'bin', 'crop')},
        'crop1':        {'bin': os.path.join(cmodel_folder, 'crop',        'bin', 'crop')},
        'blc0':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc1':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc2':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},
        'blc3':         {'bin': os.path.join(cmodel_folder, 'blc',         'bin', 'blc')},

	    'rgbir0':  {'bin': os.path.join(cmodel_folder, 'rgbir',  'bin', 'rgbir')}, ##
        'rgbir1':  {'bin': os.path.join(cmodel_folder, 'rgbir',  'bin', 'rgbir')}, ##

        'dpc0':         {'bin': os.path.join(cmodel_folder, 'dpc',         'bin', 'dpc')},
        'dpc1':         {'bin': os.path.join(cmodel_folder, 'dpc',         'bin', 'dpc')},

        'rgbmap0':      {'bin': os.path.join(cmodel_folder, 'rgbmap',      'bin', 'rgbmap')},
        'rgbmap1':      {'bin': os.path.join(cmodel_folder, 'rgbmap',      'bin', 'rgbmap')},
        'af0':          {'bin': os.path.join(cmodel_folder, 'af',          'bin', 'af')},
        'af1':          {'bin': os.path.join(cmodel_folder, 'af',          'bin', 'af')},
        'bnr':          {'bin': os.path.join(cmodel_folder, 'bnr',         'bin', 'bnr')},
        'wbg2':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'wbg3':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'lsc0':         {'bin': os.path.join(cmodel_folder, 'lsc',         'bin', 'lsc')},
        'lsc1':         {'bin': os.path.join(cmodel_folder, 'lsc',         'bin', 'lsc')},
        'mmap':         {'bin': os.path.join(cmodel_folder, 'mmap',     'bin', 'mmap')},
        'wbg0':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'wbg1':         {'bin': os.path.join(cmodel_folder, 'wbg',         'bin', 'wbg')},
        'ae0':          {'bin': os.path.join(cmodel_folder, 'ae',          'bin', 'ae')},
        'ae1':          {'bin': os.path.join(cmodel_folder, 'ae',          'bin', 'ae')},
        'gms0':         {'bin': os.path.join(cmodel_folder, 'gms',         'bin', 'gms')},
        'gms1':         {'bin': os.path.join(cmodel_folder, 'gms',         'bin', 'gms')},
        'cfa0':         {'bin': os.path.join(cmodel_folder, 'cfa',         'bin', 'cfa')},
        'cfa1':         {'bin': os.path.join(cmodel_folder, 'cfa',         'bin', 'cfa')},
        'lmap0':        {'bin': os.path.join(cmodel_folder, 'lmap',        'bin', 'lmap')},
        'lmap1':        {'bin': os.path.join(cmodel_folder, 'lmap',        'bin', 'lmap')},

        # ------------------------------------------ rgb domain ------------------------------------------ #
        'rgbcac0':      {'bin': os.path.join(cmodel_folder, 'rgbcac',      'bin', 'rgbcac')},
        'rgbcac1':      {'bin': os.path.join(cmodel_folder, 'rgbcac',      'bin', 'rgbcac')},
        'lcac0':        {'bin': os.path.join(cmodel_folder, 'lcac',        'bin', 'lcac')},
        'lcac1':        {'bin': os.path.join(cmodel_folder, 'lcac',        'bin', 'lcac')},
        'ccm0':         {'bin': os.path.join(cmodel_folder, 'ccm',         'bin', 'ccm')},
        'ccm1':         {'bin': os.path.join(cmodel_folder, 'ccm',         'bin', 'ccm')},
        'fusion':       {'bin': os.path.join(cmodel_folder, 'fusion',      'bin', 'fusion')},
        'hist_edge_v0': {'bin': os.path.join(cmodel_folder, 'hist_edge_v', 'bin', 'hist_edge_v')},
        'hist_edge_v1': {'bin': os.path.join(cmodel_folder, 'hist_edge_v', 'bin', 'hist_edge_v')},
        'ltm':          {'bin': os.path.join(cmodel_folder, 'ltm',         'bin', 'ltm')},
        'ygamma':       {'bin': os.path.join(cmodel_folder, 'ygamma',      'bin', 'ygamma')},
        'rgbgamma':     {'bin': os.path.join(cmodel_folder, 'rgbgamma',    'bin', 'rgbgamma'),},
        'dehaze':       {'bin': os.path.join(cmodel_folder, 'dehaze',      'bin', 'dehaze')},
        'rgb_dither':   {'bin': os.path.join(cmodel_folder, 'rgb_dither',  'bin', 'rgb_dither')},
        'clut':         {'bin': os.path.join(cmodel_folder, 'clut',        'bin', 'clut')},
        'csc':          {'bin': os.path.join(cmodel_folder, 'csc',         'bin', 'csc')},

        # ------------------------------------------ yuv domain ------------------------------------------ #
        'dci_hist0':    {'bin': os.path.join(cmodel_folder, 'dci',         'bin', 'dci')},
        'dci_map0':     {'bin': os.path.join(cmodel_folder, 'dci',         'bin', 'dci')},
        'ldci_stats0':  {'bin': os.path.join(cmodel_folder, 'ldci',        'bin', 'ldci')},
        'ldci_map0':    {'bin': os.path.join(cmodel_folder, 'ldci',        'bin', 'ldci')},
        'yuv_dither':   {'bin': os.path.join(cmodel_folder, 'yuv_dither',  'bin', 'yuv_dither')},
        'yuv444to422':  {'bin': os.path.join(cmodel_folder, 'yuv444to422', 'bin', 'yuv444to422')},
        'pre_ee':       {'bin': os.path.join(cmodel_folder, 'pre_ee',      'bin', 'pre_ee')},
        'tnr':          {'bin': os.path.join(cmodel_folder, 'tnr',         'bin', 'tnr')},
        'ynr':          {'bin': os.path.join(cmodel_folder, 'ynr',         'bin', 'ynr')},
        'cnr':          {'bin': os.path.join(cmodel_folder, 'cnr',         'bin', 'cnr')},
        'af2':          {'bin': os.path.join(cmodel_folder, 'af',          'bin', 'af')},
        'ee':           {'bin': os.path.join(cmodel_folder, 'ee' ,         'bin', 'ee')},
        'dci_hist1':    {'bin': os.path.join(cmodel_folder, 'dci',         'bin', 'dci')},
        'dci_map1':     {'bin': os.path.join(cmodel_folder, 'dci',         'bin', 'dci')},
        'ldci_stats1':  {'bin': os.path.join(cmodel_folder, 'ldci',        'bin', 'ldci')},
        'ldci_map1':    {'bin': os.path.join(cmodel_folder, 'ldci',        'bin', 'ldci')},
        'ca':           {'bin': os.path.join(cmodel_folder, 'ca',          'bin', 'ca')},
        'ycur':         {'bin': os.path.join(cmodel_folder, 'ycur',        'bin', 'ycur')},
        'ca_lite':      {'bin': os.path.join(cmodel_folder, 'ca_lite',     'bin', 'ca_lite')},
        'post_proc':    {'bin': os.path.join(cmodel_folder, 'post_proc',   'bin', 'post_proc')},
    }

    return ins_setting
