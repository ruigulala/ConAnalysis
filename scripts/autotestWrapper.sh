#!/bin/bash
# Usage: ./autotestWrapper.sh apache-21287 syncloop
APP=$1
FILES=$2
for f in $FILES*
do
    echo $f
	./autotestSyncLoop.sh $APP $f >| "finalReport_$f" 2>&1
done
