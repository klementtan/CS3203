#!/usr/bin/env python3

import os
import sys
import subprocess

import result_parser as rp

def run_autotester(autotester_exe, folder, source, query):
	source_path = os.path.join(folder, source)
	query_path = os.path.join(folder, query)

	# we know query ends in ".txt"
	xml_path = os.path.join(folder, f"output-{query[:-4]}.xml")

	print(f"    running '{query}'")
	subprocess.run([autotester_exe, source_path, query_path, xml_path], stdout=subprocess.DEVNULL)

def run_tests_in_folder(autotester_exe, folder):
	source = ""
	queries = []

	print(f"running in {folder}")

	for name in os.listdir(folder):
		thing = os.path.join(folder, name)
		if os.path.isfile(thing) and thing.endswith(".simple") and not thing.endswith("-numbered.simple"):
			if len(source) == 0:
				source = name
			else:
				print(f"multiple source files in {folder}, skipping...")
				return

		elif os.path.isfile(thing) and thing.endswith(".txt"):
			queries.append(name)

	if len(source) == 0:
		return

	print(f"test set {folder}:")
	print(f"    source: {source}")
	print(f"    queries ({len(queries)}): {queries}")

	for query in queries:
		run_autotester(autotester_exe, folder, source, query)


def run_tests(autotester_exe, folder):
	for root, dirs, files in os.walk(folder):
		if len(dirs) == 0:
			run_tests_in_folder(autotester_exe, root)
		else:
			for d in dirs:
				run_tests_in_folder(autotester_exe, os.path.join(root, d))




def clean_outputs():
	for root, dirs, files in os.walk("."):
		for file in files:
			if file.endswith(".xml"):
				uwu = os.path.join(root, file)
				print(f"removing '{uwu}'")
				os.remove(uwu)

def log_string(f, s):
	f.write((s + "\n").encode())
	print(s)


def main():
	if len(sys.argv) < 3:
		print("usage: ./test_runner.py [clean] <autotester_path> <folders>...")
		sys.exit(1)

	if sys.argv[1] == "clean":
		clean_outputs()
		return

	autotester_exe = sys.argv[1]
	if (not os.path.exists(autotester_exe)) or (not os.path.isfile(autotester_exe)):
		print(f"'{autotester_exe}' is not a valid thing")
		sys.exit(1)

	for folder in sys.argv[2:]:
		if os.path.isdir(folder):
			run_tests(autotester_exe, folder)
		else:
			print(f"'{folder}' is not a folder, skipping...")


	print("\nparsing results...")
	for folder in sys.argv[2:]:
		if os.path.isdir(folder):
			rp.parse_results_from_folder(folder)

	failed_tests = rp.get_failed_tests()
	passed_tests = rp.get_passed_tests()
	num_failed = rp.get_num_failed()
	num_passed = rp.get_num_passed()

	with open("autotester_summary.txt", "wb") as f:
		log_string(f, f"{num_passed}/{num_passed + num_failed} tests passed, {num_failed} failed")

		for (filename, tests) in failed_tests.items():
			if len(tests) == 0:
				continue
			log_string(f, f"test {filename}:")
			for t in tests:
				log_string(f, f"    {t[0]} - {t[1]}")


if __name__ == "__main__":
	main()
