/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef _GLIBCXX_USE_NANOSLEEP
#define _GLIBCXX_USE_NANOSLEEP
#endif

#include <fstream>

#include <CommonAPI/CommonAPI.h>

#ifndef COMMONAPI_INTERNAL_COMPILATION
#define COMMONAPI_INTERNAL_COMPILATION
#endif

#include <CommonAPI/DBus/DBusServiceRegistry.h>
#include <CommonAPI/DBus/DBusServicePublisher.h>
#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusUtils.h>
#include <CommonAPI/DBus/DBusRuntime.h>

#include <commonapi/tests/TestInterfaceStub.h>
#include <commonapi/tests/TestInterfaceStubDefault.h>
#include <commonapi/tests/TestInterfaceDBusStubAdapter.h>

#include <gtest/gtest.h>

#include "DemoMainLoop.h"

#include <fstream>
#include <chrono>


// all predefinedInstances will be added for this service
static const std::string dbusServiceName = "DBusServiceRegistryTest.Predefined.Service";

// dbusInterfaceName, dbusObjectPath -> commonApiAddress
static const std::unordered_map<std::pair<std::string, std::string>, std::string> predefinedInstancesMap = {
                { { "tests.Interface1", "/tests/predefined/Object1" }, "local:Interface1:predefined.Instance1" },
                { { "tests.Interface1", "/tests/predefined/Object2" }, "local:Interface1:predefined.Instance2" },
                { { "tests.Interface2", "/tests/predefined/Object1" }, "local:Interface2:predefined.Instance" }
};


class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
        configFileName_ = CommonAPI::getCurrentBinaryFileFQN();
        configFileName_ += CommonAPI::DBus::DBUS_CONFIG_SUFFIX;

        std::ofstream configFile(configFileName_);
        ASSERT_TRUE(configFile.is_open());

        for (auto& predefinedInstance : predefinedInstancesMap) {
            const std::string& dbusInterfaceName = predefinedInstance.first.first;
            const std::string& dbusObjectPath = predefinedInstance.first.second;
            const std::string& commonApiAddress = predefinedInstance.second;

            configFile << "[" << commonApiAddress << "]\n";
            configFile << "dbus_connection=" << dbusServiceName << std::endl;
            configFile << "dbus_object=" << dbusObjectPath << std::endl;
            configFile << "dbus_interface=" << dbusInterfaceName << std::endl;
            configFile << "dbus_predefined=true\n";
            configFile << std::endl;
        }

        configFile.close();
    }

    virtual void TearDown() {
        std::remove(configFileName_.c_str());
    }

    std::string configFileName_;
};


struct TestDBusServiceListener {
    CommonAPI::AvailabilityStatus lastAvailabilityStatus;
    size_t availabilityStatusCount;

    std::condition_variable statusChanged;
    std::mutex lock;

    TestDBusServiceListener(const std::string& commonApiAddress,
                            const std::shared_ptr<CommonAPI::DBus::DBusProxyConnection>& dbusConnection):
                                availabilityStatusCount(0),
                                isSubscribed(false),
                                commonApiAddress_(commonApiAddress),
                                dbusServiceRegistry_(dbusConnection->getDBusServiceRegistry()) {
    }

    ~TestDBusServiceListener() {
        if (isSubscribed) {
            unsubscribe();
        }
    }

    void subscribe() {
        ASSERT_TRUE(!isSubscribed);

        dbusServiceSubscription_= dbusServiceRegistry_->subscribeAvailabilityListener(
                        commonApiAddress_,
                        [&](const CommonAPI::AvailabilityStatus& availabilityStatus) {

            std::lock_guard<std::mutex> lockGuard(lock);

            lastAvailabilityStatus = availabilityStatus;
            availabilityStatusCount++;

            statusChanged.notify_all();

            return CommonAPI::SubscriptionStatus::RETAIN;
        });

        isSubscribed = true;
    }

    void unsubscribe() {
        ASSERT_TRUE(isSubscribed);

        dbusServiceRegistry_->unsubscribeAvailabilityListener(
            commonApiAddress_,
            dbusServiceSubscription_);
    }

