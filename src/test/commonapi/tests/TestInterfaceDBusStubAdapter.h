/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_TESTS_TEST_INTERFACE_DBUS_STUB_ADAPTER_H_
#define COMMONAPI_TESTS_TEST_INTERFACE_DBUS_STUB_ADAPTER_H_

#include "TestInterfaceStub.h"

#include <CommonAPI/DBus/DBusStubAdapterHelper.h>
#include <CommonAPI/DBus/DBusFactory.h>

namespace commonapi {
namespace tests {

typedef CommonAPI::DBus::DBusStubAdapterHelper<TestInterfaceStub> TestInterfaceDBusStubAdapterHelper;

class TestInterfaceDBusStubAdapter: public TestInterfaceStubAdapter, public TestInterfaceDBusStubAdapterHelper {
 public:
    TestInterfaceDBusStubAdapter(
            const std::string& dbusBusName,
            const std::string& dbusObjectPath,
            const std::shared_ptr<CommonAPI::DBus::DBusProxyConnection>& dbusConnection,
            const std::shared_ptr<CommonAPI::StubBase>& stub);
    
    void fireTestPredefinedTypeAttributeAttributeChanged(const uint32_t& value);
    void fireTestDerivedStructAttributeAttributeChanged(const DerivedTypeCollection::TestStructExtended& value);
    void fireTestDerivedArrayAttributeAttributeChanged(const DerivedTypeCollection::TestArrayUInt64& value);

    void fireTestPredefinedTypeBroadcastEvent(const uint32_t& uint32Value, const std::string& stringValue);

 protected:
    virtual const char* getMethodsDBusIntrospectionXmlData() const;
};

} // namespace tests
} // namespace commonapi

#endif // COMMONAPI_TESTS_TEST_INTERFACE_DBUS_STUB_ADAPTER_H_
