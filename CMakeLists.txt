cmake_minimum_required(VERSION 3.18)
project(cedilla VERSION 1.0)
enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADERS_DIR ${CMAKE_BINARY_DIR}/include/cedilla)
set(UCD_FILE    ${CMAKE_BINARY_DIR}/UnicodeData.txt)

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

find_program(CLANG_FORMAT NAMES clang-format-20 clang-format REQUIRED)

# Code-generator: reads UnicodeData.txt, writes the compressed name tables.
add_executable(namesgen namesgen.cpp)

# Generate include/cedilla/cp_to_name.hpp in the build directory.
# The file is: <compressed tables from namesgen> + <runtime lookup from names.hpp>
add_custom_command(
    COMMENT "Generating include/cedilla/cp_to_name.hpp"
    OUTPUT  ${HEADERS_DIR}/cp_to_name.hpp
    COMMAND ${CMAKE_COMMAND} -E make_directory ${HEADERS_DIR}
    COMMAND namesgen ${UCD_FILE} ${HEADERS_DIR}/cp_to_name.gen
    COMMAND ${CMAKE_COMMAND}
            -DTEMPLATE=${PROJECT_SOURCE_DIR}/names.hpp
            -DINSERT=${HEADERS_DIR}/cp_to_name.gen
            -DOUTPUT=${HEADERS_DIR}/cp_to_name.hpp
            -P ${PROJECT_SOURCE_DIR}/cmake/splice.cmake
    COMMAND ${CLANG_FORMAT} -i ${HEADERS_DIR}/cp_to_name.hpp
    DEPENDS
        ${UCD_FILE}
        namesgen
        ${PROJECT_SOURCE_DIR}/names.hpp
        ${PROJECT_SOURCE_DIR}/cmake/splice.cmake
)

add_custom_target(gen_header DEPENDS ${HEADERS_DIR}/cp_to_name.hpp)

# Header-only library wrapping cp_to_name.hpp
add_library(cedilla INTERFACE)
add_dependencies(cedilla gen_header)
target_include_directories(cedilla INTERFACE ${CMAKE_BINARY_DIR}/include)

add_executable(test_names test.cpp)
add_dependencies(test_names gen_header)
target_include_directories(test_names PRIVATE ${CMAKE_BINARY_DIR}/include)
target_compile_features(test_names PRIVATE cxx_std_17)
if(NOT MSVC)
    target_compile_options(test_names PRIVATE -Wall -Wextra)
endif()
add_test(NAME test_names COMMAND test_names ${UCD_FILE})
