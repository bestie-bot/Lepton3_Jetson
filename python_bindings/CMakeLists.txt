# cmake build file for Lepton3 python bindings

find_package(Python3 REQUIRED COMPONENTS Development)

add_library(Lepton3 SHARED Lepton3Module.cpp)

target_include_directories(Lepton3 PRIVATE ${Python3_INCLUDE_DIRS}
                            ${CMAKE_CURRENT_SOURCE_DIR}/grabber_lib/include)

target_link_libraries(Lepton3 PRIVATE ${Python3_LIBRARIES}
                                            lepton3_grabber)

set_target_properties(Lepton3 PROPERTIES PREFIX "")
set_target_properties(Lepton3 PROPERTIES SUFFIX ".so")

install(TARGETS Lepton3 DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python3.6/dist-packages)
