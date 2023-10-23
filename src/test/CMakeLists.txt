# Copyright (C) 2013-2017 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required (VERSION 2.8.1)

pkg_check_modules(GLIB glib-2.0)

# Do not put the build path into RPATH as we are going to install the tests
# Correct soulution would be to provide an install target
set(CMAKE_SKIP_BUILD_RPATH TRUE)

include_directories(.
       ./src-gen/core
       ./src-gen/dbus
       ${COMMONAPI_INCLUDE_DIRS}
       ${gtest_SOURCE_DIR}/include
       ${GLIB_INCLUDE_DIRS}
)

if (MSVC)
 set(TEST_LINK_LIBRARIES CommonAPI-DBus CommonAPI gtest )
else()
 set(TEST_LINK_LIBRARIES -Wl,--no-as-needed CommonAPI-DBus -Wl,--as-needed CommonAPI ${DL_LIBRARY} gtest ${PTHREAD_LIBRARY})
endif()

##############################################################################
# DBusConnectionTest
##############################################################################

add_executable(DBusConnectionTest DBusConnectionTest.cpp)

target_link_libraries(DBusConnectionTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusVariantOutputStreamTest
##############################################################################

add_executable(DBusVariantOutputStreamTest DBusVariantOutputStreamTest.cpp)

target_link_libraries(DBusVariantOutputStreamTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusDaemonProxyTest
##############################################################################


add_executable(DBusDaemonProxyTest DBusDaemonProxyTest.cpp)

target_link_libraries(DBusDaemonProxyTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusVariantTest
##############################################################################

add_executable(DBusVariantTest DBusVariantTest.cpp)

target_link_libraries(DBusVariantTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusClientIdTest
##############################################################################

add_executable(DBusClientIdTest DBusClientIdTest.cpp)

target_link_libraries(DBusClientIdTest ${TEST_LINK_LIBRARIES})

##############################################################################
# Add for every test a dependency to gtest
##############################################################################

add_dependencies(DBusConnectionTest gtest)
add_dependencies(DBusVariantOutputStreamTest gtest)
add_dependencies(DBusDaemonProxyTest gtest)
add_dependencies(DBusVariantTest gtest)
add_dependencies(DBusClientIdTest gtest)

##############################################################################
# Add tests to the target build_tests
##############################################################################

add_dependencies(build_tests DBusConnectionTest)
add_dependencies(build_tests DBusVariantOutputStreamTest)
add_dependencies(build_tests DBusDaemonProxyTest)
add_dependencies(build_tests DBusVariantTest)
add_dependencies(build_tests DBusClientIdTest)

##############################################################################
# Add tests
##############################################################################

add_test(NAME DBusConnectionTest COMMAND DBusConnectionTest)
add_test(NAME DBusVariantOutputStreamTest COMMAND DBusVariantOutputStreamTest)
add_test(NAME DBusDaemonProxyTest COMMAND DBusDaemonProxyTest)
add_test(NAME DBusVariantTest COMMAND DBusVariantTest)
add_test(NAME DBusClientIdTest COMMAND DBusClientIdTest)
