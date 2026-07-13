cmake_minimum_required(VERSION 3.10)
project(aylang CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/src")

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
list(REMOVE_ITEM LIB_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/main.cpp")

add_library(${PROJECT_NAME}_lib STATIC ${LIB_SOURCES})
target_include_directories(${PROJECT_NAME}_lib PUBLIC ${PROJECT_INCLUDE})

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)

option(BUILD_TESTING "Build the unit tests" ON)
if(BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
  )
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  include(GoogleTest)

  file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp")

  add_executable(unit_tests ${TEST_SOURCES})
  target_link_libraries(unit_tests PRIVATE ${PROJECT_NAME}_lib GTest::gtest_main)

  gtest_discover_tests(unit_tests)
endif()
