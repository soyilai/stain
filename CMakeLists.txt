cmake_minimum_required(VERSION 3.25)
project(stain VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(yoga REQUIRED)
find_package(tl-expected REQUIRED)

add_library(stain STATIC
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
)

target_include_directories(stain PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(stain PUBLIC yogacore tl::expected)
target_compile_options(stain PRIVATE -Wall -Wextra -Wpedantic)

include(GNUInstallDirs)

install(TARGETS stain
    EXPORT stainTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
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
option(STAIN_BUILD_TESTS "Build tests" ON)

if(STAIN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(STAIN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
