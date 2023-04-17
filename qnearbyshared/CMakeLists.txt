find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

set(SOURCES
    main.cpp
        )

set(HEADERS
        )


add_executable(QNearbyShare ${SOURCES} ${HEADERS})
target_include_directories(QNearbyShare PRIVATE ../libqnearbyshare)
target_link_libraries(QNearbyShare libqnearbyshare Qt::Core Qt::Network)
