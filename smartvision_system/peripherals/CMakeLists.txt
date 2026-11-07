# can_node component CMakeLists.txt

add_library(peripherals STATIC
    can_bus.cc
    pan_tilt_node.cc
)

target_include_directories(peripherals PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