 private:
    bool isSubscribed;
    std::string commonApiAddress_;
    std::shared_ptr<CommonAPI::DBus::DBusServiceRegistry> dbusServiceRegistry_;
    CommonAPI::DBus::DBusServiceRegistry::DBusServiceSubscription dbusServiceSubscription_;
};


class DBusServiceRegistryTestDBusStubAdapter: public CommonAPI::DBus::DBusStubAdapter {
public:
    DBusServiceRegistryTestDBusStubAdapter(const std::shared_ptr<CommonAPI::DBus::DBusFactory>& factory,
                                           const std::string& commonApiAddress,
                                           const std::string& dbusInterfaceName,
                                           const std::string& dbusBusName,
                                           const std::string& dbusObjectPath,
                                           const std::shared_ptr<CommonAPI::DBus::DBusProxyConnection>& dbusConnection) :
                    DBusStubAdapter(factory,
                                    commonApiAddress,
                                    dbusInterfaceName,
                                    dbusBusName,
                                    dbusObjectPath,
                                    dbusConnection,
                                    false),
                    introspectionCount(0) {
    }

    void deactivateManagedInstances() {
    }

    virtual const char* getMethodsDBusIntrospectionXmlData() const {
        introspectionCount++;
        return "";
    }

    virtual bool onInterfaceDBusMessage(const CommonAPI::DBus::DBusMessage& dbusMessage) {
        return false;
    }

    virtual bool onInterfaceDBusFreedesktopPropertiesMessage(const CommonAPI::DBus::DBusMessage& dbusMessage) {
        return false;
    }

    mutable size_t introspectionCount;
};

class DBusServiceRegistryTest: public ::testing::Test {
 protected:
    virtual void SetUp() {

        auto runtime = std::dynamic_pointer_cast<CommonAPI::DBus::DBusRuntime>(CommonAPI::Runtime::load());

        clientFactory = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(runtime->createFactory());
        serviceFactory = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(runtime->createFactory());

        clientDBusConnection = clientFactory->getDbusConnection();
        clientConnectionRegistry = clientDBusConnection->getDBusServiceRegistry();
        serviceDBusConnection = serviceFactory->getDbusConnection();
    }

    virtual void TearDown() {
        if (clientDBusConnection && clientDBusConnection->isConnected()) {
            clientDBusConnection->disconnect();
        }

        if (serviceDBusConnection && serviceDBusConnection->isConnected()) {
            serviceDBusConnection->disconnect();
        }
    }

    bool waitForAvailabilityStatusChanged(TestDBusServiceListener& testDBusServiceListener,
                                          const CommonAPI::AvailabilityStatus& availabilityStatus) {
        std::unique_lock<std::mutex> lock(testDBusServiceListener.lock);
        auto waitResult =
                        testDBusServiceListener.statusChanged.wait_for(
                                        lock,
                                        std::chrono::milliseconds(4000),
                                        [&]() {return testDBusServiceListener.lastAvailabilityStatus == availabilityStatus;});
        return waitResult;
    }

    std::shared_ptr<CommonAPI::DBus::DBusFactory> clientFactory;
    std::shared_ptr<CommonAPI::DBus::DBusFactory> serviceFactory;

    std::shared_ptr<CommonAPI::DBus::DBusConnection> clientDBusConnection;
    std::shared_ptr<CommonAPI::DBus::DBusConnection> serviceDBusConnection;

    std::shared_ptr<CommonAPI::DBus::DBusServiceRegistry> clientConnectionRegistry;
    //std::shared_ptr<CommonAPI::DBus::DBusServiceRegistry> serviceConnectionRegistry;
};


TEST_F(DBusServiceRegistryTest, DBusConnectionHasRegistry) {
    clientDBusConnection->connect();

    auto dbusServiceRegistry = clientDBusConnection->getDBusServiceRegistry();
    EXPECT_FALSE(!dbusServiceRegistry);
}

