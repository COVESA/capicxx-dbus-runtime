/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef _GLIBCXX_USE_NANOSLEEP
#define _GLIBCXX_USE_NANOSLEEP
#endif

#include <CommonAPI/CommonAPI.h>

#define COMMONAPI_INTERNAL_COMPILATION
#include <CommonAPI/DBus/DBusObjectManagerStub.h>
#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusInputStream.h>
#include <CommonAPI/DBus/DBusFactory.h>
#include <CommonAPI/DBus/DBusRuntime.h>
#include <CommonAPI/DBus/DBusStubAdapter.h>
#include <CommonAPI/DBus/DBusServicePublisher.h>

#include <CommonAPI/ProxyManager.h>

#include "commonapi/tests/managed/RootInterfaceStubDefault.h"
#include "commonapi/tests/managed/LeafInterfaceStubDefault.h"

#include "commonapi/tests/managed/RootInterfaceDBusProxy.h"
#include "commonapi/tests/managed/LeafInterfaceProxy.h"


#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <memory>

static const std::string rootAddress = "local:commonapi.tests.managed.RootInterface:commonapi.tests.managed.RootInterface";
static const std::string leafInstance = "commonapi.tests.managed.RootInterface.LeafInterface";
static const std::string leafAddress = "local:commonapi.tests.managed.LeafInterface:" + leafInstance;

static const std::string dbusServiceName = "CommonAPI.DBus.DBusObjectManagerStubTest";
static const std::string& dbusObjectManagerStubPath = "/commonapi/dbus/test/DBusObjectManagerStub";
static const std::string& managedDBusObjectPathPrefix = "/commonapi/dbus/test/DBusObjectManagerStub/ManagedObject";


class DBusManagedTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
        leafStatus = CommonAPI::AvailabilityStatus::UNKNOWN;
        runtime = std::dynamic_pointer_cast<CommonAPI::DBus::DBusRuntime>(CommonAPI::Runtime::load());
        serviceFactory = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(runtime->createFactory());
        clientFactory = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(runtime->createFactory());

        proxyDBusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(proxyDBusConnection_->connect());

        stubDBusConnection_ = serviceFactory->getDbusConnection();
        ASSERT_TRUE(stubDBusConnection_->connect());
        ASSERT_TRUE(bool(stubDBusConnection_->getDBusObjectManager()));
        ASSERT_TRUE(stubDBusConnection_->requestServiceNameAndBlock(dbusServiceName));
    }

    virtual void TearDown() {
        runtime->getServicePublisher()->unregisterService(rootAddress);

        stubDBusConnection_->disconnect();
        stubDBusConnection_.reset();

        proxyDBusConnection_->disconnect();
        proxyDBusConnection_.reset();
    }

    std::shared_ptr<CommonAPI::DBus::DBusRuntime> runtime;
    std::shared_ptr<CommonAPI::DBus::DBusFactory> serviceFactory;
    std::shared_ptr<CommonAPI::DBus::DBusFactory> clientFactory;

    CommonAPI::AvailabilityStatus leafStatus;

    void getManagedObjects(const std::string& dbusObjectPath,
                           CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict& dbusObjectPathAndInterfacesDict) {
        auto dbusMessageCall = CommonAPI::DBus::DBusMessage::createMethodCall(
                        dbusServiceName,
                        dbusObjectPath,
                        CommonAPI::DBus::DBusObjectManagerStub::getInterfaceName(),
                        "GetManagedObjects");

        CommonAPI::DBus::DBusError dbusError;
        auto dbusMessageReply = proxyDBusConnection_->sendDBusMessageWithReplyAndBlock(dbusMessageCall, dbusError);

        ASSERT_FALSE(dbusError) << dbusError.getMessage();
        ASSERT_TRUE(dbusMessageReply.isMethodReturnType());
        ASSERT_TRUE(dbusMessageReply.hasSignature("a{oa{sa{sv}}}"));

        CommonAPI::DBus::DBusInputStream dbusInputStream(dbusMessageReply);

        dbusInputStream >> dbusObjectPathAndInterfacesDict;
        ASSERT_FALSE(dbusInputStream.hasError());
    }

    void getIntrospectionData(const std::string& dbusObjectPath, std::string& introspectionDataXml) {
        auto dbusMessageCall = CommonAPI::DBus::DBusMessage::createMethodCall(
                        dbusServiceName,
                        dbusObjectPath,
                        "org.freedesktop.DBus.Introspectable",
                        "Introspect");
        CommonAPI::DBus::DBusError dbusError;
        auto dbusMessageReply = proxyDBusConnection_->sendDBusMessageWithReplyAndBlock(dbusMessageCall, dbusError);

        ASSERT_FALSE(dbusError) << dbusError.getMessage();
        ASSERT_TRUE(dbusMessageReply.isMethodReturnType());
        ASSERT_TRUE(dbusMessageReply.hasSignature("s"));

        CommonAPI::DBus::DBusInputStream dbusInputStream(dbusMessageReply);

        dbusInputStream >> introspectionDataXml;
        ASSERT_FALSE(dbusInputStream.hasError());
    }

    std::shared_ptr<CommonAPI::DBus::DBusConnection> proxyDBusConnection_;
    std::shared_ptr<CommonAPI::DBus::DBusConnection> stubDBusConnection_;
 public:
    //Callback on singnal...
    void managedObjectSignalled(std::string address, CommonAPI::AvailabilityStatus status) {
        //ASSERT_EQ(address, leafAddress);
        leafStatus = status;
    }


};

