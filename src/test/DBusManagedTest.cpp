// Copyright (C) 2013-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef _GLIBCXX_USE_NANOSLEEP
#define _GLIBCXX_USE_NANOSLEEP
#endif

#include <CommonAPI/CommonAPI.hpp>

#define COMMONAPI_INTERNAL_COMPILATION
#include <CommonAPI/DBus/DBusObjectManagerStub.hpp>
#include <CommonAPI/DBus/DBusConnection.hpp>
#include <CommonAPI/DBus/DBusInputStream.hpp>
#include <CommonAPI/DBus/DBusFactory.hpp>
#include <CommonAPI/DBus/DBusStubAdapter.hpp>

#include <CommonAPI/ProxyManager.hpp>

#include "v1_0/commonapi/tests/managed/RootInterfaceStubDefault.hpp"
#include "v1_0/commonapi/tests/managed/LeafInterfaceStubDefault.hpp"
#include "v1_0/commonapi/tests/managed/BranchInterfaceStubDefault.hpp"

#include "v1_0/commonapi/tests/managed/RootInterfaceProxy.hpp"
#include "v1_0/commonapi/tests/managed/RootInterfaceDBusProxy.hpp"
#include "v1_0/commonapi/tests/managed/LeafInterfaceProxy.hpp"
#include "v1_0/commonapi/tests/managed/SecondRootStubDefault.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <memory>

#define VERSION v1_0

static const std::string domain = "local";
static const std::string rootAddress = "commonapi.tests.managed.RootInterface";
static const std::string leafInstance = "commonapi.tests.managed.RootInterface.LeafInterface";
static const std::string branchInstance = "commonapi.tests.managed.RootInterface.BranchInterface";
static const std::string secondLeafInstance = "commonapi.tests.managed.RootInterface.LeafInterface2";
static const std::string leafAddress = "local:commonapi.tests.managed.LeafInterface:" + leafInstance;
static const std::string branchAddress = "local:commonapi.tests.managed.BranchInterface:" + branchInstance;

static const std::string dbusServiceName = "CommonAPI.DBus.DBusObjectManagerStubTest";

static const std::string rootInterfaceName = "/commonapi/tests/managed/RootInterface";
static const std::string rootObjectPath = "/commonapi/tests/managed/RootInterface";


