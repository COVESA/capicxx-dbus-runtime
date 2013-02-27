/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef _GLIBCXX_USE_NANOSLEEP
#define _GLIBCXX_USE_NANOSLEEP
#endif

#include <CommonAPI/DBus/DBusInputStream.h>
#include <CommonAPI/DBus/DBusMessage.h>
#include <CommonAPI/DBus/DBusProxy.h>
#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusStubAdapter.h>
#include <CommonAPI/DBus/DBusUtils.h>

#include <commonapi/tests/TestInterfaceDBusProxy.h>
#include <commonapi/tests/TestInterfaceDBusStubAdapter.h>
#include <commonapi/tests/TestInterfaceStubDefault.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <vector>


static const std::string commonApiAddress = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService";
static const std::string commonApiServiceName = "CommonAPI.DBus.tests.DBusProxyTestInterface";
static const std::string interfaceName = "CommonAPI.DBus.tests.DBusProxyTestInterface";
static const std::string busName = "CommonAPI.DBus.tests.DBusProxyTestService";
static const std::string objectPath = "/CommonAPI/DBus/tests/DBusProxyTestService";


class ProxyTest: public ::testing::Test {
protected:

    virtual void TearDown() {
        proxyDBusConnection_->disconnect();

        stubAdapter_.reset();

        if (stubDBusConnection_) {
            if (stubDBusConnection_->isConnected()) {
                // uses dbus read_write_dispatch() which might dispatch events, which might cause a dead lock
                // stubDBusConnection_->releaseServiceName(busName);
                stubDBusConnection_->disconnect();
            }
            stubDBusConnection_.reset();
        }
    }

    void SetUp() {
        proxyDBusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(proxyDBusConnection_->connect());

        proxy_ = std::make_shared<commonapi::tests::TestInterfaceDBusProxy>(
                        commonApiAddress,
                        interfaceName,
                        busName,
                        objectPath,
                        proxyDBusConnection_);
    }

    void registerTestStub() {
        stubDBusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(stubDBusConnection_->connect());

        ASSERT_TRUE(stubDBusConnection_->requestServiceNameAndBlock(busName));

        auto stubDefault = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
        stubAdapter_ = std::make_shared<commonapi::tests::TestInterfaceDBusStubAdapter>(
                        commonApiAddress,
                        interfaceName,
                        busName,
                        objectPath,
                        stubDBusConnection_,
                        stubDefault);
        stubAdapter_->init();
    }

    void proxyRegisterForAvailabilityStatus() {
        proxyAvailabilityStatus_ = CommonAPI::AvailabilityStatus::UNKNOWN;

        proxy_->getProxyStatusEvent().subscribe([&](const CommonAPI::AvailabilityStatus& availabilityStatus) {
            proxyAvailabilityStatus_ = availabilityStatus;
        });
    }

    bool proxyWaitForAvailabilityStatus(const CommonAPI::AvailabilityStatus& availabilityStatus) const {
        std::chrono::milliseconds loopWaitDuration(100);

        if (proxyAvailabilityStatus_ == availabilityStatus)
            return true;

        for (int i = 0; i < 10; i++) {
            std::this_thread::sleep_for(loopWaitDuration);

            if (proxyAvailabilityStatus_ == availabilityStatus)
                return true;
        }

        return false;
    }

    std::shared_ptr<CommonAPI::DBus::DBusConnection> proxyDBusConnection_;
    std::shared_ptr<commonapi::tests::TestInterfaceDBusProxy> proxy_;
    CommonAPI::AvailabilityStatus proxyAvailabilityStatus_;

    std::shared_ptr<CommonAPI::DBus::DBusConnection> stubDBusConnection_;
    std::shared_ptr<commonapi::tests::TestInterfaceDBusStubAdapter> stubAdapter_;
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

TEST_F(ProxyTest, ServiceRegistry) {
	std::shared_ptr<CommonAPI::DBus::DBusProxyConnection> connection = proxy_->getDBusConnection();
	auto registry = connection->getDBusServiceRegistry();
	ASSERT_FALSE(!registry);
}

TEST_F(ProxyTest, DBusProxyStatusEventBeforeServiceIsRegistered) {
    proxyRegisterForAvailabilityStatus();

    ASSERT_NE(proxyAvailabilityStatus_, CommonAPI::AvailabilityStatus::AVAILABLE);

    registerTestStub();

    ASSERT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::AVAILABLE));

    stubDBusConnection_->disconnect();

    ASSERT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::NOT_AVAILABLE));
}

