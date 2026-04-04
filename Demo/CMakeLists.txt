cmake_minimum_required(VERSION 3.16)
project(ALL_AI_Example CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(CURL REQUIRED)

add_executable(all-ai main.cpp)

target_include_directories(all-ai PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include # nlohmann/json
)

target_link_libraries(all-ai PRIVATE CURL::libcurl)