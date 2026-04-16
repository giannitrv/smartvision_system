# pantilt component CMakeLists.txt

add_library(pantilt STATIC
    I2C.cc
    PCA9685.cc
    PID.cc
    pantilt.cc
    servo.cc
)

target_include_directories(pantilt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