TEST_F(DBusManagedTest, RegisterRoot) {
    auto rootStub = std::make_shared<commonapi::tests::managed::RootInterfaceStubDefault>();
    runtime->getServicePublisher()->registerService(rootStub, rootAddress, serviceFactory);

    CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;
    getManagedObjects("/", dbusObjectPathAndInterfacesDict);

    EXPECT_FALSE(dbusObjectPathAndInterfacesDict.empty());

    runtime->getServicePublisher()->unregisterService(rootAddress);
}


TEST_F(DBusManagedTest, RegisterLeafUnmanaged) {
    auto leafStub = std::make_shared<commonapi::tests::managed::LeafInterfaceStubDefault>();
    runtime->getServicePublisher()->registerService(leafStub, leafAddress, serviceFactory);

    CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;
    getManagedObjects("/", dbusObjectPathAndInterfacesDict);

    EXPECT_FALSE(dbusObjectPathAndInterfacesDict.empty());

    runtime->getServicePublisher()->unregisterService(leafAddress);
}

TEST_F(DBusManagedTest, RegisterLeafManaged) {
    const char* rootInterfaceName = "/commonapi/tests/managed/RootInterface";
    const char* rootObjectPath = "/commonapi/tests/managed/RootInterface";

    auto rootStub = std::make_shared<commonapi::tests::managed::RootInterfaceStubDefault>();
    runtime->getServicePublisher()->registerService(rootStub, rootAddress, serviceFactory);

    std::shared_ptr<commonapi::tests::managed::RootInterfaceDBusProxy> rootProxy = std::make_shared<
                    commonapi::tests::managed::RootInterfaceDBusProxy>(
                                    clientFactory,
                                    rootAddress,
                                    rootInterfaceName,
                                    dbusServiceName,
                                    rootObjectPath,
                                    proxyDBusConnection_
                    );
    rootProxy->init();

    for (uint32_t i = 0; !rootProxy->isAvailable() && i < 200; ++i) {
        usleep(10 * 1000);
    }
    ASSERT_TRUE(rootProxy->isAvailable());

    CommonAPI::ProxyManager& proxyManagerLeafInterface = rootProxy->getProxyManagerLeafInterface();
    proxyManagerLeafInterface.getInstanceAvailabilityStatusChangedEvent().subscribe(
                    std::bind(&DBusManagedTest::managedObjectSignalled,
                              this,
                              std::placeholders::_1,
                              std::placeholders::_2));

    auto leafStub = std::make_shared<commonapi::tests::managed::LeafInterfaceStubDefault>();

    bool reg = rootStub->registerManagedStubLeafInterface(leafStub, leafInstance);
    ASSERT_TRUE(reg);

    for (uint32_t i = 0; leafStatus != CommonAPI::AvailabilityStatus::AVAILABLE && i < 50; ++i) {
        usleep(1000);
    }
    ASSERT_TRUE(leafStatus == CommonAPI::AvailabilityStatus::AVAILABLE);

    CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;

    getManagedObjects("/", dbusObjectPathAndInterfacesDict);
    ASSERT_FALSE(dbusObjectPathAndInterfacesDict.empty());
    dbusObjectPathAndInterfacesDict.clear();
    getManagedObjects(rootInterfaceName, dbusObjectPathAndInterfacesDict);
    ASSERT_FALSE(dbusObjectPathAndInterfacesDict.empty());

    bool deregistered = rootStub->deregisterManagedStubLeafInterface(leafInstance);
    ASSERT_TRUE(deregistered);

    for (uint32_t i = 0; leafStatus != CommonAPI::AvailabilityStatus::NOT_AVAILABLE && i < 200; ++i) {
        usleep(10 * 1000);
    }
    ASSERT_TRUE(leafStatus == CommonAPI::AvailabilityStatus::NOT_AVAILABLE);

    dbusObjectPathAndInterfacesDict.clear();
    getManagedObjects(rootInterfaceName, dbusObjectPathAndInterfacesDict);
    ASSERT_TRUE(dbusObjectPathAndInterfacesDict.empty());

    runtime->getServicePublisher()->unregisterService(rootAddress);
    dbusObjectPathAndInterfacesDict.clear();
    getManagedObjects("/", dbusObjectPathAndInterfacesDict);
    ASSERT_TRUE(dbusObjectPathAndInterfacesDict.empty());
}

