UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
	LIBS=-lnuma
endif

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
TOOLS_SOURCES=$(shell ls tools)
TESTS_SOURCES=$(shell ls tests)

APPS=$(APPS_SOURCES:.cpp=)
TOOLS=$(TOOLS_SOURCES:.cpp=)
TESTS=$(TESTS_SOURCES:.cpp=)

APPS_EXES=$(APPS:%=$(EXEDIR)/%)
TOOLS_EXES=$(TOOLS:%=$(EXEDIR)/%)
TESTS_EXES=$(TESTS:%=$(EXEDIR)/%)

all: $(APPS_EXES) $(TOOLS_EXES)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(EXEDIR):
	mkdir -p $(EXEDIR)

$(APPS_EXES): $(OBJECTS) $(APP_SOURCES) $(EXEDIR)
	$(CXX) $(CXXFLAGS) $(@:bin%=apps%.cpp) $(OBJECTS) $(LIBS) -o $@ -Iinclude 

$(TOOLS_EXES): $(OBJECTS) $(EXEDIR)
	$(CXX) $(CXXFLAGS) $(@:bin%=tools%.cpp) $(OBJECTS) $(LIBS) -o $@ -Iinclude 

$(TESTS_EXES): $(OBJECTS) $(EXEDIR)
	$(CXX) $(CXXFLAGS) $(@:bin%=tests%.cpp) $(OBJECTS) $(LIBS) -o $@ -Iinclude 

$(OBJECTS): $(SOURCES) $(HEADERS) $(OBJDIR)
	$(CXX) $(CXXFLAGS) -Iinclude $(LIB_INCS) -o $@ -c $(@:build%.o=src%.cpp) 

clean:
	rm -rf $(OBJDIR) $(EXEDIR)

