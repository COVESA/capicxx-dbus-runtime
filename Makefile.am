ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST =
MOSTLYCLEANFILES =

AM_CPPFLAGS = \
		${COMMONAPI_CFLAGS} \
		${DBUS_CFLAGS} \
		-I$(top_srcdir)/src

LDADD = \
		${COMMONAPI_LIBS} \
		${DBUS_LIBS}

lib_LTLIBRARIES =
lib_LIBRARIES = 

# ------------------------------------------------------------------------------
lib_LTLIBRARIES += libmurmurhash-internal.la

libmurmurhash_internal_la_SOURCES = \
		src/murmurhash/MurmurHash3.h \
		src/murmurhash/MurmurHash3.cpp

# ------------------------------------------------------------------------------
lib_LTLIBRARIES += libCommonAPI-DBus.la

libCommonAPI_DBus_la_SOURCES = \
		src/CommonAPI/DBus/DBusConnection.cpp \
		src/CommonAPI/DBus/DBusDaemonProxy.cpp \
		src/CommonAPI/DBus/DBusError.cpp \
		src/CommonAPI/DBus/DBusFunctionalHash.cpp \
		src/CommonAPI/DBus/DBusInputStream.cpp \
		src/CommonAPI/DBus/DBusMessage.cpp \
		src/CommonAPI/DBus/DBusObjectManager.cpp \
		src/CommonAPI/DBus/DBusOutputStream.cpp \
		src/CommonAPI/DBus/DBusProxy.cpp \
		src/CommonAPI/DBus/DBusFactory.cpp \
		src/CommonAPI/DBus/DBusRuntime.cpp \
		src/CommonAPI/DBus/DBusServiceRegistry.cpp \
		src/CommonAPI/DBus/DBusServiceStatusEvent.cpp \
		src/CommonAPI/DBus/DBusStubAdapter.cpp

libCommonAPI_DBus_la_LIBADD = \
		libmurmurhash-internal.la \
		${COMMONAPI_LIBS}

CommonAPI_DBus_includedir=$(includedir)/CommonAPI-${VERSION}/CommonAPI/DBus
CommonAPI_DBus_include_HEADERS = \
		src/CommonAPI/DBus/DBusAttribute.h \
        src/CommonAPI/DBus/DBusConnection.h \
        src/CommonAPI/DBus/DBusDaemonProxy.h \
        src/CommonAPI/DBus/DBusError.h \
        src/CommonAPI/DBus/DBusEvent.h \
        src/CommonAPI/DBus/DBusFunctionalHash.h \
        src/CommonAPI/DBus/DBusHelper.h \
        src/CommonAPI/DBus/DBusInputStream.h \
        src/CommonAPI/DBus/DBusMessage.h \
        src/CommonAPI/DBus/DBusMultiEvent.h \
        src/CommonAPI/DBus/DBusObjectManager.h \
        src/CommonAPI/DBus/DBusOutputStream.h \
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
		src/test/commonapi/tests/TestInterfaceStubDefault.cpp

check_PROGRAMS = \
		DBusVariantTest \
		DBusTypeStreamTest \
		DBusVariantOutputStreamTest


TESTS =	${check_PROGRAMS}

DBusVariantTest_SOURCES = src/test/DBusVariantTest.cpp
DBusVariantTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusVariantTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusVariantTest_LDADD = ${LDADD} ${GTEST_LIBS} libCommonAPI-DBus.la

DBusTypeStreamTest_SOURCES = src/test/DBusTypeStreamTest.cpp
DBusTypeStreamTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusTypeStreamTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusTypeStreamTest_LDADD = ${LDADD} ${GTEST_LIBS} libCommonAPI-DBus.la

DBusVariantOutputStreamTest_SOURCES = src/test/DBusVariantOutputStreamTest.cpp
DBusVariantOutputStreamTest_CPPFLAGS = ${AM_CPPFLAGS} ${GTEST_CPPFLAGS}
DBusVariantOutputStreamTest_CXXFLAGS = ${GTEST_CXXFLAGS}
DBusVariantOutputStreamTest_LDADD = ${LDADD} ${GTEST_LIBS} libCommonAPI-DBus.la

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
