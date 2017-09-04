#!/bin/bash
# Usage: ./tsanOutputConverger.sh folder1 folder2 output_folder

if [ ! $# -eq 5 ]; then
  echo -e "Usage: ./convergerWrapper.sh [syncloop | verifier | conanalysis] run1.out run2.out run3.out run4.out"
  exit 1
fi

TYPE=$1
RUN1=$2
RUN2=$3
RUN3=$4
RUN4=$5

mkdir run1 run2 run3 run4

cd run1
$CONANAL_ROOT/scripts/tsanOutputParser.py --mode normal --outputtype $TYPE --input ../$2 --output run1_
cd ..

cd run2
$CONANAL_ROOT/scripts/tsanOutputParser.py --mode normal --outputtype $TYPE --input ../$3 --output run2_
cd ..

cd run3
$CONANAL_ROOT/scripts/tsanOutputParser.py --mode normal --outputtype $TYPE --input ../$4 --output run3_
cd ..

cd run4
$CONANAL_ROOT/scripts/tsanOutputParser.py --mode normal --outputtype $TYPE --input ../$5 --output run4_
cd ..

$CONANAL_ROOT/scripts/tsanOutputConverger.sh run1 run2 tmp1
$CONANAL_ROOT/scripts/tsanOutputConverger.sh tmp1 run3 tmp2
$CONANAL_ROOT/scripts/tsanOutputConverger.sh tmp2 run4 tmp3

cp -r tmp3 result
cd result
fdupes . -dqN

echo "Done Final Merging. Please check the result folder!"
