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

Notice that, right now, we're only allowing 1 race record. Because different
race records may require different binaries(called by main using subprocess). 
Currently, our LLVM pass can only takes in one bc file.
'''

# All the defined regular expressions
regCallStackLine = re.compile("==[0-9]*==[\s]*(at|by) "
        "[0-9A-Fx]*: ([a-zA-Z0-9_:~]*)(\([0-9A-Za-z\*_, ]*\))? "
        "(\([a-zA-Z0-9_\.]*:[0-9]*\))")
regLineBreak = re.compile("==[0-9]*==[\s]*$")
regReportStart = re.compile("==[0-9]*== (Possible data race during read|"
        "This conflicts with a previous read)")

def main(args):
    try:
        fin = open(args.raceReportIn, "r")
    except IOError:
        sys.stderr.write('Error: Input file does not exist!\n')
        exit(1)
    fout = open(args.raceReportOut, "w")
    ifReportStart = False
    for line in fin:
        callStackLine = regCallStackLine.match(line)
        lineBreak = regLineBreak.match(line)
        reportStart = regReportStart.match(line)
        if reportStart:
            ifReportStart = True
        elif callStackLine:
            if ifReportStart:
                if callStackLine.group(2) != "mythread_wrapper":
                    fout.write(callStackLine.group(2) + " " 
                        + callStackLine.group(4) + "\n")
                else:
                    break
        elif lineBreak:
            if ifReportStart:
                ifReportStart = False
    fin.close()
    fout.close()
    
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
