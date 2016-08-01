#!/bin/bash
# Usage: ./tsanOutputConverger.sh folder1 folder2 output_folder

set -x

if [ ! $# -eq 3 ]; then
  echo -e "Usage: ./tsanOutputConverger.sh folder1 folder2 output_folder"
  exit 1
fi

FOLDER1=$1
FOLDER2=$2
FOLDERMERGED=$3

# Check whether the two comparison folders exist.
if [ ! -d "$FOLDER1" ] || [ ! -d "$FOLDER2" ]; then
    echo "Error: Comparison folders does not exist."
    exit 1
fi

# Check whether the two comparison folders are different.
if [ "$1" == "$2" ]; then
    echo "Error: Comparison folders should be different."
    exit 1
fi

# Check if the result folder exists.
if [ -d "$FOLDERMERGED" ]; then
    echo "Error: Destination folder $3 exists."
    exit 1
fi

cp -r $FOLDER1 $FOLDERMERGED


for f1 in $FOLDER2/*
do
    IFDIFFERENT=1
    for f2 in $FOLDER1/*
    do
        diff $f1 $f2 > /dev/null
        if [ $? -eq 0 ] ; then
            IFDIFFERENT=0
        else
            continue
        fi
    done
    if [ "$IFDIFFERENT" -eq 1 ]; then
        cp $f1 $FOLDERMERGED
    fi

done

echo "Done Merging. Please check the result folder $FOLDERMERGED!"
