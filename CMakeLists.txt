cmake_minimum_required(VERSION 3.31)
project(mlc CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

include_directories(include)

add_subdirectory(src)
add_subdirectory(thirdparty)

# clang-format targets (our code only, not thirdparty):
#   cmake --build build --target format         # rewrite files in place
#   cmake --build build --target format-check   # verify, non-zero exit on diff
find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE MLC_FORMAT_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/include/*.h
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i --style=file ${MLC_FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Formatting sources with clang-format"
        VERBATIM
    )
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE} --style=file --dry-run --Werror ${MLC_FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking source formatting with clang-format"
        VERBATIM
    )
else()
    message(STATUS "clang-format not found; 'format' target unavailable")
endif()
