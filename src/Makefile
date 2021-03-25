
# REFER: https://www.tutorialspoint.com/makefile/makefile_dependencies.htm
# REFER: https://stackoverflow.com/questions/30543286/fatal-error-iostream-no-such-file-or-directory-in-compiling-c-program-using-gc/30543316
# REFER: https://stackoverflow.com/questions/3220277/what-do-the-makefile-symbols-and-mean#:~:text=The%20%24%40%20and%20%24%3C%20are%20called,to%20create%20the%20output%20file.&text=The%20%2Do%20specifies%20the%20output,this%20article%20about%20Linux%20Makefiles.


# REFER: https://stackoverflow.com/questions/5541946/cflags-ccflags-cxxflags-what-exactly-do-these-variables-control
CXX      = g++
CXXFLAGS = -std=c++14


all: MyContainer.out

MyContainer.out: MyContainer.cpp MyDebugger.hpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -O2

debug: MyContainer_debug.out

MyContainer_debug.out: MyContainer.cpp MyDebugger.hpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -DDEBUGGING_ON


# REFER: https://stackoverflow.com/questions/2145590/what-is-the-purpose-of-phony-in-a-makefile/2145605
.PHONY: clean
clean:
	rm -f MyContainer.out MyContainer_debug.out

