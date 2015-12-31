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
sudo apt-get install build-essential subversion swig python2.7-dev libedit-dev libncurses5-dev cmake inotify-tools
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
## Install submodules
Currently, we're using whole-program-llvm to build the target project into one single llvm bitcode file.
We're using whole-program-llvm as a submodule of our project. The following are the steps to set up whole-program-llvm.
Other than this, we also using another git repository to reference all the source code of bugs of analyzed.

* Initialize and update submodules
```
cd path-to-ConAnalysis-source
git submodule update --init --recursive
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
cd $CONANAL_ROOT
```
* Build ConAnalysis using CMake
```
mkdir build
cd build
cmake ..
make
```
* Run our LLVM analysis pass on libsafe.
```
ctest -R libsafe
```
Then go to the folder contains the actual test output. 
```
cd $CONANAL_ROOT/build/Testing/Temporary
vim LastTest.log
```
For each test case, there is a folder under TESTS named standard-output contains all the verified standarded output.

Or you can go to $CONANAL_ROOT/TESTS/libsafe-cve-1125 and 
```
./run.sh no_race_detector
```
for an automatic run which contains the race detection and static analysis. The output will be in
```
$CONANAL_ROOT/build/TESTS/libsafe-cve-1125/final*
```

## Future work
Now you have finished all the required steps. You can enjoy the following hacking work on our project.
If you've encounted any problems, send an email to Rui Gu(me) at rui.gu3@gmail.com or opening an issue on github.


