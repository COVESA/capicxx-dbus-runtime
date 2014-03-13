/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <gtest/gtest.h>
#include <commonapi/tests/DerivedTypeCollection.h>
#include <commonapi/tests/TestFreedesktopInterfaceProxy.h>
#include <commonapi/tests/TestFreedesktopInterfaceStub.h>
#include <commonapi/tests/TestFreedesktopInterfaceStubDefault.h>
#include <commonapi/tests/TestFreedesktopDerivedInterfaceProxy.h>
#include <commonapi/tests/TestFreedesktopDerivedInterfaceStubDefault.h>


#ifndef COMMONAPI_INTERNAL_COMPILATION
#define COMMONAPI_INTERNAL_COMPILATION
#endif
#include <CommonAPI/DBus/DBusRuntime.h>

#ifndef COMMONAPI_INTERNAL_COMPILATION
#define COMMONAPI_INTERNAL_COMPILATION
#endif

static const std::string commonApiAddress =
                "local:CommonAPI.DBus.tests.DBusProxyTestFreedesktopPropertiesInterface:CommonAPI.DBus.tests.DBusProxyTestFreedesktopPropertiesInterface";

class FreedesktopPropertiesTest: public ::testing::Test {
protected:
    void SetUp() {
        std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
        servicePublisher_ = runtime->getServicePublisher();

        ASSERT_TRUE(servicePublisher_ != NULL);

        serviceFactory_ = runtime->createFactory();
        proxyFactory_ = runtime->createFactory();

        ASSERT_TRUE(serviceFactory_ != NULL);
        ASSERT_TRUE(proxyFactory_ != NULL);

        proxy_ = proxyFactory_->buildProxy<commonapi::tests::TestFreedesktopInterfaceProxy>(commonApiAddress);

        registerTestStub();

        for (unsigned int i = 0; !proxy_->isAvailable() && i < 100; ++i) {
            usleep(10000);
        }
        ASSERT_TRUE(proxy_->isAvailable());
    }

    virtual void TearDown() {
        deregisterTestStub();
        usleep(30000);
    }

    void registerTestStub() {
        testStub_ = std::make_shared<commonapi::tests::TestFreedesktopInterfaceStubDefault>();
        const bool isServiceRegistered = servicePublisher_->registerService(testStub_, commonApiAddress, serviceFactory_);

        ASSERT_TRUE(isServiceRegistered);
    }


    void deregisterTestStub() {
        const bool isStubAdapterUnregistered = servicePublisher_->unregisterService(commonApiAddress);
        ASSERT_TRUE(isStubAdapterUnregistered);
    }

    std::shared_ptr<CommonAPI::Factory> serviceFactory_, proxyFactory_;
    std::shared_ptr<commonapi::tests::TestFreedesktopInterfaceProxyDefault> proxy_;
    std::shared_ptr<commonapi::tests::TestFreedesktopInterfaceStubDefault> testStub_;
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher_;
};

TEST_F(FreedesktopPropertiesTest, GetBasicTypeAttribute) {
    CommonAPI::CallStatus callStatus(CommonAPI::CallStatus::REMOTE_ERROR);
    uint32_t value;

    auto& testAttribute = proxy_->getTestPredefinedTypeAttributeAttribute();

    testAttribute.getValue(callStatus, value);

    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
}

TEST_F(FreedesktopPropertiesTest, GetAndSetBasicTypeAttribute) {
    CommonAPI::CallStatus callStatus(CommonAPI::CallStatus::REMOTE_ERROR);
    uint32_t value;

    auto& testAttribute = proxy_->getTestPredefinedTypeAttributeAttribute();

    testAttribute.getValue(callStatus, value);

    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    callStatus = CommonAPI::CallStatus::REMOTE_ERROR;
    uint32_t newValue = 7;

    testAttribute.setValue(newValue, callStatus, value);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(value, 7);

    value = 0;
    callStatus = CommonAPI::CallStatus::REMOTE_ERROR;
    testAttribute.getValue(callStatus, value);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(value, 7);
}

