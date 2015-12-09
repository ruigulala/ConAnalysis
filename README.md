# ConAnalysis
Concurrency Attack Analysis.
Right now, all the developement is under Ubuntu 14.04 LTS.

## Install LLVM 3.6.1 & clang 3.6.1.

* Download the source code of LLVM 3.6.1 from the following website.
```
http://llvm.org/releases/download.html
```
* Decompress LLVM 3.6.1 source code using
```
tar -xvf llvm-3.6.1.src.tar.xz
```

* Download the source code of clang 3.6.1 from the following website.
```
http://llvm.org/releases/download.html
```

* Decompress clang 3.6.1 source code in the previous llvm source code folder.
The path is path-to-llvm-source/tools/
```
tar -xvf cfe-3.6.1.src.tar.xz -C llvm-3.6.1.src/tools/
```

* Rename the clang source code folder to clang
The clang source code folder is cfe-3.6.1.src under llvm-3.6.1.src/tools/
```
mv cfe-3.6.1.src clang
```

* Compile LLVM

Goto path-to-llvm-source, the folder name is llvm-3.6.1.src

Make sure you replace the path-to... with your own path name!!!!
```
cd path-to-llvm-source
```

Install the following dependencies.
```
sudo apt-get update
sudo apt-get install build-essential subversion swig python2.7-dev libedit-dev libncurses5-dev 
```
Build LLVM together with Clang using CMake
```
mkdir build
cd build
cmake ..
make
```
After this step, under path-to-llvm-source/build/bin, you'll see all the executables including clang and clang++ etc.
```
sudo make install
```
## Install whole-program-llvm
Currently, we're using whole-program-llvm to build the target project into one single llvm bitcode file.
We're using whole-program-llvm as a submodule of our project. The following are the steps to set up whole-program-llvm.

* Initialize whole-program-llvm
```
cd path-to-ConAnalysis-source/whole-program-llvm
git submodule init && git submodule update
```
Now, you'll see the source code under this folder.

* Setup the enviroment of whole-program-llvm
whole-program-llvm will require some enviroment variable setup. You can put the following bash command into your ~/.bashrc file.
Make sure you replace the path-to... with your own path name!!!!
```
export CONANAL_ROOT=path-to-ConAnalysis-source
alias wllvm=$CONANAL_ROOT/whole-program-llvm/wllvm
export WLLVM_HOME=$CONANAL_ROOT/whole-program-llvm
export PATH=${WLLVM_HOME}:${PATH}
export LLVM_COMPILER=clang
export WLLVM_OUTPUT=WARNING
```
Don't forget to update ~/.bashrc using
```
source ~/.bashrc
```
or just simply open another terminal window.

## Build ConAnalysis project
Now, since you've installed all the dependencies of ConAnalysis project, you can build it now.

* Goto ConAnalysis source code folder
```
cd path-to-ConAnalysis-source
```
* Build ConAnalysis using CMake
```
mkdir build
cd build
cmake ..
make
```
* Run our LLVM analysis pass on libsafe
```
make test
```
If you goto 
```
cd $CONANAL_ROOT/build/Testing/Temporary
vim LastTest.log
```
You'll see the following output at the end.
The first part is a dump of LLVM IR with virtual registers labeled in an monotonically increasing order.
```
---------------------------------------
             ConAnalysis     
---------------------------------------

FUNCTION make_config_log_state

BASIC BLOCK entry
%1: call  XXX XXX XXX XXX llvm.dbg.value 
%2: call  XXX XXX XXX XXX llvm.dbg.value 
%3: call  p XXX apr_palloc 
%4: bitcast %3 
%5: call  XXX XXX XXX XXX llvm.dbg.value 
%6: call  p XXX XXX apr_array_make 
%7: getelementptr %4 XXX XXX  
%8: store %6 %7 
%9: getelementptr %4 XXX XXX  
%10:  store XXX %9 
%11:  getelementptr %4 XXX XXX  
%12:  store XXX %11  
%13:  getelementptr %4 XXX XXX  
%14:  store XXX %13  
%15:  call  p XXX apr_table_make 
%16:  getelementptr %4 XXX XXX  
%17:  store %15 %16  
%18:  getelementptr %4 XXX XXX  
%19:  load  %18  
%20:  call  %19 XXX XXX apr_table_setn 
%21:  bitcast %4 
%22:  ret %21  
```
The second part is how ConAnalysis analyze the code. It includes how ConAnalysis traverses between functions indicated by the input callstack.
```
---------------------------------------
       part1_getCorruptedIRs
---------------------------------------
Original Callstack: Go into "ap_buffered_log_writer"
Adding corrupted variable: %1827
Adding %1825 to crptPtr list
Add %1827 to crpt list
Param No.0 %1825 contains corruption.
"ap_buffered_log_writer" calls "flush_log"
Callstack PUSH flush_log
Corrupted Arg: buf
Add arg 0x507c1f0 to crptPtr list
Add %1115 to crpt list
Add %1127 to crpt list
Param No.2 %1127 contains corruption.
"flush_log" calls "apr_file_write"
Callstack PUSH apr_file_write
Couldn't obtain the source code of function "apr_file_write"
Callstack POP apr_file_write
Add %1129 to crpt list
Callstack POP flush_log
"ap_buffered_log_writer" calls "apr_palloc"
Callstack PUSH apr_palloc
Couldn't obtain the source code of function "apr_palloc"
Callstack POP apr_palloc
"ap_buffered_log_writer" calls "apr_file_write"
Callstack PUSH apr_file_write
Couldn't obtain the source code of function "apr_file_write"
Callstack POP apr_file_write
Add %1874 to crpt list
Add %1877 to crpt list
Add %1896 to crpt list
Add %1902 to crpt list
Callstack POP ap_buffered_log_writer
Original Callstack: Go into "set_buffered_logs_on"
Callstack POP set_buffered_logs_on
```
The third part is the output of part 1(forward dataflow analysis result). It indicates which LLVM virtual registers are corrupted. About the virtual register number, you can refer it to the first part of the result.
```
---------------------------------------
           Part 1 Result
---------------------------------------
[ 1827 1828 1829 1830 1831 1115 1116 1117 1118 1127 1129 1130 1832 1874 1875 1877 1880 1896 1902 1903 1904 1905 1795 ]
```
The fourth part first displays the dangerous operation input and the interaction between its dominators and dataflow analysis.
```
Dangerous Operation Basic Block & Instruction
entry & 1809
ap_buffered_log_writer_init (loggers/mod_log_config.c:1322)
...
---------------------------------------
           Part 3 Result
---------------------------------------
[ 1827 1828 1829 1830 1831 1874 1875 1877 1880 1902 ]
```
## Future work
Now you have finished all the required steps. You can enjoy the following hacking work on our project.
If you've encounted any problems, send an email to Rui Gu(me) at rui.gu3@gmail.com or opening an issue on github.


