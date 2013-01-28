/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_TESTS_TEST_INTERFACE_PROXY_H_
#define COMMONAPI_TESTS_TEST_INTERFACE_PROXY_H_

#include "TestInterfaceProxyBase.h"
#include <CommonAPI/AttributeExtension.h>
#include <CommonAPI/Factory.h>

namespace commonapi {
namespace tests {

template <typename ... _AttributeExtensions>
class TestInterfaceProxy: virtual public TestInterface, virtual public TestInterfaceProxyBase, public _AttributeExtensions... {
 public:
    TestInterfaceProxy(std::shared_ptr<CommonAPI::Proxy> delegate);
    ~TestInterfaceProxy();

    virtual TestPredefinedTypeAttributeAttribute& getTestPredefinedTypeAttributeAttribute();
    virtual TestDerivedStructAttributeAttribute& getTestDerivedStructAttributeAttribute();
    virtual TestDerivedArrayAttributeAttribute& getTestDerivedArrayAttributeAttribute();

    virtual TestPredefinedTypeBroadcastEvent& getTestPredefinedTypeBroadcastEvent();


    virtual void testVoidPredefinedTypeMethod(const uint32_t& uint32Value, const std::string& stringValue, CommonAPI::CallStatus& callStatus);
    virtual std::future<CommonAPI::CallStatus> testVoidPredefinedTypeMethodAsync(const uint32_t& uint32Value, const std::string& stringValue, TestVoidPredefinedTypeMethodAsyncCallback callback);

    virtual void testPredefinedTypeMethod(const uint32_t& uint32InValue, const std::string& stringInValue, CommonAPI::CallStatus& callStatus, uint32_t& uint32OutValue, std::string& stringOutValue);
    virtual std::future<CommonAPI::CallStatus> testPredefinedTypeMethodAsync(const uint32_t& uint32InValue, const std::string& stringInValue, TestPredefinedTypeMethodAsyncCallback callback);

    virtual void testVoidDerivedTypeMethod(const DerivedTypeCollection::TestEnumExtended2& testEnumExtended2Value, const DerivedTypeCollection::TestMap& testMapValue, CommonAPI::CallStatus& callStatus);
    virtual std::future<CommonAPI::CallStatus> testVoidDerivedTypeMethodAsync(const DerivedTypeCollection::TestEnumExtended2& testEnumExtended2Value, const DerivedTypeCollection::TestMap& testMapValue, TestVoidDerivedTypeMethodAsyncCallback callback);

    virtual void testDerivedTypeMethod(const DerivedTypeCollection::TestEnumExtended2& testEnumExtended2InValue, const DerivedTypeCollection::TestMap& testMapInValue, CommonAPI::CallStatus& callStatus, DerivedTypeCollection::TestEnumExtended2& testEnumExtended2OutValue, DerivedTypeCollection::TestMap& testMapOutValue);
    virtual std::future<CommonAPI::CallStatus> testDerivedTypeMethodAsync(const DerivedTypeCollection::TestEnumExtended2& testEnumExtended2InValue, const DerivedTypeCollection::TestMap& testMapInValue, TestDerivedTypeMethodAsyncCallback callback);

    virtual void testUnionMethod(const DerivedTypeCollection::TestUnionIn& inParam, CommonAPI::CallStatus& callStatus, DerivedTypeCollection::TestUnionIn& outParam);
    virtual std::future<CommonAPI::CallStatus> testUnionMethodAsync(const DerivedTypeCollection::TestUnionIn& inParam, TestUnionMethodAsyncCallback callback);

    virtual std::string getAddress() const;
    virtual const std::string& getDomain() const;
    virtual const std::string& getServiceId() const;
    virtual const std::string& getInstanceId() const;
    virtual bool isAvailable() const;
    virtual CommonAPI::ProxyStatusEvent& getProxyStatusEvent();
    virtual CommonAPI::InterfaceVersionAttribute& getInterfaceVersionAttribute();

 private:
    std::shared_ptr<TestInterfaceProxyBase> delegate_;
};

namespace TestInterfaceExtensions {
    template <template <typename > class _ExtensionType>
    class TestPredefinedTypeAttributeAttributeExtension {
     public:
        typedef _ExtensionType<TestInterfaceProxyBase::TestPredefinedTypeAttributeAttribute> extension_type;
    
        static_assert(std::is_base_of<typename CommonAPI::AttributeExtension<TestInterfaceProxyBase::TestPredefinedTypeAttributeAttribute>, extension_type>::value,
                      "Not CommonAPI Attribute Extension!");
    
        TestPredefinedTypeAttributeAttributeExtension(TestInterfaceProxyBase& proxy): attributeExtension_(proxy.getTestPredefinedTypeAttributeAttribute()) {
        }
    
        inline extension_type& getTestPredefinedTypeAttributeAttributeExtension() {
            return attributeExtension_;
        }
    
     private:
        extension_type attributeExtension_;
    };

    template <template <typename > class _ExtensionType>
    class TestDerivedStructAttributeAttributeExtension {
     public:
        typedef _ExtensionType<TestInterfaceProxyBase::TestDerivedStructAttributeAttribute> extension_type;
    
