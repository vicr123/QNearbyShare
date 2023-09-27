set(SOURCES sendingfile.cpp nearbysharetarget.cpp transferprogress.cpp qnearbysharedbus.cpp)

set(HEADERS sendingfile.h qnearbysharedbus.h nearbysharetarget.h transferprogress.h dbusconstants.h dbuserrors.h)

add_library(libqnearbyshare-dbus-types STATIC ${SOURCES} ${HEADERS})
target_link_libraries(libqnearbyshare-dbus-types Qt::Core Qt::DBus)
