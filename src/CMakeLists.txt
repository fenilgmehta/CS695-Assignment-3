# REFER: https://github.com/fenilgmehta/Misc-Programming/blob/master/CMakeLists.txt

cmake_minimum_required(VERSION 3.8)
project("CS695-Assignment-3")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE on)

add_library(MyDebugger.o OBJECT MyDebugger.hpp)
add_executable(MyContainer.out ./MyContainer.cpp $<TARGET_OBJECTS:MyDebugger.o>)

#file(GLOB sourceFILES src/*.cpp src/*.c)
#file(GLOB sourceFILES *.cpp *.c)
#
#FOREACH (sourceFile ${sourceFILES})
#    get_filename_component(sourceFile_Path ${sourceFile} PATH)
#    MESSAGE(STATUS "Process file, path: ${sourceFile}")
#
#    get_filename_component(sourceFile_Name ${sourceFile} NAME)
#    string(REPLACE " " "_" sourceFile_Name ${sourceFile_Name})
#    string(REPLACE "'" "" sourceFile_Name ${sourceFile_Name})
#    string(REPLACE "\"" "" sourceFile_Name ${sourceFile_Name})
#    string(REPLACE "(" "_" sourceFile_Name ${sourceFile_Name})
#    string(REPLACE ")" "_" sourceFile_Name ${sourceFile_Name})
#    MESSAGE(STATUS "Process file, name: \"${sourceFile_Name}\"")
#
#    add_executable("${sourceFile_Name}" "${sourceFile}")
#endforeach (sourceFile)
