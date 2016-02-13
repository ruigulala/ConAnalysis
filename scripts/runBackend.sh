#!/bin/bash

if [ ! $# -eq 2 ]; then
  echo -e "Please input the argument"
  echo -e "./runBackend.sh mysql-35589 tsan_race_report*"
  exit -1
fi

for f in $2*
do
 echo "Processing $f"
 # do something on $f
 ./autotest.sh $1 $f 2>| "output_$f"
done