TEST_F(DBusManagedTest, RegisterLeafManagedAndCreateProxyForLeaf) {
    const char* rootInterfaceName = "/commonapi/tests/managed/RootInterface";
    const char* rootObjectPath = "/commonapi/tests/managed/RootInterface";

    auto rootStub = std::make_shared<commonapi::tests::managed::RootInterfaceStubDefault>();
    runtime->getServicePublisher()->registerService(rootStub, rootAddress, serviceFactory);

    std::shared_ptr<commonapi::tests::managed::RootInterfaceDBusProxy> rootProxy = std::make_shared<
                    commonapi::tests::managed::RootInterfaceDBusProxy>(
                                    clientFactory,
                                    rootAddress,
                                    rootInterfaceName,
                                    dbusServiceName,
                                    rootObjectPath,
                                    proxyDBusConnection_
                    );
    rootProxy->init();

    for (uint32_t i = 0; !rootProxy->isAvailable() && i < 200; ++i) {
        usleep(10 * 1000);
    }
    ASSERT_TRUE(rootProxy->isAvailable());

    CommonAPI::ProxyManager& proxyManagerLeafInterface = rootProxy->getProxyManagerLeafInterface();
    proxyManagerLeafInterface.getInstanceAvailabilityStatusChangedEvent().subscribe(std::bind(&DBusManagedTest::managedObjectSignalled, this, std::placeholders::_1, std::placeholders::_2));

    auto leafStub = std::make_shared<commonapi::tests::managed::LeafInterfaceStubDefault>();
    bool reg = rootStub->registerManagedStubLeafInterface(leafStub, leafInstance);
    ASSERT_TRUE(reg);

    sleep(2);

    auto leafProxy = proxyManagerLeafInterface.buildProxy<commonapi::tests::managed::LeafInterfaceProxy>(leafInstance);
    for (uint32_t i = 0; !leafProxy->isAvailable() && i < 500; ++i) {
        usleep(10 * 1000);
    }

    ASSERT_TRUE(leafProxy->isAvailable());

    CommonAPI::CallStatus callStatus;
    commonapi::tests::managed::LeafInterface::testLeafMethodError error;
    int outInt;
    std::string outString;
    leafProxy->testLeafMethod(42, "Test", callStatus, error, outInt, outString);

    ASSERT_TRUE(callStatus == CommonAPI::CallStatus::SUCCESS);

    bool dereg = rootStub->deregisterManagedStubLeafInterface(leafInstance);

    CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;
    dbusObjectPathAndInterfacesDict.clear();
    getManagedObjects(rootInterfaceName, dbusObjectPathAndInterfacesDict);
    ASSERT_TRUE(dbusObjectPathAndInterfacesDict.empty());

    runtime->getServicePublisher()->unregisterService(rootAddress);
    dbusObjectPathAndInterfacesDict.clear();
    getManagedObjects("/", dbusObjectPathAndInterfacesDict);
    ASSERT_TRUE(dbusObjectPathAndInterfacesDict.empty());
}

