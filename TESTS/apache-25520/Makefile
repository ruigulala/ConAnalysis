# Retain intermediate bitcode files
.PRECIOUS: %.bc

# The default target builds the plugin
plugin:
	make -C ../../lib/ConAnal

# create .bc from source
apache-25520.bc:
	clang -emit-llvm -O0 -g -c loggers/mod_log_config.c -o apache-25520.bc
	opt -mem2reg apache-25520.bc -o apache-25520.mem2reg
	mv apache-25520.mem2reg apache-25520.bc

# create human-readable assembly (.ll) from .bc
%.ll: %.bc
	llvm-dis -f $^

# create executable from .bc
%.exe: %.bc
	llc $^
	gcc $*.s -o $*.exe

# create bitcode optimized to keep vars in registers
%.mem2reg: %.bc
	opt -mem2reg $^ -o $*.mem2reg
	mv $*.mem2reg $*.bc

# run ConAnalysis on a .bc file
%.ConAnalysis: %.bc plugin
	opt -load /home/ruigu/Workspace/ConAnalysis/build/lib/ConAnal/libConAnalysis.so -ConAnalysis $*.bc > /dev/null

# run ConAnalysis on a .bc file
%.DOL: %.bc plugin
	opt -load /home/ruigu/Workspace/ConAnalysis/build/lib/DOL/libDOL.so -DOL $*.bc > /dev/null

# clean up generated files
clean:
	rm -f *.bc *.exe *.ll *.s
