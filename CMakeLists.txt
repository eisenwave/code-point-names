cmake_minimum_required(VERSION 3.18)
project(get_code_point_name VERSION 1.0)

# Per-project binary dir so FetchContent consumers don't have their tree polluted.
set(HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
set(UCD_FILE    ${CMAKE_CURRENT_BINARY_DIR}/UnicodeData.txt)

# Only enable testing and require clang-format when this is the top-level build.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
    find_program(CLANG_FORMAT NAMES clang-format-20 clang-format REQUIRED)
else()
    find_program(CLANG_FORMAT NAMES clang-format-20 clang-format)
endif()

# Download UnicodeData.txt into the build directory if absent.
if(NOT EXISTS ${UCD_FILE})
    message(STATUS "Downloading UnicodeData.txt for Unicode 14.0 ...")
    file(DOWNLOAD
        "https://unicode.org/Public/14.0.0/ucd/UnicodeData.txt"
        "${UCD_FILE}"
        SHOW_PROGRESS
        STATUS _dl_status
    )
    list(GET _dl_status 0 _dl_err)
    if(_dl_err)
        message(FATAL_ERROR "Failed to download UnicodeData.txt: ${_dl_status}")
    endif()
endif()

# Code-generator: reads UnicodeData.txt, writes the compressed name tables.
add_executable(namesgen namesgen.cpp)
target_compile_features(namesgen PRIVATE cxx_std_23)
set_target_properties(namesgen PROPERTIES CXX_EXTENSIONS OFF)

# Generate include/get_code_point_name.hpp in the build directory.
# The file is: <compressed tables from namesgen> + <runtime lookup from names.hpp>
if(CLANG_FORMAT)
    set(_fmt_command COMMAND ${CLANG_FORMAT} -i ${HEADERS_DIR}/get_code_point_name.hpp)
endif()
# In top-level builds, copy the result back into include/ so developers
# don't have to do it manually after changing names.hpp or namesgen.cpp.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_copy_command
        COMMAND ${CMAKE_COMMAND} -E copy
                ${HEADERS_DIR}/get_code_point_name.hpp
                ${PROJECT_SOURCE_DIR}/include/get_code_point_name.hpp
    )
endif()
add_custom_command(
    COMMENT "Generating include/get_code_point_name.hpp"
    OUTPUT  ${HEADERS_DIR}/get_code_point_name.hpp
    COMMAND ${CMAKE_COMMAND} -E make_directory ${HEADERS_DIR}
    COMMAND namesgen ${UCD_FILE} ${HEADERS_DIR}/get_code_point_name.gen
    COMMAND ${CMAKE_COMMAND}
            -DTEMPLATE=${PROJECT_SOURCE_DIR}/names.hpp
            -DINSERT=${HEADERS_DIR}/get_code_point_name.gen
            -DOUTPUT=${HEADERS_DIR}/get_code_point_name.hpp
            -P ${PROJECT_SOURCE_DIR}/cmake/splice.cmake
    ${_fmt_command}
    ${_copy_command}
    DEPENDS
        ${UCD_FILE}
        namesgen
        ${PROJECT_SOURCE_DIR}/names.hpp
        ${PROJECT_SOURCE_DIR}/cmake/splice.cmake
)

add_custom_target(gen_header DEPENDS ${HEADERS_DIR}/get_code_point_name.hpp)

# Header-only library
add_library(get_code_point_name INTERFACE)
add_library(get_code_point_name::get_code_point_name ALIAS get_code_point_name)
add_dependencies(get_code_point_name gen_header)
target_include_directories(get_code_point_name INTERFACE
    $<BUILD_INTERFACE:${HEADERS_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(get_code_point_name INTERFACE cxx_std_17)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_executable(test_names test.cpp)
    add_dependencies(test_names gen_header)
    target_include_directories(test_names PRIVATE ${HEADERS_DIR})
    target_compile_features(test_names PRIVATE cxx_std_20)
    if(NOT MSVC)
        target_compile_options(test_names PRIVATE -Wall -Wextra)
    endif()
    add_test(NAME test_names COMMAND test_names ${UCD_FILE})
endif()
