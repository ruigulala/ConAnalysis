# LLDB Analysis Usage Manual
### Setup
* Create a symlink called 'target' that points to your executable
* Create a file called 'arguments.txt' that contains all the arguments to be passed to the selected executable
* Create a folder called 'tsan_reports' that contains files each with one tsan report
* Create a shell script called 'benchmark.sh' that will run your test suite on the executable

