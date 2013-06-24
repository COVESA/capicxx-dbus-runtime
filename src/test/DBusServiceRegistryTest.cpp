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

#ifndef COMMONAPI_INTERNAL_COMPILATION
#define COMMONAPI_INTERNAL_COMPILATION
#endif

#include <CommonAPI/DBus/DBusServiceRegistry.h>
#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusUtils.h>

#include <commonapi/tests/TestInterfaceStub.h>
#include <commonapi/tests/TestInterfaceStubDefault.h>
#include <commonapi/tests/TestInterfaceDBusStubAdapter.h>

#include <gtest/gtest.h>

#include "DemoMainLoop.h"


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
        configFileName_ = CommonAPI::DBus::getCurrentBinaryFileFQN();
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


class DBusServiceRegistryTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
    }

	virtual void TearDown() {
	}
};


TEST_F(DBusServiceRegistryTest, CanBeConstructed) {
	std::shared_ptr<CommonAPI::DBus::DBusConnection> dbusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
    CommonAPI::DBus::DBusServiceRegistry* registry = new CommonAPI::DBus::DBusServiceRegistry(dbusConnection);
    ASSERT_TRUE(registry != NULL);
    delete registry;
}


TEST_F(DBusServiceRegistryTest, DBusConnectionHasRegistry) {
    auto dbusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
    dbusConnection->connect();
    auto serviceRegistry = dbusConnection->getDBusServiceRegistry();
    ASSERT_FALSE(!serviceRegistry);
}

TEST_F(DBusServiceRegistryTest, DBusAddressTranslatorPredefinedWorks) {
    std::vector<CommonAPI::DBus::DBusServiceAddress> loadedPredefinedInstances;

    CommonAPI::DBus::DBusAddressTranslator::getInstance().getPredefinedInstances(dbusServiceName, loadedPredefinedInstances);

    ASSERT_EQ(loadedPredefinedInstances.size(), predefinedInstancesMap.size());

    for (auto& dbusServiceAddress : loadedPredefinedInstances) {
        const std::string& loadedDBusServiceName = std::get<0>(dbusServiceAddress);
        const std::string& loadedDBusObjectPath = std::get<1>(dbusServiceAddress);
        const std::string& loadedDBusInterfaceName = std::get<2>(dbusServiceAddress);

        ASSERT_EQ(loadedDBusServiceName, dbusServiceName);

        auto predefinedInstanceIterator = predefinedInstancesMap.find({ loadedDBusInterfaceName, loadedDBusObjectPath });
        const bool predefinedInstanceFound = (predefinedInstanceIterator != predefinedInstancesMap.end());

        ASSERT_TRUE(predefinedInstanceFound);

        const std::string& commonApiAddress = predefinedInstanceIterator->second;
        const std::string& predefinedDBusInterfaceName = predefinedInstanceIterator->first.first;
        const std::string& predefinedDBusObjectPath = predefinedInstanceIterator->first.second;

        ASSERT_EQ(loadedDBusInterfaceName, predefinedDBusInterfaceName);
        ASSERT_EQ(loadedDBusObjectPath, predefinedDBusObjectPath);

        std::string foundDBusInterfaceName;
        std::string foundDBusServiceName;
        std::string foundDBusObjectPath;

        CommonAPI::DBus::DBusAddressTranslator::getInstance().searchForDBusAddress(
                        commonApiAddress,
                        foundDBusInterfaceName,
                        foundDBusServiceName,
                        foundDBusObjectPath);

        ASSERT_EQ(foundDBusInterfaceName, predefinedDBusInterfaceName);
        ASSERT_EQ(foundDBusServiceName, dbusServiceName);
        ASSERT_EQ(foundDBusObjectPath, predefinedDBusObjectPath);
    }
}

