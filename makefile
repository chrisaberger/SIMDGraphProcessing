.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .hpp
# replace the CXX variable with a path to a C++11 compatible compiler.
ifeq ($(INTEL), 1)
# if you wish to use the Intel compiler, please do "make INTEL=1".
    CXX ?= /opt/intel/bin/icpc
ifeq ($(DEBUG),1)
    CXXFLAGS = -std=c++0x -O3 -Wall -ansi -fopenmp -xAVX -DDEBUG=1 -D_GLIBCXX_DEBUG   -ggdb
else
    CXXFLAGS = -std=c++0x -O3 -Wall -ansi -fopenmp -xAVX -DNDEBUG=1  -ggdb
endif # debug
else #intel
    CXX ?= g++-4.7
ifeq ($(DEBUG),1)
    CXXFLAGS = -mavx -std=c++0x -fopenmp -pedantic -ggdb -DDEBUG=1 -D_GLIBCXX_DEBUG -Wall -Wextra  -Wcast-align  
else
    CXXFLAGS = -mavx -std=c++0x -fopenmp -pedantic -O3 -Wall -Wextra  -Wcast-align  
endif #debug
endif #intel

all: Matrix.o

HEADERS= $(shell ls include/*hpp)
OBJDIR=build
EXEDIR=bin

$(shell mkdir -p $(OBJDIR))
$(shell mkdir -p $(EXEDIR))

Matrix.o: include/Matrix.hpp src/Matrix.cpp
	$(CXX) $(CXXFLAGS) -c src/Matrix.cpp -Iinclude -o $(OBJDIR)/$@

UNAME := $(shell uname)

OBJECTS= $(OBJDIR)/Matrix.o

.DEFAULT:  apps/$@.cpp
	$(CXX) $(CXXFLAGS)  -o $(EXEDIR)/$@ apps/$@.cpp  $(OBJECTS) -Iinclude

clean: 
	rm -rf $(OBJDIR) $(EXEDIR)

