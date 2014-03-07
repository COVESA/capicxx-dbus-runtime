/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef _GLIBCXX_USE_NANOSLEEP
#define _GLIBCXX_USE_NANOSLEEP
#endif

#ifndef COMMONAPI_INTERNAL_COMPILATION
#define COMMONAPI_INTERNAL_COMPILATION
#endif

#include <CommonAPI/DBus/DBusInputStream.h>
#include <CommonAPI/DBus/DBusMessage.h>
#include <CommonAPI/DBus/DBusProxy.h>
#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusRuntime.h>
#include <CommonAPI/DBus/DBusStubAdapter.h>
#include <CommonAPI/DBus/DBusServicePublisher.h>
#include <CommonAPI/DBus/DBusFactory.h>

#include <commonapi/tests/TestInterfaceDBusProxy.h>
#include <commonapi/tests/TestInterfaceDBusStubAdapter.h>
#include <commonapi/tests/TestInterfaceStubDefault.h>

#include <commonapi/tests/ExtendedInterfaceProxy.h>
#include <commonapi/tests/ExtendedInterfaceDBusProxy.h>
#include <commonapi/tests/ExtendedInterfaceDBusStubAdapter.h>
#include <commonapi/tests/ExtendedInterfaceStubDefault.h>

#ifndef COMMONAPI_INTERNAL_COMPILATION
#define COMMONAPI_INTERNAL_COMPILATION
#endif

#include "DBusTestUtils.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <vector>


static const std::string commonApiAddress = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService";
static const std::string commonApiAddressExtended = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService2";
static const std::string commonApiServiceName = "CommonAPI.DBus.tests.DBusProxyTestInterface";
static const std::string interfaceName = "CommonAPI.DBus.tests.DBusProxyTestInterface";
static const std::string busName = "CommonAPI.DBus.tests.DBusProxyTestService";
static const std::string objectPath = "/CommonAPI/DBus/tests/DBusProxyTestService";
static const std::string objectPathExtended = "/CommonAPI/DBus/tests/DBusProxyTestService2";
static const std::string commonApiAddressFreedesktop = "local:org.freedesktop.XYZ:CommonAPI.DBus.tests.DBusProxyTestInterface";


class ProxyTest: public ::testing::Test {
protected:
    void SetUp() {
        runtime_ = std::dynamic_pointer_cast<CommonAPI::DBus::DBusRuntime>(CommonAPI::Runtime::load());

        serviceFactory_ = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(runtime_->createFactory());

        proxyDBusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(proxyDBusConnection_->connect());

        proxy_ = std::make_shared<commonapi::tests::TestInterfaceDBusProxy>(
                        serviceFactory_,
                        commonApiAddress,
                        interfaceName,
                        busName,
                        objectPath,
                        proxyDBusConnection_);
        proxy_->init();
    }

    std::shared_ptr<CommonAPI::DBus::DBusRuntime> runtime_;
    std::shared_ptr<CommonAPI::DBus::DBusFactory> serviceFactory_;

    virtual void TearDown() {
        usleep(300000);
    }

    void registerTestStub() {
        stubDefault_ = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
        bool isTestStubAdapterRegistered_ = runtime_->getServicePublisher()->registerService<commonapi::tests::TestInterfaceStub>(stubDefault_, commonApiAddress, serviceFactory_);
        ASSERT_TRUE(isTestStubAdapterRegistered_);

        usleep(100000);
    }

    void registerExtendedStub() {
        stubExtended_ = std::make_shared<commonapi::tests::ExtendedInterfaceStubDefault>();

        bool isExtendedStubAdapterRegistered_ = runtime_->getServicePublisher()->registerService<commonapi::tests::ExtendedInterfaceStub>(stubExtended_, commonApiAddressExtended, serviceFactory_);
        ASSERT_TRUE(isExtendedStubAdapterRegistered_);

        usleep(100000);
    }

    void deregisterTestStub() {
        const bool isStubAdapterUnregistered = CommonAPI::DBus::DBusServicePublisher::getInstance()->unregisterService(
                        commonApiAddress);
        ASSERT_TRUE(isStubAdapterUnregistered);
        stubDefault_.reset();
        isTestStubAdapterRegistered_ = false;
    }

