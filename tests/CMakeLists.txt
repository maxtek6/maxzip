include(FetchContent)
FetchContent_Declare(
    maxtest
    GIT_REPOSITORY https://github.com/maxtek6/maxtest.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(maxtest)

maxtest_add_executable(
    unit
    unit.cpp
    ${MAXZIP_SOURCES})

target_link_libraries(unit PRIVATE maxzip)

target_include_directories(unit
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        ${ZSTD_INCLUDE_DIR}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
)


if(MAXZIP_COVER)
    if(WIN32)
        message("skipping code coverage for windows")
    else()
        target_compile_options(unit PRIVATE -fprofile-arcs -ftest-coverage -g -O0)
        target_link_libraries(unit PRIVATE gcov "--coverage")        
        add_custom_target(
            cover
            DEPENDS unit)
        add_custom_command(
            TARGET cover
            COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR}/.. -e ${CMAKE_CURRENT_SOURCE_DIR})
        endif()
endif()

maxtest_add_test(unit brotli::block)
maxtest_add_test(unit zlib::block)
maxtest_add_test(unit zstd::block)
maxtest_add_test(unit brotli::stream)