TEST_F(DBusServiceRegistryTest, SubscribeBeforeConnectWorks) {
    TestDBusServiceListener testDBusServiceListener("local:Interface1:predefined.Instance1", clientDBusConnection);
    testDBusServiceListener.subscribe();

    ASSERT_TRUE(clientDBusConnection->connect());

    EXPECT_TRUE(waitForAvailabilityStatusChanged(
        testDBusServiceListener,
        CommonAPI::AvailabilityStatus::NOT_AVAILABLE));
    usleep(300 * 1000);
    EXPECT_EQ(testDBusServiceListener.availabilityStatusCount, 1);

    ASSERT_TRUE(serviceDBusConnection->connect());
    ASSERT_TRUE(serviceDBusConnection->requestServiceNameAndBlock(dbusServiceName));
    auto testDBusStubAdapter = std::make_shared<DBusServiceRegistryTestDBusStubAdapter>(
                    serviceFactory,
                    "local:Interface1:predefined.Instance1",
                    "tests.Interface1",
                    dbusServiceName,
                    "/tests/predefined/Object1",
                    serviceDBusConnection);
    CommonAPI::DBus::DBusServicePublisher::getInstance()->registerService(testDBusStubAdapter);

    EXPECT_TRUE(waitForAvailabilityStatusChanged(
        testDBusServiceListener,
        CommonAPI::AvailabilityStatus::AVAILABLE));
    usleep(5000000);
    EXPECT_LE(testDBusServiceListener.availabilityStatusCount, 3);
    EXPECT_EQ(testDBusStubAdapter->introspectionCount, 1);

    CommonAPI::DBus::DBusServicePublisher::getInstance()->unregisterService(testDBusStubAdapter->getAddress());

    EXPECT_TRUE(waitForAvailabilityStatusChanged(
        testDBusServiceListener,
        CommonAPI::AvailabilityStatus::NOT_AVAILABLE));
    EXPECT_LE(testDBusServiceListener.availabilityStatusCount, 4);
}

TEST_F(DBusServiceRegistryTest, SubscribeBeforeConnectWithServiceWorks) {
    ASSERT_TRUE(serviceDBusConnection->connect());
    auto testDBusStubAdapter = std::make_shared<DBusServiceRegistryTestDBusStubAdapter>(
                    serviceFactory,
                    "local:Interface1:predefined.Instance1",
                    "tests.Interface1",
                    dbusServiceName,
                    "/tests/predefined/Object1",
                    serviceDBusConnection);
    CommonAPI::DBus::DBusServicePublisher::getInstance()->registerService(testDBusStubAdapter);

    TestDBusServiceListener testDBusServiceListener("local:Interface1:predefined.Instance1", clientDBusConnection);
    testDBusServiceListener.subscribe();
    ASSERT_TRUE(clientDBusConnection->connect());
    EXPECT_TRUE(waitForAvailabilityStatusChanged(
        testDBusServiceListener,
        CommonAPI::AvailabilityStatus::AVAILABLE));
    usleep(300 * 1000);
    EXPECT_EQ(testDBusServiceListener.availabilityStatusCount, 1);
    EXPECT_EQ(testDBusStubAdapter->introspectionCount, 1);

    serviceDBusConnection->disconnect();

    EXPECT_TRUE(waitForAvailabilityStatusChanged(
        testDBusServiceListener,
        CommonAPI::AvailabilityStatus::NOT_AVAILABLE));
    usleep(300 * 1000);
    EXPECT_EQ(testDBusServiceListener.availabilityStatusCount, 2);

    CommonAPI::DBus::DBusServicePublisher::getInstance()->unregisterService(testDBusStubAdapter->getAddress());
}

