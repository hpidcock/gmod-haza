cmake_minimum_required(VERSION 2.8)

include_directories(../include)

set(CMAKE_SHARED_LIBRARY_PREFIX "")

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
set(CMAKE_SHARED_LIBRARY_SUFFIX "_osx.dll")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
set(CMAKE_SHARED_LIBRARY_SUFFIX "_linux.dll")
endif(CMAKE_SYSTEM_NAME MATCHES "Windows")

add_library(gm_oosocks SHARED gm_oosocks.cpp)
set_target_properties(gm_oosocks PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
target_link_libraries(gm_oosocks Ws2_32)
endif(CMAKE_SYSTEM_NAME MATCHES "Windows")