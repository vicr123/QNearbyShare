set(SOURCES main.cpp dbus/dbusnearbysharemanager.cpp dbus/dbusnearbysharesession.cpp dbus/dbushelpers.cpp
            dbus/dbusnearbysharelistener.cpp dbus/dbusnearbysharediscovery.cpp)

set(HEADERS dbus/dbusnearbysharemanager.h dbus/dbusnearbysharesession.h dbus/dbushelpers.h
            dbus/dbusnearbysharelistener.h dbus/dbusnearbysharediscovery.h)

add_executable(QNearbyShare-daemon ${SOURCES} ${HEADERS})
target_include_directories(QNearbyShare-daemon PRIVATE ../libqnearbyshare-server ../dbus-types
                                                       ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(QNearbyShare-daemon libqnearbyshare-server Qt::Core Qt::Network Qt::DBus
                      libqnearbyshare-dbus-types)
set_target_properties(QNearbyShare-daemon PROPERTIES OUTPUT_NAME "qnearbyshared")

configure_file(com.vicr123.qnearbyshare.service.in com.vicr123.qnearbyshare.service)

install(TARGETS QNearbyShare-daemon RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/com.vicr123.qnearbyshare.service
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services)
