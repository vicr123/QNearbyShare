find_package(Protobuf REQUIRED)

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

option(USE_OPENSSL "Use OpenSSL" OFF)


add_library(libqnearbyshare ${SOURCES} ${HEADERS})
target_link_libraries(libqnearbyshare Qt::Core Qt::Network QtZeroConf qnearbyshared-proto ${CRYPTO_LIBRARY})

if(USE_OPENSSL)
    find_package(OpenSSL REQUIRED)

    target_sources(libqnearbyshare PRIVATE nearbyshare/cryptography/opensslcryptography.cpp)
    target_link_libraries(libqnearbyshare OpenSSL::SSL OpenSSL::Crypto)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CryptoPP REQUIRED libcryptopp IMPORTED_TARGET)

    target_sources(libqnearbyshare PRIVATE nearbyshare/cryptography/cryptoppcryptograhy.cpp)
    target_link_libraries(libqnearbyshare PkgConfig::CryptoPP)
endif()