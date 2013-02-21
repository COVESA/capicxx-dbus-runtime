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

#include <commonapi/tests/TestInterfaceDBusProxy.h>
#include <commonapi/tests/TestInterfaceDBusStubAdapter.h>
#include <commonapi/tests/TestInterfaceStubDefault.h>

#include <stdint.h>
#include <vector>
#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>
#include <string>


const static std::string commonApiAddress = "local:com.bmw.test.Echo:com.bmw.test";
const static std::string commonApiServiceName = "com.bmw.test.Echo";
const static std::string interfaceName = "com.bmw.test.Echo";
const static std::string connectionName = "com.bmw.test";
const static std::string objectPath = "/com/bmw/test";


class ProxyTest: public ::testing::Test {
protected:

    virtual void TearDown() {
        dbusConnection_->disconnect();
    }

    void SetUp() {
        dbusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(dbusConnection_->connect());

        proxy_ = std::make_shared<commonapi::tests::TestInterfaceDBusProxy>(
                        commonApiAddress,
                        interfaceName,
                        connectionName,
                        objectPath,
                        dbusConnection_);
    }

    std::shared_ptr<CommonAPI::DBus::DBusConnection> dbusConnection_;
    std::shared_ptr<commonapi::tests::TestInterfaceDBusProxy> proxy_;
};

TEST_F(ProxyTest, HasCorrectConnectionName) {
  std::string actualName = proxy_->getDBusBusName();
  EXPECT_EQ(connectionName, actualName);
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

TEST_F(ProxyTest, ServiceStatus) {
    dbusConnection_->requestServiceNameAndBlock(connectionName);

    std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stubDefault = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
    std::shared_ptr<commonapi::tests::TestInterfaceDBusStubAdapter> stubAdapter =  std::make_shared<commonapi::tests::TestInterfaceDBusStubAdapter>(
                    commonApiAddress,
                    interfaceName,
                    connectionName,
                    objectPath,
                    dbusConnection_,
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
                    connectionName,
                    objectPath,
                    dbusConnection_,
                    stubDefault);

    stubAdapter->init();

    bool registered = dbusConnection_->requestServiceNameAndBlock(connectionName);
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
