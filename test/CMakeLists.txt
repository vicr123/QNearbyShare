add_subdirectory(googletest)

set(SOURCES
        cryptography-test.cpp
        )

add_executable(tests ${SOURCES})
target_include_directories(tests PRIVATE googletest/googletest/include googletest/googletest ../libqnearbyshare-server)
target_link_libraries(tests libqnearbyshare-server gtest gtest_main)