    void deregisterExtendedStub() {
        const bool isStubAdapterUnregistered = runtime_->getServicePublisher()->unregisterService(
                        commonApiAddressExtended);
        ASSERT_TRUE(isStubAdapterUnregistered);
        stubExtended_.reset();
        isExtendedStubAdapterRegistered_ = false;
    }

    void proxyRegisterForAvailabilityStatus() {
        proxyAvailabilityStatus_ = CommonAPI::AvailabilityStatus::UNKNOWN;

        proxyStatusSubscription_ = proxy_->getProxyStatusEvent().subscribe([&](const CommonAPI::AvailabilityStatus& availabilityStatus) {
            proxyAvailabilityStatus_ = availabilityStatus;
        });
        usleep(100000);
    }

    void proxyDeregisterForAvailabilityStatus() {
        proxy_->getProxyStatusEvent().unsubscribe(proxyStatusSubscription_);
    }

    bool proxyWaitForAvailabilityStatus(const CommonAPI::AvailabilityStatus& availabilityStatus) const {
        for (int i = 0; i < 10; i++) {
            if (proxyAvailabilityStatus_ == availabilityStatus) {
                return true;
            }
            usleep(200000);
        }

        return false;
    }

    bool isExtendedStubAdapterRegistered_;
    bool isTestStubAdapterRegistered_;

    std::shared_ptr<CommonAPI::DBus::DBusConnection> proxyDBusConnection_;
    std::shared_ptr<commonapi::tests::TestInterfaceDBusProxy> proxy_;
    CommonAPI::AvailabilityStatus proxyAvailabilityStatus_;

    CommonAPI::ProxyStatusEvent::Subscription proxyStatusSubscription_;

    std::shared_ptr<commonapi::tests::ExtendedInterfaceStubDefault> stubExtended_;
    std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stubDefault_;
};

TEST_F(ProxyTest, HasCorrectConnectionName) {
    std::string actualName = proxy_->getDBusBusName();
    EXPECT_EQ(busName, actualName);
}

TEST_F(ProxyTest, HasCorrectObjectPath) {
    std::string actualPath = proxy_->getDBusObjectPath();
    EXPECT_EQ(objectPath, actualPath);
}

TEST_F(ProxyTest, HasCorrectInterfaceName) {
    std::string actualName = proxy_->getInterfaceName();
    EXPECT_EQ(interfaceName, actualName);
}

TEST_F(ProxyTest, IsNotAvailable) {
    bool isAvailable = proxy_->isAvailable();
    EXPECT_FALSE(isAvailable);
}

TEST_F(ProxyTest, IsConnected) {
    ASSERT_TRUE(proxy_->getDBusConnection()->isConnected());
}

TEST_F(ProxyTest, AssociatedConnectionHasServiceRegistry) {
    std::shared_ptr<CommonAPI::DBus::DBusProxyConnection> connection = proxy_->getDBusConnection();
    auto registry = connection->getDBusServiceRegistry();
    ASSERT_FALSE(!registry);
}

TEST_F(ProxyTest, DBusProxyStatusEventBeforeServiceIsRegistered) {
    proxyRegisterForAvailabilityStatus();

    EXPECT_NE(proxyAvailabilityStatus_, CommonAPI::AvailabilityStatus::AVAILABLE);

    registerTestStub();

    EXPECT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::AVAILABLE));

    deregisterTestStub();
    usleep(100000);

    EXPECT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::NOT_AVAILABLE));

    proxyDeregisterForAvailabilityStatus();
}

/*
This test fails in Windows. Calling disconnect and then connect again somehow
damages the connection in libdbus. In Linux this all works fine.
*/
#ifndef WIN32
TEST_F(ProxyTest, DBusProxyStatusEventAfterServiceIsRegistered) {
    proxyDBusConnection_->disconnect();

    registerTestStub();

    EXPECT_TRUE(proxyDBusConnection_->connect());

    proxyRegisterForAvailabilityStatus();

    EXPECT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::AVAILABLE));

    deregisterTestStub();
    usleep(100000);

    EXPECT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::NOT_AVAILABLE));

    proxyDeregisterForAvailabilityStatus();
}
#endif

