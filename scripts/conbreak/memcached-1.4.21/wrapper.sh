#!/bin/bash

# Error handling
yell() { echo "$0: $*" >&2; }
die() { yell "$*"; exit 111; }
try() { "$@" || die "cannot $*"; }

# User defined variables
tsan_reports_folder='tsan_reports'
lldb_output='lldb_out.txt'

cd trigger

for file in $tsan_reports_folder/*; do
	echo "Testing $file..."

	# Create symlink to tsan report for trigger to read
	ln -sf $file report.txt

	# Startup script (optional)
	if [[ -f startup.sh ]]; then
		sh startup.sh &>/dev/null
	fi

	# Start lldb through Expect script
	pushd $CONANAL_ROOT/concurrency-exploits/memcached-1.4.21/memcached-1.4.21
	make test
	popd

	# Shutdown script (optional)
	if [[ -f shutdown.sh ]]; then
		sh shutdown.sh &>/dev/null
	fi

	# In case exit wasn't clean so next run there aren't any errors
	pkill lldb
	pkill target

	# Parse lldb output for match, report results to file
	grep_output="$(grep '*** HALT ***' $lldb_output)"

	if [[ -n "$grep_output" ]]; then
		# TODO: Grab variable values from lldb report and print them
		cat 'report.txt' >> ../out.txt
		printf '\n' >> ../out.txt
	else
		# TODO: Print reason for irreproducibility
		cat 'report.txt' >> ../nr.txt
		printf '\n' >> ../nr.txt
	fi

done

# Cleanup temporary files
rm 'report.txt'
rm $lldb_output
