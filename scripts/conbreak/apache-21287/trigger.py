#!/usr/bin/python2.7

import lldb
import sys
import random
import time
import threading

# USER SET GLOBAL VARIABLES
# Stable state wait time (in seconds, ie. 0.1 = 100ms), default = 1
WAIT_TIME = 0.1

# DO NOT CHANGE
LAST_BREAK = 0
RUNNING = False
OBJ_ARR = []
STATUS_FOUND = False

FILE_READ = ""
LINE_NUM_READ = 0
FILE_WRITE = ""
LINE_NUM_WRITE = 0

# Locks
# TODO: Right now locking mechanism is very coarse grained.  Future work should aim
# to improve the granularity of locks to increase performance
timer_lock = threading.Lock()
process_lock = threading.Lock()


def set_trigger(in_read, in_write):
	print "Setting breakpoints..."
	target = lldb.debugger.GetSelectedTarget()

	global FILE_READ
	global LINE_NUM_READ
	global FILE_WRITE
	global LINE_NUM_WRITE

#	in_read = "apr_strings.c:114:11"
#	in_write = "mod_mem_cache.c:889:42"

	tokens_read = in_read.split(":")
	tokens_write = in_write.split(":")

	FILE_READ = tokens_read[0]
	LINE_NUM_READ = int(tokens_read[1])

	FILE_WRITE = tokens_write[0]
	LINE_NUM_WRITE = int(tokens_write[1])

	bp_read = target.BreakpointCreateByLocation(FILE_READ, LINE_NUM_READ)
	bp_write = target.BreakpointCreateByLocation(FILE_WRITE, LINE_NUM_WRITE)

	bp_read.SetScriptCallbackFunction("trigger.read_callback")
	bp_write.SetScriptCallbackFunction("trigger.write_callback")

	update_timer()
	timer()

	print("Configuration done!")


# Release a thread if stable state is reached
def timer():
	process_lock.acquire()

	global RUNNING

	if RUNNING and time.time() - LAST_BREAK > WAIT_TIME:
		process = lldb.debugger.GetSelectedTarget().GetProcess()

		RUNNING = False

		update_timer()
		release_bp()

		RUNNING = True
		process.Continue()

	process_lock.release()

	if STATUS_FOUND:
		print "### STATUS: MATCH FOUND ###"

	# We don't care about timing drift, we just want timer() to be called periodically
	threading.Timer(0.1, timer).start()

def update_timer():
	timer_lock.acquire()

	global LAST_BREAK
	LAST_BREAK = time.time()

	timer_lock.release()


# Check to see if every thread is at a breakpoint
# TODO: We need a better way to check if all threads are currently suspended..
# Sometimes not all threads will be suspended but program still stops, leaving 
# the job of releasing BPs to fall back to the timeout
def all_bp_hit():
	# Old mechanism. Just check if all threads are suspended..
#	for t in lldb.debugger.GetSelectedTarget().GetProcess():
#		if not t.IsSuspended():
#			return False
#
#	return True

	c = 0
	res = False

	for t in lldb.debugger.GetSelectedTarget().GetProcess():
		if t.IsSuspended():
			c += 1

	if c >= 10:
		res = True
	
	return res

# Expects process to already be stopped.  Will resume thread, but expects 
# process.Continue() to be called by calling function
def release_bp():
	process = lldb.debugger.GetSelectedTarget().GetProcess()

	# Check if process is invalid, can cause errors
	if process.IsValid() == False:
		print "######## WARNING: PROCESS IS INVALID ########"
		return

	global OBJ_ARR

	obj_arr_len = len(OBJ_ARR)

	# No more suspended threads...
	if obj_arr_len == 0:
		return

	rand = random.randrange(0, obj_arr_len)
	thread = process.GetThreadByID(OBJ_ARR[rand][-1])

	print str(time.time()) + " >>>>>>>>>> INFO: Releasing thread " + str(thread.GetThreadID())

	# Make sure process is stopped before modifying thread states
	if not process.is_stopped:
		process.Stop()

	if thread.Resume() == False:
		print "### ERROR IN THREAD.RESUME() ###"
		return

	del OBJ_ARR[rand]


# TSAN will sometimes report inaccurate line numbers causing get_addr to fail
def get_addr(frame, filename, line_num):
	filespec = lldb.SBFileSpec(filename, False)
	if not filespec.IsValid():
		print " ####### FILESPEC INVALID ####### "
	
	target = lldb.debugger.GetSelectedTarget()
	source_mgr = target.GetSourceManager()
	stream = lldb.SBStream()
	source_mgr.DisplaySourceLinesWithLineNumbers(filespec, line_num, 0, 0, "", stream)

	# Needs some refinement.  Hacky so just exit on error
	try:
		src_line = stream.GetData().split("\n")[0]

		src_line = [src_line]
		break_chars = [" ", "\n", ",", ";", "(", ")"]

		# Split source line up using break_chars as delimiters
		for char in break_chars:
			src_line = [s.split(char) for s in src_line]
			src_line = [item for sublist in src_line for item in sublist]

		# Remove blank strings from list
		src_line = filter(None, src_line)

		# Remove escape characters (can cause errors)
		escapes = ''.join([chr(char) for char in range(1, 32)])
		src_line = [c for c in src_line if c.translate(None, escapes) != ""]

		# Remove operators
		break_chars = ["++", "--", "!", "*", "&"]
		for op in break_chars:
			src_line = [token.replace(op, "") for token in src_line]

		# Remove blank strings again..
		src_line = filter(None, src_line)