TEST_F(ProxyTest, ServiceStatus) {
    registerTestStub();

    std::vector<std::string> availableDBusServices;

    //Service actually IS available!
    for (int i = 0; i < 5; i++) {
        availableDBusServices = proxyDBusConnection_->getDBusServiceRegistry()->getAvailableServiceInstances(
                        commonApiServiceName,
                        "local");

        if (!availableDBusServices.empty()) {
            break;
        }
    }

    auto found = std::find(availableDBusServices.begin(), availableDBusServices.end(), commonApiAddress);

    EXPECT_TRUE(availableDBusServices.begin() != availableDBusServices.end());
    EXPECT_TRUE(found != availableDBusServices.end());

    deregisterTestStub();
}

TEST_F(ProxyTest, isServiceInstanceAlive) {
    registerTestStub();

    bool isInstanceAlive = proxyDBusConnection_->getDBusServiceRegistry()->isServiceInstanceAlive(interfaceName, busName, objectPath);

    for (int i = 0; !isInstanceAlive && i < 10; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        isInstanceAlive = proxyDBusConnection_->getDBusServiceRegistry()->isServiceInstanceAlive(interfaceName, busName, objectPath);
    }

    EXPECT_TRUE(isInstanceAlive);

    deregisterTestStub();
}

TEST_F(ProxyTest, IsAvailableBlocking) {
    registerTestStub();

    // blocking in terms of "if it's still unknown"
    for (int i = 0; !proxy_->isAvailableBlocking() && i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EXPECT_TRUE(proxy_->isAvailableBlocking());

    deregisterTestStub();
}

TEST_F(ProxyTest, HasNecessaryAttributesAndEvents) {
    (proxy_->getInterfaceVersionAttribute());
    (proxy_->getProxyStatusEvent());
}

TEST_F(ProxyTest, TestInterfaceVersionAttribute) {
    CommonAPI::InterfaceVersionAttribute& versionAttribute = proxy_->getInterfaceVersionAttribute();
    CommonAPI::Version version;
    CommonAPI::CallStatus status;
    ASSERT_NO_THROW(versionAttribute.getValue(status, version));
    ASSERT_EQ(CommonAPI::CallStatus::NOT_AVAILABLE, status);
}

TEST_F(ProxyTest, AsyncCallbacksAreCalledIfServiceNotAvailable) {
    commonapi::tests::DerivedTypeCollection::TestEnumExtended2 testInputStruct;
    commonapi::tests::DerivedTypeCollection::TestMap testInputMap;
    std::promise<bool> wasCalledPromise;
    std::future<bool> wasCalledFuture = wasCalledPromise.get_future();
    proxy_->testDerivedTypeMethodAsync(testInputStruct, testInputMap, [&] (
                    const CommonAPI::CallStatus& callStatus,
                    const commonapi::tests::DerivedTypeCollection::TestEnumExtended2&,
                    const commonapi::tests::DerivedTypeCollection::TestMap&) {
        ASSERT_EQ(callStatus, CommonAPI::CallStatus::NOT_AVAILABLE);
        wasCalledPromise.set_value(true);
    }
                    );
    ASSERT_TRUE(wasCalledFuture.get());
}


TEST_F(ProxyTest, CallMethodFromExtendedInterface) {
    registerExtendedStub();

    auto extendedProxy = serviceFactory_->buildProxy<commonapi::tests::ExtendedInterfaceProxy>(commonApiAddressExtended);

    // give the proxy time to become available
    for (uint32_t i = 0; !extendedProxy->isAvailable() && i < 200; ++i) {
        usleep(20 * 1000);
    }

    EXPECT_TRUE(extendedProxy->isAvailable());

    uint32_t inInt;
    bool wasCalled = false;
    extendedProxy->TestIntMethodExtendedAsync(
                    inInt,
                    [&](const CommonAPI::CallStatus& callStatus) {
                        ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
                        wasCalled = true;
                    });
    usleep(100000);

    EXPECT_TRUE(wasCalled);
    deregisterExtendedStub();
}

TEST_F(ProxyTest, CallMethodFromParentInterface) {
    registerExtendedStub();

    auto extendedProxy = serviceFactory_->buildProxy<commonapi::tests::ExtendedInterfaceProxy>(commonApiAddressExtended);

    for (uint32_t i = 0; !extendedProxy->isAvailable() && i < 200; ++i) {
        usleep(20 * 1000);
    }
    EXPECT_TRUE(extendedProxy->isAvailable());

    bool wasCalled = false;
    extendedProxy->testEmptyMethodAsync(
                    [&](const CommonAPI::CallStatus& callStatus) {
                        ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
                        wasCalled = true;
                    });
    usleep(100000);
    EXPECT_TRUE(wasCalled);

    deregisterExtendedStub();
}

TEST_F(ProxyTest, CanHandleRemoteAttribute) {
    registerTestStub();

    for (uint32_t i = 0; !proxy_->isAvailable() && i < 200; ++i) {
        usleep(20 * 1000);
    }
    ASSERT_TRUE(proxy_->isAvailable());

    CommonAPI::CallStatus callStatus(CommonAPI::CallStatus::REMOTE_ERROR);
    uint32_t value;

    auto& testAttribute = proxy_->getTestPredefinedTypeAttributeAttribute();

    testAttribute.getValue(callStatus, value);

    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    value = 7;
    uint32_t responseValue;
    testAttribute.setValue(value, callStatus, responseValue);

    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(value, responseValue);

    usleep(50000);
    deregisterTestStub();
}


TEST_F(ProxyTest, CanHandleRemoteAttributeFromParentInterface) {
    registerExtendedStub();

    std::shared_ptr<CommonAPI::DBus::DBusFactory> proxyFactory_ = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(runtime_->createFactory());

    auto extendedProxy = proxyFactory_->buildProxy<commonapi::tests::ExtendedInterfaceProxy>(commonApiAddressExtended);


    for (uint32_t i = 0; !extendedProxy->isAvailable() && i < 200; ++i) {
        usleep(20 * 1000);
    }
    ASSERT_TRUE(extendedProxy->isAvailable());

    CommonAPI::CallStatus callStatus(CommonAPI::CallStatus::REMOTE_ERROR);
    uint32_t value;

    //usleep(200000000);

    auto& testAttribute = extendedProxy->getTestPredefinedTypeAttributeAttribute();

    testAttribute.getValue(callStatus, value);

    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    value = 7;
    uint32_t responseValue;
    testAttribute.setValue(value, callStatus, responseValue);

    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(value, responseValue);

    usleep(50000);
    deregisterExtendedStub();
}

TEST_F(ProxyTest, ProxyCanFetchVersionAttributeFromInheritedInterfaceStub) {
    registerExtendedStub();

    auto extendedProxy = serviceFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(commonApiAddressExtended);

    for (uint32_t i = 0; !extendedProxy->isAvailable() && i < 200; ++i) {
        usleep(20 * 1000);
    }
    EXPECT_TRUE(extendedProxy->isAvailable());


    CommonAPI::InterfaceVersionAttribute& versionAttribute = extendedProxy->getInterfaceVersionAttribute();

    CommonAPI::Version version;
    bool wasCalled = false;

    std::future<CommonAPI::CallStatus> futureVersion = versionAttribute.getValueAsync([&](const CommonAPI::CallStatus& callStatus, CommonAPI::Version version) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
        EXPECT_TRUE(version.Major > 0 || version.Minor > 0);
        wasCalled = true;
    });

    futureVersion.wait();
    usleep(100000);

    EXPECT_TRUE(wasCalled);

    deregisterExtendedStub();
}

