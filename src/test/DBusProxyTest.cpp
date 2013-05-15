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

#include <commonapi/tests/TestInterfaceDBusProxy.h>
#include <commonapi/tests/TestInterfaceDBusStubAdapter.h>
#include <commonapi/tests/TestInterfaceStubDefault.h>

#include "DBusTestUtils.h"

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

    virtual void TearDown() {
        usleep(30000);
    }

    void registerTestStub() {
        stubDBusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(stubDBusConnection_->connect());

        auto stubDefault = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
        stubAdapter_ = std::make_shared<commonapi::tests::TestInterfaceDBusStubAdapter>(
                        commonApiAddress,
                        interfaceName,
                        busName,
                        objectPath,
                        stubDBusConnection_,
                        stubDefault);
        stubAdapter_->init();

        bool serviceNameAcquired = stubDBusConnection_->requestServiceNameAndBlock(busName);

        for(unsigned int i = 0; !serviceNameAcquired && i < 100; i++) {
            usleep(10000);
            serviceNameAcquired = stubDBusConnection_->requestServiceNameAndBlock(busName);
        }
        ASSERT_TRUE(serviceNameAcquired);
        usleep(500000);
    }

    void deregisterTestStub() {
    	stubAdapter_->deinit();
    	stubAdapter_.reset();

		if (stubDBusConnection_->isConnected()) {
			stubDBusConnection_->disconnect();
		}
		stubDBusConnection_.reset();
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
            if (proxyAvailabilityStatus_ == availabilityStatus)
                return true;
            usleep(100000);
        }

        return false;
    }

    std::shared_ptr<CommonAPI::DBus::DBusConnection> proxyDBusConnection_;
    std::shared_ptr<commonapi::tests::TestInterfaceDBusProxy> proxy_;
    CommonAPI::AvailabilityStatus proxyAvailabilityStatus_;

    CommonAPI::ProxyStatusEvent::Subscription proxyStatusSubscription_;

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

    stubDBusConnection_->disconnect();

    EXPECT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::NOT_AVAILABLE));

    deregisterTestStub();
    proxyDeregisterForAvailabilityStatus();
}

TEST_F(ProxyTest, DBusProxyStatusEventAfterServiceIsRegistered) {
    proxyDBusConnection_->disconnect();

    registerTestStub();

    EXPECT_TRUE(proxyDBusConnection_->connect());

    proxyRegisterForAvailabilityStatus();

    EXPECT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::AVAILABLE));

    stubDBusConnection_->disconnect();

    EXPECT_TRUE(proxyWaitForAvailabilityStatus(CommonAPI::AvailabilityStatus::NOT_AVAILABLE));

    deregisterTestStub();
    proxyDeregisterForAvailabilityStatus();
}

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
	CommonAPI::InterfaceVersionAttribute& versionAttribute = (proxy_->getInterfaceVersionAttribute());
	CommonAPI::ProxyStatusEvent& statusEvent = (proxy_->getProxyStatusEvent());
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
    proxy_->testDerivedTypeMethodAsync(testInputStruct, testInputMap, [&] (const CommonAPI::CallStatus& callStatus,
                                                                          const commonapi::tests::DerivedTypeCollection::TestEnumExtended2&,
                                                                          const commonapi::tests::DerivedTypeCollection::TestMap&) {
                    ASSERT_EQ(callStatus, CommonAPI::CallStatus::NOT_AVAILABLE);
                    wasCalledPromise.set_value(true);
            }
    );
    ASSERT_TRUE(wasCalledFuture.get());
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
