# Copyright (C) 2013-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required (VERSION 2.8.1)

pkg_check_modules(GLIB glib-2.0)

include_directories(.
       ./src-gen
       ${COMMONAPI_INCLUDE_DIRS}
       ${DBus_INCLUDE_DIRS}
       ${gtest_SOURCE_DIR}/include
       ${GLIB_INCLUDE_DIRS}
)

set(VERSION "v1_0")

set(TestInterfaceSources src-gen/commonapi/tests/DerivedTypeCollection.cpp
                         src-gen/${VERSION}/commonapi/tests/TestInterfaceStubDefault.cpp)

set(TestInterfaceDBusSources ${TestInterfaceSources}
                             src-gen/${VERSION}/commonapi/tests/TestInterfaceDBusProxy.cpp
                             src-gen/${VERSION}/commonapi/tests/TestInterfaceDBusStubAdapter.cpp
                             src-gen/${VERSION}/fake/legacy/service/LegacyInterfaceDBusProxy.cpp)


set(FreedesktopPropertiesDBusSources ${TestInterfaceSources}
                                     src-gen/${VERSION}/commonapi/tests/TestFreedesktopInterfaceDBusProxy.cpp
                                     src-gen/${VERSION}/commonapi/tests/TestFreedesktopInterfaceDBusStubAdapter.cpp
                                     src-gen/${VERSION}/commonapi/tests/TestFreedesktopInterfaceStubDefault.cpp
                                     src-gen/${VERSION}/commonapi/tests/TestFreedesktopDerivedInterfaceDBusProxy.cpp
                                     src-gen/${VERSION}/commonapi/tests/TestFreedesktopDerivedInterfaceDBusStubAdapter.cpp
                                     src-gen/${VERSION}/commonapi/tests/TestFreedesktopDerivedInterfaceStubDefault.cpp)

set(ManagedSources src-gen/${VERSION}/commonapi/tests/managed/LeafInterfaceStubDefault.cpp
                   src-gen/${VERSION}/commonapi/tests/managed/BranchInterfaceStubDefault.cpp
                   src-gen/${VERSION}/commonapi/tests/managed/RootInterfaceStubDefault.cpp
                   src-gen/${VERSION}/commonapi/tests/managed/SecondRootStubDefault.cpp)

set(ManagedDBusSources ${ManagedSources}
                       src-gen/${VERSION}/commonapi/tests/managed/RootInterfaceDBusProxy.cpp
                       src-gen/${VERSION}/commonapi/tests/managed/RootInterfaceDBusStubAdapter.cpp
                       src-gen/${VERSION}/commonapi/tests/managed/LeafInterfaceDBusProxy.cpp
                       src-gen/${VERSION}/commonapi/tests/managed/LeafInterfaceDBusStubAdapter.cpp
                       src-gen/${VERSION}/commonapi/tests/managed/BranchInterfaceDBusProxy.cpp
                       src-gen/${VERSION}/commonapi/tests/managed/BranchInterfaceDBusStubAdapter.cpp
                       src-gen/${VERSION}/commonapi/tests/managed/SecondRootDBusProxy.cpp
                       src-gen/${VERSION}/commonapi/tests/managed/SecondRootDBusStubAdapter.cpp)

set(ExtendedInterfaceSources src-gen/${VERSION}/commonapi/tests/ExtendedInterfaceDBusProxy.cpp
                             src-gen/${VERSION}/commonapi/tests/ExtendedInterfaceDBusStubAdapter.cpp
                             src-gen/${VERSION}/commonapi/tests/ExtendedInterfaceStubDefault.cpp)

set(LIB_LINK_LIBRARIES -Wl,--no-as-needed CommonAPI-DBus -Wl,--as-needed CommonAPI)

set(TEST_LINK_LIBRARIES -Wl,--no-as-needed CommonAPI-DBus -Wl,--as-needed CommonAPI ${DBus_LDFLAGS} ${DL_LIBRARY} gtest ${PTHREAD_LIBRARY})

set(TEST_LINK_LIBRARIES_WITHOUT_COMMONAPI_DBUS CommonAPI gtest ${PTHREAD_LIBRARY})

##### FIXME: disabled due to removal of MiddlewareInfo from CommonAPI
##### add_library(CommonAPI-Fake SHARED dbusDynamicLoadingTests/fakeTestBinding/FakeBinding.cpp)
##### set_target_properties(CommonAPI-Fake PROPERTIES VERSION 1.0.0 SOVERSION 1 LINKER_LANGUAGE C)
##### target_link_libraries(CommonAPI-Fake ${LIB_LINK_LIBRARIES})

