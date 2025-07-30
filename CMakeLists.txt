cmake_minimum_required(VERSION 3.20)

project(maxzip)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(MAXZIP_TESTS "Build tests" OFF)
option(MAXZIP_COVER "Build with code coverage" OFF)
option(MAXZIP_VENDORED "Use vendored libraries" OFF)

if(MAXZIP_VENDORED)
    include(FetchContent)

    option(BROTLI_DISABLE_TESTS "" ON)
    FetchContent_Declare(
        brotli
        GIT_REPOSITORY https://github.com/google/brotli.git
        GIT_TAG        v1.0.9
    )
    FetchContent_MakeAvailable(brotli)

    option(ZLIB_BUILD_EXAMPLES OFF)
    FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG        v1.3.1
    )
    FetchContent_MakeAvailable(zlib)

    set(BUILD_TESTING OFF)
    option(ZSTD_BUILD_TESTS OFF)
    FetchContent_Declare(
        zstd
        GIT_REPOSITORY https://github.com/facebook/zstd.git
        GIT_TAG        v1.5.7
        SOURCE_SUBDIR build/cmake
    )
    FetchContent_MakeAvailable(zstd)
    set(ZSTD_INCLUDE_DIR "${zstd_SOURCE_DIR}/lib")

    set(MAXZIP_LIBRARIES
        zlibstatic
        libzstd_static
        brotlicommon-static
        brotlienc-static
        brotlidec-static
    )

endif()

file(GLOB MAXZIP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(maxzip SHARED
    ${MAXZIP_SOURCES}
)

add_library(maxzip_a STATIC
    ${MAXZIP_SOURCES}
)

target_include_directories(maxzip
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${ZSTD_INCLUDE_DIR}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_include_directories(maxzip_a
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${ZSTD_INCLUDE_DIR}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(maxzip
    PUBLIC
        ${MAXZIP_LIBRARIES}
)

target_link_libraries(maxzip_a
    PUBLIC
        ${MAXZIP_LIBRARIES}
)

if(MAXZIP_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()