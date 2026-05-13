find_package(GTest REQUIRED)
include(CTest)

add_executable(stain_color_test color_test.cpp)
target_link_libraries(stain_color_test PRIVATE stain GTest::gtest GTest::gtest_main)
add_test(NAME color_test COMMAND stain_color_test)

add_executable(stain_buffer_test buffer_test.cpp)
target_link_libraries(stain_buffer_test PRIVATE stain GTest::gtest GTest::gtest_main)
add_test(NAME buffer_test COMMAND stain_buffer_test)

add_executable(stain_signal_test signal_test.cpp)
target_link_libraries(stain_signal_test PRIVATE stain GTest::gtest GTest::gtest_main)
add_test(NAME signal_test COMMAND stain_signal_test)

add_executable(stain_keybind_test keybind_test.cpp)
target_link_libraries(stain_keybind_test PRIVATE stain GTest::gtest GTest::gtest_main)
add_test(NAME keybind_test COMMAND stain_keybind_test)

add_executable(stain_edit_buffer_test edit_buffer_test.cpp)
target_link_libraries(stain_edit_buffer_test PRIVATE stain GTest::gtest GTest::gtest_main)
add_test(NAME edit_buffer_test COMMAND stain_edit_buffer_test)
