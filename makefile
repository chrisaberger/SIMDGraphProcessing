.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .hpp

# replace the CXX variable with a path to a C++11 compatible compiler.
ifeq ($(INTEL), 1)
# if you wish to use the Intel compiler, please do "make INTEL=1".
    CXX ?= /opt/intel/bin/icpc
ifeq ($(DEBUG),1)
    override CXXFLAGS += -std=c++0x -O3 -Wall -ansi -fopenmp -xAVX -DDEBUG=1 -D_GLIBCXX_DEBUG   -ggdb
else
    override CXXFLAGS += -std=c++0x -O3 -Wall -ansi -fopenmp -xAVX -DNDEBUG=1  -ggdb
endif # debug
else #intel
    CXX ?= g++-4.7
ifeq ($(DEBUG),1)
    override CXXFLAGS += -mavx -std=c++0x -fopenmp -pedantic -ggdb -DDEBUG=1 -D_GLIBCXX_DEBUG -Wall -Wextra  -Wcast-align  
else
    override CXXFLAGS += -mavx -std=c++0x -fopenmp -pedantic -O3 -Wall -Wextra  -Wcast-align  
endif #debug
endif #intel

OBJDIR=build
EXEDIR=bin
HEADERS=$(wildcard include/*hpp)
SOURCES=$(wildcard src/*cpp)
OBJECTS=$(SOURCES:src/%.cpp=$(OBJDIR)/%.o)
APPS_SOURCES=$(shell ls apps)
APPS=$(APPS_SOURCES:.cpp=)
APPS_EXES=$(APPS:%=$(EXEDIR)/%)

all: $(APPS_EXES)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(EXEDIR):
	mkdir -p $(EXEDIR)

$(APPS_EXES): $(OBJECTS) $(APP_SOURCES) $(EXEDIR)
	$(CXX) $(CXXFLAGS) -o $@ -I include $(@:bin%=apps%.cpp) $(OBJECTS) 

$(OBJECTS): $(SOURCES) $(HEADERS) $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $(@:build%.o=src%.cpp) -I include -o $@

clean:
	rm -rf $(OBJDIR) $(EXEDIR)

