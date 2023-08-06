protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
        device_to_device_messages.proto
        offline_wire_formats.proto
        securegcm.proto
        securemessage.proto
        ukey.proto
        wire_format.proto)

add_library(qnearbyshared-proto ${PROTO_SRCS} ${PROTO_HEADERS})
target_link_libraries(qnearbyshared-proto ${Protobuf_LIBRARIES} absl_log_internal_message absl_log_internal_check_op)
target_include_directories(qnearbyshared-proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})