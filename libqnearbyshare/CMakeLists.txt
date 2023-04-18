find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

set(SOURCES
        nearbyshare/nearbyshareserver.cpp
        nearbyshare/nearbysocket.cpp
        nearbyshare/endpointinfo.cpp
        nearbyshare/cryptography.cpp
        nearbyshare/nearbyshareclient.cpp
        nearbyshare/abstractnearbypayload.cpp
        nearbyshare/nearbypayload.cpp
        )

set(HEADERS
        nearbyshare/nearbyshareserver.h
        nearbyshare/nearbysocket.h
        nearbyshare/endpointinfo.h
        nearbyshare/cryptography.h
        nearbyshare/nearbyshareclient.h
        nearbyshare/abstractnearbypayload.h
        nearbyshare/nearbypayload.h
        )

add_subdirectory(QtZeroConf)
add_subdirectory(proto)

add_library(libqnearbyshare ${SOURCES} ${HEADERS})
target_link_libraries(libqnearbyshare Qt::Core Qt::Network QtZeroConf qnearbyshared-proto OpenSSL::SSL OpenSSL::Crypto)
