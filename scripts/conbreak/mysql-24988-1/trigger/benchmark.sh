#!/bin/bash

# The wrapper script will call this script once lldb is loaded
# and the target executable is ready

# Put your benchmarking / bug-triggering input code here...
cd scripts

./client1.sh &>/dev/null &
./client2.sh &>/dev/null &

sleep 5

./client1.sh &>/dev/null &
./client2.sh &>/dev/null &

trap "pkill client1; pkill client2" TERM

# Wait until TERM signal from wrapper.sh
wait $!

