find_package(Protobuf REQUIRED)

set(SOURCES
        nearbyshare/nearbyshareserver.cpp
        nearbyshare/nearbysocket.cpp
        nearbyshare/endpointinfo.cpp
        nearbyshare/cryptography.cpp
        nearbyshare/nearbyshareclient.cpp
        nearbyshare/abstractnearbypayload.cpp
        nearbyshare/nearbypayload.cpp
        nearbyshare/nearbysharediscovery.cpp
        )

set(HEADERS
        nearbyshare/nearbyshareserver.h
        nearbyshare/nearbysocket.h
        nearbyshare/endpointinfo.h
        nearbyshare/cryptography.h
        nearbyshare/nearbyshareclient.h
        nearbyshare/abstractnearbypayload.h
        nearbyshare/nearbypayload.h
        nearbyshare/nearbysharediscovery.h nearbyshare/nearbyshareconstants.h)

add_subdirectory(QtZeroConf EXCLUDE_FROM_ALL)
add_subdirectory(proto)

option(USE_OPENSSL "Use OpenSSL" OFF)

add_library(libqnearbyshare-server STATIC ${SOURCES} ${HEADERS})
set_target_properties(libqnearbyshare-server PROPERTIES
        OUTPUT_NAME "qnearbyshare-server"
        )
target_link_libraries(libqnearbyshare-server Qt::Core Qt::Network QtZeroConf qnearbyshared-proto ${CRYPTO_LIBRARY})

if (USE_OPENSSL)
    find_package(OpenSSL REQUIRED)

    target_sources(libqnearbyshare-server PRIVATE nearbyshare/cryptography/opensslcryptography.cpp)
    target_link_libraries(libqnearbyshare-server OpenSSL::SSL OpenSSL::Crypto)
else ()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CryptoPP REQUIRED libcryptopp IMPORTED_TARGET)

    target_sources(libqnearbyshare-server PRIVATE nearbyshare/cryptography/cryptoppcryptograhy.cpp)
    target_link_libraries(libqnearbyshare-server PkgConfig::CryptoPP)
endif()