TEST_F(DBusManagedTest, PropagateTeardown) {
    const char* rootInterfaceName = "/commonapi/tests/managed/RootInterface";
    const char* rootObjectPath = "/commonapi/tests/managed/RootInterface";

    auto rootStub = std::make_shared<commonapi::tests::managed::RootInterfaceStubDefault>();
    runtime->getServicePublisher()->registerService(rootStub, rootAddress, serviceFactory);


    std::shared_ptr<commonapi::tests::managed::RootInterfaceDBusProxy> rootProxy = std::make_shared<
                    commonapi::tests::managed::RootInterfaceDBusProxy>(
                                    clientFactory,
                                    rootAddress,
                                    rootInterfaceName,
                                    dbusServiceName,
                                    rootObjectPath,
                                    proxyDBusConnection_
                    );
    rootProxy->init();

    for (uint32_t i = 0; !rootProxy->isAvailable() && i < 200; ++i) {
        usleep(10 * 1000);
    }
    ASSERT_TRUE(rootProxy->isAvailable());

    CommonAPI::ProxyManager& proxyManagerLeafInterface = rootProxy->getProxyManagerLeafInterface();
    proxyManagerLeafInterface.getInstanceAvailabilityStatusChangedEvent().subscribe(std::bind(&DBusManagedTest::managedObjectSignalled, this, std::placeholders::_1, std::placeholders::_2));

    auto leafStub = std::make_shared<commonapi::tests::managed::LeafInterfaceStubDefault>();
    bool reg = rootStub->registerManagedStubLeafInterface(leafStub, leafInstance);
    ASSERT_TRUE(reg);

    sleep(2);

    auto leafProxy = proxyManagerLeafInterface.buildProxy<commonapi::tests::managed::LeafInterfaceProxy>(leafInstance);

    for (uint32_t i = 0; !leafProxy->isAvailable() && i < 500; ++i) {
        usleep(10 * 1000);
    }

    ASSERT_TRUE(leafProxy->isAvailable());

    CommonAPI::CallStatus callStatus;
    commonapi::tests::managed::LeafInterface::testLeafMethodError error;
    int outInt;
    std::string outString;
    leafProxy->testLeafMethod(42, "Test", callStatus, error, outInt, outString);

    ASSERT_TRUE(callStatus == CommonAPI::CallStatus::SUCCESS);

    bool dereg = runtime->getServicePublisher()->unregisterService(rootAddress);
    ASSERT_TRUE(dereg);

    for (uint32_t i = 0; leafStatus != CommonAPI::AvailabilityStatus::NOT_AVAILABLE && i < 100; ++i) {
        usleep(10 * 1000);
    }
    ASSERT_TRUE(leafStatus == CommonAPI::AvailabilityStatus::NOT_AVAILABLE);

    CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;
    dbusObjectPathAndInterfacesDict.clear();
    getManagedObjects("/", dbusObjectPathAndInterfacesDict);
    ASSERT_TRUE(dbusObjectPathAndInterfacesDict.empty());
}

//XXX: Needs tests for invalid instances for the children.
//XXX: Also later on need auto generated instance ID test and
//XXX: Also check transitive teardown

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