add_library(DBusGen-TestInterface SHARED ${TestInterfaceDBusSources})
set_target_properties(DBusGen-TestInterface PROPERTIES VERSION 3.0.0 SOVERSION 3 LINKER_LANGUAGE C)
target_link_libraries(DBusGen-TestInterface ${LIB_LINK_LIBRARIES})

add_library(SomeOtherNameForGeneratedDBus SHARED ${TestInterfaceDBusSources})
set_target_properties(SomeOtherNameForGeneratedDBus PROPERTIES VERSION 3.0.0 SOVERSION 3 LINKER_LANGUAGE C)
target_link_libraries(SomeOtherNameForGeneratedDBus ${LIB_LINK_LIBRARIES})

##############################################################################
# DBusManagedTest
##############################################################################

##### FIXME: serviceFactory not part of CommonAPI 3.0
##### add_executable(DBusManagedTest DBusManagedTest.cpp
#####                               ${ManagedDBusSources})
##### target_link_libraries(DBusManagedTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusObjectManagerStubTest
##############################################################################

##### FIXME: SubscriptionStatus not part of CommonAPI 3.0
##### add_executable(DBusObjectManagerStubTest DBusObjectManagerStubTest.cpp)
##### target_link_libraries(DBusObjectManagerStubTest ${TEST_LINK_LIBRARIES} ${GLIB_LDFLAGS})

##############################################################################
# DBusMainLoopIntegrationTest
##############################################################################

add_executable(DBusMainLoopIntegrationTest DBusMainLoopIntegrationTest.cpp
                                        ${TestInterfaceDBusSources})
target_link_libraries(DBusMainLoopIntegrationTest ${TEST_LINK_LIBRARIES} ${GLIB_LDFLAGS})

##############################################################################
# DBusServiceRegistryTest
##############################################################################

##### FIXME: getCurrentBinaryFileFQN, SubscriptionStatus not part of CommonAPI 3.0 
##### add_executable(DBusServiceRegistryTest DBusServiceRegistryTest.cpp
#####                                       ${TestInterfaceDBusSources})
#####
##### target_link_libraries(DBusServiceRegistryTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusAddressTranslatorTest
##############################################################################


add_executable(DBusAddressTranslatorTest DBusAddressTranslatorTest.cpp
                                         ${TestInterfaceDBusSources})

