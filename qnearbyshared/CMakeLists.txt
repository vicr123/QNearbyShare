find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

set(SOURCES
    main.cpp
    dbus/dbusnearbysharemanager.cpp
    dbus/dbusnearbysharesession.cpp
    dbus/dbushelpers.cpp
)

set(HEADERS
    dbus/constants.h
    dbus/dbusnearbysharemanager.h
    dbus/dbusnearbysharesession.h
    dbus/dbushelpers.h
)

#qt_generate_dbus_interface(dbus/dbusnearbysharemanager.h OPTIONS "-a")
#qt_generate_dbus_interface(dbus/dbusnearbysharesession.h OPTIONS "-a")
#qt_add_dbus_adaptor(SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dbusnearbysharemanager.xml dbus/dbusnearbysharemanager.h DBusNearbyShareManager manageradaptor)
#qt_add_dbus_adaptor(SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dbusnearbysharesession.xml dbus/dbusnearbysharesession.h DBusNearbyShareSession sessionadaptor)

add_executable(QNearbyShare-daemon ${SOURCES} ${HEADERS})
target_include_directories(QNearbyShare-daemon PRIVATE ../libqnearbyshare ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(QNearbyShare-daemon libqnearbyshare Qt::Core Qt::Network Qt::DBus)
set_target_properties(QNearbyShare-daemon PROPERTIES
    OUTPUT_NAME "qnearbyshared"
)