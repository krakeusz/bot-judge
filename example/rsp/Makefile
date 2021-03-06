target := rsp_engine
# define build options
# compile options
CXXFLAGS := -Wall -Wextra -std=c++11 -Wshadow -Werror -O2
# link options
#LDFLAGS :=  -L/usr/lib/x86_64-linux-gnu/
# link libraries
#LDLIBS := -lgtest -lgtest_main -lpthread

# construct list of .cpp and their corresponding .o and .d files
sources  := $(wildcard *.cpp)
includes := -I../../inc/
objects  := $(sources:.cpp=.o)
dep_file := Makefile.dep

# file disambiguity is achieved via the '.PHONY' directive
.PHONY : all clean

# main goal for 'make' is the first target, here 'all'
# 'all' is always assumed to be a target, and not a file
# usage: 'make all' or simply 'make', since this is the first target
#
all: $(target) noop scissors rock random

noop: botnoop/botnoop.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

rock: botrock/botrock.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

scissors: botscissors/botscissors.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

random: botrandom/botrandom.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

rsp_engine: $(objects) ../../build/libengine_main.a
	$(CXX) $(CXXFLAGS) -o $@ $^

# even if there exists a file 'clean', 'make clean' will execute its commands
# and won't ever assume 'clean' is an up-to-date file
# usage: 'make clean'
#
clean :
	$(RM) $(target) $(dep_file) $(objects) noop rock scissors random


# rule for creating .o files
#
.cpp.o :
	$(CXX) $(CXXFLAGS) $(includes) -c $< -o $@


# rule to make dependencies within one file
#
depend $(dep_file):
	@echo Makefile - creating dependencies for: $(sources)
	@$(RM) $(dep_file)
	@$(CXX) -E -MM $(CXXFLAGS) $(includes) $(sources) >> $(dep_file)


# include dependency file only when 'make clean' is not specified
# this is done to prevent recreating the dependency file when the clean target is specified
# if no dependencies exist, on the other hand, no warning will be issued:'-' key
#
ifeq (,$(findstring clean,$(MAKECMDGOALS)))
-include $(dep_file)
endif
