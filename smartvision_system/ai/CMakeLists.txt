# AI component CMakeLists.txt

if (ENABLE_ASAN)
	message(STATUS "BUILD WITH ADDRESS SANITIZER")
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

set(rknpu_yolov8_file rknpu2/yolov8.cc)

# Models and assets are located in the parent 'model' directory
set(MODEL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../model)

add_library(ai_yolov8 STATIC
    ai.cc
    postprocess.cc
    ${rknpu_yolov8_file}
)

target_link_libraries(ai_yolov8 PUBLIC
    imageutils
    fileutils
    imagedrawing    
    ${LIBRKNNRT}
    ${OpenCV_LIBRARIES}
    dl
)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(ai_yolov8 PUBLIC
    log
)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(ai_yolov8 PUBLIC Threads::Threads)
endif()

target_include_directories(ai_yolov8 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBRKNNRT_INCLUDES}
    ${OpenCV_INCLUDE_DIRS}
)

# Install rules for models if this component is installed
install(DIRECTORY ${MODEL_DIR}/ DESTINATION model)