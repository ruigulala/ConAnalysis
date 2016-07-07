#!/usr/bin/python2.7

import lldb
import commands
import argparse
import sys

CBREAKPOINT_SET = set()
THREAD_ARR = []

def ifCBreakpointsReached():
	for br in CBREAKPOINT_SET:
		if br.GetHitCount() == 0:
			return False
	return True


def SetConcurrentBreakpoint(debugger, command, result, internal_dict):
	args = command.strip().split(":")
	filename = args[0]
	linenum = int(args[1])

	print "Breakpoint set at %s:%d" % (filename, linenum)

	target = debugger.GetSelectedTarget()
	breakpoint = target.BreakpointCreateByLocation(filename, linenum)
	CBREAKPOINT_SET.add(breakpoint)

	debugger.HandleCommand('breakpoint command add -F conbreak.breakpoint_callback')


def breakpoint_callback(frame, bp_loc, dict):
	thread = frame.GetThread()
	process = thread.GetProcess()
	ID = thread.GetThreadID()
	print "A sub-breakpoint is triggered by thread %d." % (ID)

	global THREAD_ARR
	obj = [str(frame.FindVariable("obj")).split()[-1]]
	obj.append(ID)

	print ">>>>>>>>>>>> obj = " + str(obj)
	print ">>>>>>>>>>>> arr = " + str(THREAD_ARR)

	# Is there a better way to check if addr matches and tid does not?
	intersection = [i for i in THREAD_ARR if i[0] == obj[0] and i[1] != obj[1]]
	if len(intersection) > 0:
		print ">>>>>>>>>>>> FOUND!"
		print intersection[0]
		print obj
		print "********************************* HALT *********************************"

		# Stop everything, return control to user
		for t in process:
			t.Suspend()
		process.Stop()

	else:
		print ">>>>>>>>>>>> Nope. Continuing..."
		THREAD_ARR.append(obj)

		# Stop threads at a breakpoint, let all other threads go
		full = True
		for t in process:
			if all(t.GetThreadID() != tup[1] for tup in THREAD_ARR):
				t.Resume()
				full = False
			else:
				t.Suspend()

		# If all threads already stopped and no match, release all threads and try again
		if full:
			del THREAD_ARR[:]
			for t in process:
				t.Resume()

		process.Continue()


def __lldb_init_module(debugger, dict):
	# This initializer is being run from LLDB in the embedded command interpreter
	# Add any commands contained in this module to LLDB
	debugger.HandleCommand('command script add -f conbreak.SetConcurrentBreakpoint cbr')
	print "The \"cbr\" python command has been installed and is ready for use."






