ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = LICENSE LICENSE_dbus_patch LICENSE_MurmurHash
MOSTLYCLEANFILES =

AM_CPPFLAGS = \
        -include $(top_builddir)/build-aux/config.h \
        -I$(top_srcdir)/src \
        -I$(top_srcdir)/src/test \
        ${COMMONAPI_CFLAGS} \
        ${DBUS_CFLAGS}


AM_LDFLAGS = \
        ${COMMONAPI_LIBS} \
        ${DBUS_LIBS}

noinst_LTLIBRARIES =
lib_LTLIBRARIES =

# Library interface (not package version!)
LIBCOMMONAPI_DBUS_CURRENT=2
LIBCOMMONAPI_DBUS_REVISION=0
LIBCOMMONAPI_DBUS_AGE=0

# ------------------------------------------------------------------------------
noinst_LTLIBRARIES += libmurmurhash-internal.la

libmurmurhash_internal_la_SOURCES = \
        src/murmurhash/MurmurHash3.h \
        src/murmurhash/MurmurHash3.cpp

# ------------------------------------------------------------------------------
lib_LTLIBRARIES += libCommonAPI-DBus.la

libCommonAPI_DBus_la_SOURCES = \
        src/CommonAPI/DBus/DBusAddressTranslator.cpp \
        src/CommonAPI/DBus/DBusConnection.cpp \
        src/CommonAPI/DBus/DBusDaemonProxy.cpp \
        src/CommonAPI/DBus/DBusError.cpp \
        src/CommonAPI/DBus/DBusFunctionalHash.cpp \
        src/CommonAPI/DBus/DBusInputStream.cpp \
        src/CommonAPI/DBus/DBusMainLoopContext.cpp \
        src/CommonAPI/DBus/DBusMessage.cpp \
        src/CommonAPI/DBus/DBusObjectManager.cpp \
        src/CommonAPI/DBus/DBusOutputStream.cpp \
        src/CommonAPI/DBus/DBusProxyBase.cpp \
        src/CommonAPI/DBus/DBusProxy.cpp \
        src/CommonAPI/DBus/DBusFactory.cpp \
        src/CommonAPI/DBus/DBusRuntime.cpp \
        src/CommonAPI/DBus/DBusServiceRegistry.cpp \
        src/CommonAPI/DBus/DBusServiceStatusEvent.cpp \
        src/CommonAPI/DBus/DBusStubAdapter.cpp

libCommonAPI_DBus_la_LIBADD = \
        libmurmurhash-internal.la \
        ${COMMONAPI_LIBS}
libCommonAPI_DBus_la_LDFLAGS = \
        ${AM_LDFLAGS} \
        -version-info ${LIBCOMMONAPI_DBUS_CURRENT}:${LIBCOMMONAPI_DBUS_REVISION}:${LIBCOMMONAPI_DBUS_AGE}

CommonAPI_DBus_includedir=$(includedir)/CommonAPI-${VERSION}/CommonAPI/DBus
CommonAPI_DBus_include_HEADERS = \
        src/CommonAPI/DBus/DBusAddressTranslator.h \
        src/CommonAPI/DBus/DBusAttribute.h \
        src/CommonAPI/DBus/DBusConnection.h \
        src/CommonAPI/DBus/DBusDaemonProxy.h \
        src/CommonAPI/DBus/DBusError.h \
        src/CommonAPI/DBus/DBusEvent.h \
        src/CommonAPI/DBus/DBusFunctionalHash.h \
        src/CommonAPI/DBus/DBusHelper.h \
        src/CommonAPI/DBus/DBusInputStream.h \
        src/CommonAPI/DBus/DBusMainLoopContext.h \
        src/CommonAPI/DBus/DBusMessage.h \
        src/CommonAPI/DBus/DBusMultiEvent.h \
        src/CommonAPI/DBus/DBusObjectManager.h \
        src/CommonAPI/DBus/DBusOutputStream.h \
        src/CommonAPI/DBus/DBusProxyBase.h \
        src/CommonAPI/DBus/DBusProxy.h \
        src/CommonAPI/DBus/DBusProxyAsyncCallbackHandler.h \
        src/CommonAPI/DBus/DBusProxyConnection.h \
        src/CommonAPI/DBus/DBusFactory.h \
        src/CommonAPI/DBus/DBusProxyHelper.h \
        src/CommonAPI/DBus/DBusRuntime.h \
        src/CommonAPI/DBus/DBusSerializableArguments.h \
        src/CommonAPI/DBus/DBusServiceRegistry.h \
        src/CommonAPI/DBus/DBusServiceStatusEvent.h \
        src/CommonAPI/DBus/DBusStubAdapter.h \
        src/CommonAPI/DBus/DBusStubAdapterHelper.h \
        src/CommonAPI/DBus/DBusUtils.h

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = CommonAPI-DBus.pc

