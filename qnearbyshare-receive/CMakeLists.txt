set(SOURCES receiver.cpp main.cpp)

set(HEADERS receiver.h)

add_executable(QNearbyShare-receive ${SOURCES} ${HEADERS})
target_link_libraries(QNearbyShare-receive Qt::Core Qt::DBus libqnearbyshare-dbus-types)
target_include_directories(QNearbyShare-receive PRIVATE ../dbus-types)
set_target_properties(QNearbyShare-receive PROPERTIES OUTPUT_NAME "qnearbyshare-receive")

install(TARGETS QNearbyShare-receive RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
