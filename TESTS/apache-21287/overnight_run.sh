#!/bin/bash

# This script combines the race detector frontend and the LLVM static analysis
# backend. The frontend will keep runnning and generating race reports. The
# script will constantly check the result of frontend and parse the output to
# feed into backend.

BUILD_DIRECTORY=$CONANAL_ROOT/build

if [ $# -gt "1" ]
then
    echo "Usage: $0 [ no_race_detector | no_static_analysis ]"
    echo "Example (enable race detection and llvm analysis): $0"
    echo "Example (llvm static analysis only): $0 no_race_detector"
    exit 1;
fi

if [ "$1" != "no_race_detector" ]
    valgrind --version
    if [ $? -ne 0 ]
    then
        echo "Error: Please install valgrind before running this script."
        echo "We recommend using valgrind 3.11."
    fi
    if [ ! -f $CONANAL_ROOT/concurrency-exploits/apache-21287/apache-install/bin/apachectl ]
        echo "Please build apache-2.0.48 before running this script."
        echo "Checkout mk.sh under concurrency-exploits/apache-21287"
        exit 1
    valgrind --tool=helgrind --trace-children=yes --read-var-info=yes $CONANAL_ROOT/concurrency-exploits/apache-21287/apache-install/bin/apachectl start > valgrind_output 2>&1 &
    echo "Race detector started."

if [ "$1" != "no_static_analysis" ]
    echo "hahaha"

