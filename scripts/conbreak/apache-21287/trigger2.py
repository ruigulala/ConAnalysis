#!/usr/bin/python2.7

import lldb
import commands
import argparse
import sys

COUNT = 0
OBJ_ARR = []
THREAD_STATS = []

def set_trigger():
	print "Setting breakpoints..."
	target = lldb.debugger.GetSelectedTarget()
	bp_read = target.BreakpointCreateByLocation("apr_tables.c", 597)
	bp_write = target.BreakpointCreateByLocation("mod_mem_cache.c", 691)

	bp_read.SetScriptCallbackFunction("trigger2.read_callback")
	bp_write.SetScriptCallbackFunction("trigger2.write_callback")
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


def write_callback(frame, bp_loc, dict):
	thread = frame.GetThread()
	process = thread.GetProcess()
	ID = thread.GetThreadID()

	global COUNT
	global OBJ_ARR

	COUNT += 1
	if len(OBJ_ARR) < 5:
		#print ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>" + str(frame.FindVariable("buf"))
		
		obj = str(frame.FindVariable("buf")).split()[-2]
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


def read_callback(frame, bp_loc, dict):
	thread = frame.GetThread()
	process = thread.GetProcess()
	ID = thread.GetThreadID()

	#print ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>" + str(frame.FindVariable("key"))

	global OBJ_ARR
	print "INFO:  OBJ_ARR = " + str(OBJ_ARR)

	key1 = str(frame.FindVariable("key")).split()[-2]
	key2 = str(frame.FindVariable("next_elt").GetChildMemberWithName("key")).split()[-2]

	print "READ:  Reading &key1 = " + key1
	print "READ:  Reading &key2 = " + key2

	if key1 in OBJ_ARR:
		print ">>>>>>>>>> FREE:  Found match at " + key1
		print "**************************** HALT ****************************"
		update_run(process, 1)
	elif key2 in OBJ_ARR:
		print ">>>>>>>>>> FREE:  Found match at " + key2
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








