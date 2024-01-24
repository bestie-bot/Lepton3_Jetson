SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -pthread")

find_package(OpenCV REQUIRED)

add_executable (custom_temp_app custom_temp_app.cpp)
target_link_libraries (custom_temp_app LINK_PUBLIC 
                       lepton3_grabber 
                       ${OpenCV_LIBS})
