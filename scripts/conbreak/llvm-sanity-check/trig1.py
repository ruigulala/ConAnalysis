#!/usr/bin/python2.7

import lldb
import commands
import argparse
import sys

def set_trigger():
	print "Setting breakpoints in mod_mem_cache.c at lines 354 and 653..."
	target = lldb.debugger.GetSelectedTarget()
	bp_read = target.BreakpointCreateByLocation("test.c", 12)
	bp_write = target.BreakpointCreateByName("set_status")

	bp_read.SetScriptCallbackFunction("trig1.read_callback")
	bp_write.SetScriptCallbackFunction("trig1.write_callback")
	print("Configuration done!")


def read_callback(frame, bp_loc, dict):
	thread = frame.GetThread()
	process = thread.GetProcess()
	ID = thread.GetThreadID()

	obj = str(frame.FindVariable("status"))
	print "READ: tid=" + str(ID) + "   status=" + obj
	
	thread.Resume()
	process.Continue()


def write_callback(frame, bp_loc, dict):
	thread = frame.GetThread()
	process = thread.GetProcess()
	ID = thread.GetThreadID()

	print ">>>>>>>>>> SUSPENDING WRITE THREAD"

	thread.Suspend()
	process.Continue()
	

def __lldb_init_module(debugger, dict):
    #debugger.HandleCommand('command script add -f trigger.set_trigger trig')
    #print "The \"trig\" python command has been installed and is ready for use."
	print "Setting trigger..."
	set_trigger()
	print "Starting server..."
	debugger.HandleCommand('run')






