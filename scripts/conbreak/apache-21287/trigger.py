#!/usr/bin/python2.7

import lldb
import commands
import argparse
import sys

COUNT = 0
OBJ_ARR = []
THREAD_STATS = []

def set_trigger():
	print "Setting breakpoints in mod_mem_cache.c at lines 354 and 653..."
	target = lldb.debugger.GetSelectedTarget()
	bp_read = target.BreakpointCreateByLocation("mod_mem_cache.c", 354)
	bp_write = target.BreakpointCreateByLocation("mod_mem_cache.c", 653)
	bp_free = target.BreakpointCreateByName("cleanup_cache_object")

	bp_read.SetScriptCallbackFunction("trigger.read_callback")
	bp_write.SetScriptCallbackFunction("trigger.write_callback")
	bp_free.SetScriptCallbackFunction("trigger.free_callback")
	#debugger.HandleCommand('breakpoint command add -F trigger.trigger_callback')
	print("Configuration done!")


def update_run(process, stop):
	if stop:
		for t in process:
			t.Suspend()
		process.Stop()
	else:
		for t in process:
			found = False
			for stat in THREAD_STATS:
				if t.GetThreadID() == stat[0]:
					found = True
					if stat[1] == 0:
						t.Suspend()
					else:
						t.Resume()

			if not found:
				t.Resume()
		
		process.Continue()


def read_callback(frame, bp_loc, dict):
	thread = frame.GetThread()
	process = thread.GetProcess()
	ID = thread.GetThreadID()
	#print "READ:  A sub-breakpoint has been triggered by thread %d" % (ID)

	obj = frame.FindVariable("obj")
	cleanup = obj.GetChildMemberWithName("cleanup")
	
	print "READ:  Checking obj->cleanup at " + str(obj).split()[-1] + "..."
	print "READ:  obj->cleanup = " + str(cleanup).split()[-1]
	
	if len([1 for x in THREAD_STATS if x[0] == ID]) == 0:
		THREAD_STATS.append([ID, 1])
	else:
		for tup in THREAD_STATS:
			if tup[0] == ID:
				tup[1] = 1

	update_run(process, 0)


def write_callback(frame, bp_loc, dict):
	thread = frame.GetThread()
	process = thread.GetProcess()
	ID = thread.GetThreadID()
	#print "WRITE: A sub-breakpoint has been triggered by thread %d" % (ID)

	global COUNT
	global OBJ_ARR

	COUNT += 1
	if len(OBJ_ARR) < 5:
		obj = str(frame.FindVariable("obj")).split()[-1]
		OBJ_ARR.append(obj)

		print ">>>>>>>>>> WRITE: Blocking at " + obj + "..."
		if len([1 for x in THREAD_STATS if x[0] == ID]) == 0:
			THREAD_STATS.append([ID, 0])
		else:
			for tup in THREAD_STATS:
				if tup[0] == ID:
					tup[1] = 0
	else:
		if len([1 for x in THREAD_STATS if x[0] == ID]) == 0:
			THREAD_STATS.append([ID, 1])
		else:
			for tup in THREAD_STATS:
				if tup[0] == ID:
					tup[1] = 1
	

	if COUNT > 25:
		print ">>>>>>>>>> TIMEOUT: Suspending all threads..."
		COUNT = 0
		update_run(process, 1)
	else:
		update_run(process, 0)


def free_callback(frame, bp_loc, dict):
	thread = frame.GetThread()
	process = thread.GetProcess()
	ID = thread.GetThreadID()
	#print "FREE:  A sub-breakpoint has been triggered by thread %d" % (ID)

	global OBJ_ARR
	print "INFO:  OBJ_ARR = " + str(OBJ_ARR)

	obj = str(frame.FindVariable("obj")).split()[-1]
	cleanup = frame.FindVariable("obj").GetChildMemberWithName("cleanup")

	print "FREE:  Freeing &obj = " + obj
	print "FREE:  obj->cleanup = " + str(cleanup).split()[-1] 

	if obj in OBJ_ARR:
		print ">>>>>>>>>> FREE:  Found match at " + obj
		print "**************************** HALT ****************************"
		update_run(process, 1)
	else:
		if len([1 for x in THREAD_STATS if x[0] == ID]) == 0:
			THREAD_STATS.append([ID, 1])
		else:
			for tup in THREAD_STATS:
				if tup[0] == ID:
					tup[1] = 1

		update_run(process, 0)


def __lldb_init_module(debugger, dict):
    #debugger.HandleCommand('command script add -f trigger.set_trigger trig')
    #print "The \"trig\" python command has been installed and is ready for use."
	print "Setting trigger..."
	set_trigger()
	print "Starting server..."
	debugger.HandleCommand('run -k start -X')








