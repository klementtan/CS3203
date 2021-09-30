#!/usr/bin/env python3

import os
import sys
from xml.etree import ElementTree as ET

passed_tests = { }
failed_tests = { }
num_failed = 0
num_passed = 0

test_times = []

def get_dupes(xs):
	dupes = set()
	uniq = set()
	for x in xs:
		if x in uniq:
			dupes.add(x)
		else:
			uniq.add(x)
	return dupes


def parse_one(filename):
	# god, this language sucks
	global passed_tests
	global failed_tests
	global num_failed
	global num_passed

	global test_times

	try:
		root = ET.parse(filename).getroot()
	except Exception as e:
		print(f"malformed xml file '{filename}': {e}")
		return

	passed_tests[filename] = []
	failed_tests[filename] = []


	for query in root.iter("query"):
		name = query.find("id").get("comment")
		num = query.find("id").text

		test_name_str = f"{filename}/{num} - {name}"
		tmp = query.find("correct")
		if tmp is None:
			if query.find("exception") is not None:
				print(f"warning: {test_name_str} threw exception")
				failed_tests[filename].append((num, name))
				num_failed += 1
				continue
			elif query.find("timeout") is not None:
				print(f"query '{test_name_str}' timed out")
				num_failed += 1
				continue
			else:
				print(f"warning: {test_name_str} has no correct result")
				continue

		if query.find("failed") is not None:
			failed_tests[filename].append((num, name))
			num_failed += 1
		elif query.find("passed") is not None:
			passed_tests[filename].append((num, name))
			num_passed += 1
		else:
			print(f"warning: {test_name_str} neither passed nor failed...")
			continue

		if query.find("time_taken") is not None:
			time_taken = float(query.find("time_taken").text)
			test_times.append(time_taken)


		correct_ans = query.find("correct").text
		if correct_ans is None:
			continue

		dupes = get_dupes(correct_ans.split(","))

		if len(dupes) > 0:
			print(f"warning: duplicate answers for {test_name_str}:")
			print(f"    {dupes}")



def iterate_dir(dir):
	for root, dirs, files in os.walk(dir):
		for file in files:
			if file.endswith(".xml"):
				parse_one(os.path.join(root, file))

def parse_results_from_folder(folder):
	iterate_dir(folder)

def get_failed_tests():
	return failed_tests

def get_passed_tests():
	return passed_tests

def get_num_failed():
	return num_failed

def get_num_passed():
	return num_passed

def get_test_times():
	return test_times

def main():
	if len(sys.argv) < 2:
		print("usage: ./parse_result_xml.py [-q] <folder|result.xml>...")
		sys.exit(1)

	inputs = sys.argv[1:]

	quiet = False
	if sys.argv[1] == "-q":
		quiet = True
		inputs = sys.argv[2:]

	if len(inputs) == 0:
		print("at least one input must be given")
		sys.exit(1)

	for file in inputs:
		if os.path.isdir(file):
			iterate_dir(file)
		else:
			parse_one(file)

	print(f"{num_passed}/{num_passed + num_failed} tests passed, {num_failed} failed")

	for (name, tests) in failed_tests.items():
		if len(tests) == 0:
			continue

		if name.startswith("output-"):
			name = name[len("output-"):]

		print(f"test {name}:")
		for t in tests:
			print(f"    {t[0]} - {t[1]}")

	if not quiet:
		sys.exit(num_failed)
	else:
		return

if __name__ == "__main__":
	main()
