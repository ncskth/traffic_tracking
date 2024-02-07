project(recorder)

cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3")
find_package(MetavisionSDK COMPONENTS core driver ui REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main MetavisionSDK::core MetavisionSDK::driver MetavisionSDK::ui)
target_link_libraries(main ${OpenCV_LIBS})
# target_link_libraries(main -lopencv_calib3d)
# target_link_libraries(main -lopencv_core)


target_include_directories(main PRIVATE /usr/include/opencv4)
