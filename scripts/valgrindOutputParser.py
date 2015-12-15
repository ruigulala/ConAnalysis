#!/usr/bin/python2.7
import argparse
import re
import sys

'''
This python script transfers valgrind race report to the format required by
our LLVM pass.
A typical valgrind race report line
==7832==    at 0x8068BB7: ap_buffered_log_writer (mod_log_config.c:1345)
will be changed to
ap_buffered_log_writer (mod_log_config.c:1345)

'''

# All the defined regular expressions
regCallStackLine = re.compile("==[0-9]*==[\s]*(at|by) "
        "[0-9A-Fx]*: ([a-zA-Z0-9_:~]*)(\([0-9A-Za-z\*_, ]*\))? "
        "(\([a-zA-Z0-9_\.]*:[0-9]*\))")
# Detects the output of a racing variable
regRacingVar = re.compile('==[0-9]*==  (Location|Address) [a-z0-9 ]*'
        '"([0-9A-Za-z_\.\->]*)"')
        #'.*')
# Detects the start of a variable read
regReportRStart = re.compile("==[0-9]*== (Possible data race during read|"
        "This conflicts with a previous read).*")
# Detects the start of a race report block
regReportBStart = re.compile("==[0-9]*== --------------------------------"
        "--------------------------------")
# Detects a line break
regLineBreak = re.compile("==[0-9]*==[\s]*$")

def main(args):
    fileNumCounter = 0
    ifReadStart = False
    ifEnd = True
    lineBreakCounter = 0

    try:
        fin = open(args.raceReportIn, "r")
    except IOError:
        sys.stderr.write('Error: Input file does not exist!\n')
        exit(1)

    resultList = []
    for line in fin:
        readStart = regReportRStart.match(line)
        callStackLine = regCallStackLine.match(line)
        lineBreak = regLineBreak.match(line)
        racingVar = regRacingVar.match(line)
        if readStart:
            lineBreakCounter = 0
            ifReadStart = True
            ifEnd = False
        elif callStackLine:
            if ifReadStart and not ifEnd:
                if callStackLine.group(2) != "mythread_wrapper":
                    resultList.append(callStackLine.group(2) + " " 
                        + callStackLine.group(4) + "\n")
                else:
                    ifEnd = True
        elif lineBreak:
            lineBreakCounter += 1
        elif racingVar:
            if lineBreakCounter <= 1:
                if len(resultList) > 0:
                    resultList.insert(0, racingVar.group(2) + "\n") 
                    fout = open(args.raceReportOut + str(fileNumCounter), "w")
                    for item in resultList:
                        fout.write("%s" % item)
                    ifReadStart = False
                    ifEnd = True
                    fileNumCounter += 1
                    fout.close()
                    del resultList[:]
    fin.close()

if __name__=='__main__':
    parser = argparse.ArgumentParser(description='Valgrind output parser')
    parser.add_argument('--input', type=str, dest="raceReportIn",
            action="store", default="none", required=True,
            help="Valgrind raw race report")
    parser.add_argument('--output', type=str, dest="raceReportOut",
            action="store", default="none", required=True,
            help="Parsed race report")
    args = parser.parse_args()
    main(args)