TEST_F(ProxyTest, DBusProxyStatusEventAfterServiceIsRegistered) {
    proxyDBusConnection_->disconnect();

    registerTestStub();

    ASSERT_TRUE(proxyDBusConnection_->connect());

    proxyRegisterForAvailabilityStatus();

    ASSERT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::AVAILABLE));

    stubDBusConnection_->disconnect();

    ASSERT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::NOT_AVAILABLE));
}

TEST_F(ProxyTest, ServiceStatus) {
    registerTestStub();

    std::vector<std::string> availableDBusServices;
    availableDBusServices = proxyDBusConnection_->getDBusServiceRegistry()->getAvailableServiceInstances(
                    commonApiServiceName,
                    "local");

	auto found = std::find(availableDBusServices.begin(), availableDBusServices.end(), commonApiAddress);

	ASSERT_TRUE(availableDBusServices.begin() != availableDBusServices.end());
	ASSERT_TRUE(found != availableDBusServices.end());
}

TEST_F(ProxyTest, isServiceInstanceAlive) {
    registerTestStub();

    bool isInstanceAlive = proxyDBusConnection_->getDBusServiceRegistry()->isServiceInstanceAlive(interfaceName, busName, objectPath);

    for (int i = 0; !isInstanceAlive && i < 10; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        isInstanceAlive = proxyDBusConnection_->getDBusServiceRegistry()->isServiceInstanceAlive(interfaceName, busName, objectPath);
    }

    EXPECT_TRUE(isInstanceAlive);
}

TEST_F(ProxyTest, IsAvailableBlocking) {
    registerTestStub();

    // blocking in terms of "if it's still uknown"
    bool isAvailable = proxy_->isAvailableBlocking();
    for (int i = 0; !isAvailable && i < 10; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        isAvailable = proxy_->isAvailableBlocking();
    }

    EXPECT_TRUE(isAvailable);
}

TEST_F(ProxyTest, HasNecessaryAttributesAndEvents) {
	CommonAPI::InterfaceVersionAttribute& versionAttribute = (proxy_->getInterfaceVersionAttribute());
	CommonAPI::ProxyStatusEvent& statusEvent = (proxy_->getProxyStatusEvent());
}

TEST_F(ProxyTest, IsConnected) {
  ASSERT_TRUE(proxy_->getDBusConnection()->isConnected());
}

TEST_F(ProxyTest, TestInterfaceVersionAttribute) {
	CommonAPI::InterfaceVersionAttribute& versionAttribute = proxy_->getInterfaceVersionAttribute();
	CommonAPI::Version version;
	CommonAPI::CallStatus status = versionAttribute.getValue(version);
	ASSERT_EQ(CommonAPI::CallStatus::NOT_AVAILABLE, status);
}

TEST_F(ProxyTest, AsyncCallbacksAreCalledIfServiceNotAvailable) {
    commonapi::tests::DerivedTypeCollection::TestEnumExtended2 testInputStruct;
    commonapi::tests::DerivedTypeCollection::TestMap testInputMap;
    bool wasCalled = false;
    proxy_->testDerivedTypeMethodAsync(testInputStruct, testInputMap, [&] (const CommonAPI::CallStatus& callStatus,
                                                                          const commonapi::tests::DerivedTypeCollection::TestEnumExtended2&,
                                                                          const commonapi::tests::DerivedTypeCollection::TestMap&) {
                    ASSERT_EQ(callStatus, CommonAPI::CallStatus::NOT_AVAILABLE);
                    wasCalled = true;
            }
    );
    sleep(1);
    ASSERT_TRUE(wasCalled);
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
