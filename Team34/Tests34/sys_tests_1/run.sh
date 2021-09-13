#!/usr/bin/env bash

set -e
echo "running autotester... (system tests 1)"

if [[ -z AUTOTESTER_EXE ]]; then
	AUTOTESTER_EXE=../../Code34/build/src/autotester/autotester
fi

if [[ ! -e $AUTOTESTER_EXE ]]; then
	AUTOTESTER_EXE=../../Code34/objs/autotester
fi

if [[ ! -e $AUTOTESTER_EXE ]]; then
	echo "could not find autotester!"
	exit 1
fi

for f in *.txt; do
	echo "running" $f "..."
	if [[ "$*" == "-q" ]]; then
		$AUTOTESTER_EXE Source.simple $f output-${f%.txt}.xml > /dev/null
	else
		$AUTOTESTER_EXE Source.simple $f output-${f%.txt}.xml
	fi
done

