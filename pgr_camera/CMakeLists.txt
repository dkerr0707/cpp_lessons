cmake_minimum_required(VERSION 3.16)
project(pgr_hello CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Spinnaker SDK is installed to /opt/spinnaker by the official installer.
# Override on the command line if it lives elsewhere:
#   cmake -DSPINNAKER_ROOT=/path/to/spinnaker ..
set(SPINNAKER_ROOT "/opt/spinnaker" CACHE PATH "Spinnaker SDK install root")

find_path(SPINNAKER_INCLUDE_DIR Spinnaker.h
    HINTS "${SPINNAKER_ROOT}/include"
    REQUIRED)

find_library(SPINNAKER_LIBRARY Spinnaker
    HINTS "${SPINNAKER_ROOT}/lib"
    REQUIRED)

# Put final executables in ./bin/ alongside the source dir (matches the
# pattern of the top-level exercises).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

foreach(target hello_world camera_info)
    add_executable(${target} ${target}.cpp)
    target_include_directories(${target} PRIVATE ${SPINNAKER_INCLUDE_DIR})
    target_link_libraries(${target} PRIVATE ${SPINNAKER_LIBRARY})
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    # Embed the Spinnaker library path so the binary finds libSpinnaker.so at
    # runtime without needing LD_LIBRARY_PATH.
    set_target_properties(${target} PROPERTIES
        BUILD_RPATH "${SPINNAKER_ROOT}/lib")
endforeach()

# streaming demo: needs OpenCV for the display window.
find_package(OpenCV REQUIRED COMPONENTS core highgui)
add_executable(streaming streaming.cpp)
target_include_directories(streaming PRIVATE ${SPINNAKER_INCLUDE_DIR})
target_link_libraries(streaming PRIVATE ${SPINNAKER_LIBRARY} ${OpenCV_LIBS})
target_compile_options(streaming PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(streaming PROPERTIES
    BUILD_RPATH "${SPINNAKER_ROOT}/lib")

# streaming_gst demo: GStreamer pipeline instead of an OpenCV window.
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET
    gstreamer-1.0 gstreamer-app-1.0)
add_executable(streaming_gst streaming_gst.cpp)
target_include_directories(streaming_gst PRIVATE ${SPINNAKER_INCLUDE_DIR})
target_link_libraries(streaming_gst PRIVATE ${SPINNAKER_LIBRARY} PkgConfig::GST)
target_compile_options(streaming_gst PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(streaming_gst PROPERTIES
    BUILD_RPATH "${SPINNAKER_ROOT}/lib")