        static_assert(std::is_base_of<typename CommonAPI::AttributeExtension<TestInterfaceProxyBase::TestDerivedStructAttributeAttribute>, extension_type>::value,
                      "Not CommonAPI Attribute Extension!");
    
        TestDerivedStructAttributeAttributeExtension(TestInterfaceProxyBase& proxy): attributeExtension_(proxy.getTestDerivedStructAttributeAttribute()) {
        }
    
        inline extension_type& getTestDerivedStructAttributeAttributeExtension() {
            return attributeExtension_;
        }
    
     private:
        extension_type attributeExtension_;
    };

    template <template <typename > class _ExtensionType>
    class TestDerivedArrayAttributeAttributeExtension {
     public:
        typedef _ExtensionType<TestInterfaceProxyBase::TestDerivedArrayAttributeAttribute> extension_type;
    
        static_assert(std::is_base_of<typename CommonAPI::AttributeExtension<TestInterfaceProxyBase::TestDerivedArrayAttributeAttribute>, extension_type>::value,
                      "Not CommonAPI Attribute Extension!");
    
        TestDerivedArrayAttributeAttributeExtension(TestInterfaceProxyBase& proxy): attributeExtension_(proxy.getTestDerivedArrayAttributeAttribute()) {
        }
    
        inline extension_type& getTestDerivedArrayAttributeAttributeExtension() {
            return attributeExtension_;
        }
    
