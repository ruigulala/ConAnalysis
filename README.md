# ConAnalysis
Concurrency Attack Analysis.
Right now, all the developement is under Ubuntu 14.04 LTS.

## Install LLVM 3.6.1 & clang 3.6.1.

1. Download the source code of LLVM 3.6.1 from the following website.
> http://llvm.org/releases/download.html

2. Decompress LLVM 3.6.1 source code using
> tar -xvf llvm-3.6.1.src.tar.xz  

3. Download the source code of clang 3.6.1 from the following website.
> http://llvm.org/releases/download.html

4. Decompress clang 3.6.1 source code in the previous llvm source code folder.
The path is path-to-llvm-source/tools/
> tar -xvf cfe-3.6.1.src.tar.xz -C llvm-3.6.1.src/tools/

5. Rename the clang source code folder to clang
The clang source code folder is cfe-3.6.1.src under llvm-3.6.1.src/tools/
> mv cfe-3.6.1.src clang

6. Compile LLVM

Goto path-to-llvm-source, the folder name is llvm-3.6.1.src

Make sure you replace the path-to... with your own path name!!!!

> cd path-to-llvm-source
Install the following dependencies.
> sudo apt-get update

> sudo apt-get install build-essential subversion swig python2.7-dev libedit-dev libncurses5-dev 
Build LLVM together with Clang using CMake
> mkdir build

> cd build

> cmake ..

> make
After this step, under path-to-llvm-source/build/bin, you'll see all the executables including clang and clang++ etc.
> sudo make install

## Install whole-program-llvm
Currently, we're using whole-program-llvm to build the target project into one single llvm bitcode file.
We're using whole-program-llvm as a submodule of our project. The following are the steps to set up whole-program-llvm.

1. Initialize whole-program-llvm
> cd path-to-ConAnalysis-source/whole-program-llvm

> git submodule init && git submodule update
Now, you'll see the source code under this folder.

2. Setup the enviroment of whole-program-llvm
whole-program-llvm will require some enviroment variable setup. You can put the following bash command into your ~/.bashrc file.
Make sure you replace the path-to... with your own path name!!!!
> export CONANAL_ROOT=path-to-ConAnalysis-source

> alias wllvm=$CONANAL_ROOT/whole-program-llvm/wllvm

> export WLLVM_HOME=$CONANAL_ROOT/whole-program-llvm

> export PATH=${WLLVM_HOME}:${PATH}

> export LLVM_COMPILER=clang

> export WLLVM_OUTPUT=WARNING
Don't forget to update ~/.bashrc using
> source ~/.bashrc
or just simply open another terminal window.

## Build ConAnalysis project
Now, since you've installed all the dependencies of ConAnalysis project, you can build it now.
1. Goto ConAnalysis source code folder
> cd path-to-ConAnalysis-source

2. Build ConAnalysis using CMake
> mkdir build

> cd build

> cmake ..

> make

3. Run our LLVM analysis pass on libsafe
> make test
If you goto 
> cd $CONANAL_ROOT/build/Testing/Temporary

> vim LastTest.log
You'll see the following output at the end.
```
---------------------------------------
           Part 1 Result     
---------------------------------------
[ 5 ]

Replaying input:
Read from file part2_loc.txt
Funcname:strcpy
FileName:intercept.c
LineNum:166


Dangerous Operation Basic Block & Instruction
if.then11 & 633
strcpy (intercept.c:166)
Binary file (standard input) matches
<end of output>
Test time =   0.12 sec
----------------------------------------------------------
Test Passed.
"libsafe-cve-1125.test" end time: Oct 18 00:19 EDT
"libsafe-cve-1125.test" time elapsed: 00:00:00
----------------------------------------------------------
```

## Future work
Now you have finished all the required steps. You can enjoy the following hacking work on our project.
If you've encounted any problems, send an email to Rui Gu(me) at rui.gu3@gmail.com or opening an issue on github.


