find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

set(SOURCES
        main.cpp

        )

set(HEADERS

        receiver.cpp receiver.h)

add_executable(QNearbyShare-receive ${SOURCES} ${HEADERS})
target_include_directories(QNearbyShare-receive PRIVATE ../libqnearbyshare ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(QNearbyShare-receive libqnearbyshare Qt::Core Qt::DBus)
set_target_properties(QNearbyShare-receive PROPERTIES
        OUTPUT_NAME "qnearbyshare-receive"
        )