# ------------------------------------------------------------------------------
if ENABLE_TESTS

TestInterfaceSources = \
        src/test/commonapi/tests/DerivedTypeCollection.cpp \
        src/test/commonapi/tests/TestInterfaceDBusProxy.cpp \
        src/test/commonapi/tests/TestInterfaceDBusStubAdapter.cpp \
        src/test/commonapi/tests/TestInterfaceStubDefault.cpp \
        src/test/fakeLegacyService/fake/legacy/service/LegacyInterfaceDBusProxy.cpp

check_PROGRAMS = \
        DBusMainLoopIntegrationTest \
        DBusConnectionTest \
        DBusServiceRegistryTest \
        DBusProxyTest \
        DBusAddressTranslatorTest \
        DBusInputStreamTest \
        DBusOutputStreamTest \
        DBusRuntimeTest \
        DBusFactoryTest \
        DBusVariantTest \
        DBusTypeStreamTest \
        DBusVariantOutputStreamTest \
        DBusDaemonProxyTest \
        DBusCommunicationTest \
        DBusMultipleConnectionTest \
        DBusBenchmarkingTest


TESTS = ${check_PROGRAMS}
LDADD_FOR_GTEST = libCommonAPI-DBus.la ${GTEST_LIBS} ${LDADD}

DBusMainLoopIntegrationTest_SOURCES = \
        src/test/DBusMainLoopIntegrationTest.cpp \
        ${TestInterfaceSources}
DBusMainLoopIntegrationTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS} ${GLIB_CFLAGS}
DBusMainLoopIntegrationTest_CXXFLAGS = ${GTEST_CXXFLAGS} ${GLIB_CFLAGS}
DBusMainLoopIntegrationTest_LDADD = ${LDADD_FOR_GTEST} ${GLIB_LIBS}

DBusServiceRegistryTest_SOURCES = \
        src/test/DBusServiceRegistryTest.cpp \
        ${TestInterfaceSources}
DBusServiceRegistryTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusServiceRegistryTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusServiceRegistryTest_LDADD = ${LDADD_FOR_GTEST}

DBusAddressTranslatorTest_SOURCES = \
        src/test/DBusAddressTranslatorTest.cpp \
        ${TestInterfaceSources}
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

DBusBenchmarkingTest_SOURCES = src/test/DBusBenchmarkingTest.cpp
DBusBenchmarkingTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusBenchmarkingTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusBenchmarkingTest_LDADD = ${LDADD_FOR_GTEST}

DBusCommunicationTest_SOURCES = \
        src/test/DBusCommunicationTest.cpp \
        ${TestInterfaceSources}
DBusCommunicationTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusCommunicationTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusCommunicationTest_LDADD = ${LDADD_FOR_GTEST}

DBusDaemonProxyTest_SOURCES = src/test/DBusDaemonProxyTest.cpp
DBusDaemonProxyTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusDaemonProxyTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusDaemonProxyTest_LDADD = ${LDADD_FOR_GTEST}

DBusInputStreamTest_SOURCES = \
        src/test/DBusInputStreamTest.cpp \
        ${TestInterfaceSources}
DBusInputStreamTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusInputStreamTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusInputStreamTest_LDADD = ${LDADD_FOR_GTEST}

DBusOutputStreamTest_SOURCES = \
        src/test/DBusOutputStreamTest.cpp \
        ${TestInterfaceSources}
DBusOutputStreamTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusOutputStreamTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusOutputStreamTest_LDADD = ${LDADD_FOR_GTEST}

DBusFactoryTest_SOURCES = \
        src/test/DBusFactoryTest.cpp \
        ${TestInterfaceSources}
DBusFactoryTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusFactoryTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusFactoryTest_LDADD = ${LDADD_FOR_GTEST}

DBusRuntimeTest_SOURCES = src/test/DBusRuntimeTest.cpp
DBusRuntimeTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusRuntimeTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusRuntimeTest_LDADD = ${LDADD_FOR_GTEST}

DBusMultipleConnectionTest_SOURCES = \
        src/test/DBusMultipleConnectionTest.cpp \
        ${TestInterfaceSources}
DBusMultipleConnectionTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusMultipleConnectionTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusMultipleConnectionTest_LDADD = ${LDADD_FOR_GTEST}

DBusProxyTest_SOURCES = \
        src/test/DBusProxyTest.cpp \
        ${TestInterfaceSources}
DBusProxyTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusProxyTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusProxyTest_LDADD = ${LDADD_FOR_GTEST}

DBusVariantTest_SOURCES = src/test/DBusVariantTest.cpp
DBusVariantTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusVariantTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusVariantTest_LDADD = ${LDADD_FOR_GTEST}

endif


# ------------------------------------------------------------------------------
# Doxygen
include $(top_srcdir)/aminclude.am 
EXTRA_DIST += doxygen.cfg
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
