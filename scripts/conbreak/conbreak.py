#!/usr/bin/python2.7

import lldb
import commands
import argparse
import sys

CBREAKPOINT_SET = set()

def ifCBreakpointsReached():
    for br in CBREAKPOINT_SET:
        if br.GetHitCount() == 0:
            return False
    return True

def SetConcurrentBreakpoint(debugger, command, result, internal_dict):
    
    # A hack in order to let ArgumentParser work
    sys.argv = ("cbr " + command).split()
    
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', type=str, dest="file_name", action="store",
            help="File name for the cbreakpoint")
    parser.add_argument('-l', type=int, dest="line_num", action="store",
        help="Line number of the cbreakpoint")
    args = parser.parse_args()
    
    target = debugger.GetSelectedTarget()
    # Use API to set the breakpoint
    breakpoint = target.BreakpointCreateByLocation(args.file_name, args.line_num)
    CBREAKPOINT_SET.add(breakpoint)
    debugger.HandleCommand('breakpoint command add -F cbreak.breakpoint_callback')

def breakpoint_callback(frame, bp_loc, dict):

    thread = frame.GetThread()
    process = thread.GetProcess()
    ID = thread.GetThreadID()
    print "A sub-breakpoint is triggered by thread %d." % (ID)

    for t in process:
        if t.GetStopReason() == lldb.eStopReasonBreakpoint:
            t.Suspend()

    if not ifCBreakpointsReached():
        process.Continue()
    else:
        for t in process:
            t.Resume()
        process.Stop()

def __lldb_init_module(debugger, dict):
    # This initializer is being run from LLDB in the embedded command interpreter
    # Add any commands contained in this module to LLDB
    debugger.HandleCommand('command script add -f conbreak.SetConcurrentBreakpoint cbr')
    print "The \"cbr\" python command has been installed and is ready for use."






