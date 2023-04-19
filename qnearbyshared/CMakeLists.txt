find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

set(SOURCES
    main.cpp
    dbus/dbusnearbysharemanager.cpp
    dbus/dbusnearbysharesession.cpp
    dbus/dbushelpers.cpp
    dbus/dbusnearbysharelistener.cpp
)

set(HEADERS
    dbus/constants.h
    dbus/dbusnearbysharemanager.h
    dbus/dbusnearbysharesession.h
    dbus/dbushelpers.h
    dbus/dbusnearbysharelistener.h
)

add_executable(QNearbyShare-daemon ${SOURCES} ${HEADERS})
target_include_directories(QNearbyShare-daemon PRIVATE ../libqnearbyshare ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(QNearbyShare-daemon libqnearbyshare Qt::Core Qt::Network Qt::DBus)
set_target_properties(QNearbyShare-daemon PROPERTIES
    OUTPUT_NAME "qnearbyshared"
)