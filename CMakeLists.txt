cmake_minimum_required(VERSION 3.22)
project(LPR LANGUAGES CXX CUDA)

# Set C++ standard and enable modern practices
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # optional, Disable compiler-specific extensions

# set(CMAKE_BUILD_TYPE Debug)  # Automatically adds -g
# OR
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
# Enable debug symbols by default if no build type is set
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Configure where binaries will be placed
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")# Enable all features your current CPU supports
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
# Force AVX2
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mavx2")
# Or force AVX-512
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mavx512f")




option(CUDA_USE_STATIC_CUDA_RUNTIME OFF)

# For larger projects
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()


# Install target (for packaging)
# install(TARGETS text_reader DESTINATION bin)
# install(DIRECTORY data/ DESTINATION share/text_reader/data)


find_package(PkgConfig REQUIRED)
# find_package(CUDA REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
include_directories(${GST_INCLUDE_DIRS})
pkg_check_modules(GLIB REQUIRED glib-2.0 gobject-2.0 nlohmann_json  gstreamer-base-1.0 
                                gstreamer-rtsp-server-1.0 gstreamer-rtsp-1.0 gstreamer-1.0 gstreamer-video-1.0)
                            
add_definitions(${GST_CFLAGS_OTHER})

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    message("embed_platform on")
    include_directories(/usr/local/cuda/targets/aarch64-linux/include)
    link_directories(/usr/local/cuda/targets/aarch64-linux/lib)
else()
    message("embed_platform off")
    include_directories(/usr/local/cuda/include)
    link_directories(/usr/local/cuda/lib64)
endif()


include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include)
include_directories(/opt/nvidia/deepstream/deepstream-7.1/sources/includes)
include_directories(/usr/include/gstreamer-1.0)
include_directories(/usr/include/nlohmann)
include_directories(/usr/local/cuda/include)
include_directories(/opt/nvidia/deepstream/deepstream-7.1/service-maker/includes/)
link_directories(/opt/nvidia/deepstream/deepstream/lib/)
link_directories(/opt/nvidia/deepstream/deepstream/lib/gst-plugins)
link_directories(/usr/local/cuda/lib64/)
link_directories(/usr/local/cuda/targets/x86_64-linux/lib/)
# include_directories(${PROJECT_SOURCE_DIR}/camera_manager.hpp)

# Collect source files
file(GLOB SOURCES "src/*.cpp")
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")
# set(SOURCES src/main.cpp)


# missing initializer for member 'NvDsInferDims::d' [-Werror=missing-field-initializers] NvDsInferDims dims = {0};
# set_source_files_properties(
#     src/nv_infer_server_manager.cpp
#     src/main.cpp
#     src/pipeline_manager.cpp
#     src/nv_tracker_manager.cpp
#     src/nv_osd_manager.cpp
#     PROPERTIES COMPILE_FLAGS "-Wno-missing-field-initializers"
# )


# Create the executable
add_executable(${PROJECT_NAME}  ${SOURCES})

# Copy data files to build directory
file(COPY config/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config)

# Set compiler warnings
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall -Wextra -pedantic -Werror>
)

# Optional: set compiler warnings (GCC/Clang/MSVC)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()


# Optionally set CUDA-specific flags
set_target_properties(${PROJECT_NAME} PROPERTIES
    CUDA_STANDARD 17                         # or 14, 20 depending on your code. Specifies which C++ standard to use for both host and device code.
    CUDA_STANDARD_REQUIRED ON               # Enforces the standard. Enforces that standard—avoids falling back to older versions silently.
    CUDA_SEPARABLE_COMPILATION ON           # Needed if kernels are in .cu files. Required if your project has multiple .cu files calling each other.
    # CUDA_ARCHITECTURES 86                   # Set to your GPU's arch (e.g., 86 for Ampere)Helps CMake compile only for the correct GPU architecture (you can use CMake's CUDA_ARCHITECTURES values).
)
# Optional: configure output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/bin
)
# https://cmake.org/cmake/help/latest/prop_tgt/CUDA_ARCHITECTURES.html
# https://cmake.org/cmake/help/latest/policy/CMP0146.html

# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build_${CMAKE_BUILD_TYPE}/bin)
# file(COPY config/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config)
# Set output directory per build type
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build_${CMAKE_BUILD_TYPE}/bin)
# # Copy config folder to the executable output directory after build
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     ${PROJECT_SOURCE_DIR}/config
#     $<TARGET_FILE_DIR:${PROJECT_NAME}>/config
# )


# Include current directory for headers
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${GLIB_LIBRARIES})

target_link_libraries(${PROJECT_NAME} gstbase-1.0 gstreamer-1.0 gstrtp-1.0 gstvideo-1.0 gstrtspserver-1.0)
target_link_libraries(${PROJECT_NAME} cudart cuda)
target_link_libraries(${PROJECT_NAME} nvdsgst_infer nvds_meta nvds_inferutils
                                      nvdsgst_meta nvds_utils nvdsgst_helper nvds_meta)