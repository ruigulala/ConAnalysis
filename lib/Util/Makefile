DEBUG = ../../Debug/lib
# add optLoads.o to the following line when ready
OBJS = SourceLineMapping.o

$(DEBUG)/SourceLineMapping.so: $(OBJS)
  mkdir -p $(DEBUG)
  g++ -std=c++11 $(OBJS) -shared -o $@ -fPIC

%.o: %.cpp
  g++ -std=c++11 -c -fno-rtti `llvm-config --cppflags` -g -fPIC $< -o $@

clean:
  rm -f $(DEBUG)/SourceLineMapping.so $(OBJS)