target_link_libraries(DBusAddressTranslatorTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusConnectionTest
##############################################################################


add_executable(DBusConnectionTest DBusConnectionTest.cpp)

target_link_libraries(DBusConnectionTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusTypeStreamTest
##############################################################################

##### FIXME: CommonAPI::Typewriter not part of CommonAPI 3.0 
##### add_executable(DBusTypeStreamTest DBusTypeStreamTest.cpp)
##### 
##### target_link_libraries(DBusTypeStreamTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusVariantOutputStreamTest
##############################################################################

add_executable(DBusVariantOutputStreamTest DBusVariantOutputStreamTest.cpp)

target_link_libraries(DBusVariantOutputStreamTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusCommunicationTest
##############################################################################

add_executable(DBusCommunicationTest DBusCommunicationTest.cpp
                                     ${TestInterfaceDBusSources})

target_link_libraries(DBusCommunicationTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusDaemonProxyTest
##############################################################################


add_executable(DBusDaemonProxyTest DBusDaemonProxyTest.cpp)

target_link_libraries(DBusDaemonProxyTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusInputStreamTest
##############################################################################

add_executable(DBusInputStreamTest DBusInputStreamTest.cpp
                                   ${TestInterfaceDBusSources})

target_link_libraries(DBusInputStreamTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusOutputStreamTest
##############################################################################


add_executable(DBusOutputStreamTest DBusOutputStreamTest.cpp
                                    ${TestInterfaceDBusSources})

target_link_libraries(DBusOutputStreamTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusFactoryTest
##############################################################################

add_executable(DBusFactoryTest DBusFactoryTest.cpp
                               ${TestInterfaceDBusSources})

target_link_libraries(DBusFactoryTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusMultipleConnectionTest
##############################################################################

add_executable(DBusMultipleConnectionTest DBusMultipleConnectionTest.cpp
                                          ${TestInterfaceDBusSources})

target_link_libraries(DBusMultipleConnectionTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusProxyTest
##############################################################################

##### FIXME: generated ExtendedInterfaceProxy.hpp contains serveral errors
##### add_executable(DBusProxyTest DBusProxyTest.cpp
#####                            ${TestInterfaceDBusSources}
#####                            ${ExtendedInterfaceSources})
#####
##### target_link_libraries(DBusProxyTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusFreedesktopPropertiesTest
##############################################################################

##### FIXME: generated TestFreedesktopDerivedInterfaceProxy.hpp contains serveral errors
##### add_executable(DBusFreedesktopPropertiesTest DBusFreedesktopPropertiesTest.cpp
#####                                            ${FreedesktopPropertiesDBusSources})
#####
##### target_link_libraries(DBusFreedesktopPropertiesTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusServicePublisherTest
##############################################################################

add_executable(DBusServicePublisherTest DBusServicePublisherTest.cpp
                                        ${TestInterfaceDBusSources})

target_link_libraries(DBusServicePublisherTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusVariantTest
##############################################################################

add_executable(DBusVariantTest DBusVariantTest.cpp)

target_link_libraries(DBusVariantTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusRuntimeTest
##############################################################################

add_executable(DBusRuntimeTest DBusRuntimeTest.cpp)

target_link_libraries(DBusRuntimeTest ${TEST_LINK_LIBRARIES})

#####
##### FIXME: commented out disfunctional test, due to different middleware loading mechanism                     
#####
###################################################################################                       
###### DBusDynamicLoadingBasicTest                                                                        
###################################################################################                       
#####                                                                                                     
#####add_executable(DBusDynamicLoadingBasicTest ${TestInterfaceSources}                                   
#####                                           dbusDynamicLoadingTests/DBusDynamicLoadingBasicTest.cpp)  
#####                                                                                                     
#####target_link_libraries(DBusDynamicLoadingBasicTest ${TEST_LINK_LIBRARIES})                            
#####                                                                                                     
###################################################################################                       
###### DBusDynamicLoadingMultipleDefinitionTest                                                           
###################################################################################                       
#####                                                                                                     
#####add_executable(DBusDynamicLoadingMultipleDefinitionTest ${TestInterfaceSources}                      
#####                                                        dbusDynamicLoadingTests/DBusDynamicLoadingMul
#####                                                                                                     
#####target_link_libraries(DBusDynamicLoadingMultipleDefinitionTest ${TEST_LINK_LIBRARIES})               
#####                                                                                                     
###################################################################################                       
###### DBusDynamicLoadingMultipleBindingsTest                                                             
###################################################################################                       
#####                                                                                                     
#####add_executable(DBusDynamicLoadingMultipleBindingsTest ${TestInterfaceSources}                        
#####                                                      dbusDynamicLoadingTests/DBusDynamicLoadingMulti
#####                                                                                                     
#####target_link_libraries(DBusDynamicLoadingMultipleBindingsTest ${TEST_LINK_LIBRARIES})                 
#####                                                                                                     
###################################################################################                       
###### DBusDynamicLoadingMixedValidityTest                                                                
###################################################################################                       
#####                                                                                                     
#####add_executable(DBusDynamicLoadingMixedValidityTest ${TestInterfaceSources}                           
#####                                                   dbusDynamicLoadingTests/DBusDynamicLoadingMixedVal
#####                                                                                                     
#####target_link_libraries(DBusDynamicLoadingMixedValidityTest ${TEST_LINK_LIBRARIES})                    
#####                                                                                                     
###################################################################################                       
###### DBusDynamicLoadingNoValidityTest                                                                   
###################################################################################                       
#####                                                                                                     
#####add_executable(DBusDynamicLoadingNoValidityTest ${TestInterfaceSources}                              
#####                                                dbusDynamicLoadingTests/DBusDynamicLoadingNoValidityT
#####                                                                                                     
#####target_link_libraries(DBusDynamicLoadingNoValidityTest ${TEST_LINK_LIBRARIES_WITHOUT_COMMONAPI_DBUS})

##############################################################################
# DBusClientIdTest
##############################################################################

add_executable(DBusClientIdTest DBusClientIdTest.cpp)

target_link_libraries(DBusClientIdTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusBroadcastTest
##############################################################################

add_executable(DBusBroadcastTest ${TestInterfaceDBusSources}
                                 DBusBroadcastTest.cpp)

target_link_libraries(DBusBroadcastTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusPolymorphicTest
##############################################################################


add_executable(DBusPolymorphicTest ${TestInterfaceDBusSources}
                                   DBusPolymorphicTest.cpp)

target_link_libraries(DBusPolymorphicTest ${TEST_LINK_LIBRARIES})

##############################################################################
# DBusLoadTest
##############################################################################

add_executable(DBusLoadTest DBusLoadTest.cpp
                            ${TestInterfaceDBusSources})

target_link_libraries(DBusLoadTest ${TEST_LINK_LIBRARIES})

##############################################################################
# Add for every test a dependency to gtest
##############################################################################

##### add_dependencies(DBusManagedTest gtest)
##### add_dependencies(DBusObjectManagerStubTest gtest)
add_dependencies(DBusMainLoopIntegrationTest gtest)
##### add_dependencies(DBusServiceRegistryTest gtest)
add_dependencies(DBusAddressTranslatorTest gtest)
add_dependencies(DBusConnectionTest gtest)
##### add_dependencies(DBusTypeStreamTest gtest)
add_dependencies(DBusVariantOutputStreamTest gtest)
add_dependencies(DBusCommunicationTest gtest)
add_dependencies(DBusDaemonProxyTest gtest)
add_dependencies(DBusInputStreamTest gtest)
add_dependencies(DBusOutputStreamTest gtest)
add_dependencies(DBusFactoryTest gtest)
add_dependencies(DBusMultipleConnectionTest gtest)
##### add_dependencies(DBusProxyTest gtest)
##### add_dependencies(DBusFreedesktopPropertiesTest gtest)
add_dependencies(DBusServicePublisherTest gtest)
add_dependencies(DBusVariantTest gtest)
add_dependencies(DBusRuntimeTest gtest)                
##### add_dependencies(DBusDynamicLoadingBasicTest gtest)
##### add_dependencies(DBusDynamicLoadingMultipleDefinitionTest gtest)
##### add_dependencies(DBusDynamicLoadingMultipleBindingsTest gtest)
##### add_dependencies(DBusDynamicLoadingMixedValidityTest gtest)
##### add_dependencies(DBusDynamicLoadingNoValidityTest gtest)
add_dependencies(DBusClientIdTest gtest)
add_dependencies(DBusBroadcastTest gtest)
add_dependencies(DBusPolymorphicTest gtest)
add_dependencies(DBusLoadTest gtest)

##############################################################################
# Add tests to the target build_tests
##############################################################################

##### add_dependencies(build_tests CommonAPI-Fake)
add_dependencies(build_tests DBusGen-TestInterface)
add_dependencies(build_tests SomeOtherNameForGeneratedDBus)
##### add_dependencies(build_tests DBusManagedTest)
##### add_dependencies(build_tests DBusObjectManagerStubTest)
add_dependencies(build_tests DBusMainLoopIntegrationTest)
##### add_dependencies(build_tests DBusServiceRegistryTest)
add_dependencies(build_tests DBusAddressTranslatorTest)
add_dependencies(build_tests DBusConnectionTest)
##### add_dependencies(build_tests DBusTypeStreamTest)
add_dependencies(build_tests DBusVariantOutputStreamTest)
add_dependencies(build_tests DBusCommunicationTest)
add_dependencies(build_tests DBusDaemonProxyTest)
add_dependencies(build_tests DBusInputStreamTest)
add_dependencies(build_tests DBusOutputStreamTest)
add_dependencies(build_tests DBusFactoryTest)
add_dependencies(build_tests DBusMultipleConnectionTest)
##### add_dependencies(build_tests DBusProxyTest)
##### add_dependencies(build_tests DBusFreedesktopPropertiesTest)
add_dependencies(build_tests DBusServicePublisherTest)
add_dependencies(build_tests DBusVariantTest)
add_dependencies(build_tests DBusRuntimeTest)              
##### add_dependencies(build_tests DBusDynamicLoadingBasicTest)
##### add_dependencies(build_tests DBusDynamicLoadingMultipleDefinitionTest)
##### add_dependencies(build_tests DBusDynamicLoadingMultipleBindingsTest)
##### add_dependencies(build_tests DBusDynamicLoadingMixedValidityTest)
##### add_dependencies(build_tests DBusDynamicLoadingNoValidityTest)
add_dependencies(build_tests DBusClientIdTest)
add_dependencies(build_tests DBusBroadcastTest)
add_dependencies(build_tests DBusPolymorphicTest)
add_dependencies(build_tests DBusLoadTest)

##############################################################################
# Add tests
##############################################################################

##### add_test(NAME DBusManagedTest COMMAND DBusManagedTest)
##### add_test(NAME DBusObjectManagerStubTest COMMAND DBusObjectManagerStubTest)
add_test(NAME DBusMainLoopIntegrationTest COMMAND DBusMainLoopIntegrationTest)
##### add_test(NAME DBusServiceRegistryTest COMMAND DBusServiceRegistryTest)
add_test(NAME DBusAddressTranslatorTest WORKING_DIRECTORY ${libcommonapi-dbus_SOURCE_DIR} COMMAND DBusAddressTranslatorTest)
add_test(NAME DBusConnectionTest COMMAND DBusConnectionTest)
##### add_test(NAME DBusTypeStreamTest COMMAND DBusTypeStreamTest)
add_test(NAME DBusVariantOutputStreamTest COMMAND DBusVariantOutputStreamTest)
add_test(NAME DBusCommunicationTest COMMAND DBusCommunicationTest)
add_test(NAME DBusDaemonProxyTest COMMAND DBusDaemonProxyTest)
add_test(NAME DBusInputStreamTest COMMAND DBusInputStreamTest)
add_test(NAME DBusOutputStreamTest COMMAND DBusOutputStreamTest)
add_test(NAME DBusFactoryTest COMMAND DBusFactoryTest)
add_test(NAME DBusMultipleConnectionTest COMMAND DBusMultipleConnectionTest)
##### add_test(NAME DBusProxyTest COMMAND DBusProxyTest)
##### add_test(NAME DBusFreedesktopPropertiesTest COMMAND DBusFreedesktopPropertiesTest)
add_test(NAME DBusServicePublisherTest COMMAND DBusServicePublisherTest)
add_test(NAME DBusVariantTest COMMAND DBusVariantTest)
add_test(NAME DBusRuntimeTest COMMAND DBusRuntimeTest)
##### add_test(NAME DBusDynamicLoadingBasicTest COMMAND DBusDynamicLoadingBasicTest)
##### add_test(NAME DBusDynamicLoadingMultipleDefinitionTest COMMAND DBusDynamicLoadingMultipleDefinitionTest)
##### add_test(NAME DBusDynamicLoadingMultipleBindingsTest COMMAND DBusDynamicLoadingMultipleBindingsTest)
##### add_test(NAME DBusDynamicLoadingMixedValidityTest COMMAND DBusDynamicLoadingMixedValidityTest)
##### add_test(NAME DBusDynamicLoadingNoValidityTest COMMAND DBusDynamicLoadingNoValidityTest)
add_test(NAME DBusClientIdTest COMMAND DBusClientIdTest)
add_test(NAME DBusBroadcastTest COMMAND DBusBroadcastTest)
add_test(NAME DBusPolymorphicTest COMMAND DBusPolymorphicTest)
add_test(NAME DBusLoadTest COMMAND DBusLoadTest)

#DBusManagedTest_SOURCES = \
#        src/test/DBusManagedTest.cpp \
#        ${ManagedDBusSources}
#DBusManagedTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS} ${GLIB_CFLAGS}
#DBusManagedTest_CXXFLAGS = ${GTEST_CXXFLAGS} ${GLIB_CFLAGS}
#DBusManagedTest_LDADD = ${LDADD_FOR_GTEST} ${GLIB_LIBS}

#AM_CPPFLAGS = \
#        -include $(top_builddir)/build-aux/config.h \
#        -I$(top_srcdir)/src \
#        -I$(top_srcdir)/src/test \
#        ${COMMONAPI_CFLAGS} \
#        ${DBUS_CFLAGS} \
#        -DCOMMONAPI_INTERNAL_COMPILATION \
#        -DCOMMONAPI_DBUS_VERSION_MAJOR=${LIBCOMMONAPI_DBUS_CURRENT} \
#        -DCOMMONAPI_DBUS_VERSION_MINOR=${LIBCOMMONAPI_DBUS_REVISION}

#COMMONAPI_CFLAGS = -I/usr/local/include/CommonAPI-2.1  
#COMMONAPI_LIBS = -L/usr/local/lib -lCommonAPI  
#DBUS_CFLAGS = -I/usr/local/include/dbus-1.0 -I/usr/local/lib/dbus-1.0/include  
#DBUS_LIBS = -L/usr/local/lib -ldbus-1 -lpthread -lrt  

#GTEST_CPPFLAGS:    -I/home/genius/googletest/gtest-1.7.0/include -I/home/genius/googletest/gtest-1.7.0/include

#GLIB_CFLAGS='-I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include  '



#GTEST_CXXFLAGS:    -pthread

#LDADD_FOR_GTEST_WITHOUT_LIBCOMMON_API_DBUS = ${GTEST_LIBS} ${LDADD}
#LDADD_FOR_GTEST = libCommonAPI-DBus.la ${LDADD_FOR_GTEST_WITHOUT_LIBCOMMON_API_DBUS}
#GTEST_LIBS:        /home/genius/googletest/gtest-1.7.0/lib/libgtest.la -pthread


#GLIB_LIBS='-lglib-2.0  '

