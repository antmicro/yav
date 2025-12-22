cmake_minimum_required(VERSION 3.20)
project(fbo)

set(CMAKE_CXX_STANDARD 20)
include(FetchContent)
include(CTest)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb
    GIT_TAG        f1c79c02822848a9bed4315b12c8c8f3761e1296
)

FetchContent_MakeAvailable(stb)

add_library(yavo OBJECT
        src/core/framebuffer.cpp
        src/core/framebuffer.hpp
        src/core/logger.hpp
        src/core/image.cpp
        src/core/image.hpp
        src/core/screen.hpp
        src/core/format.cpp
        src/core/format.hpp
        src/core/screen.cpp
)

add_executable(yav
        src/view/main.cpp
)

add_executable(yav_unit_test src/view/test.cpp)
target_include_directories(yav_unit_test PRIVATE src/)
target_link_libraries(yav_unit_test PRIVATE yavo)
add_test(NAME "YAV Unit Test" COMMAND yav_unit_test)

target_link_libraries(yav PRIVATE yavo)
target_include_directories(yavo PRIVATE src/ ${stb_SOURCE_DIR})
target_include_directories(yav PRIVATE src/)
install(TARGETS yav)
