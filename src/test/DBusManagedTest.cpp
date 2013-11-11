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

#include "commonapi/tests/managed/RootInterfaceProxy.h"
#include "commonapi/tests/managed/RootInterfaceDBusProxy.h"
#include "commonapi/tests/managed/LeafInterfaceProxy.h"
#include "commonapi/tests/managed/SecondRootStubDefault.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <memory>

static const std::string rootAddress =
                "local:commonapi.tests.managed.RootInterface:commonapi.tests.managed.RootInterface";
static const std::string leafInstance = "commonapi.tests.managed.RootInterface.LeafInterface";
static const std::string secondLeafInstance = "commonapi.tests.managed.RootInterface.LeafInterface2";
static const std::string leafAddress = "local:commonapi.tests.managed.LeafInterface:" + leafInstance;

static const std::string dbusServiceName = "CommonAPI.DBus.DBusObjectManagerStubTest";

static const std::string rootInterfaceName = "/commonapi/tests/managed/RootInterface";
static const std::string rootObjectPath = "/commonapi/tests/managed/RootInterface";


const CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict getManagedObjects(const std::string& dbusObjectPath,
                                                                                                std::shared_ptr<CommonAPI::DBus::DBusConnection> connection) {
    auto dbusMessageCall = CommonAPI::DBus::DBusMessage::createMethodCall(
                    dbusServiceName,
                    dbusObjectPath,
                    CommonAPI::DBus::DBusObjectManagerStub::getInterfaceName(),
                    "GetManagedObjects");

    CommonAPI::DBus::DBusError dbusError;
    auto dbusMessageReply = connection->sendDBusMessageWithReplyAndBlock(dbusMessageCall, dbusError);

    CommonAPI::DBus::DBusInputStream dbusInputStream(dbusMessageReply);

    CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;
    dbusInputStream >> dbusObjectPathAndInterfacesDict;

    return dbusObjectPathAndInterfacesDict;
}


class DBusManagedTest: public ::testing::Test {
protected:
    virtual void SetUp() {
        leafStatus_ = CommonAPI::AvailabilityStatus::UNKNOWN;
        runtime_ = std::dynamic_pointer_cast<CommonAPI::DBus::DBusRuntime>(CommonAPI::Runtime::load());
        serviceFactory = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(runtime_->createFactory());
        clientFactory = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(runtime_->createFactory());

        proxyDBusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(proxyDBusConnection_->connect());

        stubDBusConnection_ = serviceFactory->getDbusConnection();
        ASSERT_TRUE(stubDBusConnection_->connect());
        ASSERT_TRUE(bool(stubDBusConnection_->getDBusObjectManager()));
        ASSERT_TRUE(stubDBusConnection_->requestServiceNameAndBlock(dbusServiceName));
    }

    virtual void TearDown() {
        runtime_->getServicePublisher()->unregisterService(rootAddress);

        stubDBusConnection_->disconnect();
        stubDBusConnection_.reset();

        proxyDBusConnection_->disconnect();
        proxyDBusConnection_.reset();
    }

    std::shared_ptr<CommonAPI::DBus::DBusRuntime> runtime_;
    std::shared_ptr<CommonAPI::DBus::DBusFactory> serviceFactory;
    std::shared_ptr<CommonAPI::DBus::DBusFactory> clientFactory;

    std::shared_ptr<CommonAPI::DBus::DBusConnection> proxyDBusConnection_;
    std::shared_ptr<CommonAPI::DBus::DBusConnection> stubDBusConnection_;

    CommonAPI::AvailabilityStatus leafStatus_;

public:
    void managedObjectSignalled(std::string address, CommonAPI::AvailabilityStatus status) {
        leafStatus_ = status;
    }
};

TEST_F(DBusManagedTest, RegisterRoot) {
    auto rootStub = std::make_shared<commonapi::tests::managed::RootInterfaceStubDefault>();
    runtime_->getServicePublisher()->registerService(rootStub, rootAddress, serviceFactory);

    auto dbusObjectPathAndInterfacesDict = getManagedObjects("/", proxyDBusConnection_);

    EXPECT_FALSE(dbusObjectPathAndInterfacesDict.empty());

    runtime_->getServicePublisher()->unregisterService(rootAddress);
}

TEST_F(DBusManagedTest, RegisterLeafUnmanaged) {
    auto leafStub = std::make_shared<commonapi::tests::managed::LeafInterfaceStubDefault>();
    runtime_->getServicePublisher()->registerService(leafStub, leafAddress, serviceFactory);

    auto dbusObjectPathAndInterfacesDict = getManagedObjects("/", proxyDBusConnection_);

    EXPECT_FALSE(dbusObjectPathAndInterfacesDict.empty());

    runtime_->getServicePublisher()->unregisterService(leafAddress);
}