// this test does not build its proxies within SetUp
class ProxyTest2: public ::testing::Test {
protected:
    virtual void SetUp() {
        runtime_ = std::dynamic_pointer_cast<CommonAPI::DBus::DBusRuntime>(CommonAPI::Runtime::load());
        serviceFactory_ = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(runtime_->createFactory());
    }

    virtual void TearDown() {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    void registerTestStub(const std::string commonApiAddress) {
        stubDefault_ = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
        bool isTestStubAdapterRegistered_ = runtime_->getServicePublisher()->registerService<
                        commonapi::tests::TestInterfaceStub>(stubDefault_, commonApiAddress, serviceFactory_);
        ASSERT_TRUE(isTestStubAdapterRegistered_);

        usleep(100000);
    }

    void deregisterTestStub(const std::string commonApiAddress) {
        const bool isStubAdapterUnregistered = CommonAPI::DBus::DBusServicePublisher::getInstance()->unregisterService(
                        commonApiAddress);
        ASSERT_TRUE(isStubAdapterUnregistered);
        stubDefault_.reset();
        isTestStubAdapterRegistered_ = false;
    }

    void proxyRegisterForAvailabilityStatus() {
        proxyAvailabilityStatus_ = CommonAPI::AvailabilityStatus::UNKNOWN;

        proxyStatusSubscription_ = proxy_->getProxyStatusEvent().subscribe(
                        [&](const CommonAPI::AvailabilityStatus& availabilityStatus) {
                            proxyAvailabilityStatus_ = availabilityStatus;
                        });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void proxyDeregisterForAvailabilityStatus() {
        proxy_->getProxyStatusEvent().unsubscribe(proxyStatusSubscription_);
    }

    bool proxyWaitForAvailabilityStatus(const CommonAPI::AvailabilityStatus& availabilityStatus) const {
        for (int i = 0; i < 100; i++) {
            if (proxyAvailabilityStatus_ == availabilityStatus)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return false;
    }

    bool isTestStubAdapterRegistered_;
    std::shared_ptr<CommonAPI::DBus::DBusRuntime> runtime_;
    std::shared_ptr<CommonAPI::DBus::DBusFactory> serviceFactory_;

    std::shared_ptr<CommonAPI::DBus::DBusConnection> proxyDBusConnection_;
    std::shared_ptr<commonapi::tests::TestInterfaceDBusProxy> proxy_;
    CommonAPI::AvailabilityStatus proxyAvailabilityStatus_;

    CommonAPI::ProxyStatusEvent::Subscription proxyStatusSubscription_;

    std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stubDefault_;
};

TEST_F(ProxyTest2, DBusProxyStatusEventAfterServiceIsRegistered) {
    registerTestStub(commonApiAddress);

    proxyDBusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
    ASSERT_TRUE(proxyDBusConnection_->connect());

    proxy_ = std::make_shared<commonapi::tests::TestInterfaceDBusProxy>(
                    serviceFactory_,
                    commonApiAddress,
                    interfaceName,
                    busName,
                    objectPath,
                    proxyDBusConnection_);
    proxy_->init();

    proxyRegisterForAvailabilityStatus();

    EXPECT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::AVAILABLE));

    deregisterTestStub(commonApiAddress);
    usleep(100000);

    EXPECT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::NOT_AVAILABLE));

    proxyDeregisterForAvailabilityStatus();
}

TEST_F(ProxyTest2, DBusProxyCanUseOrgFreedesktopAddress) {
    registerTestStub(commonApiAddressFreedesktop);

    std::shared_ptr<commonapi::tests::TestInterfaceProxyDefault> proxy =
                    serviceFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(commonApiAddressFreedesktop);

    for (int i = 0; i < 100; i++) {
        if (proxy->isAvailable())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    const bool proxyIsAvailable = proxy->isAvailable();

    EXPECT_TRUE(proxyIsAvailable);

    if(proxyIsAvailable) { // if we are not available, we failed the test and do not check a method call
        bool wasCalled = false;
        proxy->testEmptyMethodAsync(
                        [&](const CommonAPI::CallStatus& callStatus) {
                            ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
                            wasCalled = true;
                        });
        usleep(100000);
        EXPECT_TRUE(wasCalled);
    }

    deregisterTestStub(commonApiAddressFreedesktop);
}

#ifndef WIN32
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
