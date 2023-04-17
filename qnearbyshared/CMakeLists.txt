find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

set(SOURCES
    main.cpp
        )

set(HEADERS
        )


add_executable(QNearbyShare-daemon ${SOURCES} ${HEADERS})
target_include_directories(QNearbyShare-daemon PRIVATE ../libqnearbyshare)
target_link_libraries(QNearbyShare-daemon libqnearbyshare Qt::Core Qt::Network)
set_target_properties(QNearbyShare-daemon PROPERTIES
    OUTPUT_NAME "qnearbyshared"
)