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
#include <stdint.h>
#include <vector>
#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>
#include <string>


class TestProxy: public CommonAPI::DBus::DBusProxy {
public:
  TestProxy(const std::shared_ptr<CommonAPI::DBus::DBusConnection>& dbusConnection);
  ~TestProxy() = default;

protected:
  void getOwnVersion(uint16_t& ownVersionMajor, uint16_t& ownVersionMinor) const;

};

class TestStubAdapter: public CommonAPI::DBus::DBusStubAdapter {
public:
    TestStubAdapter(const std::shared_ptr<CommonAPI::DBus::DBusConnection>& dbusConnection);
protected:
    bool onInterfaceDBusMessage(const CommonAPI::DBus::DBusMessage& dbusMessage);
    const char* getMethodsDBusIntrospectionXmlData() const;
};

const char* TestStubAdapter::getMethodsDBusIntrospectionXmlData() const {
    return "";
}

bool TestStubAdapter::onInterfaceDBusMessage(const CommonAPI::DBus::DBusMessage& dbusMessage) {
    return true;
}

TestStubAdapter::TestStubAdapter(const std::shared_ptr<CommonAPI::DBus::DBusConnection>& dbusConnection) :
        CommonAPI::DBus::DBusStubAdapter(
        "local:random.common:api.address",
        "com.bmw.test.Echo",
        "com.bmw.test.Echo",
        "/com/bmw/test/Echo",
        dbusConnection) {
}

TestProxy::TestProxy(const std::shared_ptr<CommonAPI::DBus::DBusConnection>& dbusConnection) :
		CommonAPI::DBus::DBusProxy(
		"local:random.common:api.address",
		"com.bmw.test.Echo",
        "com.bmw.test.Echo",
        "/com/bmw/test/Echo",
        dbusConnection) {
}

void TestProxy::getOwnVersion(uint16_t& ownVersionMajor, uint16_t& ownVersionMinor) const {
}


const static std::string ID = "com.bmw.test.Echo";


class ProxyTest: public ::testing::Test {
protected:

    virtual void TearDown() {
        dbusConnection_->disconnect();
    }

    void SetUp() {
        dbusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(dbusConnection_->connect());
        proxy_ = std::make_shared<TestProxy>(dbusConnection_);
    }

    std::shared_ptr<CommonAPI::DBus::DBusConnection> dbusConnection_;
    std::shared_ptr<TestProxy> proxy_;
};

TEST_F(ProxyTest, HasCorrectBusName) {
  std::string actualName = proxy_->getDBusBusName();
  EXPECT_EQ("com.bmw.test.Echo", actualName);
}

TEST_F(ProxyTest, HasCorrectObjectPath) {
  std::string actualPath = proxy_->getDBusObjectPath();
  EXPECT_EQ("/com/bmw/test/Echo", actualPath);
}

TEST_F(ProxyTest, HasCorrectInterfaceName) {
  std::string actualName = proxy_->getInterfaceName();
  EXPECT_EQ("com.bmw.test.Echo", actualName);
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
    std::shared_ptr<TestStubAdapter> stub_ = std::make_shared<TestStubAdapter>(dbusConnection_);
	stub_->init();
    dbusConnection_->requestServiceNameAndBlock(ID);

	std::vector<std::string> actuallyAvailableServices;


	actuallyAvailableServices = dbusConnection_->getDBusServiceRegistry()->getAvailableServiceInstances("com.bmw.test.Echo",
			"local");

	std::string toFind = "com.bmw.test.Echo";
	auto found = std::find(actuallyAvailableServices.begin(), actuallyAvailableServices.end(), toFind);

	ASSERT_TRUE(actuallyAvailableServices.begin() != actuallyAvailableServices.end());
}

TEST_F(ProxyTest, IsAvailableBlocking) {
    std::shared_ptr<TestStubAdapter> stub = std::make_shared<TestStubAdapter>(dbusConnection_);
    stub->init();
    bool registered = dbusConnection_->requestServiceNameAndBlock(ID);
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