TEST_F(DBusServiceRegistryTest, SubscribeAfterConnectWithServiceWorks) {
    ASSERT_TRUE(serviceDBusConnection->connect());
    auto testDBusStubAdapter = std::make_shared<DBusServiceRegistryTestDBusStubAdapter>(
                    serviceFactory,
                    "local:Interface1:predefined.Instance1",
                    "tests.Interface1",
                    dbusServiceName,
                    "/tests/predefined/Object1",
                    serviceDBusConnection);
    CommonAPI::DBus::DBusServicePublisher::getInstance()->registerService(testDBusStubAdapter);

    ASSERT_TRUE(clientDBusConnection->connect());
    TestDBusServiceListener testDBusServiceListener("local:Interface1:predefined.Instance1", clientDBusConnection);
    EXPECT_EQ(testDBusStubAdapter->introspectionCount, 0);

    testDBusServiceListener.subscribe();

    EXPECT_TRUE(waitForAvailabilityStatusChanged(
        testDBusServiceListener,
        CommonAPI::AvailabilityStatus::AVAILABLE));
    EXPECT_EQ(testDBusServiceListener.availabilityStatusCount, 1);
    EXPECT_EQ(testDBusStubAdapter->introspectionCount, 1);

    CommonAPI::DBus::DBusServicePublisher::getInstance()->unregisterService(testDBusStubAdapter->getAddress());

    EXPECT_TRUE(waitForAvailabilityStatusChanged(
        testDBusServiceListener,
        CommonAPI::AvailabilityStatus::NOT_AVAILABLE));
    EXPECT_EQ(testDBusServiceListener.availabilityStatusCount, 2);
    EXPECT_EQ(testDBusStubAdapter->introspectionCount, 1);
}

TEST_F(DBusServiceRegistryTest, DBusAddressTranslatorPredefinedWorks) {
    std::vector<CommonAPI::DBus::DBusServiceAddress> loadedPredefinedInstances;

    CommonAPI::DBus::DBusAddressTranslator::getInstance().getPredefinedInstances(dbusServiceName, loadedPredefinedInstances);

    EXPECT_EQ(loadedPredefinedInstances.size(), predefinedInstancesMap.size());

    for (auto& dbusServiceAddress : loadedPredefinedInstances) {
        const std::string& loadedDBusServiceName = std::get<0>(dbusServiceAddress);
        const std::string& loadedDBusObjectPath = std::get<1>(dbusServiceAddress);
        const std::string& loadedDBusInterfaceName = std::get<2>(dbusServiceAddress);

        EXPECT_EQ(loadedDBusServiceName, dbusServiceName);

        auto predefinedInstanceIterator = predefinedInstancesMap.find({ loadedDBusInterfaceName, loadedDBusObjectPath });
        const bool predefinedInstanceFound = (predefinedInstanceIterator != predefinedInstancesMap.end());

        EXPECT_TRUE(predefinedInstanceFound);

        const std::string& commonApiAddress = predefinedInstanceIterator->second;
        const std::string& predefinedDBusInterfaceName = predefinedInstanceIterator->first.first;
        const std::string& predefinedDBusObjectPath = predefinedInstanceIterator->first.second;

        EXPECT_EQ(loadedDBusInterfaceName, predefinedDBusInterfaceName);
        EXPECT_EQ(loadedDBusObjectPath, predefinedDBusObjectPath);

        std::string foundDBusInterfaceName;
        std::string foundDBusServiceName;
        std::string foundDBusObjectPath;

        CommonAPI::DBus::DBusAddressTranslator::getInstance().searchForDBusAddress(
                        commonApiAddress,
                        foundDBusInterfaceName,
                        foundDBusServiceName,
                        foundDBusObjectPath);

        EXPECT_EQ(foundDBusInterfaceName, predefinedDBusInterfaceName);
        EXPECT_EQ(foundDBusServiceName, dbusServiceName);
        EXPECT_EQ(foundDBusObjectPath, predefinedDBusObjectPath);
    }
}


CommonAPI::DBus::DBusMessage getNewFakeNameOwnerChangedMessage() {
    static const char* objectPath = "/";
    static const char* interfaceName = "org.freedesktop.DBus";
    static const char* signalName = "NameOwnerChanged";
    return CommonAPI::DBus::DBusMessage::createSignal(objectPath, interfaceName, signalName, "sss");
}


//XXX This test requires CommonAPI::DBus::DBusServiceRegistry::onDBusDaemonProxyNameOwnerChangedEvent to be public!

