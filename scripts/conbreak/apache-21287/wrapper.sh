#!/bin/bash

tsan_reports_folder="tsan_reports"
lldb_command_file="lldb_cmd.txt"

cd trigger

for file in $tsan_reports_folder/*
do
	# Create symlink to tsan report for trigger to read
	ln -sf $file report.txt

	# Start lldb through Expect script
	./interface.exp &>/dev/null	&

	# Wait for lldb to start up
	sleep 5

	# Send bug-triggering input
	./benchmark.sh

	# In case exit wasn't clean so next run there aren't any errors
	pkill lldb
	pkill target

	# TODO: Parse out.txt for match, report results to file

done

# Cleanup temporary files
#rm report.txt
