find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

set(SOURCES
        main.cpp

        )

set(HEADERS

        receiver.cpp receiver.h)

add_executable(QNearbyShare-receive ${SOURCES} ${HEADERS})
target_link_libraries(QNearbyShare-receive Qt::Core Qt::DBus)
set_target_properties(QNearbyShare-receive PROPERTIES
        OUTPUT_NAME "qnearbyshare-receive"
        )

install(TARGETS QNearbyShare-receive
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )