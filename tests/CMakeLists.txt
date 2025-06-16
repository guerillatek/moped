if (CMAKE_BUILD_TYPE MATCHES Debug)
    MESSAGE("DEBUG")
    add_compile_options(-g)
    else ()
    add_compile_definitions(BOOST_DIASABLE_ASSERTS)
    add_compile_options(-Ofast)
endif ()

find_package(Boost REQUIRED COMPONENTS program_options iostreams unit_test_framework)

file(GLOB_RECURSE MOPEDTESTSRC ./*.cpp)
add_executable(MopedTests ${MOPEDTESTSRC})
target_include_directories(MopedTests PRIVATE ../..)
target_include_directories(MopedTests PRIVATE ../../..)
set_target_properties(MopedTests PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/tests)
target_link_libraries(MopedTests ${Yaml_LIBRARY} log4cxx boost_program_options boost_container boost_json boost_unit_test_framework moped)
