# Core component CMakeLists.txt

add_executable(smartvision
    main.cc
    smartvision.cc
    broadcaster.cc
    command_server.cc
    protocol.cc
)

# Set RPATH so the executable can find shared libraries in the 'lib' folder
set_target_properties(smartvision PROPERTIES INSTALL_RPATH "$ORIGIN/lib")

install(TARGETS smartvision DESTINATION .)

target_include_directories(smartvision PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/smartvision_system/3rdparty
    ${GST_INCLUDE_DIRS}
    ${GST_RTSP_SERVER_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(smartvision PRIVATE
    ai_yolov8
    fileutils
    imagedrawing
    imageutils
    ${GST_LIBRARIES}
    ${GST_RTSP_SERVER_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${OpenCV_LIBRARIES}
)

target_compile_options(smartvision PRIVATE
    ${GST_CFLAGS_OTHER}
    ${GST_RTSP_SERVER_CFLAGS_OTHER}
    ${GST_APP_CFLAGS_OTHER}
    ${OpenCV_CFLAGS_OTHER}
)
