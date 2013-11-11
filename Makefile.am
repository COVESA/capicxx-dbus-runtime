ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = LICENSE LICENSE_dbus_patch LICENSE_MurmurHash
MOSTLYCLEANFILES =

# Library interface (not package version!)
LIBCOMMONAPI_DBUS_CURRENT=3
LIBCOMMONAPI_DBUS_REVISION=0
LIBCOMMONAPI_DBUS_AGE=0

AM_CPPFLAGS = \
        -include $(top_builddir)/build-aux/config.h \
        -I$(top_srcdir)/src \
        -I$(top_srcdir)/src/test \
        ${COMMONAPI_CFLAGS} \
        ${DBUS_CFLAGS} \
        -DCOMMONAPI_INTERNAL_COMPILATION \
        -DCOMMONAPI_DBUS_VERSION_MAJOR=${LIBCOMMONAPI_DBUS_CURRENT} \
        -DCOMMONAPI_DBUS_VERSION_MINOR=${LIBCOMMONAPI_DBUS_REVISION}


AM_LDFLAGS = \
        ${COMMONAPI_LIBS} \
        ${DBUS_LIBS}

noinst_LTLIBRARIES =
lib_LTLIBRARIES =

# ------------------------------------------------------------------------------
noinst_LTLIBRARIES += libmurmurhash-internal.la

libmurmurhash_internal_la_SOURCES = \
        src/murmurhash/MurmurHash3.h \
        src/murmurhash/MurmurHash3.cpp

# ------------------------------------------------------------------------------
noinst_LTLIBRARIES += libpugixml-internal.la

libpugixml_internal_la_SOURCES = \
        src/pugixml/pugiconfig.hpp \
        src/pugixml/pugixml.cpp \
        src/pugixml/pugixml.hpp
# ------------------------------------------------------------------------------
lib_LTLIBRARIES += libCommonAPI-DBus.la

libCommonAPI_DBus_la_SOURCES = \
        src/CommonAPI/DBus/DBusAddressTranslator.cpp \
        src/CommonAPI/DBus/DBusClientId.cpp \
        src/CommonAPI/DBus/DBusConfiguration.cpp \
        src/CommonAPI/DBus/DBusConnection.cpp \
        src/CommonAPI/DBus/DBusDaemonProxy.cpp \
        src/CommonAPI/DBus/DBusError.cpp \
        src/CommonAPI/DBus/DBusFunctionalHash.cpp \
        src/CommonAPI/DBus/DBusInputStream.cpp \
        src/CommonAPI/DBus/DBusMainLoopContext.cpp \
        src/CommonAPI/DBus/DBusMessage.cpp \
        src/CommonAPI/DBus/DBusObjectManager.cpp \
        src/CommonAPI/DBus/DBusObjectManagerStub.cpp \
        src/CommonAPI/DBus/DBusOutputStream.cpp \
        src/CommonAPI/DBus/DBusProxyBase.cpp \
        src/CommonAPI/DBus/DBusProxy.cpp \
        src/CommonAPI/DBus/DBusProxyManager.cpp \
        src/CommonAPI/DBus/DBusFactory.cpp \
        src/CommonAPI/DBus/DBusRuntime.cpp \
        src/CommonAPI/DBus/DBusServicePublisher.cpp \
        src/CommonAPI/DBus/DBusServiceRegistry.cpp \
        src/CommonAPI/DBus/DBusStubAdapter.cpp

libCommonAPI_DBus_la_LIBADD = \
        libmurmurhash-internal.la \
        libpugixml-internal.la \
        ${COMMONAPI_LIBS}
libCommonAPI_DBus_la_LDFLAGS = \
        ${AM_LDFLAGS} \
        -version-info ${LIBCOMMONAPI_DBUS_CURRENT}:${LIBCOMMONAPI_DBUS_REVISION}:${LIBCOMMONAPI_DBUS_AGE}

CommonAPI_DBus_pugi_includedir=$(includedir)/CommonAPI-${COMMONAPI_API_HEADER_VERSION}/pugixml
CommonAPI_DBus_pugi_include_HEADERS = \
        src/pugixml/pugixml.hpp \
        src/pugixml/pugiconfig.hpp

