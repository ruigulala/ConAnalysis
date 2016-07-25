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

def send_signal():
    """Test that lldb command 'process signal SIGUSR1' sends a signal to the inferior process."""

    exe = os.path.join(os.getcwd(), "a.out")

    # Create a target by the debugger.
    target = dbg.CreateTarget(exe)
    assertTrue(target, VALID_TARGET)

    # Now create a breakpoint on main.c by name 'c'.
    breakpoint = target.BreakpointCreateByLocation('main.c', line)
    assertTrue(breakpoint and
                    breakpoint.GetNumLocations() == 1,
                    VALID_BREAKPOINT)

    # Get the breakpoint location from breakpoint after we verified that,
    # indeed, it has one location.
    location = breakpoint.GetLocationAtIndex(0)
    assertTrue(location and
                    location.IsEnabled(),
                    VALID_BREAKPOINT_LOCATION)

    # Now launch the process, no arguments & do not stop at entry point.
    launch_info = lldb.SBLaunchInfo([exe])
    launch_info.SetWorkingDirectory(get_process_working_directory())

    process_listener = lldb.SBListener("signal_test_listener")
    launch_info.SetListener(process_listener)
    error = lldb.SBError()
    process = target.Launch(launch_info, error)
    assertTrue(process, PROCESS_IS_VALID)

    runCmd("process handle -n False -p True -s True SIGUSR1")

    thread = lldbutil.get_stopped_thread(process, lldb.eStopReasonBreakpoint)
    assertTrue(thread.IsValid(), "We hit the first breakpoint.")

    # After resuming the process, send it a SIGUSR1 signal.

    setAsync(True)

    assertTrue(process_listener.IsValid(), "Got a good process listener")

    # Disable our breakpoint, we don't want to hit it anymore...
    breakpoint.SetEnabled(False)

    # Now continue:
    process.Continue()

    # If running remote test, there should be a connected event
    if lldb.remote_platform:
        match_state(process_listener, lldb.eStateConnected)

    match_state(process_listener, lldb.eStateRunning)

    # Now signal the process, and make sure it stops:
    process.Signal(signal.SIGUSR1)

    match_state(process_listener, lldb.eStateStopped)

    # Now make sure the thread was stopped with a SIGUSR1:
    threads = lldbutil.get_stopped_threads(process, lldb.eStopReasonSignal)
    assertTrue(len(threads) == 1, "One thread stopped for a signal.")
    thread = threads[0]

    assertTrue(thread.GetStopReasonDataCount() >= 1, "There was data in the event.")
    assertTrue(thread.GetStopReasonDataAtIndex(0) == signal.SIGUSR1, "The stop signal was SIGUSR1")

def KeyForOptions(options, debugger):
    key = None
    if options.key:
        key = options.key
    elif options.block:
        frame = GetSelectedFrame(debugger)
        block = GetSelectedFrame(debugger).EvaluateExpression(options.argument)
        debugger.HandleCommand('expr struct $BlockLiteral {void *isa; int flags; int reserved; void (*invoke)(void *, ...);};')
        blockCast = frame.EvaluateExpression("($BlockLiteral *)%d" % block.GetValueAsUnsigned())
        invoke = blockCast.GetChildMemberWithName("invoke")
        key = "%d" % invoke.GetValueAsUnsigned()
    elif options.argument:
        # Fix this to check the SBValues type and probably use GetValueAsUnsigned for pointers
        argument = str(options.argument)
        result = GetSelectedThread(debugger).GetFrameAtIndex(0).EvaluateExpression(argument)
        key = str(result.GetObjectDescription())
    # Filter for non object sbvalues returning none or and empty object descriptions returning nil
    if key == "<nil>":
        key = None
    if key == "None":
        key = None
    if key == None:
        print "Unable to get a key for {a}".format(a=options.argument)
    return key

def __lldb_init_module(debugger, dict):
    # This initializer is being run from LLDB in the embedded command interpreter
    # Add any commands contained in this module to LLDB
    print "Initializing cbreak module"
    debugger.HandleCommand('command script add -f cbreak.SetConcurrentBreakpoint cbr')
