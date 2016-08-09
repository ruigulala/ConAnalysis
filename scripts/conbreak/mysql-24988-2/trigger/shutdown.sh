#!/bin/bash

# This script will be run after each lldb analysis
# Like startup.sh, this script is completely optional.  Only include this
# file if your program has additional cleanup/shutdown procedures beyond just
# killing the target executable (handled by wrapper.sh)

# Shutdown code starts here
sysbench --test=oltp --mysql-db=dbca --mysql-user=root cleanup &>/dev/null

pkill mysql
pkill mysqld