TEST_F(DBusServiceRegistryTest, PredefinedInstances) {
    auto stubDBusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();

    ASSERT_TRUE(stubDBusConnection->connect());
	ASSERT_TRUE(stubDBusConnection->requestServiceNameAndBlock(dbusServiceName));

    auto proxyDBusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
    auto dbusServiceRegistry = proxyDBusConnection->getDBusServiceRegistry();
    std::unordered_map<std::string, std::promise<CommonAPI::AvailabilityStatus> > instanceStatusPromises;
    std::unordered_map<std::string, CommonAPI::DBus::DBusServiceRegistry::Subscription> instanceSubscriptions;

    for (auto& predefinedInstance : predefinedInstancesMap) {
        const std::string& commonApiAddress = predefinedInstance.second;

        instanceSubscriptions[commonApiAddress] = dbusServiceRegistry->subscribeAvailabilityListener(
                        commonApiAddress,
                        [&] (const CommonAPI::AvailabilityStatus& availabilityStatus) -> CommonAPI::SubscriptionStatus {
            instanceStatusPromises[commonApiAddress].set_value(availabilityStatus);
            return CommonAPI::SubscriptionStatus::RETAIN;
        });
    }

    ASSERT_TRUE(proxyDBusConnection->connect());

    for (auto& predefinedInstance : predefinedInstancesMap) {
        const std::string& dbusInterfaceName = predefinedInstance.first.first;
        const std::string& dbusObjectPath = predefinedInstance.first.second;
        const std::string& commonApiAddress = predefinedInstance.second;

        auto instanceStatusFuture = instanceStatusPromises[commonApiAddress].get_future();
        auto instanceStatusFutureStatus = instanceStatusFuture.wait_for(std::chrono::milliseconds(2000));
        const bool instanceReady = CommonAPI::DBus::checkReady(instanceStatusFutureStatus);

        ASSERT_TRUE(instanceReady);

        std::promise<CommonAPI::AvailabilityStatus> postInstanceStatusPromise;
        auto postInstanceSubscription = dbusServiceRegistry->subscribeAvailabilityListener(
                        commonApiAddress,
                        [&] (const CommonAPI::AvailabilityStatus& availabilityStatus) -> CommonAPI::SubscriptionStatus {
            postInstanceStatusPromise.set_value(availabilityStatus);
            return CommonAPI::SubscriptionStatus::RETAIN;
        });

        auto postInstanceStatusFuture = postInstanceStatusPromise.get_future();
        auto postInstanceStatusFutureStatus = postInstanceStatusFuture.wait_for(std::chrono::milliseconds(2000));
        const bool postInstanceReady = CommonAPI::DBus::checkReady(postInstanceStatusFutureStatus);

        ASSERT_TRUE(postInstanceReady);

        dbusServiceRegistry->unsubscribeAvailabilityListener(commonApiAddress, postInstanceSubscription);
        dbusServiceRegistry->unsubscribeAvailabilityListener(commonApiAddress, instanceSubscriptions[commonApiAddress]);


        bool isInstanceAlive = dbusServiceRegistry->isServiceInstanceAlive(dbusInterfaceName, dbusServiceName, dbusObjectPath);
        for (int i = 0; !isInstanceAlive && i < 100; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            isInstanceAlive = dbusServiceRegistry->isServiceInstanceAlive(dbusInterfaceName, dbusServiceName, dbusObjectPath);
        }

        ASSERT_TRUE(isInstanceAlive);


        std::vector<std::string> availableDBusServiceInstances = dbusServiceRegistry->getAvailableServiceInstances(dbusInterfaceName);
        bool availableInstanceFound = false;

        for (auto& availableInstance : availableDBusServiceInstances) {
            if (availableInstance == commonApiAddress) {
                availableInstanceFound = true;
                break;
            }
        }

        ASSERT_TRUE(availableInstanceFound);
    }
}


const char* serviceAddress_ = "local:test.service.name:test.instance.name";
const char* serviceName_ = "test.service.name";
const char* nonexistingServiceAddress_ = "local:nonexisting.service.name:nonexisting.instance.name";
const char* nonexistingServiceName_ = "nonexisting.service.name";

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
    ASSERT_TRUE(result);
}

TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, RecognizesInstanceOfNonexistingServiceAsDead) {
    bool result = clientFactory_->isServiceInstanceAlive(nonexistingServiceAddress_);
    ASSERT_FALSE(result);
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

    ASSERT_TRUE(futureResult.get());
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

    ASSERT_FALSE(futureResult.get());
}


TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, FindsInstancesOfExistingTestService) {
    ASSERT_EQ(1, clientFactory_->getAvailableServiceInstances(serviceName_).size());
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

    ASSERT_EQ(1, result.size());
}


TEST_F(DBusServiceDiscoveryTestWithPredefinedRemote, FindsNoInstancesOfNonexistingTestService) {
    std::vector<std::string> result = clientFactory_->getAvailableServiceInstances(nonexistingServiceName_);
    ASSERT_EQ(0, result.size());
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

    ASSERT_EQ(0, futureResult.get().size());
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