#		print "Tokens >>>>>>>>>>>> " + str(src_line)

		# Try to find a variable that matches a token and save its address
		addrs = []
		for token in src_line:
			# obj = frame.EvaluateExpression(token)
			obj = frame.GetValueForVariablePath(token)

#			print token + " => " + str(obj) + " => " + str(obj.GetAddress())

			# Very hacky way to verify extracted variable is valid
			# TODO: Is there a better way to do this?
			# obj.IsValid() does not work, frame.FindVariable() cannot find globals,
			# target.FindFirstGlobalVariable() can't resolve complex expressions.  Only 
			# method I haven't really tried is manually searching frame.GetVariables()
			if str(obj.GetAddress()) != "No value":
				addrs.append(obj.GetAddress().__hex__())
	
	except (KeyboardInterrupt, SystemExit):
		raise

	except:
		print "####### ERROR: Unable to extract variable name from source #######"
		print sys.exc_info()[0]
		exit()

	if len(addrs) == 0:
		print "####### ERROR: No variables found #######"
		exit()

	return addrs


# Match if addresses match and instructions differ
def match(arr):
	matches = []

	for obj in OBJ_ARR:

		# Only considered a match if instructions differ
		# TODO: Change so only one has to be a WRITE
		if obj[-2] != arr[-2]:

			# Check through all watched addresses
			for addr1 in obj[:-2]:
				for addr2 in arr[:-2]:

					# Add address and tid to list if addresses match
					if addr1 == addr2:
						matches.append([addr1, obj[-1]])

	return matches


def read_callback(frame, bp_loc, dict):
	thread = frame.GetThread()
	process = thread.GetProcess()
	ID = thread.GetThreadID()

	process_lock.acquire()

	global OBJ_ARR
	global RUNNING
	global STATUS_FOUND

	RUNNING = False
	thread.Suspend()

	addrs = get_addr(frame, FILE_READ, LINE_NUM_READ)

	for obj in addrs:
		print str(time.time()) + " READ:  [" + str(ID) + "] Checking " + obj + "..."

	addrs.append("R")
	addrs.append(ID)

	matches = match(addrs)
	if len(matches) > 0:
		print ">>>>>>>>>> READ:  [" + str(ID) + "] Found match!"

		for m in matches:
			print "addr=" + m[0] + "  tid1=" + str(m[1]) + "  tid2=" + str(ID)

		print "**************************************************************"
		print "**************************** HALT ****************************"
		print "**************************************************************"

		STATUS_FOUND = True
		process.Stop()

	else:
		update_timer()

		OBJ_ARR.append(addrs)

		# Randomly select a thread to be released if all threads are suspended
		if all_bp_hit():
			release_bp()

		RUNNING = True
		process.Continue()

	process_lock.release()


def write_callback(frame, bp_loc, dict):
	thread = frame.GetThread()
	process = thread.GetProcess()
	ID = thread.GetThreadID()

	process_lock.acquire()

	global OBJ_ARR
	global RUNNING

	RUNNING = False
	thread.Suspend()

	addrs = get_addr(frame, FILE_WRITE, LINE_NUM_WRITE)

	for obj in addrs:
		print str(time.time()) + " WRITE: [" + str(ID) + "] Setting  " + obj + "..."

	addrs.append("W")
	addrs.append(ID)

	matches = match(addrs)
	if len(matches) > 0:
		print ">>>>>>>>>> WRITE: [" + str(ID) + "] Found match!"

		for m in matches:
			print "addr=" + m[0] + "  tid1=" + str(m[1]) + "  tid2=" + str(ID)

		print "**************************************************************"
		print "**************************** HALT ****************************"
		print "**************************************************************"

		STATUS_FOUND = True
		process.Stop()

	else:
		update_timer()

		OBJ_ARR.append(addrs)

		# Randomly select a thread to be released if all threads are suspended
		if all_bp_hit():
			release_bp()

		RUNNING = True
		process.Continue()

	process_lock.release()


# def __lldb_init_module(debugger, dict):
#	print "Setting trigger..."
#	set_trigger()
#
#	# TODO: Let user specify program flags
#	debugger.HandleCommand('run -k start -X')


# Initialization and whatnot
def main():
	if len(sys.argv) < 4:
		print ( "Usage: python2.7 trigger.py [BP1 <file:lineno>] [BP2 <file:lineno>] "
			    "<executable> [args <arg1> <arg2> ...]" )
		exit()

	exe = sys.argv[4]
	debugger = lldb.SBDebugger.Create()
	target = debugger.CreateTargetWithFileAndArch(exe, lldb.LLDB_ARCH_DEFAULT)

	if target:
		set_trigger()
	else:
		print "### ERROR: Unable to create target ###"
		exit()

	# Start execution
	target.LaunchSimple(sys.argv[5:], None, os.getcwd())


if __name__ == "__main__":
    main()






