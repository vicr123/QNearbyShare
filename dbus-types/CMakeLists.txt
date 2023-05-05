set(SOURCES

        )

set(HEADERS

        sendingfile.cpp sendingfile.h qnearbysharedbus.cpp qnearbysharedbus.h nearbysharetarget.cpp nearbysharetarget.h transferprogress.cpp transferprogress.h dbusconstants.h dbuserrors.h)

add_library(libqnearbyshare-dbus-types STATIC ${SOURCES} ${HEADERS})
target_link_libraries(libqnearbyshare-dbus-types Qt::Core Qt::DBus)