CommonAPI_DBus_includedir=$(includedir)/CommonAPI-${COMMONAPI_API_HEADER_VERSION}/CommonAPI/DBus
CommonAPI_DBus_include_HEADERS = \
        src/CommonAPI/DBus/DBusAddressTranslator.h \
        src/CommonAPI/DBus/DBusAttribute.h \
        src/CommonAPI/DBus/DBusClientId.h \
        src/CommonAPI/DBus/DBusConfiguration.h \
        src/CommonAPI/DBus/DBusConnection.h \
        src/CommonAPI/DBus/DBusConnectionBusType.h \
        src/CommonAPI/DBus/DBusDaemonProxy.h \
        src/CommonAPI/DBus/DBusError.h \
        src/CommonAPI/DBus/DBusEvent.h \
        src/CommonAPI/DBus/DBusFunctionalHash.h \
        src/CommonAPI/DBus/DBusHelper.h \
        src/CommonAPI/DBus/DBusInputStream.h \
        src/CommonAPI/DBus/DBusInstanceAvailabilityStatusChangedEvent.h \
        src/CommonAPI/DBus/DBusInterfaceHandler.h \
        src/CommonAPI/DBus/DBusMainLoopContext.h \
        src/CommonAPI/DBus/DBusMessage.h \
        src/CommonAPI/DBus/DBusMultiEvent.h \
        src/CommonAPI/DBus/DBusObjectManager.h \
        src/CommonAPI/DBus/DBusObjectManagerStub.h \
        src/CommonAPI/DBus/DBusOutputStream.h \
        src/CommonAPI/DBus/DBusProxyBase.h \
        src/CommonAPI/DBus/DBusProxy.h \
        src/CommonAPI/DBus/DBusProxyAsyncCallbackHandler.h \
        src/CommonAPI/DBus/DBusProxyConnection.h \
        src/CommonAPI/DBus/DBusProxyManager.h \
        src/CommonAPI/DBus/DBusFactory.h \
        src/CommonAPI/DBus/DBusProxyHelper.h \
        src/CommonAPI/DBus/DBusRuntime.h \
        src/CommonAPI/DBus/DBusSelectiveEvent.h \
        src/CommonAPI/DBus/DBusSerializableArguments.h \
        src/CommonAPI/DBus/DBusServicePublisher.h \
        src/CommonAPI/DBus/DBusServiceRegistry.h \
        src/CommonAPI/DBus/DBusStubAdapter.h \
        src/CommonAPI/DBus/DBusStubAdapterHelper.h \
        src/CommonAPI/DBus/DBusUtils.h \
        src/CommonAPI/DBus/DBusLegacyVariant.h

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = CommonAPI-DBus.pc

# ------------------------------------------------------------------------------
if ENABLE_TESTS

lib_LTLIBRARIES += libCommonAPI-Fake.la

libCommonAPI_Fake_la_SOURCES = \
        src/test/dbusDynamicLoadingTests/fakeTestBinding/FakeBinding.cpp

libCommonAPI_Fake_la_LIBADD = \
        ${COMMONAPI_LIBS}
libCommonAPI_Fake_la_LDFLAGS = \
        ${AM_LDFLAGS} \
        -version-info 1:0:0

CommonAPI_Fake_includedir=$(includedir)/CommonAPI-${COMMONAPI_API_HEADER_VERSION}/CommonAPI/Fake
CommonAPI_Fake_include_HEADERS = \
        src/test/dbusDynamicLoadingTests/fakeTestBinding/FakeBinding.h

TestInterfaceSources = \
        src/test/commonapi/tests/DerivedTypeCollection.cpp \
        src/test/commonapi/tests/TestInterfaceStubDefault.cpp

TestInterfaceDBusSources = \
        ${TestInterfaceSources} \
        src/test/commonapi/tests/TestInterfaceDBusProxy.cpp \
        src/test/commonapi/tests/TestInterfaceDBusStubAdapter.cpp \
        src/test/fake/legacy/service/LegacyInterfaceDBusProxy.cpp

