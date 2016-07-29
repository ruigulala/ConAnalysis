#!/usr/bin/python2.7

import lldb
import commands
import argparse
import sys
import random
import time
import threading

# USER SET GLOBAL VARIABLES
# Stable state wait time (in seconds, ie. 0.1 = 100ms), default = 1
WAIT_TIME = 1

# DO NOT CHANGE
LAST_BREAK = 0
RUNNING = True
OBJ_ARR = []
THREAD_ARR = []


def set_trigger():
	target = lldb.debugger.GetSelectedTarget()
	bp_read = target.BreakpointCreateByLocation("apr_tables.c", 597)
	bp_write = target.BreakpointCreateByLocation("mod_mem_cache.c", 691)

	bp_read.SetScriptCallbackFunction("trigger2.read_callback")
	bp_write.SetScriptCallbackFunction("trigger2.write_callback")
	print("Configuration done!")


# Release a thread if stable state is reached
def timer():
	threading.Timer(0.1, timer).start()

	if RUNNING and time.time() - LAST_BREAK > WAIT_TIME:
		release_bp()


def update_timer():
	global LAST_BREAK
	LAST_BREAK = time.time()


# Check to see if every thread is at a breakpoint
def all_bp_hit():
	for t in lldb.debugger.GetSelectedTarget().GetProcess():
		if not t.IsSuspended():
			return False

	return True


def release_bp():
	process = lldb.debugger.GetSelectedTarget().GetProcess()
	
	# Check if process is invalid, can cause errors
	if process.IsValid() == False:
		# print "######## WARNING: PROCESS IS INVALID ########"
		return

	obj_arr_len = len(OBJ_ARR)

	# No more suspended threads...
	if obj_arr_len == 0:
		return

	global OBJ_ARR
	global THREAD_ARR

	rand = random.randrange(0, obj_arr_len)
	thread = process.GetThreadByID(THREAD_ARR[rand])

	print str(time.time()) + " >>>>>>>>>> TIMEOUT: Releasing thread " + str(thread.GetThreadID())

	del OBJ_ARR[rand]
	del THREAD_ARR[rand]

	thread.Resume()
	process.Continue()


def read_callback(frame, bp_loc, dict):
	thread = frame.GetThread()
	process = thread.GetProcess()
	ID = thread.GetThreadID()

	obj = frame.FindVariable("obj")
	cleanup = obj.GetChildMemberWithName("cleanup")
	
	# Print is specific to bug #1
	#print "READ: [" + str(ID) + "] Checking obj->cleanup at " + str(obj).split()[-1] + "..."
	#print "READ: [" + str(ID) + "] obj->cleanup = " + str(cleanup).split()[-1]

	global OBJ_ARR
	global THREAD_ARR
	global RUNNING

	thread.Suspend()

	# TODO: Need better way to find TSAN reported variable addresses
	obj1 = str(frame.FindVariable("key")).split()[-2]
	obj2 = str(frame.FindVariable("next_elt").GetChildMemberWithName("key")).split()[-2]
	print str(time.time()) + " READ:  [" + str(ID) + "] Checking " + obj1 + " and " + obj2 + "..."
	

	# TODO TODO TODO: Generalize address matching algorithm
	if obj in OBJ_ARR:
		print ">>>>>>>>>> READ:  [" + str(ID) + "] Found match!"
		print "addr=" + obj + "  tid1=" + str(THREAD_ARR[OBJ_ARR.index(obj)]) + "  tid2=" + str(ID)
		print "**************************** HALT ****************************"
		RUNNING = False
		process.Stop()

	else:
		update_timer()

		OBJ_ARR.append(obj)
		THREAD_ARR.append(ID)

		# Randomly select a thread to be released if all threads are suspended
		if all_bp_hit():
			release_bp()
		else:
			process.Continue()


def write_callback(frame, bp_loc, dict):
	thread = frame.GetThread()
	process = thread.GetProcess()
	ID = thread.GetThreadID()

	global OBJ_ARR
	global THREAD_ARR
	global RUNNING

	thread.Suspend()

	# TODO: Need better way to find TSAN reported variable addresses
	obj = str(frame.FindVariable("buf")).split()[-2]
	print str(time.time()) + " WRITE: [" + str(ID) + "] Setting " + obj + "..."

	if obj in OBJ_ARR:
		print ">>>>>>>>>> WRITE: [" + str(ID) + "] Found match!"
		print "addr=" + obj + "  tid1=" + str(THREAD_ARR[OBJ_ARR.index(obj)]) + "  tid2=" + str(ID)
		print "**************************** HALT ****************************"
		RUNNING = False
		process.Stop()

	else:
		update_timer()

		OBJ_ARR.append(obj)
		THREAD_ARR.append(ID)

		# Randomly select a thread to be released if all threads are suspended
		if all_bp_hit():
			release_bp()
		else:
			process.Continue()


def __lldb_init_module(debugger, dict):
	print "Setting trigger..."
	set_trigger()
	timer()
	debugger.HandleCommand('run -k start -X')



