#function(protobuf_append_cpp SRC HDR)
#    protobuf_generate_cpp(SOURCE HEADER ${ARGC})
#    list(APPEND ${SRC} ${SOURCE})
#    list(APPEND ${HDR} ${HEADER})
#
#    set(${SRC} ${${SRC}} PARENT_SCOPE)
#    set(${HDR} ${${HDR}} PARENT_SCOPE)
#endfunction()

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS device_to_device_messages.proto offline_wire_formats.proto securegcm.proto securemessage.proto ukey.proto wire_format.proto)
#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS offline_wire_formats.proto)
#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS securegcm.proto)
#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS securemessage.proto)
#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ukey.proto)
#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS wire_format.proto)

add_library(qnearbyshared-proto ${PROTO_SRCS} ${PROTO_HEADERS})
target_link_libraries(qnearbyshared-proto ${Protobuf_LIBRARIES})
target_include_directories(qnearbyshared-proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})