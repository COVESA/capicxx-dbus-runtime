/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <CommonAPI/Runtime.h>
#include <CommonAPI/Factory.h>
#include <CommonAPI/DBus/DBusServiceRegistry.h>
#include <CommonAPI/DBus/DBusConnection.h>

#include <commonapi/tests/TestInterfaceStub.h>
#include <commonapi/tests/TestInterfaceStubDefault.h>
#include <commonapi/tests/TestInterfaceDBusStubAdapter.h>

#include <gtest/gtest.h>

#include <iostream>


class DBusServiceRegistryTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
    }

	virtual void TearDown() {
	}
};


TEST_F(DBusServiceRegistryTest, CanBeConstructed) {
    CommonAPI::DBus::DBusServiceRegistry* registry = new CommonAPI::DBus::DBusServiceRegistry();
    ASSERT_TRUE(registry != NULL);
}


TEST_F(DBusServiceRegistryTest, DBusConnectionHasRegistry) {
    auto dbusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
    auto serviceRegistry = dbusConnection->getDBusServiceRegistry();
    ASSERT_FALSE(!serviceRegistry);
}

TEST_F(DBusServiceRegistryTest, ServiceStatusEventCanBeFetched) {
    auto dbusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
    auto dbusServiceRegistry = dbusConnection->getDBusServiceRegistry();
    dbusConnection->connect();
    ASSERT_NO_FATAL_FAILURE(auto serviceStatusEvent = dbusServiceRegistry->getServiceStatusEvent());
}


TEST_F(DBusServiceRegistryTest, ServiceStatusEventCanBeSubscribedTo) {
    auto dbusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
    auto dbusServiceRegistry = dbusConnection->getDBusServiceRegistry();
    dbusConnection->connect();
    auto serviceStatusEvent = dbusServiceRegistry->getServiceStatusEvent();
    std::string eventName_ = "local:some:address";
    serviceStatusEvent.subscribe(eventName_,
            [&] (const std::string& eventName, const CommonAPI::AvailabilityStatus& availabilityStatus) -> CommonAPI::SubscriptionStatus {
                return CommonAPI::SubscriptionStatus::RETAIN;
            }
    );
}


TEST_F(DBusServiceRegistryTest, ServiceStatusEventGetsFirstCall) {
    auto dbusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
    auto dbusServiceRegistry = dbusConnection->getDBusServiceRegistry();
    dbusConnection->connect();
    auto serviceStatusEvent = dbusServiceRegistry->getServiceStatusEvent();
    std::string eventName_ = "local:some:address";
    bool called = false;
    serviceStatusEvent.subscribe(eventName_,
            [&] (const std::string& eventName, const CommonAPI::AvailabilityStatus& availabilityStatus) -> CommonAPI::SubscriptionStatus {
                called = true;
                return CommonAPI::SubscriptionStatus::RETAIN;
            }
    );
    while(!called) {
        usleep(1);
    }
    ASSERT_TRUE(called);
}


TEST_F(DBusServiceRegistryTest, SubscriptionToServiceStatusEventCanBeCancelled) {
    auto dbusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
    auto dbusServiceRegistry = dbusConnection->getDBusServiceRegistry();
    dbusConnection->connect();
    auto serviceStatusEvent = dbusServiceRegistry->getServiceStatusEvent();
    std::string eventName_ = "local:some:address";
    serviceStatusEvent.subscribe(eventName_,
            [&] (const std::string& eventName, const CommonAPI::AvailabilityStatus& availabilityStatus) -> CommonAPI::SubscriptionStatus {
                return CommonAPI::SubscriptionStatus::CANCEL;
            }
    );
    sleep(1);
}


class DBusServiceRegistryTestWithPredefinedRemote: public ::testing::Test {
 protected:
    virtual void SetUp() {
        dbusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        dbusServiceRegistry_ = dbusConnection_->getDBusServiceRegistry();
        dbusConnection_->connect();

        dbusStubConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        dbusStubConnection_->connect();

        auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

        dbusStubConnection_->requestServiceNameAndBlock("test.instance.name");
        auto stubAdapter = std::make_shared<commonapi::tests::TestInterfaceDBusStubAdapter>(
                        "local:test.service.name:test.instance.name",
                        "test.service.name",
                        "test.instance.name",
                        "/test/instance/name",
                        dbusStubConnection_,
                        stub);
        stubAdapter->init();
    }

    virtual void TearDown() {
    }

    std::shared_ptr<CommonAPI::DBus::DBusConnection> dbusConnection_;
    std::shared_ptr<CommonAPI::DBus::DBusConnection> dbusStubConnection_;
    std::shared_ptr<CommonAPI::DBus::DBusServiceRegistry> dbusServiceRegistry_;
};


TEST_F(DBusServiceRegistryTestWithPredefinedRemote, RecognizesCommonAPIDBusServiceInstanceAsAlive) {
    ASSERT_TRUE(dbusServiceRegistry_->isServiceInstanceAlive("test.service.name", "test.instance.name", "/test/instance/name"));
}


TEST_F(DBusServiceRegistryTestWithPredefinedRemote, FindsCommonAPIDBusServiceInstance) {
    auto availableServices = dbusServiceRegistry_->getAvailableServiceInstances("test.service.name", "local");
    ASSERT_EQ(1, availableServices.size());
    bool serviceFound;
    for(auto it = availableServices.begin(); it != availableServices.end(); ++it) {
        if(*it == "local:test.service.name:test.instance.name") {
            serviceFound = true;
        }
    }
    ASSERT_TRUE(serviceFound);
}



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
