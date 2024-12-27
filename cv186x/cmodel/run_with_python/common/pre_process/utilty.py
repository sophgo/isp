import os
import numpy as np

def find_raw(dir):
	for root, dirs, files in os.walk(dir):
		for file in files:
			if (".raw" in file):
				return root+"/"+file

def find_index_for_raw(raw_name_list, key_str):
	for i, ele in enumerate(raw_name_list):
		if (key_str in ele):
			return i

def raw_name_parsing(raw_img, iso=None, lv=None):
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
	raw_name_info['img_height'] = int(raw_name_list[0].split('x')[1])

	bayer_id = find_index_for_raw(raw_name_list, '-color')
	raw_name_info['bayerid'] = int(raw_name_list[bayer_id].split('=')[1])

	bit_index = find_index_for_raw(raw_name_list, '-bits')
	raw_name_info['bit_num'] = int(raw_name_list[bit_index].split('=')[1])

	frame_index = find_index_for_raw(raw_name_list, '-frame')
	raw_name_info['frame_num'] = int(raw_name_list[frame_index].split('=')[1])

	hdr_frame = find_index_for_raw(raw_name_list, '-hdr')
	raw_name_info['hdr_frame'] = int(raw_name_list[hdr_frame].split('=')[1])

	if raw_name_info['hdr_frame'] == 0:
		raw_name_info['hdr_frame'] = 1

	if raw_name_info['hdr_frame'] == 1 and raw_name_info['wdr_flag'] == 1:
		raw_name_info['hdr_frame'] = 2


	raw_name_info['img_width'] = int(raw_name_info['img_width_wdr'] / raw_name_info['hdr_frame']) 

	# add iso & lv
	if iso and lv:
		raw_name_info["iso"] = iso
		raw_name_info["lv"] = lv

	return raw_name_info

def raw2ppm_np(raw_img, raw_name_info, outputDir):
	raw_dirname = raw_name_info['raw_dirname']
	frame_num = raw_name_info['frame_num']
	img_height = raw_name_info['img_height']
	img_width = raw_name_info['img_width']
	img_width_wdr = raw_name_info['img_width_wdr']
	wdr_flag = raw_name_info['wdr_flag']
	bit_num = raw_name_info['bit_num']
	hdr_frame = raw_name_info['hdr_frame']

	img = np.fromfile(raw_img, dtype=np.uint16)
	img.byteswap(True)
	img = np.reshape(img, (frame_num, img_height, img_width_wdr))

	for frame_idx in range(frame_num):
		if outputDir == "":
			src_le_name = os.path.join(raw_dirname, 'src_le_' + f'{frame_idx:03}' + '.ppm')
			src_me_name = os.path.join(raw_dirname, 'src_me_' + f'{frame_idx:03}' + '.ppm')
			src_se_name = os.path.join(raw_dirname, 'src_se_' + f'{frame_idx:03}' + '.ppm')
		else:
			src_le_name = os.path.join(outputDir, 'src_le_' + f'{frame_idx:03}' + '.ppm')
			src_me_name = os.path.join(outputDir, 'src_me_' + f'{frame_idx:03}' + '.ppm')
			src_se_name = os.path.join(outputDir, 'src_se_' + f'{frame_idx:03}' + '.ppm')
		with open(src_le_name, 'wb') as le_ppm, open(src_se_name, 'wb') as se_ppm:
			if hdr_frame == 3: 
				me_ppm = open(src_me_name, 'wb')
				[img_le, img_me, img_se] = np.hsplit(img[frame_idx], 3)
				le_ppm.write(bytearray('P5\n', 'ascii'))
				le_ppm.write(bytearray('# ' + 'LE raw2ppm for run_with_python' + '\n', 'ascii'))
				le_ppm.write(bytearray(f'{img_width} {img_height}\n', 'ascii'))
				le_ppm.write(bytearray(f'{2**bit_num-1}\n', 'ascii'))
				le_ppm.write(img_le.tobytes())

				me_ppm.write(bytearray('P5\n', 'ascii'))
				me_ppm.write(bytearray('# ' + 'ME raw2ppm for run_with_python' + '\n', 'ascii'))
				me_ppm.write(bytearray(f'{img_width} {img_height}\n', 'ascii'))
				me_ppm.write(bytearray(f'{2**bit_num-1}\n', 'ascii'))
				me_ppm.write(img_le.tobytes())

				se_ppm.write(bytearray('P5\n', 'ascii'))
				se_ppm.write(bytearray('# ' + 'SE raw2ppm for run_with_python' + '\n', 'ascii'))
				se_ppm.write(bytearray(f'{img_width} {img_height}\n', 'ascii'))
				se_ppm.write(bytearray(f'{2**bit_num-1}\n', 'ascii'))
				se_ppm.write(img_se.tobytes())
			elif hdr_frame == 2:
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
				se_ppm.write(np.zeros((img_height, img_width), dtype=np.uint16).tobytes())
