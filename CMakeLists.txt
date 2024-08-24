cmake_minimum_required(VERSION 3.10)

project(echoserver)
include_directories(${PROJECT_SOURCE_DIR}/include)
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_LIST)
add_executable(echoserver ${SRC_LIST})

find_package(GTest REQUIRED)
aux_source_directory(${PROJECT_SOURCE_DIR}/test TEST_LIST)
add_executable(runTests ${TEST_LIST})
target_link_libraries(runTests GTest::GTest GTest::Main)

enable_testing()
add_test(NAME UnitTest COMMAND runTests)