//TEST_F(DBusServiceRegistryTest, RevertedNameOwnerChangedEventsWork) {
//    std::shared_ptr<CommonAPI::DBus::DBusConnection> registryConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
//    std::shared_ptr<CommonAPI::DBus::DBusServiceRegistry> registry = std::make_shared<CommonAPI::DBus::DBusServiceRegistry>(registryConnection);
//
//    registry->init();
//
//    CommonAPI::DBus::DBusMessage fakeNameOwnerChangedSignal1 = getNewFakeNameOwnerChangedMessage();
//    CommonAPI::DBus::DBusMessage fakeNameOwnerChangedSignal2 = getNewFakeNameOwnerChangedMessage();
//    CommonAPI::DBus::DBusMessage fakeNameOwnerChangedSignal3 = getNewFakeNameOwnerChangedMessage();
//
//    CommonAPI::DBus::DBusOutputStream outStream1(fakeNameOwnerChangedSignal1);
//    CommonAPI::DBus::DBusOutputStream outStream2(fakeNameOwnerChangedSignal2);
//    CommonAPI::DBus::DBusOutputStream outStream3(fakeNameOwnerChangedSignal3);
//
//    std::string serviceName = "my.fake.service";
//    std::string emptyOwner = "";
//    std::string newOwner1 = ":1:505";
//    std::string newOwner2 = ":1:506";
//
//    outStream1 << serviceName << emptyOwner << newOwner1;
//    outStream1.flush();
//    outStream2 << serviceName << emptyOwner << newOwner2;
//    outStream2.flush();
//    outStream3 << serviceName << newOwner1 << emptyOwner;
//    outStream3.flush();
//
//    registry->onDBusDaemonProxyNameOwnerChangedEvent(serviceName, emptyOwner, newOwner1);
//    registry->onDBusDaemonProxyNameOwnerChangedEvent(serviceName, emptyOwner, newOwner2);
//    registry->onDBusDaemonProxyNameOwnerChangedEvent(serviceName, newOwner1, emptyOwner);
//}


TEST_F(DBusServiceRegistryTest, DISABLED_PredefinedInstances) {
//    auto stubDBusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
//
//    ASSERT_TRUE(stubDBusConnection->connect());
//	ASSERT_TRUE(stubDBusConnection->requestServiceNameAndBlock(dbusServiceName));
//
//    auto proxyDBusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
//    auto dbusServiceRegistry = proxyDBusConnection->getDBusServiceRegistry();
//    std::unordered_map<std::string, std::promise<CommonAPI::AvailabilityStatus> > instanceStatusPromises;
//    std::unordered_map<std::string, CommonAPI::DBus::DBusServiceRegistry::Subscription> instanceSubscriptions;
//
//    for (auto& predefinedInstance : predefinedInstancesMap) {
//        const std::string& commonApiAddress = predefinedInstance.second;
//
//        instanceSubscriptions[commonApiAddress] = dbusServiceRegistry->subscribeAvailabilityListener(
//                        commonApiAddress,
//                        [&] (const CommonAPI::AvailabilityStatus& availabilityStatus) -> CommonAPI::SubscriptionStatus {
//            instanceStatusPromises[commonApiAddress].set_value(availabilityStatus);
//            return CommonAPI::SubscriptionStatus::RETAIN;
//        });
//    }
//
//    ASSERT_TRUE(proxyDBusConnection->connect());
//
//    for (auto& predefinedInstance : predefinedInstancesMap) {
//        const std::string& dbusInterfaceName = predefinedInstance.first.first;
//        const std::string& dbusObjectPath = predefinedInstance.first.second;
//        const std::string& commonApiAddress = predefinedInstance.second;
//
//        auto instanceStatusFuture = instanceStatusPromises[commonApiAddress].get_future();
//        auto instanceStatusFutureStatus = instanceStatusFuture.wait_for(std::chrono::milliseconds(2000));
//        const bool instanceReady = CommonAPI::DBus::checkReady(instanceStatusFutureStatus);
//
//        ASSERT_TRUE(instanceReady);
//
//        std::promise<CommonAPI::AvailabilityStatus> postInstanceStatusPromise;
//        auto postInstanceSubscription = dbusServiceRegistry->subscribeAvailabilityListener(
//                        commonApiAddress,
//                        [&] (const CommonAPI::AvailabilityStatus& availabilityStatus) -> CommonAPI::SubscriptionStatus {
//            postInstanceStatusPromise.set_value(availabilityStatus);
//            return CommonAPI::SubscriptionStatus::RETAIN;
//        });
//
//        auto postInstanceStatusFuture = postInstanceStatusPromise.get_future();
//        auto postInstanceStatusFutureStatus = postInstanceStatusFuture.wait_for(std::chrono::milliseconds(2000));
//        const bool postInstanceReady = CommonAPI::DBus::checkReady(postInstanceStatusFutureStatus);
//
//        ASSERT_TRUE(postInstanceReady);
//
//        dbusServiceRegistry->unsubscribeAvailabilityListener(commonApiAddress, postInstanceSubscription);
//        dbusServiceRegistry->unsubscribeAvailabilityListener(commonApiAddress, instanceSubscriptions[commonApiAddress]);
//
//
//        bool isInstanceAlive = dbusServiceRegistry->isServiceInstanceAlive(dbusInterfaceName, dbusServiceName, dbusObjectPath);
//        for (int i = 0; !isInstanceAlive && i < 100; i++) {
//            std::this_thread::sleep_for(std::chrono::milliseconds(10));
//            isInstanceAlive = dbusServiceRegistry->isServiceInstanceAlive(dbusInterfaceName, dbusServiceName, dbusObjectPath);
//        }
//
//        ASSERT_TRUE(isInstanceAlive);
//
//
//        std::vector<std::string> availableDBusServiceInstances = dbusServiceRegistry->getAvailableServiceInstances(dbusInterfaceName);
//        bool availableInstanceFound = false;
//
//        for (auto& availableInstance : availableDBusServiceInstances) {
//            if (availableInstance == commonApiAddress) {
//                availableInstanceFound = true;
//                break;
//            }
//        }
//
//        ASSERT_TRUE(availableInstanceFound);
//    }
}


