/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef DEMO_POSITIONING_H_
#define DEMO_POSITIONING_H_


#include <common-api-dbus/dbus-serializable-struct.h>
#include <vector>


namespace common {
namespace api {
namespace test {


struct TestStruct: public common::api::dbus::SerializableStruct {
	TestStruct() = default;
	TestStruct(const uint32_t& fromIntValue, const double& fromDoubleValue);

	virtual common::api::dbus::DBusInputMessageStream& readFromDBusInputMessageStream(
			common::api::dbus::DBusInputMessageStream& inputMessageStream);

	virtual common::api::dbus::DBusOutputMessageStream& writeToDBusOutputMessageStream(
			common::api::dbus::DBusOutputMessageStream& outputMessageStream) const;

	uint32_t intValue;
	double doubleValue;
};


struct TestStructExtended: public TestStruct {
	TestStructExtended() = default;

	TestStructExtended(const uint32_t& fromIntValue, const double& fromDoubleValue, const std::string& fromStringValue);

	virtual common::api::dbus::DBusInputMessageStream& readFromDBusInputMessageStream(
			common::api::dbus::DBusInputMessageStream& inputMessageStream);

	virtual common::api::dbus::DBusOutputMessageStream& writeToDBusOutputMessageStream(
			common::api::dbus::DBusOutputMessageStream& outputMessageStream) const;

	std::string stringValue;
};


typedef std::vector<TestStruct> TestStructArray;
typedef std::vector<TestStructExtended> TestStructExtendedArray;


} //namespace test

namespace dbus {

template<>
struct Alignment<common::api::test::TestStruct>: SizeConstant<8> { };

template<>
struct Alignment<common::api::test::TestStructExtended>: SizeConstant<8> { };

} //namespace dbus
} //namespace api
} //namespace common

#endif /* DEMO_POSITIONING_H_ */
