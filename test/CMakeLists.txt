find_package(GTest QUIET)
if (NOT GTest_FOUND)
    add_subdirectory(googletest)
endif ()

set(SOURCES cryptography-test.cpp)

add_executable(tests ${SOURCES})
target_include_directories(tests PRIVATE ../libqnearbyshare-server)
target_link_libraries(tests libqnearbyshare-server GTest::gtest_main)

add_test(NAME test COMMAND tests)
