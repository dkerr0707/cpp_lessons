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
