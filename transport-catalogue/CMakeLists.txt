cmake_minimum_required(VERSION 3.11)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
  graph.proto
  map_renderer.proto
  svg.proto
  transport_catalogue.proto
  transport_router.proto)

set(TC_TARGET transport_catalogue)
set(TC_SRC
  domain.cpp
  geo.cpp
  json.cpp
  json_builder.cpp
  json_reader.cpp
  main.cpp
  map_renderer.cpp
  request_handler.cpp
  serialization.cpp
  svg.cpp
  transport_catalogue.cpp
  transport_router.cpp)

add_executable(${TC_TARGET} ${PROTO_SRCS} ${PROTO_HDRS} ${TC_SRC})
target_include_directories(${TC_TARGET} PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(${TC_TARGET} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG"
  "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG"
  "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(${TC_TARGET}
  "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>"
  Threads::Threads)