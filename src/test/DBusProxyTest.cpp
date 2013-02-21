/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <dbus/dbus.h>
#include <CommonAPI/DBus/DBusInputStream.h>
#include <CommonAPI/DBus/DBusMessage.h>
#include <CommonAPI/DBus/DBusProxy.h>
#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusStubAdapter.h>
#include <CommonAPI/DBus/DBusUtils.h>

#include <commonapi/tests/TestInterfaceDBusProxy.h>
#include <commonapi/tests/TestInterfaceDBusStubAdapter.h>
#include <commonapi/tests/TestInterfaceStubDefault.h>

#include <stdint.h>
#include <vector>
#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>
#include <string>


static const std::string commonApiAddress = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService";
static const std::string commonApiServiceName = "CommonAPI.DBus.tests.DBusProxyTest";
static const std::string interfaceName = "CommonAPI.DBus.tests.DBusProxyTestInterface";
static const std::string busName = "CommonAPI.DBus.tests.DBusProxyTestService";
static const std::string objectPath = "/CommonAPI/DBus/tests/DBusProxyTest/TestObject";


class ProxyTest: public ::testing::Test {
protected:

    virtual void TearDown() {
        proxyDBusConnection_->disconnect();

        stubAdapter_.reset();
        stubDBusConnection_.reset();
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

    bool proxyWaitForServiceAvailabilityStatus(CommonAPI::AvailabilityStatus& availabilityStatus) {
        std::promise<CommonAPI::AvailabilityStatus> availabilityStatusPromise;

        auto subscription = proxy_->getProxyStatusEvent().subscribe([&](const CommonAPI::AvailabilityStatus& availabilityStatus) {
            std::cout << "Got availabilityStatus" << (int) availabilityStatus << std::endl;
            availabilityStatusPromise.set_value(availabilityStatus);
        });

        auto availabilityStatusFuture = availabilityStatusPromise.get_future();
        auto waitStatus = availabilityStatusFuture.wait_for(std::chrono::milliseconds(1000));
        const bool availabilityStatusFutureReady = CommonAPI::DBus::checkReady(waitStatus);

        proxy_->getProxyStatusEvent().unsubscribe(subscription);

        if (availabilityStatusFutureReady)
            availabilityStatus = availabilityStatusFuture.get();

        return availabilityStatusFutureReady;
    }

    std::shared_ptr<CommonAPI::DBus::DBusConnection> proxyDBusConnection_;
    std::shared_ptr<commonapi::tests::TestInterfaceDBusProxy> proxy_;

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
    proxyDBusConnection_->disconnect();

    registerTestStub();

    proxyDBusConnection_->connect();

    CommonAPI::AvailabilityStatus availabilityStatus;
    const bool availabilityStatusSet = proxyWaitForServiceAvailabilityStatus(availabilityStatus);

    ASSERT_TRUE(availabilityStatusSet);
    ASSERT_EQ(availabilityStatus, CommonAPI::AvailabilityStatus::AVAILABLE);
}

TEST_F(ProxyTest, ServiceStatus) {
    proxyDBusConnection_->requestServiceNameAndBlock(busName);

    std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stubDefault = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
    std::shared_ptr<commonapi::tests::TestInterfaceDBusStubAdapter> stubAdapter =  std::make_shared<commonapi::tests::TestInterfaceDBusStubAdapter>(
                    commonApiAddress,
                    interfaceName,
                    busName,
                    objectPath,
                    proxyDBusConnection_,
                    stubDefault);

    stubAdapter->init();

    auto testConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
    testConnection->connect();

	std::vector<std::string> actuallyAvailableServices;
	actuallyAvailableServices = testConnection->getDBusServiceRegistry()->getAvailableServiceInstances(commonApiServiceName,
			"local");

	auto found = std::find(actuallyAvailableServices.begin(), actuallyAvailableServices.end(), commonApiAddress);

	ASSERT_TRUE(actuallyAvailableServices.begin() != actuallyAvailableServices.end());
	ASSERT_TRUE(found != actuallyAvailableServices.end());

	testConnection->disconnect();
}

TEST_F(ProxyTest, IsAvailableBlocking) {
    std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stubDefault = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
    std::shared_ptr<commonapi::tests::TestInterfaceDBusStubAdapter> stubAdapter =  std::make_shared<commonapi::tests::TestInterfaceDBusStubAdapter>(
                    commonApiAddress,
                    interfaceName,
                    busName,
                    objectPath,
                    proxyDBusConnection_,
                    stubDefault);

    stubAdapter->init();

    bool registered = proxyDBusConnection_->requestServiceNameAndBlock(busName);
    bool isAvailable = proxy_->isAvailableBlocking();
    EXPECT_EQ(registered, isAvailable);
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

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
