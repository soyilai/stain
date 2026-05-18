add_executable(demo demo.cpp)
target_link_libraries(demo PRIVATE stain)

add_executable(framebuffer framebuffer.cpp)
target_link_libraries(framebuffer PRIVATE stain)

add_executable(animation animation.cpp)
target_link_libraries(animation PRIVATE stain)

if(STAIN_ENABLE_IMAGE_PROTOCOLS)
    add_executable(image image.cpp)
    target_link_libraries(image PRIVATE stain)
endif()
