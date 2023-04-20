cmake_minimum_required(VERSION 3.25)
project(QNearbyShare)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

set(BUILD_TESTS OFF CACHE BOOL "Build Tests")

find_package(QT REQUIRED NAMES Qt6)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network DBus REQUIRED)
include(GNUInstallDirs)

enable_testing()

add_compile_definitions(QNEARBYSHARE_DBUS_SERVICE="com.vicr123.qnearbyshare")
add_compile_definitions(QNEARBYSHARE_DBUS_SERVICE_ROOT_PATH="/com/vicr123/qnearbyshare")

add_subdirectory(libqnearbyshare-server)
add_subdirectory(qnearbyshared)
add_subdirectory(qnearbyshare-receive)

if (BUILD_TESTS)
    add_subdirectory(test)
endif ()