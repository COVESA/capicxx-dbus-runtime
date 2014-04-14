/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gtest/gtest.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <functional>
#include <memory>
#include <stdint.h>
#include <string>
#include <array>
#include <utility>
#include <tuple>
#include <type_traits>
#include <future>

#include <CommonAPI/CommonAPI.h>

#define COMMONAPI_INTERNAL_COMPILATION

#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusProxy.h>
#include <CommonAPI/DBus/DBusRuntime.h>

#include "commonapi/tests/PredefinedTypeCollection.h"
#include "commonapi/tests/DerivedTypeCollection.h"
#include "commonapi/tests/TestInterfaceProxy.h"
#include "commonapi/tests/TestInterfaceStubDefault.h"

#include "commonapi/tests/TestInterfaceDBusProxy.h"

class TestInterfaceStubFinal: public commonapi::tests::TestInterfaceStubDefault {
public:
    void testPredefinedTypeMethod(const std::shared_ptr<CommonAPI::ClientId> clientId,
                                  uint32_t uint32InValue,
                                  std::string stringInValue,
                                  uint32_t& uint32OutValue,
                                  std::string& stringOutValue) {

        uint32OutValue = uint32InValue;
        stringOutValue = stringInValue;
    }
};

class DBusLoadTest: public ::testing::Test {
protected:
    virtual void SetUp() {
        runtime_ = CommonAPI::Runtime::load();
        ASSERT_TRUE((bool )runtime_);
        CommonAPI::DBus::DBusRuntime* dbusRuntime = dynamic_cast<CommonAPI::DBus::DBusRuntime*>(&(*runtime_));
        ASSERT_TRUE(dbusRuntime != NULL);

        proxyFactory_ = runtime_->createFactory();
        ASSERT_TRUE((bool )proxyFactory_);
        stubFactory_ = runtime_->createFactory();
        ASSERT_TRUE((bool )stubFactory_);

        servicePublisher_ = runtime_->getServicePublisher();
        ASSERT_TRUE((bool )servicePublisher_);

        callSucceeded_.resize(numCallsPerProxy_ * numProxies_);
        std::fill(callSucceeded_.begin(), callSucceeded_.end(), false);
    }

    virtual void TearDown() {
        usleep(1000000);
    }

public:
    void TestPredefinedTypeMethodAsyncCallback(
                                               const uint32_t callId,
                                               const uint32_t in1,
                                               const std::string in2,
                                               const CommonAPI::CallStatus& callStatus,
                                               const uint32_t& out1,
                                               const std::string& out2) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
        EXPECT_EQ(out1, in1);
        EXPECT_EQ(out2, in2);
        mutexCallSucceeded_.lock();
        ASSERT_FALSE(callSucceeded_[callId]);
        callSucceeded_[callId] = true;
        mutexCallSucceeded_.unlock();
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<CommonAPI::Factory> proxyFactory_;
    std::shared_ptr<CommonAPI::Factory> stubFactory_;
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher_;
    std::vector<bool> callSucceeded_;
    std::mutex mutexCallSucceeded_;

    static const std::string serviceAddress_;
    static const uint32_t numCallsPerProxy_;
    static const uint32_t numProxies_;
};

const std::string DBusLoadTest::serviceAddress_ =
                "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService";
const uint32_t DBusLoadTest::numCallsPerProxy_ = 100;

#ifdef WIN32
// test with just 50 proxies under windows as it becomes very slow with more ones
const uint32_t DBusLoadTest::numProxies_ = 50;
#else
const uint32_t DBusLoadTest::numProxies_ = 100;
#endif

// Multiple proxies in one thread, one stub
TEST_F(DBusLoadTest, SingleClientMultipleProxiesSingleStubCallsSucceed) {
    std::array<std::shared_ptr<commonapi::tests::TestInterfaceProxyBase>, numProxies_> testProxies;

    for (unsigned int i = 0; i < numProxies_; i++) {
        testProxies[i] = proxyFactory_->buildProxy < commonapi::tests::TestInterfaceProxy > (serviceAddress_);
        ASSERT_TRUE((bool )testProxies[i]);
    }

    auto stub = std::make_shared<TestInterfaceStubFinal>();
    bool serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
    for (auto i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);

    bool allProxiesAvailable = false;
    for (unsigned int i = 0; !allProxiesAvailable && i < 100; ++i) {
        allProxiesAvailable = true;
        for (unsigned int j = 0; j < numProxies_; ++j) {
            allProxiesAvailable = allProxiesAvailable && testProxies[j]->isAvailable();
        }
        if (!allProxiesAvailable)
            usleep(10000);
    }
    ASSERT_TRUE(allProxiesAvailable);

    uint32_t callId = 0;
    for (unsigned int i = 0; i < numCallsPerProxy_; i++) {
        for (unsigned int j = 0; j < numProxies_; j++) {
            uint32_t in1 = i;
            std::string in2 = "string" + std::to_string(i) + "_" + std::to_string(j);
            testProxies[j]->testPredefinedTypeMethodAsync(
                            in1,
                            in2,
                            std::bind(
                                            &DBusLoadTest::TestPredefinedTypeMethodAsyncCallback,
                                            this,
                                            callId++,
                                            in1,
                                            in2,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3));
        }
    }

    bool allCallsSucceeded = false;
    for (unsigned int i = 0; !allCallsSucceeded && i < 100; ++i) {
        allCallsSucceeded = std::all_of(callSucceeded_.cbegin(), callSucceeded_.cend(), [](int b){ return b; });
        if (!allCallsSucceeded)
            usleep(100000);
    }
    ASSERT_TRUE(allCallsSucceeded);

    servicePublisher_->unregisterService(serviceAddress_);
}