TEST_F(DBusManagedTest, RegisterLeafManaged) {
    auto rootStub = std::make_shared<commonapi::tests::managed::RootInterfaceStubDefault>();
    runtime_->getServicePublisher()->registerService(rootStub, rootAddress, serviceFactory);

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

    for (uint32_t i = 0; leafStatus_ != CommonAPI::AvailabilityStatus::AVAILABLE && i < 50; ++i) {
        usleep(10 * 1000);
    }
    ASSERT_TRUE(leafStatus_ == CommonAPI::AvailabilityStatus::AVAILABLE);

    auto dbusObjectPathAndInterfacesDict = getManagedObjects("/", proxyDBusConnection_);
    ASSERT_FALSE(dbusObjectPathAndInterfacesDict.empty());
    dbusObjectPathAndInterfacesDict.clear();
    dbusObjectPathAndInterfacesDict = getManagedObjects(rootInterfaceName, proxyDBusConnection_);
    ASSERT_FALSE(dbusObjectPathAndInterfacesDict.empty());

    bool deregistered = rootStub->deregisterManagedStubLeafInterface(leafInstance);
    EXPECT_TRUE(deregistered);

    for (uint32_t i = 0; leafStatus_ != CommonAPI::AvailabilityStatus::NOT_AVAILABLE && i < 200; ++i) {
        usleep(10 * 1000);
    }
    EXPECT_TRUE(leafStatus_ == CommonAPI::AvailabilityStatus::NOT_AVAILABLE);

    dbusObjectPathAndInterfacesDict = getManagedObjects(rootInterfaceName, proxyDBusConnection_);
    EXPECT_TRUE(dbusObjectPathAndInterfacesDict.empty());

    runtime_->getServicePublisher()->unregisterService(rootAddress);
    dbusObjectPathAndInterfacesDict = getManagedObjects("/", proxyDBusConnection_);
    EXPECT_TRUE(dbusObjectPathAndInterfacesDict.empty());
}

TEST_F(DBusManagedTest, RegisterLeafManagedAndCreateProxyForLeaf) {
    auto rootStub = std::make_shared<commonapi::tests::managed::RootInterfaceStubDefault>();
    bool success = runtime_->getServicePublisher()->registerService(rootStub, rootAddress, serviceFactory);
    ASSERT_TRUE(success);

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
                    std::bind(
                                    &DBusManagedTest::managedObjectSignalled,
                                    this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));

    auto leafStub = std::make_shared<commonapi::tests::managed::LeafInterfaceStubDefault>();
    success = rootStub->registerManagedStubLeafInterface(leafStub, leafInstance);
    ASSERT_TRUE(success);

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

    EXPECT_TRUE(callStatus == CommonAPI::CallStatus::SUCCESS);

    success = rootStub->deregisterManagedStubLeafInterface(leafInstance);
    EXPECT_TRUE(success);

    auto dbusObjectPathAndInterfacesDict = getManagedObjects(rootInterfaceName, proxyDBusConnection_);
    EXPECT_TRUE(dbusObjectPathAndInterfacesDict.empty());

    success = runtime_->getServicePublisher()->unregisterService(rootAddress);
    EXPECT_TRUE(success);

    dbusObjectPathAndInterfacesDict = getManagedObjects("/", proxyDBusConnection_);
    EXPECT_TRUE(dbusObjectPathAndInterfacesDict.empty());
}

TEST_F(DBusManagedTest, PropagateTeardown) {
    auto rootStub = std::make_shared<commonapi::tests::managed::RootInterfaceStubDefault>();
    bool success = runtime_->getServicePublisher()->registerService(rootStub, rootAddress, serviceFactory);

    ASSERT_TRUE(success);

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
                    std::bind(
                                    &DBusManagedTest::managedObjectSignalled,
                                    this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));

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

    bool dereg = runtime_->getServicePublisher()->unregisterService(rootAddress);
    ASSERT_TRUE(dereg);

    for (uint32_t i = 0; leafStatus_ != CommonAPI::AvailabilityStatus::NOT_AVAILABLE && i < 100; ++i) {
        usleep(10 * 1000);
    }
    ASSERT_TRUE(leafStatus_ == CommonAPI::AvailabilityStatus::NOT_AVAILABLE);

    auto dbusObjectPathAndInterfacesDict = getManagedObjects("/", proxyDBusConnection_);
    ASSERT_TRUE(dbusObjectPathAndInterfacesDict.empty());
}

