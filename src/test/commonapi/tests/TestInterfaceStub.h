/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_TESTS_TEST_INTERFACE_STUB_H_
#define COMMONAPI_TESTS_TEST_INTERFACE_STUB_H_

#include <CommonAPI/SerializableVariant.h>
#include <unordered_map>
#include <cstdint>
#include <test/commonapi/tests/DerivedTypeCollection.h>
#include <CommonAPI/InputStream.h>
#include <vector>
#include <string>
#include <memory>
#include <CommonAPI/OutputStream.h>
#include "TestInterface.h"
#include <CommonAPI/Stub.h>

namespace commonapi {
namespace tests {

class TestInterfaceStubAdapter: virtual public CommonAPI::StubAdapter, public TestInterface {
 public:
    virtual void fireTestPredefinedTypeAttributeAttributeChanged(const uint32_t& TestPredefinedTypeAttribute) = 0;
    virtual void fireTestDerivedStructAttributeAttributeChanged(const DerivedTypeCollection::TestStructExtended& TestDerivedStructAttribute) = 0;
    virtual void fireTestDerivedArrayAttributeAttributeChanged(const DerivedTypeCollection::TestArrayUInt64& TestDerivedArrayAttribute) = 0;

    virtual void fireTestPredefinedTypeBroadcastEvent(const uint32_t& uint32Value, const std::string& stringValue) = 0;
};


class TestInterfaceStubRemoteEvent {
 public:
    virtual ~TestInterfaceStubRemoteEvent() { }

    virtual bool onRemoteSetTestPredefinedTypeAttributeAttribute(uint32_t TestPredefinedTypeAttribute) = 0;
    virtual void onRemoteTestPredefinedTypeAttributeAttributeChanged() = 0;

    virtual bool onRemoteSetTestDerivedStructAttributeAttribute(DerivedTypeCollection::TestStructExtended TestDerivedStructAttribute) = 0;
    virtual void onRemoteTestDerivedStructAttributeAttributeChanged() = 0;

    virtual bool onRemoteSetTestDerivedArrayAttributeAttribute(DerivedTypeCollection::TestArrayUInt64 TestDerivedArrayAttribute) = 0;
    virtual void onRemoteTestDerivedArrayAttributeAttributeChanged() = 0;

};


class TestInterfaceStub : public CommonAPI::Stub<TestInterfaceStubAdapter , TestInterfaceStubRemoteEvent> {
 public:
    virtual const uint32_t& getTestPredefinedTypeAttributeAttribute() = 0;
    virtual const DerivedTypeCollection::TestStructExtended& getTestDerivedStructAttributeAttribute() = 0;
    virtual const DerivedTypeCollection::TestArrayUInt64& getTestDerivedArrayAttributeAttribute() = 0;

    virtual void testVoidPredefinedTypeMethod(uint32_t uint32Value, std::string stringValue) = 0;
    virtual void testPredefinedTypeMethod(uint32_t uint32InValue, std::string stringInValue, uint32_t& uint32OutValue, std::string& stringOutValue) = 0;
    virtual void testVoidDerivedTypeMethod(DerivedTypeCollection::TestEnumExtended2 testEnumExtended2Value, DerivedTypeCollection::TestMap testMapValue) = 0;
    virtual void testDerivedTypeMethod(DerivedTypeCollection::TestEnumExtended2 testEnumExtended2InValue, DerivedTypeCollection::TestMap testMapInValue, DerivedTypeCollection::TestEnumExtended2& testEnumExtended2OutValue, DerivedTypeCollection::TestMap& testMapOutValue) = 0;
    virtual void testUnionMethod(DerivedTypeCollection::TestUnionIn inParam, DerivedTypeCollection::TestUnionIn& outParam) = 0;
    
    virtual void fireTestPredefinedTypeBroadcastEvent(const uint32_t& uint32Value, const std::string& stringValue) = 0;
};

} // namespace tests
} // namespace commonapi

#endif // COMMONAPI_TESTS_TEST_INTERFACE_STUB_H_
