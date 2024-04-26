cmake_minimum_required(VERSION 3.15)

project(grpcDemo)

set(CMAKE_CXX_STANDARD 17)

include(common.cmake)

# Proto file
get_filename_component(transfer_proto "transferfile.proto" ABSOLUTE)
get_filename_component(transfer_proto_path "${transfer_proto}" PATH)

# Generated sources
set(transfer_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/transferfile.pb.cc")
set(transfer_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/transferfile.pb.h")
set(transfer_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/transferfile.grpc.pb.cc")
set(transfer_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/transferfile.grpc.pb.h")
add_custom_command(
      OUTPUT "${transfer_proto_srcs}" "${transfer_proto_hdrs}" "${transfer_grpc_srcs}" "${transfer_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${transfer_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${transfer_proto}"
      DEPENDS "${transfer_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# transfer_grpc_proto
add_library(transfer_grpc_proto
  ${transfer_grpc_srcs}
  ${transfer_grpc_hdrs}
  ${transfer_proto_srcs}
  ${transfer_proto_hdrs})
target_link_libraries(transfer_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})


# Targets greeter_[async_](client|server)
foreach(_target
  transfer_client transfer_server)
  add_executable(${_target} "${_target}.cpp")
  target_link_libraries(${_target}
    transfer_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()

foreach(_target
  tcp_client tcp_server)
  add_executable(${_target} "${_target}.cpp")
endforeach()