     private:
        extension_type attributeExtension_;
    };

} // namespace TestInterfaceExtensions

//
// TestInterfaceProxy Implementation
//
template <typename ... _AttributeExtensions>
TestInterfaceProxy<_AttributeExtensions...>::TestInterfaceProxy(std::shared_ptr<CommonAPI::Proxy> delegate):
        delegate_(std::dynamic_pointer_cast<TestInterfaceProxyBase>(delegate)),
        _AttributeExtensions(*(std::dynamic_pointer_cast<TestInterfaceProxyBase>(delegate)))... {
}

template <typename ... _AttributeExtensions>
TestInterfaceProxy<_AttributeExtensions...>::~TestInterfaceProxy() {
}

template <typename ... _AttributeExtensions>
typename TestInterfaceProxy<_AttributeExtensions...>::TestPredefinedTypeAttributeAttribute& TestInterfaceProxy<_AttributeExtensions...>::getTestPredefinedTypeAttributeAttribute() {
    return delegate_->getTestPredefinedTypeAttributeAttribute();
}

template <typename ... _AttributeExtensions>
typename TestInterfaceProxy<_AttributeExtensions...>::TestDerivedStructAttributeAttribute& TestInterfaceProxy<_AttributeExtensions...>::getTestDerivedStructAttributeAttribute() {
    return delegate_->getTestDerivedStructAttributeAttribute();
}

template <typename ... _AttributeExtensions>
typename TestInterfaceProxy<_AttributeExtensions...>::TestDerivedArrayAttributeAttribute& TestInterfaceProxy<_AttributeExtensions...>::getTestDerivedArrayAttributeAttribute() {
    return delegate_->getTestDerivedArrayAttributeAttribute();
}


template <typename ... _AttributeExtensions>
typename TestInterfaceProxy<_AttributeExtensions...>::TestPredefinedTypeBroadcastEvent& TestInterfaceProxy<_AttributeExtensions...>::getTestPredefinedTypeBroadcastEvent() {
    return delegate_->getTestPredefinedTypeBroadcastEvent();
}


template <typename ... _AttributeExtensions>
void TestInterfaceProxy<_AttributeExtensions...>::testVoidPredefinedTypeMethod(const uint32_t& uint32Value, const std::string& stringValue, CommonAPI::CallStatus& callStatus) {
    delegate_->testVoidPredefinedTypeMethod(uint32Value, stringValue, callStatus);
}

template <typename ... _AttributeExtensions>
std::future<CommonAPI::CallStatus> TestInterfaceProxy<_AttributeExtensions...>::testVoidPredefinedTypeMethodAsync(const uint32_t& uint32Value, const std::string& stringValue, TestVoidPredefinedTypeMethodAsyncCallback callback) {
    return delegate_->testVoidPredefinedTypeMethodAsync(uint32Value, stringValue, callback);
}

template <typename ... _AttributeExtensions>
void TestInterfaceProxy<_AttributeExtensions...>::testPredefinedTypeMethod(const uint32_t& uint32InValue, const std::string& stringInValue, CommonAPI::CallStatus& callStatus, uint32_t& uint32OutValue, std::string& stringOutValue) {
    delegate_->testPredefinedTypeMethod(uint32InValue, stringInValue, callStatus, uint32OutValue, stringOutValue);
}

template <typename ... _AttributeExtensions>
std::future<CommonAPI::CallStatus> TestInterfaceProxy<_AttributeExtensions...>::testPredefinedTypeMethodAsync(const uint32_t& uint32InValue, const std::string& stringInValue, TestPredefinedTypeMethodAsyncCallback callback) {
    return delegate_->testPredefinedTypeMethodAsync(uint32InValue, stringInValue, callback);
}

template <typename ... _AttributeExtensions>
void TestInterfaceProxy<_AttributeExtensions...>::testVoidDerivedTypeMethod(const DerivedTypeCollection::TestEnumExtended2& testEnumExtended2Value, const DerivedTypeCollection::TestMap& testMapValue, CommonAPI::CallStatus& callStatus) {
    delegate_->testVoidDerivedTypeMethod(testEnumExtended2Value, testMapValue, callStatus);
}

template <typename ... _AttributeExtensions>
std::future<CommonAPI::CallStatus> TestInterfaceProxy<_AttributeExtensions...>::testVoidDerivedTypeMethodAsync(const DerivedTypeCollection::TestEnumExtended2& testEnumExtended2Value, const DerivedTypeCollection::TestMap& testMapValue, TestVoidDerivedTypeMethodAsyncCallback callback) {
    return delegate_->testVoidDerivedTypeMethodAsync(testEnumExtended2Value, testMapValue, callback);
}

template <typename ... _AttributeExtensions>
void TestInterfaceProxy<_AttributeExtensions...>::testDerivedTypeMethod(const DerivedTypeCollection::TestEnumExtended2& testEnumExtended2InValue, const DerivedTypeCollection::TestMap& testMapInValue, CommonAPI::CallStatus& callStatus, DerivedTypeCollection::TestEnumExtended2& testEnumExtended2OutValue, DerivedTypeCollection::TestMap& testMapOutValue) {
    delegate_->testDerivedTypeMethod(testEnumExtended2InValue, testMapInValue, callStatus, testEnumExtended2OutValue, testMapOutValue);
}

template <typename ... _AttributeExtensions>
std::future<CommonAPI::CallStatus> TestInterfaceProxy<_AttributeExtensions...>::testDerivedTypeMethodAsync(const DerivedTypeCollection::TestEnumExtended2& testEnumExtended2InValue, const DerivedTypeCollection::TestMap& testMapInValue, TestDerivedTypeMethodAsyncCallback callback) {
    return delegate_->testDerivedTypeMethodAsync(testEnumExtended2InValue, testMapInValue, callback);
}

template <typename ... _AttributeExtensions>
void TestInterfaceProxy<_AttributeExtensions...>::testUnionMethod(const DerivedTypeCollection::TestUnionIn& inParam, CommonAPI::CallStatus& callStatus, DerivedTypeCollection::TestUnionIn& outParam) {
    delegate_->testUnionMethod(inParam, callStatus, outParam);
}

template <typename ... _AttributeExtensions>
std::future<CommonAPI::CallStatus> TestInterfaceProxy<_AttributeExtensions...>::testUnionMethodAsync(const DerivedTypeCollection::TestUnionIn& inParam, TestUnionMethodAsyncCallback callback) {
    return delegate_->testUnionMethodAsync(inParam, callback);
}


template <typename ... _AttributeExtensions>
std::string TestInterfaceProxy<_AttributeExtensions...>::getAddress() const {
    return delegate_->getAddress();
}

template <typename ... _AttributeExtensions>
const std::string& TestInterfaceProxy<_AttributeExtensions...>::getDomain() const {
    return delegate_->getDomain();
}

template <typename ... _AttributeExtensions>
const std::string& TestInterfaceProxy<_AttributeExtensions...>::getServiceId() const {
    return delegate_->getServiceId();
}

template <typename ... _AttributeExtensions>
const std::string& TestInterfaceProxy<_AttributeExtensions...>::getInstanceId() const {
    return delegate_->getInstanceId();
}

template <typename ... _AttributeExtensions>
bool TestInterfaceProxy<_AttributeExtensions...>::isAvailable() const {
    return delegate_->isAvailable();
}

template <typename ... _AttributeExtensions>
CommonAPI::ProxyStatusEvent& TestInterfaceProxy<_AttributeExtensions...>::getProxyStatusEvent() {
    return delegate_->getProxyStatusEvent();
}

template <typename ... _AttributeExtensions>
CommonAPI::InterfaceVersionAttribute& TestInterfaceProxy<_AttributeExtensions...>::getInterfaceVersionAttribute() {
    return delegate_->getInterfaceVersionAttribute();
}

} // namespace tests
} // namespace commonapi

namespace CommonAPI {
template<template<typename > class _AttributeExtension>
struct DefaultAttributeProxyFactoryHelper<commonapi::tests::TestInterfaceProxy,
    _AttributeExtension> {
    typedef typename commonapi::tests::TestInterfaceProxy<
            commonapi::tests::TestInterfaceExtensions::TestPredefinedTypeAttributeAttributeExtension<_AttributeExtension>, 
            commonapi::tests::TestInterfaceExtensions::TestDerivedStructAttributeAttributeExtension<_AttributeExtension>, 
            commonapi::tests::TestInterfaceExtensions::TestDerivedArrayAttributeAttributeExtension<_AttributeExtension>
    > class_t;
};
}


#endif // COMMONAPI_TESTS_TEST_INTERFACE_PROXY_H_
