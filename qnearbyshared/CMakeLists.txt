find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

set(SOURCES
    main.cpp
    nearbyshare/nearbyshareserver.cpp
        nearbyshare/nearbysocket.cpp
        nearbyshare/endpointinfo.cpp
        nearbyshare/cryptography.cpp)

set(HEADERS
    nearbyshare/nearbyshareserver.h
        nearbyshare/nearbysocket.h
        nearbyshare/endpointinfo.h
        nearbyshare/cryptography.h nearbyshare/nearbyshareclient.cpp nearbyshare/nearbyshareclient.h)

add_subdirectory(QtZeroConf)
add_subdirectory(proto)

#qt_add_protobuf(NearbyShareMessages
#        GENERATE_PACKAGE_SUBFOLDERS
#        PROTO_FILES
#            proto/device_to_device_messages.proto
#            proto/offline_wire_formats.proto
#            proto/securegcm.proto
#            proto/securemessage.proto
#            proto/ukey.proto
#            proto/wire_format.proto
#)

add_executable(QNearbyShare ${SOURCES} ${HEADERS} ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(QNearbyShare PRIVATE ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(QNearbyShare Qt::Core Qt::Network QtZeroConf qnearbyshared-proto OpenSSL::SSL OpenSSL::Crypto)