class DBusManagedTestExtended: public ::testing::Test {
protected:
    virtual void SetUp() {
        runtime_ = std::dynamic_pointer_cast<CommonAPI::DBus::DBusRuntime>(CommonAPI::Runtime::load());
        serviceFactory_ = runtime_->createFactory();
        clientFactory_ = runtime_->createFactory();

        manualTestDBusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(manualTestDBusConnection_->connect());

        auto dbusServiceFactory = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(serviceFactory_);
        auto stubDBusConnection = dbusServiceFactory->getDbusConnection();
        ASSERT_TRUE(stubDBusConnection->connect());
        ASSERT_TRUE(stubDBusConnection->requestServiceNameAndBlock(dbusServiceName));
    }

    virtual void TearDown() {
        for (auto it: rootStubs_) {
            runtime_->getServicePublisher()->unregisterService(it.first);
        }

        rootStubs_.clear();
        rootProxies_.clear();

        auto dbusServiceFactory = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(serviceFactory_);
        auto stubDBusConnection = dbusServiceFactory->getDbusConnection();
        ASSERT_TRUE(stubDBusConnection->releaseServiceName(dbusServiceName));

        usleep(50000);
    }

    inline const std::string getSuffixedRootAddress(const std::string& suffix) {
        return "local:commonapi.tests.managed.RootInterface" + suffix + ":commonapi.tests.managed.RootInterface";
    }

    inline const std::string getSuffixedSecondRootAddress(const std::string& suffix) {
        return "local:commonapi.tests.managed.SecondRoot" + suffix + ":commonapi.tests.managed.SecondRoot";
    }

    inline const bool registerRootStubForSuffix(const std::string& suffix) {
        std::shared_ptr<commonapi::tests::managed::RootInterfaceStubDefault> rootStub = std::make_shared<
                        commonapi::tests::managed::RootInterfaceStubDefault>();
        const std::string rootAddress = getSuffixedRootAddress(suffix);
        rootStubs_.insert( {rootAddress, rootStub} );
        return runtime_->getServicePublisher()->registerService(rootStub, rootAddress, serviceFactory_);
    }

    inline void createRootProxyForSuffix(const std::string& suffix) {
        rootProxies_.push_back(clientFactory_->buildProxy<commonapi::tests::managed::RootInterfaceProxy>(getSuffixedRootAddress(suffix)));
    }

    template<typename _ProxyType>
    bool waitForAllProxiesToBeAvailable(const std::vector<_ProxyType>& dbusProxies) {
        bool allAreAvailable = false;
        for (size_t i = 0; i < 500 && !allAreAvailable; i++) {
            allAreAvailable = std::all_of(
                    dbusProxies.begin(),
                    dbusProxies.end(),
                    [](const _ProxyType& proxy) {
                        return proxy->isAvailable();
                    });
            usleep(10 * 1000);
        }
        return allAreAvailable;
    }

    bool registerXLeafStubsForAllRoots(uint32_t x, bool doSubscriptionsForLeafNotifications) {
        bool success = true;
        bool expectedValueForRegistration = true;
        for (auto rootStubIterator: rootStubs_) {
            if (doSubscriptionsForLeafNotifications) {
                // CommonAPI::ProxyManager& proxyManagerForLeafInterface = rootManagerProxyArray[i]->getProxyManagerLeafInterface();
                // auto subscription = subscribeWithAvailabilityFlag(proxyManagerForLeafInterface, stati[i]);
                // subscriptions.push_back(subscription);
            }
            for (uint32_t i = 0; i < x; i++) {
                std::shared_ptr<commonapi::tests::managed::LeafInterfaceStubDefault> leafStub = std::make_shared<
                                commonapi::tests::managed::LeafInterfaceStubDefault>();
                success &= (rootStubIterator.second->registerManagedStubLeafInterfaceAutoInstance(leafStub) == expectedValueForRegistration);
            }
            //As all root interfaces are registered on the same object path, the leaf interfaces also will be registered with
            //this same object path as base. The first set of leaf interfaces therefore will be registered successfully.
            //The second and third set of leaf interfaces use a different object manager that does not know about the already
            //registered leaf interfaces, and therefore tries to register the same names on the same object paths again.
            //As of now, this will fail.
            expectedValueForRegistration = false;
        }
        return success;
    }