const char serviceAddress_[] = "local:test.service.name:test.instance.name";
const char serviceName_[] = "test.service.name";
const char nonexistingServiceAddress_[] = "local:nonexisting.service.name:nonexisting.instance.name";
const char nonexistingServiceName_[] = "nonexisting.service.name";




class DBusServiceDiscoveryTestWithPredefinedRemote: public ::testing::Test {
 protected:
    virtual void SetUp() {
        runtime_ = CommonAPI::Runtime::load();
        auto serviceFactory = runtime_->createFactory();
        servicePublisher_ = runtime_->getServicePublisher();
        auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
        servicePublisher_->registerService(stub, serviceAddress_, serviceFactory);
        clientFactory_ = runtime_->createFactory();
        usleep(500 * 1000);
    }

    virtual void TearDown() {
        servicePublisher_->unregisterService(serviceAddress_);
        usleep(500 * 1000);
    }

    std::shared_ptr<CommonAPI::Factory> clientFactory_;

 private:
    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher_;
};

TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, RecognizesInstanceOfExistingServiceAsAlive) {
    bool result = clientFactory_->isServiceInstanceAlive(serviceAddress_);
    EXPECT_TRUE(result);
}

TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, RecognizesInstanceOfNonexistingServiceAsDead) {
    bool result = clientFactory_->isServiceInstanceAlive(nonexistingServiceAddress_);
    EXPECT_FALSE(result);
}


TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, RecognizesInstanceOfExistingServiceAsAliveAsync) {
    //Wait for synchronous availability of the service, then verify the async version gets the same result
    ASSERT_TRUE(clientFactory_->isServiceInstanceAlive(serviceAddress_));

    std::promise<bool> promisedResult;
    std::future<bool> futureResult = promisedResult.get_future();

    clientFactory_->isServiceInstanceAliveAsync(
                    [&] (bool isAlive) {
                        promisedResult.set_value(isAlive);
                    },
                    serviceAddress_);

    EXPECT_TRUE(futureResult.get());
}


TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, RecognizesInstanceOfNonexistingServiceAsDeadAsync) {
    //Wait for synchronous availability of the service, then verify the async version gets the same result
    ASSERT_FALSE(clientFactory_->isServiceInstanceAlive(nonexistingServiceAddress_));

    std::promise<bool> promisedResult;
    std::future<bool> futureResult = promisedResult.get_future();

    clientFactory_->isServiceInstanceAliveAsync(
                    [&] (bool isAlive) {
                        promisedResult.set_value(isAlive);
                    },
                    nonexistingServiceAddress_);

    EXPECT_FALSE(futureResult.get());
}


TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, FindsInstancesOfExistingTestService) {
    EXPECT_EQ(1, clientFactory_->getAvailableServiceInstances(serviceName_).size());
}


TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, FindsInstancesOfExistingTestServiceAsync) {
    //Wait for synchronous availability of the service, then verify the async version gets the same result
    ASSERT_EQ(1, clientFactory_->getAvailableServiceInstances(serviceName_).size());

    std::promise<std::vector<std::string>> promisedResult;
    std::future<std::vector<std::string>> futureResult = promisedResult.get_future();

    clientFactory_->getAvailableServiceInstancesAsync(
                    [&] (std::vector<std::string>& instances) {
                        promisedResult.set_value(instances);
                    },
                    serviceName_);


    std::vector<std::string> result = futureResult.get();

    EXPECT_EQ(1, result.size());
}


TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, FindsNoInstancesOfNonexistingTestService) {
    std::vector<std::string> result = clientFactory_->getAvailableServiceInstances(nonexistingServiceName_);
    EXPECT_EQ(0, result.size());
}


TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, FindsNoInstancesOfNonexistingTestServiceAsync) {
    //Wait for synchronous availability of the service, then verify the async version gets the same result
    ASSERT_EQ(0, clientFactory_->getAvailableServiceInstances(nonexistingServiceName_).size());

    std::promise<std::vector<std::string>> promisedResult;
    std::future<std::vector<std::string>> futureResult = promisedResult.get_future();

    clientFactory_->getAvailableServiceInstancesAsync(
                    [&] (std::vector<std::string>& instances) {
                        promisedResult.set_value(instances);
                    },
                    nonexistingServiceName_);

    EXPECT_EQ(0, futureResult.get().size());
}

// disable that test, because vs2013's "high_resolution_clock" is not accurate enough to measure that
// see the microsoft bug report there: https://connect.microsoft.com/VisualStudio/feedback/details/719443/
#ifndef WIN32
TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, ServiceRegistryUsesCacheForResolvingOneService) {
    std::chrono::system_clock::time_point startTimeWithColdCache = std::chrono::high_resolution_clock::now();
    ASSERT_TRUE(clientFactory_->isServiceInstanceAlive(serviceAddress_));
    std::chrono::system_clock::time_point endTimeWithColdCache = std::chrono::high_resolution_clock::now();

    unsigned long durationWithColdCache = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    endTimeWithColdCache - startTimeWithColdCache).count();

    std::chrono::system_clock::time_point startTimeWithHotCache = std::chrono::high_resolution_clock::now();
    ASSERT_TRUE(clientFactory_->isServiceInstanceAlive(serviceAddress_));
    std::chrono::system_clock::time_point endTimeWithHotCache = std::chrono::high_resolution_clock::now();

    unsigned long durationWithHotCache = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    endTimeWithHotCache - startTimeWithHotCache).count();

    ASSERT_GT(durationWithHotCache, 0);

    double speedRatio = durationWithColdCache / durationWithHotCache;

    EXPECT_GE(speedRatio, 100);
}
#endif

TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, DISABLED_ServiceRegistryUsesCacheForResolvingWholeBus) {
    std::chrono::system_clock::time_point startTimeWithColdCache = std::chrono::system_clock::now();
    ASSERT_EQ(1, clientFactory_->getAvailableServiceInstances(serviceName_).size());
    std::chrono::system_clock::time_point endTimeWithColdCache = std::chrono::system_clock::now();

    long durationWithColdCache = std::chrono::duration_cast<std::chrono::microseconds>(endTimeWithColdCache - startTimeWithColdCache).count();

    std::chrono::system_clock::time_point startTimeWithHotCache = std::chrono::system_clock::now();
    ASSERT_EQ(1, clientFactory_->getAvailableServiceInstances(serviceName_).size());
    std::chrono::system_clock::time_point endTimeWithHotCache = std::chrono::system_clock::now();

    long durationWithHotCache = std::chrono::duration_cast<std::chrono::microseconds>(endTimeWithHotCache - startTimeWithHotCache).count();

    double speedRatio = durationWithColdCache / durationWithHotCache;

    EXPECT_GE(speedRatio, 100);
}


#ifndef WIN32
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
#endif