ManagedSources = \
        src/test/commonapi/tests/managed/LeafInterfaceStubDefault.cpp \
        src/test/commonapi/tests/managed/RootInterfaceStubDefault.cpp \
        src/test/commonapi/tests/managed/SecondRootStubDefault.cpp

ManagedDBusSources = \
        ${ManagedSources} \
        src/test/commonapi/tests/managed/RootInterfaceDBusProxy.cpp \
        src/test/commonapi/tests/managed/RootInterfaceDBusStubAdapter.cpp \
        src/test/commonapi/tests/managed/LeafInterfaceDBusProxy.cpp \
        src/test/commonapi/tests/managed/LeafInterfaceDBusStubAdapter.cpp \
        src/test/commonapi/tests/managed/SecondRootDBusProxy.cpp \
        src/test/commonapi/tests/managed/SecondRootDBusStubAdapter.cpp


lib_LTLIBRARIES += libDBusGen-TestInterface.la
libDBusGen_TestInterface_la_SOURCES = \
        ${TestInterfaceDBusSources}
libDBusGen_TestInterface_la_LIBADD = \
        ${COMMONAPI_LIBS}
libDBusGen_TestInterface_la_LDFLAGS = \
        ${AM_LDFLAGS}

lib_LTLIBRARIES += libSomeOtherNameForGeneratedDBus.la
libSomeOtherNameForGeneratedDBus_la_SOURCES = \
        ${TestInterfaceDBusSources}
libSomeOtherNameForGeneratedDBus_la_LIBADD = \
        ${COMMONAPI_LIBS}
libSomeOtherNameForGeneratedDBus_la_LDFLAGS = \
        ${AM_LDFLAGS}


DYNAMIC_LOADING_TEST_SET = \
        DBusDynamicLoadingBasicTest \
        DBusDynamicLoadingMixedValidityTest \
        DBusDynamicLoadingNoValidityTest \
        DBusDynamicLoadingMultipleDefinitionTest \
        DBusDynamicLoadingMultipleBindingsTest

check_PROGRAMS = \
        DBusManagedTest \
        DBusObjectManagerStubTest \
        DBusRuntimeTest \
        ${DYNAMIC_LOADING_TEST_SET} \
        DBusMainLoopIntegrationTest \
        DBusConnectionTest \
        DBusServiceRegistryTest \
        DBusProxyTest \
        DBusAddressTranslatorTest \
        DBusInputStreamTest \
        DBusOutputStreamTest \
        DBusFactoryTest \
        DBusVariantTest \
        DBusTypeStreamTest \
        DBusVariantOutputStreamTest \
        DBusDaemonProxyTest \
        DBusCommunicationTest \
        DBusMultipleConnectionTest \
        DBusServicePublisherTest \
        DBusClientIdTest \
        DBusSelectiveBroadcastTest \
        DBusPolymorphicTest

noinst_PROGRAMS = ${check_PROGRAMS}
TESTS = ${check_PROGRAMS}

LDADD_FOR_GTEST_WITHOUT_LIBCOMMON_API_DBUS = ${GTEST_LIBS} ${LDADD}
LDADD_FOR_GTEST = libCommonAPI-DBus.la ${LDADD_FOR_GTEST_WITHOUT_LIBCOMMON_API_DBUS}

DBusManagedTest_SOURCES = \
        src/test/DBusManagedTest.cpp \
        ${ManagedDBusSources}
DBusManagedTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS} ${GLIB_CFLAGS}
DBusManagedTest_CXXFLAGS = ${GTEST_CXXFLAGS} ${GLIB_CFLAGS}
DBusManagedTest_LDADD = ${LDADD_FOR_GTEST} ${GLIB_LIBS}

DBusObjectManagerStubTest_SOURCES = src/test/DBusObjectManagerStubTest.cpp
DBusObjectManagerStubTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusObjectManagerStubTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusObjectManagerStubTest_LDADD = ${LDADD_FOR_GTEST}

DBusMainLoopIntegrationTest_SOURCES = \
        src/test/DBusMainLoopIntegrationTest.cpp \
        ${TestInterfaceDBusSources}
DBusMainLoopIntegrationTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS} ${GLIB_CFLAGS}
DBusMainLoopIntegrationTest_CXXFLAGS = ${GTEST_CXXFLAGS} ${GLIB_CFLAGS}
DBusMainLoopIntegrationTest_LDADD = ${LDADD_FOR_GTEST} ${GLIB_LIBS}

DBusServiceRegistryTest_SOURCES = \
        src/test/DBusServiceRegistryTest.cpp \
        ${TestInterfaceDBusSources}
DBusServiceRegistryTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusServiceRegistryTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusServiceRegistryTest_LDADD = ${LDADD_FOR_GTEST}

DBusAddressTranslatorTest_SOURCES = \
        src/test/DBusAddressTranslatorTest.cpp \
        ${TestInterfaceDBusSources}
DBusAddressTranslatorTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusAddressTranslatorTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusAddressTranslatorTest_LDADD = ${LDADD_FOR_GTEST}

DBusConnectionTest_SOURCES = src/test/DBusConnectionTest.cpp
DBusConnectionTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusConnectionTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusConnectionTest_LDADD = ${LDADD_FOR_GTEST}

DBusTypeStreamTest_SOURCES = src/test/DBusTypeStreamTest.cpp
DBusTypeStreamTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusTypeStreamTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusTypeStreamTest_LDADD = ${LDADD_FOR_GTEST}

DBusVariantOutputStreamTest_SOURCES = src/test/DBusVariantOutputStreamTest.cpp
DBusVariantOutputStreamTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusVariantOutputStreamTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusVariantOutputStreamTest_LDADD = ${LDADD_FOR_GTEST}

DBusCommunicationTest_SOURCES = \
        src/test/DBusCommunicationTest.cpp \
        ${TestInterfaceDBusSources}
DBusCommunicationTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusCommunicationTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusCommunicationTest_LDADD = ${LDADD_FOR_GTEST}

DBusDaemonProxyTest_SOURCES = src/test/DBusDaemonProxyTest.cpp
DBusDaemonProxyTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusDaemonProxyTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusDaemonProxyTest_LDADD = ${LDADD_FOR_GTEST}

DBusInputStreamTest_SOURCES = \
        src/test/DBusInputStreamTest.cpp \
        ${TestInterfaceDBusSources}
DBusInputStreamTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusInputStreamTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusInputStreamTest_LDADD = ${LDADD_FOR_GTEST}

DBusOutputStreamTest_SOURCES = \
        src/test/DBusOutputStreamTest.cpp \
        ${TestInterfaceDBusSources}
DBusOutputStreamTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusOutputStreamTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusOutputStreamTest_LDADD = ${LDADD_FOR_GTEST}

DBusFactoryTest_SOURCES = \
        src/test/DBusFactoryTest.cpp \
        ${TestInterfaceDBusSources}
DBusFactoryTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusFactoryTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusFactoryTest_LDADD = ${LDADD_FOR_GTEST}

DBusMultipleConnectionTest_SOURCES = \
        src/test/DBusMultipleConnectionTest.cpp \
        ${TestInterfaceDBusSources}
DBusMultipleConnectionTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusMultipleConnectionTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusMultipleConnectionTest_LDADD = ${LDADD_FOR_GTEST}

DBusProxyTest_SOURCES = \
        src/test/DBusProxyTest.cpp \
        ${TestInterfaceDBusSources}
DBusProxyTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusProxyTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusProxyTest_LDADD = ${LDADD_FOR_GTEST}

DBusServicePublisherTest_SOURCES = \
        src/test/DBusServicePublisherTest.cpp \
        ${TestInterfaceDBusSources}
DBusServicePublisherTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusServicePublisherTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusServicePublisherTest_LDADD = ${LDADD_FOR_GTEST}

DBusVariantTest_SOURCES = src/test/DBusVariantTest.cpp
DBusVariantTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusVariantTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusVariantTest_LDADD = ${LDADD_FOR_GTEST}

