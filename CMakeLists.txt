cmake_minimum_required(VERSION 3.5.0)
project(proto-console VERSION 0.1.0 LANGUAGES C CXX)

#set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH})
#set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})

find_package(fmt CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)
#include(CTest)
#enable_testing()

add_executable(protype-console main.cpp)

#set(CPACK_PROJECT_NAME ${PROJECT_NAME})
#set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
#include(CPack)


target_link_libraries(proto-console PRIVATE fmt::fmt)

#set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 14)
#SET(CMAKE_CXX_FLAGS "-std=c++20 -pthread -O3")
#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)