    void createXLeafProxiesForAllExistingLeafs() {
        for (auto rootProxyIterator: rootProxies_) {
            std::vector<std::shared_ptr<commonapi::tests::managed::LeafInterfaceProxy<>>> leafProxiesForRootX;

            CommonAPI::ProxyManager& leafProxyManager = rootProxyIterator->getProxyManagerLeafInterface();
            std::vector<std::string> availableInstances;
            CommonAPI::CallStatus status;
            leafProxyManager.getAvailableInstances(status, availableInstances);

            for (const std::string& availableInstance: availableInstances) {
                auto newLeafProxy = leafProxyManager.buildProxy<commonapi::tests::managed::LeafInterfaceProxy>(availableInstance);
                leafProxiesForRootX.push_back(newLeafProxy);
            }
            leafProxies_.push_back(std::move(leafProxiesForRootX));
        }
    }

    std::shared_ptr<CommonAPI::DBus::DBusRuntime> runtime_;
    std::shared_ptr<CommonAPI::Factory> serviceFactory_;
    std::shared_ptr<CommonAPI::Factory> clientFactory_;

    std::shared_ptr<CommonAPI::DBus::DBusConnection> manualTestDBusConnection_;

    std::unordered_map<std::string, std::shared_ptr<commonapi::tests::managed::RootInterfaceStubDefault>> rootStubs_;
    std::vector<std::shared_ptr<commonapi::tests::managed::RootInterfaceProxy<>>> rootProxies_;
    std::vector<std::vector<std::shared_ptr<commonapi::tests::managed::LeafInterfaceProxy<>>>> leafProxies_;
};


TEST_F(DBusManagedTestExtended, RegisterSeveralRootsOnSameObjectPath) {
    ASSERT_TRUE(registerRootStubForSuffix("One"));
    ASSERT_TRUE(registerRootStubForSuffix("Two"));
    ASSERT_TRUE(registerRootStubForSuffix("Three"));

    auto dbusObjectPathAndInterfacesDict = getManagedObjects("/", manualTestDBusConnection_);
    EXPECT_FALSE(dbusObjectPathAndInterfacesDict.empty());
}


TEST_F(DBusManagedTestExtended, RegisterSeveralRootsOnSameObjectPathAndCommunicate) {
    ASSERT_TRUE(registerRootStubForSuffix("One"));
    ASSERT_TRUE(registerRootStubForSuffix("Two"));
    ASSERT_TRUE(registerRootStubForSuffix("Three"));

    auto dbusObjectPathAndInterfacesDict = getManagedObjects("/", manualTestDBusConnection_);
    EXPECT_FALSE(dbusObjectPathAndInterfacesDict.empty());

    createRootProxyForSuffix("One");
    createRootProxyForSuffix("Two");
    createRootProxyForSuffix("Three");

    bool allRootProxiesAreAvailable = waitForAllProxiesToBeAvailable(rootProxies_);
    ASSERT_TRUE(allRootProxiesAreAvailable);

    CommonAPI::CallStatus callStatus;
    commonapi::tests::managed::RootInterface::testRootMethodError applicationError;
    int32_t outInt;
    std::string outString;

    for (size_t i = 0; i < rootProxies_.size(); ++i) {
        rootProxies_[i]->testRootMethod(
                            -5,
                            std::string("More Cars"),
                            callStatus,
                            applicationError,
                            outInt,
                            outString);
        EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus);
    }
}

TEST_F(DBusManagedTestExtended, RegisterSeveralRootsAndSeveralLeafsForEachOnSameObjectPath) {
    ASSERT_TRUE(registerRootStubForSuffix("One"));
    ASSERT_TRUE(registerRootStubForSuffix("Two"));
    ASSERT_TRUE(registerRootStubForSuffix("Three"));

    bool allLeafStubsWereRegistered = registerXLeafStubsForAllRoots(5, false);
    ASSERT_TRUE(allLeafStubsWereRegistered);

    auto dbusObjectPathAndInterfacesDict = getManagedObjects("/", manualTestDBusConnection_);
    EXPECT_FALSE(dbusObjectPathAndInterfacesDict.empty());
}

