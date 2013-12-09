cmake_minimum_required (VERSION 2.6)

project (Ravl2)


#find_package(Protobuf REQUIRED)
#find_package(Boost COMPONENTS program_options REQUIRED)

#include_directories(${PROTOBUF_INCLUDE_DIRS})
#include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories ("..") 

#PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS AvatarModel.proto)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")

add_library (Ravl2 RefCounter.cc Index.cc) 

add_executable(testRavl2Core testRavl2Core.cc )

target_link_libraries (testRavl2Core  Ravl2)

#target_link_libraries(meshCompile ${Boost_LIBRARIES} ${PROTOBUF_LIBRARIES})
#target_link_libraries(AvatarModel ${PROTOBUF_LIBRARIES})
