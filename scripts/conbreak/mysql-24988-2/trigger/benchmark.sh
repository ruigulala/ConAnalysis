#!/bin/bash

# The wrapper script will call this script once lldb is loaded
# and the target executable is ready

# Put your benchmarking / bug-triggering input code here...
sysbench --test=oltp --oltp-table-size=1000000 --oltp-test-mode=complex \
         --oltp-read-only=off  --num-threads=10 --max-requests=1000 \
         --mysql-db=dbca --mysql-user=root run &>/dev/null
