#!/usr/bin/env python3

import os
import sys
import subprocess

import result_parser as rp

def run_autotester(autotester_exe, folder, source, query):
	source = os.path.basename(source)
	query = os.path.basename(query)

	source_path = os.path.join(folder, source)
	query_path = os.path.join(folder, query)

	# we know query ends in ".txt"
	xml_path = os.path.join(folder, f"output-{query[:-4]}.xml")

	print(f"    running '{query} - {source}'")
	subprocess.run([autotester_exe, source_path, query_path, xml_path], stdout=subprocess.DEVNULL)


def find_source_files(folder):
	sources = []
	for name in os.listdir(folder):
		thing = os.path.join(folder, name)
		if not os.path.isfile(thing):
			continue
		if (name.endswith(".simple") or name.endswith("_source.txt")) and ("numbered" not in thing):
			sources.append(thing)

	return sources

def get_matching_source(query):
	if query.endswith("_queries.txt"):
		return f"{query[:-len('_queries.txt')]}_source.txt"
	elif query.endswith(".query"):
		return f"{query[:-len('.query')]}.simple"
	else:
		return None

def run_tests_in_folder(autotester_exe, folder):
	sources = find_source_files(folder)
	queries = []

	if len(sources) == 0:
		print(f"no sources in '{folder}', skipping")
		return

	print(f"test set {folder}:")

	for name in os.listdir(folder):
		thing = os.path.join(folder, name)
		if os.path.isfile(thing) and (thing.endswith("_queries.txt") or thing.endswith(".query")
			or (thing.endswith(".txt") and not thing.endswith("_source.txt"))):
			if len(sources) == 1:
				queries.append((sources[0], thing))
			elif (src := get_matching_source(thing)) is not None:
				queries.append((src, thing))
			else:
				print(f"could not find source for '{name}', skipping")


	for s, q in queries:
		run_autotester(autotester_exe, folder, s, q)


def run_tests(autotester_exe, folder):
	seen = set()
	for root, dirs, files in os.walk(folder):
		if root in seen:
			continue

		if len(dirs) == 0:
			seen.add(root)
			run_tests_in_folder(autotester_exe, root)
		else:
			for d in dirs:
				seen.add(os.path.join(root, d))
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
		print(f"'{autotester_exe}' is not a valid autotester executable")
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
	total_tests = num_failed + num_passed

	with open("autotester_summary.txt", "wb") as f:
		log_string(f, f"{num_passed}/{total_tests} ({100 * num_passed / max(1, total_tests):.1f}%) " +
			f"test{'' if num_passed == 1 else 's'} passed, {num_failed} failed")

		for (filename, tests) in failed_tests.items():
			if len(tests) == 0:
				continue
			log_string(f, f"test {filename}:")
			for t in tests:
				log_string(f, f"    {t[0]} - {t[1]}")


if __name__ == "__main__":
	main()
