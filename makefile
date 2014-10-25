.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .hpp
# replace the CXX variable with a path to a C++11 compatible compiler.
ifeq ($(INTEL), 1)
# if you wish to use the Intel compiler, please do "make INTEL=1".
    CXX ?= /opt/intel/bin/icpc
ifeq ($(DEBUG),1)
    override CXXFLAGS += -std=c++0x -O3 -Wall -Wno-conversion -ansi -fopenmp -xAVX -DDEBUG=1 -D_GLIBCXX_DEBUG   -ggdb
else
    override CXXFLAGS += -std=c++0x -O3 -Wall -ansi -fopenmp -xAVX -DNDEBUG=1  -ggdb -Wno-conversion
endif # debug
else #intel
    CXX ?= g++-4.7
ifeq ($(DEBUG),1)
    override CXXFLAGS += -mavx -std=c++0x -fopenmp -pedantic -ggdb -DDEBUG=1 -Wno-conversion -D_GLIBCXX_DEBUG -Wall -Wextra  -Wcast-align  
else
    override CXXFLAGS += -mavx -std=c++0x -fopenmp -pedantic -O3 -Wall -Wextra -Wno-conversion  -Wcast-align  
endif #debug
endif #intel

all: UnsignedIntegerArray.o Matrix.o MutableGraph.o 
HEADERS= $(shell ls include/*hpp)
OBJDIR=build
EXEDIR=bin

$(shell mkdir -p $(OBJDIR))
$(shell mkdir -p $(EXEDIR))

MutableGraph.o: include/MutableGraph.hpp src/MutableGraph.cpp
	$(CXX) $(CXXFLAGS) -c src/MutableGraph.cpp -Iinclude -o $(OBJDIR)/$@

UnsignedIntegerArray.o: include/UnsignedIntegerArray.hpp src/UnsignedIntegerArray.cpp
	$(CXX) $(CXXFLAGS) -c src/UnsignedIntegerArray.cpp -Iinclude -o $(OBJDIR)/$@

Matrix.o: include/Matrix.hpp src/Matrix.cpp
	$(CXX) $(CXXFLAGS) -c src/Matrix.cpp -Iinclude -o $(OBJDIR)/$@

UNAME := $(shell uname)

OBJECTS= $(OBJDIR)/Matrix.o $(OBJDIR)/MutableGraph.o $(OBJDIR)/UnsignedIntegerArray.o

undirectedEdgeListToBinary: tools/undirectedEdgeListToBinary.cpp
	$(CXX) $(CXXFLAGS)  -o $(EXEDIR)/undirectedEdgeListToBinary tools/undirectedEdgeListToBinary.cpp  $(OBJECTS) -Iinclude

directedEdgeListToBinary: tools/directedEdgeListToBinary.cpp
	$(CXX) $(CXXFLAGS)  -o $(EXEDIR)/directedEdgeListToBinary tools/directedEdgeListToBinary.cpp  $(OBJECTS) -Iinclude

test_undirected_triangle_counting: tests/test_undirected_triangle_counting.cpp
	$(CXX) $(CXXFLAGS)  -o $(EXEDIR)/test_undirected_triangle_counting tests/test_undirected_triangle_counting.cpp  $(OBJECTS) -Iinclude

test_compression_simple: tests/test_compression_simple.cpp
	$(CXX) $(CXXFLAGS)  -o $(EXEDIR)/test_compression_simple tests/test_compression_simple.cpp  $(OBJECTS) -Iinclude

test_input_loader: tests/test_input_loader.cpp
	$(CXX) $(CXXFLAGS)  -o $(EXEDIR)/test_input_loader tests/test_input_loader.cpp  $(OBJECTS) -Iinclude

test_primitives: tests/test_primitives.cpp
	$(CXX) $(CXXFLAGS)  -o $(EXEDIR)/test_primitives tests/test_primitives.cpp  $(OBJECTS) -Iinclude

test_pagerank: tests/test_pagerank.cpp
	$(CXX) $(CXXFLAGS)  -o $(EXEDIR)/test_pagerank tests/test_pagerank.cpp  $(OBJECTS) -Iinclude

test_matrix_multiply: tests/test_matrix_multiply.cpp
	$(CXX) $(CXXFLAGS)  -o $(EXEDIR)/test_matrix_multiply tests/test_matrix_multiply.cpp  $(OBJECTS) -Iinclude

.DEFAULT:  apps/$@.cpp
	$(CXX) $(CXXFLAGS)  -o $(EXEDIR)/$@ apps/$@.cpp  $(OBJECTS) -Iinclude

clean: 
	rm -rf $(OBJDIR) $(EXEDIR)

