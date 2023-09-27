cmake_minimum_required(VERSION 3.25)
project(QNearbyShare)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(FindUBSan)
include(FindASan)
include(FindMSan)
include(FindTSan)

find_package(QT REQUIRED NAMES Qt6)
find_package(
    Qt${QT_VERSION_MAJOR}
    COMPONENTS Core Network DBus
    REQUIRED)
include(GNUInstallDirs)

enable_testing()

set(QNEARBYSHARE_DBUS_SERVICE "com.vicr123.qnearbyshare")

add_compile_definitions(QNEARBYSHARE_DBUS_SERVICE="${QNEARBYSHARE_DBUS_SERVICE}")
add_compile_definitions(QNEARBYSHARE_DBUS_SERVICE_ROOT_PATH="/com/vicr123/qnearbyshare")

add_subdirectory(dbus-types)
add_subdirectory(libqnearbyshare-server)
add_subdirectory(qnearbyshared)
add_subdirectory(qnearbyshare-receive)
add_subdirectory(qnearbyshare-send)

include(CTest)

if (BUILD_TESTING)
    add_subdirectory(test)
endif ()
