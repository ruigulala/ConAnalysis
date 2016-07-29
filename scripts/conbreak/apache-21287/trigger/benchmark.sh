#!/bin/bash

# The wrapper script will call this script once lldb is loaded
# and the target executable is ready

# Put your benchmarking / bug-triggering input code here...
ab -n 1000 -c 100 127.0.1.1:7000/pippo.php?variable=88 &>/dev/null
