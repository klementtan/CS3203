#!/usr/bin/env python3

import sys
from xml.etree import ElementTree as ET

passed_tests = { }
failed_tests = { }
num_failed = 0
num_passed = 0

def get_dupes(xs):
	dupes = []
	uniq = set()
	for x in xs:
		if x in uniq:
			dupes.append(x)
		else:
			uniq.add(x)
	return dupes


def parse_one(filename):
	# god, this language sucks
	global passed_tests
	global failed_tests
	global num_failed
	global num_passed

	root = ET.parse(filename).getroot()
	passed_tests[filename] = []
	failed_tests[filename] = []


	for query in root.iter("query"):
		name = query.find("id").get("comment")
		num = query.find("id").text

		if query.find("failed") is not None:
			failed_tests[filename].append((num, name))
			num_failed += 1
		else:
			passed_tests[filename].append((num, name))
			num_passed += 1

		correct_ans = query.find("correct").text
		if correct_ans is None:
			continue

		dupes = get_dupes(correct_ans.split(","))

		if len(dupes) > 0:
			print(f"warning: duplicate results in expected answers for {num} - {name}:")
			for d in dupes:
				print(f"    {d} appears more than once")

def main():
	if len(sys.argv) < 2:
		print("usage: ./parse_result_xml.py <result.xml>")
		sys.exit(1)

	for file in sys.argv[1:]:
		parse_one(file)

	print(f"{num_passed}/{num_passed + num_failed} tests passed, {num_failed} failed")

	for (filename, tests) in failed_tests.items():
		if len(tests) == 0:
			continue
		print(f"test {filename}:")
		for t in tests:
			print(f"    {t[0]} - {t[1]}")

	sys.exit(num_failed)


if __name__ == "__main__":
	main()