TEST_F(FreedesktopPropertiesTest, CanSendAndReceiveNotificationForSingleProperty) {
    auto& testAttribute = proxy_->getTestPredefinedTypeAttributeAttribute();

    bool callbackArrived = false;

    std::function<void(const uint32_t)> listener([&](const uint32_t value) {
        ASSERT_EQ(7, value);
        callbackArrived = true;
    });

    usleep(200000);

    testAttribute.getChangedEvent().subscribe(listener);

    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::REMOTE_ERROR;
    uint32_t value;
    uint32_t newValue = 7;

    testAttribute.setValue(newValue, callStatus, value);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(value, 7);


    uint8_t waitCounter = 0;
    while(!callbackArrived && waitCounter < 10) {
        usleep(50000);
        waitCounter++;
    }

    ASSERT_TRUE(callbackArrived);
}

class FreedesktopPropertiesOnInheritedInterfacesTest: public ::testing::Test {
protected:
    void SetUp() {
        std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
        servicePublisher_ = runtime->getServicePublisher();

        ASSERT_TRUE(servicePublisher_ != NULL);

        serviceFactory_ = runtime->createFactory();
        proxyFactory_ = runtime->createFactory();

        ASSERT_TRUE(serviceFactory_ != NULL);
        ASSERT_TRUE(proxyFactory_ != NULL);

        proxy_ = proxyFactory_->buildProxy<commonapi::tests::TestFreedesktopDerivedInterfaceProxy>(commonApiAddress);

        registerTestStub();

        for (unsigned int i = 0; !proxy_->isAvailable() && i < 100; ++i) {
            usleep(10000);
        }
        ASSERT_TRUE(proxy_->isAvailable());
    }

    virtual void TearDown() {
        deregisterTestStub();
        usleep(30000);
    }

    void registerTestStub() {
        testStub_ = std::make_shared<commonapi::tests::TestFreedesktopDerivedInterfaceStubDefault>();
        const bool isServiceRegistered = servicePublisher_->registerService(testStub_, commonApiAddress, serviceFactory_);

        ASSERT_TRUE(isServiceRegistered);
    }

    void deregisterTestStub() {
        const bool isStubAdapterUnregistered = servicePublisher_->unregisterService(commonApiAddress);
        ASSERT_TRUE(isStubAdapterUnregistered);
    }


    std::shared_ptr<CommonAPI::Factory> serviceFactory_, proxyFactory_;
    std::shared_ptr<commonapi::tests::TestFreedesktopDerivedInterfaceProxyDefault> proxy_;
    std::shared_ptr<commonapi::tests::TestFreedesktopDerivedInterfaceStubDefault> testStub_;
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher_;
};

TEST_F(FreedesktopPropertiesOnInheritedInterfacesTest, CanGetAndSetRemoteAttributeFromDerivedInterface) {
    auto& testAttribute = proxy_->getTestAttributedFromDerivedInterfaceAttribute();

    CommonAPI::CallStatus callStatus(CommonAPI::CallStatus::REMOTE_ERROR);
    uint32_t value;
    testAttribute.getValue(callStatus, value);

    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    callStatus = CommonAPI::CallStatus::REMOTE_ERROR;
    uint32_t newValue = 7;

    testAttribute.setValue(newValue, callStatus, value);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(value, 7);

    value = 0;
    callStatus = CommonAPI::CallStatus::REMOTE_ERROR;
    testAttribute.getValue(callStatus, value);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(value, 7);
}

TEST_F(FreedesktopPropertiesOnInheritedInterfacesTest, CanGetAndSetRemoteAttributeFromParentInterface) {
    auto& testAttribute = proxy_->getTestPredefinedTypeAttributeAttribute();

    CommonAPI::CallStatus callStatus(CommonAPI::CallStatus::REMOTE_ERROR);
    uint32_t value;
    testAttribute.getValue(callStatus, value);

    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    callStatus = CommonAPI::CallStatus::REMOTE_ERROR;
    uint32_t newValue = 7;

    testAttribute.setValue(newValue, callStatus, value);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(value, 7);

    value = 0;
    callStatus = CommonAPI::CallStatus::REMOTE_ERROR;
    testAttribute.getValue(callStatus, value);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(value, 7);
}

#ifndef WIN32
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
