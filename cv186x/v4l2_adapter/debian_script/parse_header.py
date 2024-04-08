import os
import sys

special_header_dir_ls = ["linux", "json-c", "cvi_rtsp"]
exclude_header_ls = ["time_types.h"]

def check_exclude_header(file_path):
	ret = False
	header_name = file_path.split("/")[-1].strip()
	if header_name in exclude_header_ls:
		ret = True

	return ret

def get_header_path(file_path, makefile_dir):
	with open(file_path, "r") as f:
		lines = f.readlines()[1:]

	# get header path
	headers = set()
	strip_str_list = [" ", "\n", " \\", ":"]

	# change dir to makefile dir
	os.chdir(makefile_dir)

	for line in lines:
		for strip_str in strip_str_list:
			line = line.strip(strip_str)
		# one line contains more than one path
		line_ls = line.split(" ")
		for line_e in line_ls:
			if len(line_e) > 0 and line_e.endswith(".h") and os.path.exists(line_e):
				abs_path = os.path.abspath(line_e)
				if not check_exclude_header(abs_path):
					headers.add(abs_path)
			if not len(line_e) > 0 and os.path.exists(line_e):
				print(f"file: {line_e} not found!")

	return headers

def check_if_special_dir(header_path):
	ret = False
	ret_dir = ""

	header_path_ls = header_path.split("/")

	if len(header_path_ls) >= 2:
		special_header_dir_check = header_path_ls[-2]
	else:
		return ret, ret_dir

	for special_header_dir in special_header_dir_ls:
		if special_header_dir_check in special_header_dir:
			ret = True
			ret_dir = special_header_dir
			break

	return ret, ret_dir

def copy_headers(header_set, save_dir):
	not_special_ls = list()
	special_dict = dict()

	for header in header_set:
		is_special, special_dir = check_if_special_dir(header)
		if not is_special:
			not_special_ls.append(header)
		else:
			if special_dir in special_dict.keys():
				special_dict[special_dir].append(header)
			else:
				special_dict[special_dir] = [header]

	# normal
	if not os.path.exists(save_dir):
		os.system(f"mkdir -p {save_dir}")

	for header_path in not_special_ls:
		os.system(f"cp {header_path} {save_dir}")

	# special
	for special_dir in special_dict.keys():
		save_dir_special = os.path.join(save_dir, special_dir)
		if not os.path.exists(save_dir_special):
			os.system(f"mkdir -p {save_dir_special}")
		# copy files
		for header_path in special_dict[special_dir]:
			os.system(f"cp {header_path} {save_dir_special}")

def show_header_path(headers):
	for header in headers:
		print(header)

if __name__ == "__main__":
	if len(sys.argv) <= 3:
		print("You must specify the arg1, arg2 and arg3.\n"
		"usage: arg1: xxx.d's path; arg2: makefile dir; arg3: dir path where you want to copy the headers. Exit ...")
		exit(1)

	file_path = sys.argv[1]
	makefile_dir = sys.argv[2]
	save_header_dir = sys.argv[3]

	if not os.path.isfile(file_path):
		print(f"dependency file {file_path} not exists! Exit ...")
		exit(1)

	if not os.path.isdir(makefile_dir):
		print(f"dependency makefile dir {makefile_dir} not exists! Exit ...")
		exit(1)

	# get header file
	headers_set = get_header_path(file_path, makefile_dir)
	#show_header_path(headers_set)
	copy_headers(headers_set, save_header_dir)

