DEBUG = ../../Debug/lib
OBJS = ConAnalysis.o

$(DEBUG)/ConAnal.so: $(OBJS)
	mkdir -p $(DEBUG)
	g++ -std=c++11 $(OBJS) -shared -o $@ -fPIC

%.o: %.cpp
	g++ -std=c++11 -c -fno-rtti `llvm-config --cppflags` -g -fPIC $< -o $@

clean:
	rm -f $(DEBUG)/ConAnal.so $(OBJS)
