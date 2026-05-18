cmake_minimum_required(VERSION 3.25)
project(stain VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch missing dependencies from GitHub when not found on the system.
option(STAIN_FETCH_DEPS "Fetch missing dependencies from GitHub" ON)

if(STAIN_FETCH_DEPS)
    include(FetchContent)

    find_package(yoga QUIET)
    if(NOT yoga_FOUND)
        message(STATUS "yoga not found on system – fetching from GitHub")
        FetchContent_Declare(yoga
            GIT_REPOSITORY https://github.com/facebook/yoga.git
            GIT_TAG        v3.2.1
            GIT_SHALLOW    TRUE
        )
        FetchContent_GetProperties(yoga)
        if(NOT yoga_POPULATED)
            FetchContent_Populate(yoga)
            add_subdirectory(${yoga_SOURCE_DIR}/yoga ${yoga_BINARY_DIR})
        endif()
        if(NOT TARGET yoga::yogacore)
            add_library(yoga::yogacore ALIAS yogacore)
        endif()
    endif()

    find_package(tl-expected QUIET)
    if(NOT tl-expected_FOUND)
        message(STATUS "tl-expected not found on system – fetching from GitHub")
        FetchContent_Declare(tl-expected
            GIT_REPOSITORY https://github.com/TartanLlama/expected.git
            GIT_TAG        v1.3.1
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(tl-expected)
    endif()
else()
    find_package(yoga REQUIRED)
    find_package(tl-expected REQUIRED)
endif()

option(STAIN_ENABLE_IMAGE_PROTOCOLS "Enable Kitty/iTerm2 image protocol support (adds stb_image download)" ON)

if(STAIN_ENABLE_IMAGE_PROTOCOLS)
    # stb_image.h for decoding image files.
    set(STB_IMAGE_URL
        "https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h"
    )
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/stb_image.h)
        message(STATUS "Downloading stb_image.h")
        file(DOWNLOAD ${STB_IMAGE_URL} ${CMAKE_BINARY_DIR}/stb_image.h)
    endif()

    # stb_image_write.h for PNG encoding (iTerm2 protocol).
    set(STB_IMAGE_WRITE_URL
        "https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image_write.h"
    )
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/stb_image_write.h)
        message(STATUS "Downloading stb_image_write.h")
        file(DOWNLOAD ${STB_IMAGE_WRITE_URL} ${CMAKE_BINARY_DIR}/stb_image_write.h)
    endif()
endif()

add_library(stain
    src/color.cpp
    src/keybind.cpp
    src/buffer.cpp
    src/focus.cpp
    src/renderable.cpp
    src/terminal.cpp
    src/renderer.cpp
    src/box.cpp
    src/text.cpp
    src/scroll.cpp
    src/input.cpp
    src/select.cpp
    src/framebuffer.cpp
    src/timeline.cpp
    src/ascii_font.cpp
)

if(STAIN_ENABLE_IMAGE_PROTOCOLS)
    target_sources(stain PRIVATE src/image.cpp)
else()
    target_compile_definitions(stain PRIVATE STAIN_DISABLE_IMAGE_PROTOCOLS)
endif()

target_include_directories(stain PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(stain PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(stain PUBLIC yogacore tl::expected)
target_compile_options(stain PRIVATE -Wall -Wextra -Wpedantic)

if(NOT STAIN_ENABLE_IMAGE_PROTOCOLS)
    target_compile_definitions(stain PRIVATE STAIN_DISABLE_IMAGE_PROTOCOLS)
endif()

include(GNUInstallDirs)

install(TARGETS stain
    EXPORT stainTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/stain
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT stainTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stain
    NAMESPACE stain::
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/stainConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

set(stain_CONFIG_IN ${CMAKE_CURRENT_BINARY_DIR}/stainConfig.cmake.in)
file(WRITE ${stain_CONFIG_IN}
"@PACKAGE_INIT@

include(\"\${CMAKE_CURRENT_LIST_DIR}/stainTargets.cmake\")

check_required_components(stain)
")

configure_package_config_file(
    ${stain_CONFIG_IN}
    ${CMAKE_CURRENT_BINARY_DIR}/stainConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stain
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/stainConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/stainConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stain
)

option(STAIN_BUILD_EXAMPLES "Build examples" ON)
option(STAIN_BUILD_TESTS "Build tests" OFF)

if(STAIN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(STAIN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
