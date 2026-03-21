# Core component CMakeLists.txt

add_executable(rtsp_server
    main.cc
    smartvision.cc
)

# Set RPATH so the executable can find shared libraries in the 'lib' folder
set_target_properties(rtsp_server PROPERTIES INSTALL_RPATH "$ORIGIN/lib")

install(TARGETS rtsp_server DESTINATION .)

target_include_directories(rtsp_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GST_INCLUDE_DIRS}
    ${GST_RTSP_SERVER_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(rtsp_server PRIVATE
    ai_yolov8
    fileutils
    imagedrawing
    imageutils
    ${GST_LIBRARIES}
    ${GST_RTSP_SERVER_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${OpenCV_LIBRARIES}
)

target_compile_options(rtsp_server PRIVATE
    ${GST_CFLAGS_OTHER}
    ${GST_RTSP_SERVER_CFLAGS_OTHER}
    ${GST_APP_CFLAGS_OTHER}
    ${OpenCV_CFLAGS_OTHER}
)
