cmake_minimum_required(VERSION 3.20)
project(fbo)

set(CMAKE_CXX_STANDARD 20)
include(FetchContent)
include(CTest)
find_package(PkgConfig)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb
    GIT_TAG        f1c79c02822848a9bed4315b12c8c8f3761e1296
)

FetchContent_MakeAvailable(stb)

add_library(yavo SHARED
        src/core/framebuffer.cpp
        src/core/framebuffer.hpp
        src/core/logger.hpp
        src/core/image.cpp
        src/core/image.hpp
        src/core/format.cpp
        src/core/format.hpp
        src/core/screen.cpp
        src/core/screen.hpp
        src/core/interrupt.cpp
        src/core/interrupt.hpp
        src/core/color.cpp
        src/core/color.hpp
        src/core/viewport.cpp
        src/core/viewport.hpp
)

if (PkgConfig_FOUND)
    pkg_check_modules(LIBDRM libdrm)

    if (LIBDRM_FOUND)
        add_library(yavo_drm
                src/core/drm.cpp
                src/core/drm.hpp
        )

        pkg_check_modules(LIBDRM REQUIRED IMPORTED_TARGET libdrm)
        target_link_libraries(yavo_drm PUBLIC PkgConfig::LIBDRM)
        target_include_directories(yavo_drm PUBLIC ${LIBDRM_INCLUDE_DIRS})
        target_link_libraries(yavo PUBLIC yavo_drm)
        target_compile_definitions(yavo PUBLIC HAS_LIBDRM)
        message(STATUS "DRM support ENABLED")
        get_target_property(_drm_incs PkgConfig::LIBDRM INTERFACE_INCLUDE_DIRECTORIES)
        message(STATUS "LIBDRM includes: ${_drm_incs}")
    else()
        message(STATUS "DRM support DISABLED, libdrm not found!")
    endif()
else()
    message(STATUS "DRM support DISABLED, PkgConfig not found!")
endif()

add_executable(yav
        src/view/main.cpp
)

if ((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    set(YAV_PRIVATE_FLAGS

        -Wall
        -Wextra
        -pedantic
    )
endif()

target_compile_options(yavo PRIVATE ${YAV_PRIVATE_FLAGS})
target_compile_options(yav PRIVATE ${YAV_PRIVATE_FLAGS})

add_executable(yav_unit_test src/view/test.cpp)
target_include_directories(yav_unit_test PRIVATE src/)
target_link_libraries(yav_unit_test PRIVATE yavo)
add_test(NAME "YAV Unit Test" COMMAND yav_unit_test)

target_link_libraries(yav PRIVATE yavo)
target_include_directories(yavo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include/yavo>
)
target_include_directories(yavo PRIVATE ${stb_SOURCE_DIR})
target_include_directories(yav PRIVATE src/)

set(YAVO_EXPORT_TARGETS yavo yav)

if (TARGET yavo_drm)
    list(APPEND YAVO_EXPORT_TARGETS yavo_drm)
endif()

install(TARGETS ${YAVO_EXPORT_TARGETS}
    EXPORT yavoTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY src/core
    DESTINATION include/yavo
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT yavoTargets
    NAMESPACE yavo::
    DESTINATION lib/cmake/yavo
)
add_library(yavo::yavo ALIAS yavo)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/yavoConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/yavoConfig.cmake"
    INSTALL_DESTINATION lib/cmake/yavo
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/yavoConfig.cmake"
    DESTINATION lib/cmake/yavo
)