DBusRuntimeTest_SOURCES = src/test/DBusRuntimeTest.cpp
DBusRuntimeTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusRuntimeTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusRuntimeTest_LDADD = ${LDADD_FOR_GTEST}

DBusDynamicLoadingBasicTest_SOURCES = \
        ${TestInterfaceSources} \
        src/test/dbusDynamicLoadingTests/DBusDynamicLoadingBasicTest.cpp
DBusDynamicLoadingBasicTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusDynamicLoadingBasicTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusDynamicLoadingBasicTest_LDADD = ${LDADD_FOR_GTEST_WITHOUT_LIBCOMMON_API_DBUS}

DBusDynamicLoadingMultipleDefinitionTest_SOURCES =  \
        ${TestInterfaceSources} \
        src/test/dbusDynamicLoadingTests/DBusDynamicLoadingMultipleDefinitionTest.cpp
DBusDynamicLoadingMultipleDefinitionTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusDynamicLoadingMultipleDefinitionTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusDynamicLoadingMultipleDefinitionTest_LDADD = ${LDADD_FOR_GTEST_WITHOUT_LIBCOMMON_API_DBUS}

DBusDynamicLoadingMultipleBindingsTest_SOURCES =  \
        ${TestInterfaceSources} \
        src/test/dbusDynamicLoadingTests/DBusDynamicLoadingMultipleBindingsTest.cpp
DBusDynamicLoadingMultipleBindingsTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusDynamicLoadingMultipleBindingsTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusDynamicLoadingMultipleBindingsTest_LDADD = ${LDADD_FOR_GTEST_WITHOUT_LIBCOMMON_API_DBUS}

DBusDynamicLoadingMixedValidityTest_SOURCES =  \
        ${TestInterfaceSources} \
        src/test/dbusDynamicLoadingTests/DBusDynamicLoadingMixedValidityTest.cpp
DBusDynamicLoadingMixedValidityTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusDynamicLoadingMixedValidityTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusDynamicLoadingMixedValidityTest_LDADD = ${LDADD_FOR_GTEST_WITHOUT_LIBCOMMON_API_DBUS}

DBusDynamicLoadingNoValidityTest_SOURCES =  \
        ${TestInterfaceSources} \
        src/test/dbusDynamicLoadingTests/DBusDynamicLoadingNoValidityTest.cpp
DBusDynamicLoadingNoValidityTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusDynamicLoadingNoValidityTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusDynamicLoadingNoValidityTest_LDADD = ${LDADD_FOR_GTEST_WITHOUT_LIBCOMMON_API_DBUS}

DBusClientIdTest_SOURCES =  \
        src/test/DBusClientIdTest.cpp
DBusClientIdTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusClientIdTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusClientIdTest_LDADD = ${LDADD_FOR_GTEST}

DBusSelectiveBroadcastTest_SOURCES =  \
        ${TestInterfaceDBusSources} \
        src/test/DBusSelectiveBroadcastTest.cpp
DBusSelectiveBroadcastTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusSelectiveBroadcastTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusSelectiveBroadcastTest_LDADD = ${LDADD_FOR_GTEST}

DBusPolymorphicTest_SOURCES =  \
        ${TestInterfaceDBusSources} \
        src/test/DBusPolymorphicTest.cpp
DBusPolymorphicTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusPolymorphicTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusPolymorphicTest_LDADD = ${LDADD_FOR_GTEST}

endif


# ------------------------------------------------------------------------------
# Doxygen
include $(top_srcdir)/aminclude.am 
EXTRA_DIST += doxygen.cfg dbus-DBusMessage-add-support-for-custom-marshaling.patch
MOSTLYCLEANFILES += ${DX_CLEANFILES}

# ------------------------------------------------------------------------------
MAINTAINERCLEANFILES = \
        Makefile.in \
        aclocal.m4 \
        configure

clean-local:
	-rm -rf src-gen

maintainer-clean-local:
	-rm -rf build-aux
	-rm -f config.h.in*
	-rm -f m4/libtool*.m4
	-rm -f m4/lt*.m4
	
doc: doxygen-doc
	 asciidoc -b html -o doc/html/README.html README