// Multiple proxies in separate threads, one stub
TEST_F(DBusLoadTest, MultipleClientsSingleStubCallsSucceed) {
    std::array<std::shared_ptr<CommonAPI::Factory>, numProxies_> testProxyFactories;
    std::array<std::shared_ptr<commonapi::tests::TestInterfaceProxyBase>, numProxies_> testProxies;

    for (unsigned int i = 0; i < numProxies_; i++) {
        testProxyFactories[i] = runtime_->createFactory();
        ASSERT_TRUE((bool )testProxyFactories[i]);
        testProxies[i] = testProxyFactories[i]->buildProxy < commonapi::tests::TestInterfaceProxy > (serviceAddress_);
        ASSERT_TRUE((bool )testProxies[i]);
    }

    auto stub = std::make_shared<TestInterfaceStubFinal>();
    bool serviceRegistered = false;
    for (auto i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = servicePublisher_->registerService(stub, serviceAddress_, stubFactory_);
        if(!serviceRegistered)
            usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);

    bool allProxiesAvailable = false;
    for (unsigned int i = 0; !allProxiesAvailable && i < 100; ++i) {
        allProxiesAvailable = true;
        for (unsigned int j = 0; j < numProxies_; ++j) {
            allProxiesAvailable = allProxiesAvailable && testProxies[j]->isAvailable();
        }
        if (!allProxiesAvailable)
            usleep(10000);
    }
    ASSERT_TRUE(allProxiesAvailable);

    uint32_t callId = 0;
    for (unsigned int i = 0; i < numCallsPerProxy_; i++) {
        for (unsigned int j = 0; j < numProxies_; j++) {
            uint32_t in1 = i;
            std::string in2 = "string" + std::to_string(i) + "_" + std::to_string(j);
            testProxies[j]->testPredefinedTypeMethodAsync(
                            in1,
                            in2,
                            std::bind(
                                            &DBusLoadTest::TestPredefinedTypeMethodAsyncCallback,
                                            this,
                                            callId++,
                                            in1,
                                            in2,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3));
        }
    }

    bool allCallsSucceeded = false;
    for (unsigned int i = 0; !allCallsSucceeded && i < 100; ++i) {
        allCallsSucceeded = std::all_of(callSucceeded_.cbegin(), callSucceeded_.cend(), [](int b){ return b; });
        if (!allCallsSucceeded)
            usleep(100000);
    }
    ASSERT_TRUE(allCallsSucceeded);

    servicePublisher_->unregisterService(serviceAddress_);
}

// Multiple proxies in separate threads, multiple stubs in separate threads
TEST_F(DBusLoadTest, MultipleClientsMultipleServersCallsSucceed) {
    std::array<std::shared_ptr<CommonAPI::Factory>, numProxies_> testProxyFactories;
    std::array<std::shared_ptr<CommonAPI::Factory>, numProxies_> testStubFactories;
    std::array<std::shared_ptr<commonapi::tests::TestInterfaceProxyBase>, numProxies_> testProxies;
    std::array<std::shared_ptr<commonapi::tests::TestInterfaceStub>, numProxies_> testStubs;

    for (unsigned int i = 0; i < numProxies_; i++) {
        testProxyFactories[i] = runtime_->createFactory();
        ASSERT_TRUE((bool )testProxyFactories[i]);
        testProxies[i] = testProxyFactories[i]->buildProxy < commonapi::tests::TestInterfaceProxy > (serviceAddress_+std::to_string(i));
        ASSERT_TRUE((bool )testProxies[i]);
    }

    for (unsigned int i = 0; i < numProxies_; i++) {
        testStubFactories[i] = runtime_->createFactory();
        ASSERT_TRUE((bool )testStubFactories[i]);
        testStubs[i] = std::make_shared<TestInterfaceStubFinal>();
        ASSERT_TRUE((bool )testStubs[i]);
        bool serviceRegistered = false;
        for (auto j = 0; !serviceRegistered && j < 100; ++j) {
            serviceRegistered = servicePublisher_->registerService(testStubs[i], serviceAddress_+std::to_string(i), testStubFactories[i]);
            if(!serviceRegistered)
                usleep(10000);
        }
        ASSERT_TRUE(serviceRegistered);
    }

    bool allProxiesAvailable = false;
    for (unsigned int i = 0; !allProxiesAvailable && i < 100; ++i) {
        allProxiesAvailable = true;
        for (unsigned int j = 0; j < numProxies_; ++j) {
            allProxiesAvailable = allProxiesAvailable && testProxies[j]->isAvailable();
        }
        if (!allProxiesAvailable)
            usleep(1000000);
    }
    ASSERT_TRUE(allProxiesAvailable);

    uint32_t callId = 0;
    for (unsigned int i = 0; i < numCallsPerProxy_; i++) {
        for (unsigned int j = 0; j < numProxies_; j++) {
            uint32_t in1 = i;
            std::string in2 = "string" + std::to_string(i) + "_" + std::to_string(j);
            testProxies[j]->testPredefinedTypeMethodAsync(
                            in1,
                            in2,
                            std::bind(
                                            &DBusLoadTest::TestPredefinedTypeMethodAsyncCallback,
                                            this,
                                            callId++,
                                            in1,
                                            in2,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3));
        }
    }

    bool allCallsSucceeded = false;
    for (auto i = 0; !allCallsSucceeded && i < 100; ++i) {
        allCallsSucceeded = std::all_of(callSucceeded_.cbegin(), callSucceeded_.cend(), [](int b){ return b; });
        if (!allCallsSucceeded)
            usleep(100000);
    }
    ASSERT_TRUE(allCallsSucceeded);

    for (unsigned int i = 0; i < numProxies_; i++) {
        servicePublisher_->unregisterService(serviceAddress_+std::to_string(i));
    }
}

#ifndef WIN32
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