const CommonAPI::DBus::DBusObjectManagerStub::DBusObjectPathAndInterfacesDict getManagedObjects(const std::string& dbusObjectPath,
                                                                                                std::shared_ptr<CommonAPI::DBus::DBusConnection> connection) {
    auto dbusMessageCall = CommonAPI::DBus::DBusMessage::createMethodCall(
                    CommonAPI::DBus::DBusAddress(dbusServiceName, dbusObjectPath, CommonAPI::DBus::DBusObjectManagerStub::getInterfaceName()),
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
        runtime_ = CommonAPI::Runtime::get();

        proxyDBusConnection_ = CommonAPI::DBus::DBusConnection::getBus(CommonAPI::DBus::DBusType_t::SESSION);
        ASSERT_TRUE(proxyDBusConnection_->connect());
		stubDBusConnection_ = CommonAPI::DBus::DBusConnection::getBus(CommonAPI::DBus::DBusType_t::SESSION);
        ASSERT_TRUE(stubDBusConnection_->connect());
        ASSERT_TRUE(bool(stubDBusConnection_->getDBusObjectManager()));
        ASSERT_TRUE(stubDBusConnection_->requestServiceNameAndBlock(dbusServiceName));
    }

    virtual void TearDown() {
        //runtime_->unregisterService(rootAddress);

        stubDBusConnection_->disconnect();
        stubDBusConnection_.reset();

        proxyDBusConnection_->disconnect();
        proxyDBusConnection_.reset();
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<CommonAPI::DBus::DBusConnection> proxyDBusConnection_;
    std::shared_ptr<CommonAPI::DBus::DBusConnection> stubDBusConnection_;

    CommonAPI::AvailabilityStatus leafStatus_;

public:
    void managedObjectSignalled(std::string address, CommonAPI::AvailabilityStatus status) {
        leafStatus_ = status;
    }
};

TEST_F(DBusManagedTest, RegisterRoot) {
    auto rootStub = std::make_shared<VERSION::commonapi::tests::managed::RootInterfaceStubDefault>();
	runtime_->registerService(domain, rootAddress, rootStub);

    auto dbusObjectPathAndInterfacesDict = getManagedObjects("/", proxyDBusConnection_);

    EXPECT_FALSE(dbusObjectPathAndInterfacesDict.empty());

    //runtime_->unregisterService(rootAddress);
}

TEST_F(DBusManagedTest, RegisterLeafUnmanaged) {
	auto leafStub = std::make_shared<VERSION::commonapi::tests::managed::LeafInterfaceStubDefault>();
	runtime_->registerService(domain, leafAddress, leafStub);

    auto dbusObjectPathAndInterfacesDict = getManagedObjects("/", proxyDBusConnection_);

    EXPECT_FALSE(dbusObjectPathAndInterfacesDict.empty());

    //runtime_->unregisterService(leafAddress);
}

TEST_F(DBusManagedTest, RegisterLeafManaged) {
	auto rootStub = std::make_shared<VERSION::commonapi::tests::managed::RootInterfaceStubDefault>();
	runtime_->registerService(domain, rootAddress, rootStub);

	std::shared_ptr<VERSION::commonapi::tests::managed::RootInterfaceDBusProxy> rootProxy = std::make_shared<
		VERSION::commonapi::tests::managed::RootInterfaceDBusProxy>(
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

	auto leafStub = std::make_shared<VERSION::commonapi::tests::managed::LeafInterfaceStubDefault>();

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

    //runtime_->unregisterService(rootAddress);
    dbusObjectPathAndInterfacesDict = getManagedObjects("/", proxyDBusConnection_);
    EXPECT_TRUE(dbusObjectPathAndInterfacesDict.empty());
}

TEST_F(DBusManagedTest, RegisterLeafManagedAndCreateProxyForLeaf) {
	auto rootStub = std::make_shared<VERSION::commonapi::tests::managed::RootInterfaceStubDefault>();
	bool success = runtime_->registerService(domain, rootAddress, rootStub);
    ASSERT_TRUE(success);

	std::shared_ptr<VERSION::commonapi::tests::managed::RootInterfaceDBusProxy> rootProxy = std::make_shared<
		VERSION::commonapi::tests::managed::RootInterfaceDBusProxy>(
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

	auto leafStub = std::make_shared<VERSION::commonapi::tests::managed::LeafInterfaceStubDefault>();
    success = rootStub->registerManagedStubLeafInterface(leafStub, leafInstance);
    ASSERT_TRUE(success);

    usleep(2000000);

	auto leafProxy = proxyManagerLeafInterface.buildProxy<VERSION::commonapi::tests::managed::LeafInterfaceProxy>(leafInstance);
    for (uint32_t i = 0; !leafProxy->isAvailable() && i < 500; ++i) {
        usleep(10 * 1000);
    }

    ASSERT_TRUE(leafProxy->isAvailable());

    CommonAPI::CallStatus callStatus;
	VERSION::commonapi::tests::managed::LeafInterface::testLeafMethodError error;
    int outInt;
    std::string outString;
    leafProxy->testLeafMethod(42, "Test", callStatus, error, outInt, outString);

    EXPECT_TRUE(callStatus == CommonAPI::CallStatus::SUCCESS);

    success = rootStub->deregisterManagedStubLeafInterface(leafInstance);
    EXPECT_TRUE(success);

    auto dbusObjectPathAndInterfacesDict = getManagedObjects(rootInterfaceName, proxyDBusConnection_);
    EXPECT_TRUE(dbusObjectPathAndInterfacesDict.empty());

    //success = runtime_->unregisterService(rootAddress);
    //EXPECT_TRUE(success);

    dbusObjectPathAndInterfacesDict = getManagedObjects("/", proxyDBusConnection_);
    EXPECT_TRUE(dbusObjectPathAndInterfacesDict.empty());
}

TEST_F(DBusManagedTest, PropagateTeardown) {
	auto rootStub = std::make_shared<VERSION::commonapi::tests::managed::RootInterfaceStubDefault>();
	bool success = runtime_->registerService(domain, rootAddress, rootStub);

    ASSERT_TRUE(success);

	std::shared_ptr<VERSION::commonapi::tests::managed::RootInterfaceDBusProxy> rootProxy = std::make_shared<
		VERSION::commonapi::tests::managed::RootInterfaceDBusProxy>(
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

	auto leafStub = std::make_shared<VERSION::commonapi::tests::managed::LeafInterfaceStubDefault>();
    bool reg = rootStub->registerManagedStubLeafInterface(leafStub, leafInstance);
    ASSERT_TRUE(reg);

    usleep(2000000);

	auto leafProxy = proxyManagerLeafInterface.buildProxy<VERSION::commonapi::tests::managed::LeafInterfaceProxy>(leafInstance);

    for (uint32_t i = 0; !leafProxy->isAvailable() && i < 500; ++i) {
        usleep(10 * 1000);
    }

    ASSERT_TRUE(leafProxy->isAvailable());

    CommonAPI::CallStatus callStatus;
	VERSION::commonapi::tests::managed::LeafInterface::testLeafMethodError error;
    int outInt;
    std::string outString;
    leafProxy->testLeafMethod(42, "Test", callStatus, error, outInt, outString);

    ASSERT_TRUE(callStatus == CommonAPI::CallStatus::SUCCESS);

    //bool dereg = runtime_->unregisterService(rootAddress);
    //ASSERT_TRUE(dereg);

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
        runtime_ = CommonAPI::Runtime::get();
        leafInstanceAvailability = CommonAPI::AvailabilityStatus::UNKNOWN;
        branchInstanceAvailability = CommonAPI::AvailabilityStatus::UNKNOWN;

        manualTestDBusConnection_ = CommonAPI::DBus::DBusConnection::getBus(CommonAPI::DBus::DBusType_t::SESSION);
        ASSERT_TRUE(manualTestDBusConnection_->connect());

		auto stubDBusConnection = CommonAPI::DBus::DBusConnection::getBus(CommonAPI::DBus::DBusType_t::SESSION);
        ASSERT_TRUE(stubDBusConnection->connect());
        ASSERT_TRUE(stubDBusConnection->requestServiceNameAndBlock(dbusServiceName));
    }

    virtual void TearDown() {
        for (auto it: rootStubs_) {
            //runtime_->unregisterService(it.first);
        }

        rootStubs_.clear();
        rootProxies_.clear();

		auto stubDBusConnection = CommonAPI::DBus::DBusConnection::getBus(CommonAPI::DBus::DBusType_t::SESSION);
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
		std::shared_ptr<VERSION::commonapi::tests::managed::RootInterfaceStubDefault> rootStub = std::make_shared<
			VERSION::commonapi::tests::managed::RootInterfaceStubDefault>();
        const std::string rootAddress = getSuffixedRootAddress(suffix);
        rootStubs_.insert( {rootAddress, rootStub} );
		return runtime_->registerService(domain, rootAddress, rootStub);
    }

    inline void createRootProxyForSuffix(const std::string& suffix) {
		rootProxies_.push_back(runtime_->buildProxy<VERSION::commonapi::tests::managed::RootInterfaceProxy>(domain, getSuffixedRootAddress(suffix)));
    }

    template<typename _ProxyType>
    bool waitForAllProxiesToBeAvailable(const std::vector<_ProxyType>& dbusProxies) {
        bool allAreAvailable = false;
        for (size_t i = 0; i < 500 && !allAreAvailable; i++) {
            allAreAvailable = std::all_of(dbusProxies.begin(),
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
				std::shared_ptr<VERSION::commonapi::tests::managed::LeafInterfaceStubDefault> leafStub = std::make_shared<
					VERSION::commonapi::tests::managed::LeafInterfaceStubDefault>();
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
        for (auto rootProxyIterator : rootProxies_) {
			std::vector<std::shared_ptr<VERSION::commonapi::tests::managed::LeafInterfaceProxyDefault>> leafProxiesForRootX;

            CommonAPI::ProxyManager& leafProxyManager = rootProxyIterator->getProxyManagerLeafInterface();
            std::vector<std::string> availableInstances;
            CommonAPI::CallStatus status;
            leafProxyManager.getAvailableInstances(status, availableInstances);

            for (const std::string& availableInstance : availableInstances) {
				auto newLeafProxy = leafProxyManager.buildProxy<VERSION::commonapi::tests::managed::LeafInterfaceProxy>(availableInstance);
                leafProxiesForRootX.push_back(newLeafProxy);
            }
            leafProxies_.push_back(std::move(leafProxiesForRootX));
        }
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<CommonAPI::DBus::DBusConnection> manualTestDBusConnection_;

	std::unordered_map<std::string, std::shared_ptr<VERSION::commonapi::tests::managed::RootInterfaceStubDefault>> rootStubs_;
	std::vector<std::shared_ptr<VERSION::commonapi::tests::managed::RootInterfaceProxyDefault>> rootProxies_;
	std::vector<std::vector<std::shared_ptr<VERSION::commonapi::tests::managed::LeafInterfaceProxyDefault>>>leafProxies_;

    CommonAPI::AvailabilityStatus leafInstanceAvailability;
    CommonAPI::AvailabilityStatus branchInstanceAvailability;
public:
    void onLeafInstanceAvailabilityStatusChanged(const std::string instanceName, CommonAPI::AvailabilityStatus availabilityStatus) {
        leafInstanceAvailability = availabilityStatus;
    }

    void onBranchInstanceAvailabilityStatusChanged(const std::string instanceName, CommonAPI::AvailabilityStatus availabilityStatus) {
        branchInstanceAvailability = availabilityStatus;
    }
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
	VERSION::commonapi::tests::managed::RootInterface::testRootMethodError applicationError;
    int32_t outInt;
    std::string outString;

    for (size_t i = 0; i < rootProxies_.size(); ++i) {
        rootProxies_[i]->testRootMethod(-5,
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
    for (const auto& leafProxiesForCurrentRoot : leafProxies_) {
        ASSERT_EQ(5, leafProxiesForCurrentRoot.size())<< "in run #" << runNr++;

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

	std::shared_ptr<VERSION::commonapi::tests::managed::SecondRootStubDefault> secondRootStub = std::make_shared<
		VERSION::commonapi::tests::managed::SecondRootStubDefault>();
    const std::string rootAddressLocal = getSuffixedRootAddress("Two");
    runtime_->registerService(secondRootStub, rootAddressLocal, serviceFactory_);

	auto leafStub1 = std::make_shared<VERSION::commonapi::tests::managed::LeafInterfaceStubDefault>();
	auto leafStub2 = std::make_shared<VERSION::commonapi::tests::managed::LeafInterfaceStubDefault>();

    bool leafStub1Registered = rootStubs_.begin()->second->registerManagedStubLeafInterface(leafStub1, leafInstance);
    ASSERT_TRUE(leafStub1Registered);

    bool leafStub2Registered = secondRootStub->registerManagedStubLeafInterface(leafStub2, secondLeafInstance);
    ASSERT_TRUE(leafStub2Registered);

    runtime_->unregisterService(rootAddressLocal);
}

TEST_F(DBusManagedTestExtended, RegisterLeafsWithDistinctInterfacesOnSameRootManaged) {
    ASSERT_TRUE(registerRootStubForSuffix("One"));

    createRootProxyForSuffix("One");
    auto rootProxy = *(rootProxies_.begin());
    CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& leafInstanceAvailabilityStatusEvent =
                    rootProxy->getProxyManagerLeafInterface().getInstanceAvailabilityStatusChangedEvent();
    CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& branchInstanceAvailabilityStatusEvent =
                    rootProxy->getProxyManagerBranchInterface().getInstanceAvailabilityStatusChangedEvent();

    leafInstanceAvailabilityStatusEvent.subscribe(
                    std::bind(&DBusManagedTestExtended::onLeafInstanceAvailabilityStatusChanged,
                              this,
                              std::placeholders::_1,
                              std::placeholders::_2));
    branchInstanceAvailabilityStatusEvent.subscribe(
                    std::bind(&DBusManagedTestExtended::onBranchInstanceAvailabilityStatusChanged,
                              this,
                              std::placeholders::_1,
                              std::placeholders::_2));

	auto leafStub1 = std::make_shared<VERSION::commonapi::tests::managed::LeafInterfaceStubDefault>();
	auto leafStub2 = std::make_shared<VERSION::commonapi::tests::managed::BranchInterfaceStubDefault>();

    bool leafStub1Registered = rootStubs_.begin()->second->registerManagedStubLeafInterface(leafStub1, leafInstance);
    ASSERT_TRUE(leafStub1Registered);
    usleep(50000);

    ASSERT_EQ(CommonAPI::AvailabilityStatus::AVAILABLE, leafInstanceAvailability);

    // check that event for branch instances is not triggered by leaf instances
    ASSERT_NE(CommonAPI::AvailabilityStatus::AVAILABLE, branchInstanceAvailability);

    bool leafStub2Registered = rootStubs_.begin()->second->registerManagedStubBranchInterface(
                    leafStub2,
                    branchInstance);
    ASSERT_TRUE(leafStub2Registered);
    usleep(50000);

    ASSERT_EQ(CommonAPI::AvailabilityStatus::AVAILABLE, branchInstanceAvailability);
}

TEST_F(DBusManagedTestExtended, RegisterLeafsWithDistinctInterfacesOnSameRootUnmanaged) {
    ASSERT_TRUE(registerRootStubForSuffix("One"));

    createRootProxyForSuffix("One");
    auto rootProxy = *(rootProxies_.begin());
    CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& leafInstanceAvailabilityStatusEvent =
                    rootProxy->getProxyManagerLeafInterface().getInstanceAvailabilityStatusChangedEvent();
    CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& branchInstanceAvailabilityStatusEvent =
                    rootProxy->getProxyManagerBranchInterface().getInstanceAvailabilityStatusChangedEvent();

    leafInstanceAvailabilityStatusEvent.subscribe(
                    std::bind(&DBusManagedTestExtended::onLeafInstanceAvailabilityStatusChanged,
                              this,
                              std::placeholders::_1,
                              std::placeholders::_2));
    branchInstanceAvailabilityStatusEvent.subscribe(
                    std::bind(&DBusManagedTestExtended::onBranchInstanceAvailabilityStatusChanged,
                              this,
                              std::placeholders::_1,
                              std::placeholders::_2));

	auto leafStub1 = std::make_shared<VERSION::commonapi::tests::managed::LeafInterfaceStubDefault>();
    runtime_->getServicePublisher()->registerService(leafStub1, leafAddress, serviceFactory_);

	auto leafStub2 = std::make_shared<VERSION::commonapi::tests::managed::BranchInterfaceStubDefault>();
    runtime_->getServicePublisher()->registerService(leafStub2, branchAddress, serviceFactory_);

    usleep(50000);

    // check, that events do not get triggered by unmanaged registration
    ASSERT_EQ(CommonAPI::AvailabilityStatus::UNKNOWN, leafInstanceAvailability);
    ASSERT_EQ(CommonAPI::AvailabilityStatus::UNKNOWN, branchInstanceAvailability);

    runtime_->unregisterService(leafAddress);
    runtime_->unregisterService(branchAddress);
}

#ifndef __NO_MAIN__
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
