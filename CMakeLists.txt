cmake_minimum_required(VERSION 3.0)

project(vis)

if(UNIX)
    set(CMAKE_C_FLAG "${CMAKE_C_FLAGS} -Wall -Wextra -Wconversion -Wno-unused-parameter -Wno-ignored-qualifiers ")

    # Color for ninja and Clang on Linux and OSX
    if (CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_GENERATOR STREQUAL "Ninja")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
    endif()
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O2 -DNDEBUG")

# # Hardening
if (CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_GENERATOR STREQUAL "Ninja")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE -fstack-protector-all -D_FORTIFY_SOURCE=2")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fstack-protector-all -D_FORTIFY_SOURCE=2")
endif()

set(CMAKE_EXE_LINKER_FLAGS "-z now -z relro -pie")


set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -UNDEBUG -O0 -g -ggdb -Wall\
                        -Wextra -Wno-missing-field-initializers\
                        -Wno-unused-parameter")

if( EXISTS "${CMAKE_SOURCE_DIR}/.git" )
    execute_process(COMMAND git rev-list --count master
                   OUTPUT_VARIABLE POD_VERSION
                   OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND bash -c "echo \"${POD_VERSION}\" \
                                     > ${CMAKE_SOURCE_DIR}/version" )
else( EXISTS "${CMAKE_SOURCE_DIR}/.git" )
    execute_process(COMMAND cat ${CMAKE_SOURCE_DIR}/etc/version
                   OUTPUT_VARIABLE POD_VERSION
                   OUTPUT_STRIP_TRAILING_WHITESPACE)
endif( EXISTS "${CMAKE_SOURCE_DIR}/.git" )

add_definitions(-DVERSION=${POD_VERSION})
add_definitions(-D_XOPEN_SOURCE=700)
add_definitions(-D_POSIX_C_SOURCE=200809L)


aux_source_directory(. VIS_SOURCES)

include_directories(AFTER SYSTEM /usr/include/lua5.1)
include_directories(BEFORE .)

add_executable(vis ${VIS_SOURCES})

target_link_libraries(vis uv ncursesw c lua5.1)


