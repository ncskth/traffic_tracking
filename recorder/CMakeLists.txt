cmake_minimum_required(VERSION 3.10)
project(recorder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Find the GStreamer libraries
find_package(PkgConfig)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)

find_package(MetavisionSDK COMPONENTS core driver ui REQUIRED)

# Include GStreamer header files
include_directories(${GSTREAMER_INCLUDE_DIRS} ${GSTREAMER_APP_INCLUDE_DIRS})

# Add your source file(s)
add_executable(recorder recorder.cpp)
add_executable(snapshot snapshot.cpp)

# Link your application with GStreamer libraries
target_link_libraries(recorder ${GSTREAMER_LIBRARIES} ${GSTREAMER_APP_LIBRARIES})
target_link_libraries(recorder MetavisionSDK::core MetavisionSDK::driver MetavisionSDK::ui)
target_link_libraries(snapshot MetavisionSDK::core MetavisionSDK::driver MetavisionSDK::ui)

# Add the flags returned by pkg-config as compile options for your target
target_compile_options(recorder PUBLIC ${GSTREAMER_CFLAGS_OTHER} ${GSTREAMER_APP_CFLAGS_OTHER})
target_compile_options(snapshot PUBLIC ${GSTREAMER_CFLAGS_OTHER} ${GSTREAMER_APP_CFLAGS_OTHER})