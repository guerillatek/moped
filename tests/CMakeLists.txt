if (CMAKE_BUILD_TYPE MATCHES Debug)
    MESSAGE("DEBUG")
    add_compile_options(-g)
    else ()
    add_compile_options(-Ofast)
endif ()


file(GLOB_RECURSE MOPEDTESTSRC ./*.cpp)
add_executable(MopedTests ${MOPEDTESTSRC})
target_include_directories(MopedTests PRIVATE ../..)
target_include_directories(MopedTests PRIVATE ../../..)
set_target_properties(MopedTests PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/tests)
target_link_libraries(MopedTests ${Yaml_LIBRARY} boost_program_options boost_container  moped)

add_test(NAME MopedUnitTests COMMAND MopedTests)
