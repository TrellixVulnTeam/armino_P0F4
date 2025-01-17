#!/usr/bin/env python3

import os
import subprocess
import sys

def run_cmd(cmd):
	p = subprocess.Popen(cmd, shell=True)
	ret = p.wait()

def build_lan_doc(doc_path, target, lan):
	lan_dir = f'{doc_path}/{lan}'
	os.chdir(lan_dir)
	run_cmd('make arminodocs')
	run_cmd(f'cp -r _build ../build/{lan}')
	os.chdir(doc_path)

def build_with_target(clean, target):
	cur_dir_is_docs_dir = True
	saved_dir = os.getcwd()
	if 'ARMINO_PATH' in os.environ:
		armino_path = os.getenv('ARMINO_PATH')
		DOCS_PATH = f"{armino_path}/docs/{target}"
		cur_path = os.getcwd()
		if cur_path != DOCS_PATH:
			cur_dir_is_docs_dir = False
		print(f'DOCS_PATH set to {DOCS_PATH}')
	else:
		#print('ARMINO_PATH env is not set, set DOCS_PATH to current dir')
		DOCS_PATH = f"{os.getcwd()}/docs/{target}"

	build_dir = f'{DOCS_PATH}/build'
	run_cmd(f'rm -rf {build_dir}')
	if (clean):
		run_cmd(f'rm -rf {DOCS_PATH}/en/_build')
		run_cmd(f'rm -rf {DOCS_PATH}/en/xml')
		run_cmd(f'rm -rf {DOCS_PATH}/en/xml_in')
		run_cmd(f'rm -rf {DOCS_PATH}/en/man')
		run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/_build')
		run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/xml')
		run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/xml_in')
		run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/man')
		run_cmd(f'rm -rf {DOCS_PATH}/__pycache__')
		return;

	run_cmd(f'mkdir {build_dir}')

	build_lan_doc(DOCS_PATH, target, 'zh_CN')
	build_lan_doc(DOCS_PATH, target, 'en')

	if cur_dir_is_docs_dir == False:
		run_cmd(f'rm -rf {cur_path}/build/html/{target}')
		run_cmd(f'mv {DOCS_PATH}/build/ {cur_path}/build/html/{target}')
	os.chdir(saved_dir)

def build_doc_internal(clean, target):
	if 'ARMINO_PATH' in os.environ:
		armino_path = os.getenv('ARMINO_PATH')
	else:
		armino_path = os.getcwd()

	if not os.path.exists(armino_path + "/build/html"):
		os.makedirs(armino_path + "/build/html")

	if (target == "all"):
		build_with_target(clean, "bk7235")
		build_with_target(clean, "bk7237")
		build_with_target(clean, "bk7256")
	else:
		build_with_target(clean, target)

def build_doc(target):
	build_doc_internal(False, target)

def main(argv):
	if (len(argv) > 1 and argv[1] == "clean"):
		target = "all"
		if (len(argv) == 3):
			target = sys.argv[2]
		build_doc_internal(True, target)
	else:
		build_doc_internal(False, "all")

if __name__ == "__main__":
	main(sys.argv)
