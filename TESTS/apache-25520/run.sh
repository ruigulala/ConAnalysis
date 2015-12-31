#!/bin/bash

# This script combines our front end & back end together to analyze vulnerable
# data races. Before running this script, we're assuming you've already build
# our project using CMake.

BUILD_DIRECTORY=$CONANAL_ROOT/build

if [ $# -gt "1" ]
then
    echo "Usage: $0 [ no_race_detector | no_static_analysis ]"
    echo "Example (enable race detection and llvm analysis): $0"
    echo "Example (llvm static analysis only): $0 no_race_detector"
    exit 1;
fi

if [ "$1" != "no_race_detector" ]
then
	# Step 1: You *must* build our project using CMake before running this script.
    # Create a folder named build in the top level source code folder.
    if [ -d "$BUILD_DIRECTORY" ]
    then
        echo "Seems you've built our project, very good!"
    else
        echo "Error: Build ConAnalysis using CMake before running this script!"
        exit 1
    fi

    # Step 2: We'll run valgrind to detect race during this step.
    if [ $CONANAL_ROOT = '' ]
    then
        echo "Error: Please set CONANAL_ROOT in your enviroment!"
        exit 1
    fi

    cd $CONANAL_ROOT/concurrency-exploits/apache-25520
    if [ $? -ne 0 ]
    then
        echo "Error: Couldn't enter submodule concurrency-exploits."
        echo "Did you forget to \"git submodule update --init --recursive\""
        exit 1
    fi

    if [ -f "apache-install/bin/apachectl" ] 
    then
        echo "Seems you've built apache already, very good!"
    else
        echo "Error: You need to build libsafe before running this script."
        echo "You can use mk.sh to automatically build it."
    fi

    valgrind --version
    if [ $? -ne 0 ]
    then
        echo "Error: Please install valgrind before running this script."
        echo "We strongly recommend using valgrind 3.11."
    fi
	cd $CONANAL_ROOT/TESTS/apache-25520
    # Start valgrind here!
	valgrind --tool=helgrind --trace-children=yes --read-var-info=yes $CONANAL_ROOT/concurrency-exploits/apache-21287/apache-install/bin/apachectl -k start >| valgrind_latest.output 2>&1 &
    # Bug triggering input here!
    httperf --server=127.0.0.1 --port=7000 --uri=/index.html.en --num-conns=100 --num-calls=100
fi

if [ "$1" != "no_static_analysis" ]
then
    # Step 3: We'll run our LLVM static analysis pass to analyze the race
	# Use inotify to monitor any new files within the folder
	inotifywait -m `pwd` -e create |
		while read file; do
			echo "The race report is newly added '$file'. Start static analysis"
			# do something with the file
			cp $file $CONANAL_ROOT/build/TESTS/apache-25520/
    		cd $CONANAL_ROOT/build/TESTS/apache-25520/
			./autotest.sh apache-25520 $file
		done &

	# Monitor Valgrind's output file to add new race reports
	if [ -f valgrind_latest.output ]
    then
        echo "Using valgrind_lastest.output to analyze."
        ./valgrindOutputParser.py --input valgrind_latest.output --output race_report &
    else
        ./valgrindOutputParser.py --input standard-output/valgrind.output --output race_report &
    fi
fi
