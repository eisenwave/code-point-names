include(FetchContent)
FetchContent_Declare(
        vcpkg
        GIT_REPOSITORY https://github.com/microsoft/vcpkg
        GIT_TAG 8e4cfac84bf6fbb081123b02f8c65bad8418b25e
)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(_vcpkg_fetched "${CMAKE_BINARY_DIR}/_deps/vcpkg-src")
    if(EXISTS "${_vcpkg_fetched}/scripts/buildsystems/vcpkg.cmake")
        set(vcpkg_SOURCE_DIR "${_vcpkg_fetched}")
    else()
        FetchContent_GetProperties(vcpkg
                POPULATED vcpkg_POPULATED
                SOURCE_DIR vcpkg_SOURCE_DIR)
        if(NOT vcpkg_POPULATED)
            FetchContent_Populate(vcpkg)
            FetchContent_GetProperties(vcpkg SOURCE_DIR vcpkg_SOURCE_DIR)
        endif()
    endif()
    unset(_vcpkg_fetched)
    set(CMAKE_TOOLCHAIN_FILE "${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake")
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake")
endif()
