/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
 
#include <iostream>

#include <common-api-dbus/dbus-message.h>

#include "TestDBusSerializableStruct.h"


#define COMMON_API_TEST_TEST_STRUCT_SIGNATURE					"(ud)"
#define COMMON_API_TEST_TEST_STRUCT_ARRAY_SIGNATURE				"a" COMMON_API_TEST_TEST_STRUCT_SIGNATURE

#define COMMON_API_TEST_TEST_STRUCT_EXTENDED_SIGNATURE			"(uds)"
#define COMMON_API_TEST_TEST_STRUCT_EXTENDED_ARRAY_SIGNATURE	"a" COMMON_API_TEST_TEST_STRUCT_EXTENDED_SIGNATURE


namespace common {
namespace api {
namespace test {


TestStruct::TestStruct(const uint32_t& fromIntValue, const double& fromDoubleValue):
		intValue(fromIntValue),
		doubleValue(fromDoubleValue) {
}

common::api::dbus::DBusInputMessageStream& TestStruct::readFromDBusInputMessageStream(
		common::api::dbus::DBusInputMessageStream& inputMessageStream) {
	return inputMessageStream >> intValue
							  >> doubleValue;
}

common::api::dbus::DBusOutputMessageStream& TestStruct::writeToDBusOutputMessageStream(
		common::api::dbus::DBusOutputMessageStream& outputMessageStream) const {
	return outputMessageStream << intValue
							   << doubleValue;
}


TestStructExtended::TestStructExtended(const uint32_t& fromIntValue, const double& fromDoubleValue, const std::string& fromStringValue):
		TestStruct(fromIntValue, fromDoubleValue),
		stringValue(fromStringValue) {
}

common::api::dbus::DBusInputMessageStream& TestStructExtended::readFromDBusInputMessageStream(
		common::api::dbus::DBusInputMessageStream& inputMessageStream) {
  return TestStruct::readFromDBusInputMessageStream(inputMessageStream) >> stringValue;
}

common::api::dbus::DBusOutputMessageStream& TestStructExtended::writeToDBusOutputMessageStream(
		common::api::dbus::DBusOutputMessageStream& outputMessageStream) const {
  return TestStruct::writeToDBusOutputMessageStream(outputMessageStream) << stringValue;
}

} //namespace test
} //namespace api
} //namespace common


int main(void) {
	using namespace common::api::test;

	TestStructExtended testStructExtended(123, 456.789, "TestStructExtended");

	common::api::dbus::DBusMessage message = common::api::dbus::DBusMessage::createMethodCall(
			"com.bmw.test.TestStruct",
			"/com/bmw/test/TestStruct",
			"com.bmw.test.TestStruct",
			"SingleTestStruct",
			COMMON_API_TEST_TEST_STRUCT_EXTENDED_SIGNATURE);

	common::api::dbus::DBusOutputMessageStream outStream(message);
	outStream << testStructExtended;
	outStream.flush();
}