TEST_F(DBusManagedTestExtended, RegisterSeveralRootsAndSeveralLeafsForEachOnSameObjectPathAndCommunicate) {
    ASSERT_TRUE(registerRootStubForSuffix("One"));
    ASSERT_TRUE(registerRootStubForSuffix("Two"));
    ASSERT_TRUE(registerRootStubForSuffix("Three"));

    bool allLeafStubsWereRegistered = registerXLeafStubsForAllRoots(5, false);
    ASSERT_TRUE(allLeafStubsWereRegistered);

    auto dbusObjectPathAndInterfacesDict = getManagedObjects("/", manualTestDBusConnection_);
    EXPECT_FALSE(dbusObjectPathAndInterfacesDict.empty());

    createRootProxyForSuffix("One");
    createRootProxyForSuffix("Two");
    createRootProxyForSuffix("Three");

    //Check on existence of leaf-stubs

    bool allRootProxiesAreAvailable = waitForAllProxiesToBeAvailable(rootProxies_);
    ASSERT_TRUE(allRootProxiesAreAvailable);

    usleep(500 * 1000);

    createXLeafProxiesForAllExistingLeafs();

    usleep(500 * 1000);

    ASSERT_EQ(3, leafProxies_.size());

    uint32_t runNr = 1;
    for (const auto& leafProxiesForCurrentRoot: leafProxies_) {
        ASSERT_EQ(5, leafProxiesForCurrentRoot.size()) << "in run #" << runNr++;

        bool allLeafProxiesForCurrentRootAreAvailable = waitForAllProxiesToBeAvailable(leafProxiesForCurrentRoot);
        EXPECT_TRUE(allLeafProxiesForCurrentRootAreAvailable);

        ++runNr;
    }

//    CommonAPI::AvailabilityStatus stati[3];
//    std::vector<CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent::Subscription> subscriptions;

    //    auto leafStub = std::make_shared<commonapi::tests::managed::LeafInterfaceStubDefault>();
    //    bool success = rootStub->registerManagedStubLeafInterface(leafStub, leafInstance);
    //    ASSERT_TRUE(success);
    //
    //    sleep(2);
    //
    //    auto leafProxy = proxyManagerLeafInterface.buildProxy<commonapi::tests::managed::LeafInterfaceProxy>(leafInstance);
    //    for (uint32_t i = 0; !leafProxy->isAvailable() && i < 500; ++i) {
    //        usleep(10 * 1000);
    //    }
    //
    //    ASSERT_TRUE(leafProxy->isAvailable());
    //
    //    CommonAPI::CallStatus callStatus;
    //    commonapi::tests::managed::LeafInterface::testLeafMethodError error;
    //    int outInt;
    //    std::string outString;
    //    leafProxy->testLeafMethod(42, "Test", callStatus, error, outInt, outString);
    //
    //    EXPECT_TRUE(callStatus == CommonAPI::CallStatus::SUCCESS);
    //
    //    success = rootStub->deregisterManagedStubLeafInterface(leafInstance);
    //    EXPECT_TRUE(success);
    //
    //    CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;
    //    dbusObjectPathAndInterfacesDict.clear();
    //    getManagedObjects(rootInterfaceName, dbusObjectPathAndInterfacesDict);
    //    EXPECT_TRUE(dbusObjectPathAndInterfacesDict.empty());
    //
    //    success = runtime->getServicePublisher()->unregisterService(rootAddress);
    //    EXPECT_TRUE(success);
    //
    //    dbusObjectPathAndInterfacesDict.clear();
    //    getManagedObjects("/", dbusObjectPathAndInterfacesDict);
    //    EXPECT_TRUE(dbusObjectPathAndInterfacesDict.empty());
}

TEST_F(DBusManagedTestExtended, RegisterTwoRootsForSameLeafInterface) {
    ASSERT_TRUE(registerRootStubForSuffix("One"));

    std::shared_ptr<commonapi::tests::managed::SecondRootStubDefault> secondRootStub = std::make_shared<
                    commonapi::tests::managed::SecondRootStubDefault>();
    const std::string rootAddress = getSuffixedRootAddress("Two");
    runtime_->getServicePublisher()->registerService(secondRootStub, rootAddress, serviceFactory_);


    auto leafStub1 = std::make_shared<commonapi::tests::managed::LeafInterfaceStubDefault>();
    auto leafStub2 = std::make_shared<commonapi::tests::managed::LeafInterfaceStubDefault>();

    bool leafStub1Registered = rootStubs_.begin()->second->registerManagedStubLeafInterface(leafStub1, leafInstance);
    ASSERT_TRUE(leafStub1Registered);

    bool leafStub2Registered = secondRootStub->registerManagedStubLeafInterface(leafStub2, secondLeafInstance);
    ASSERT_TRUE(leafStub2Registered);
}

//XXX: Needs tests for invalid instances for the children.
//XXX: Also later on need auto generated instance ID test and
//XXX: If stub cannot manage, ensure the objectManager interface does not appear on dbus
//XXX: Tests if configuration file is present

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
