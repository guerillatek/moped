
cmake_minimum_required(VERSION 3.20)
project(moped)

set(CMAKE_CXX_STANDARD 23)

# set(CMAKE_VERBOSE_MAKEFILE ON)
set(BUILD_SHARED_LIBS OFF)

#add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fsanitize=address>) 
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

if (UNIX AND NOT APPLE)
    set(VCPKG_TARGET_TRIPLET "x64-linux")
elseif (APPLE)
    # Check the architecture for macOS (x64 or arm64)
    if (CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(VCPKG_TARGET_TRIPLET "arm64-osx")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-osx")
    endif()
endif()

find_library(Yaml_LIBRARY NAMES yaml HINTS /yaml/lib REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
message(STATUS "Yaml_LIBRARY=${Yaml_LIBRARY}")

find_path(Yaml_INCLUDE_DIR yaml.h HINTS /yaml/include REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
message(STATUS "Yaml_INCLUDE_DIR=${Yaml_INCLUDE_DIR}")


add_library(moped SHARED 
ParserBase.cpp)


target_compile_options(moped PRIVATE -fPIC)
target_link_options(moped PRIVATE "-Wl,--no-as-needed")

enable_testing() 
add_subdirectory(tests)