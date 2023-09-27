set(SOURCES main.cpp devicediscovery.cpp)

set(HEADERS devicediscovery.h console.cpp console.h sendjob.cpp sendjob.h)

add_executable(QNearbyShare-send ${SOURCES} ${HEADERS})
target_link_libraries(QNearbyShare-send Qt::Core Qt::DBus libqnearbyshare-dbus-types)
target_include_directories(QNearbyShare-send PRIVATE ../dbus-types)
set_target_properties(QNearbyShare-send PROPERTIES OUTPUT_NAME "qnearbyshare-send")

install(TARGETS QNearbyShare-send RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
