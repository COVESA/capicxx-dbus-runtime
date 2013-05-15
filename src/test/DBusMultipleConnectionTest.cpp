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
#include <utility>
#include <tuple>
#include <type_traits>

#include <CommonAPI/types.h>
#include <CommonAPI/Runtime.h>

#include "commonapi/tests/TestInterfaceProxy.h"
#include "commonapi/tests/TestInterfaceStubDefault.h"


const std::string serviceAddress = "local:commonapi.tests.TestInterface:commonapi.tests.TestInterface";

class DBusMultipleConnectionTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
        proxyFactory = CommonAPI::Runtime::load()->createFactory();
        stubFactory = CommonAPI::Runtime::load()->createFactory();
        ASSERT_TRUE((bool)proxyFactory);
        ASSERT_TRUE((bool)stubFactory);

        stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
        bool serviceNameAcquired = stubFactory->registerService(stub, serviceAddress);

        for(unsigned int i = 0; !serviceNameAcquired && i < 100; i++) {
            usleep(10000);
            serviceNameAcquired = stubFactory->registerService(stub, serviceAddress);
        }
        ASSERT_TRUE(serviceNameAcquired);

        proxy = proxyFactory->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress);
        ASSERT_TRUE((bool)proxy);

        for(unsigned int i = 0; !proxy->isAvailable() && i < 100; ++i) {
            usleep(10000);
        }
    }

    virtual void TearDown() {
    	stubFactory->unregisterService(serviceAddress);
    	usleep(30000);
    }

    std::shared_ptr<CommonAPI::Factory> proxyFactory;
    std::shared_ptr<CommonAPI::Factory> stubFactory;
    std::shared_ptr<commonapi::tests::TestInterfaceStubDefault> stub;
    std::shared_ptr<commonapi::tests::TestInterfaceProxy<> > proxy;

};


TEST_F(DBusMultipleConnectionTest, RemoteMethodCall) {
    uint32_t v1 = 5;
    std::string v2 = "Hai :)";
    CommonAPI::CallStatus stat;
    proxy->testVoidPredefinedTypeMethod(v1, v2, stat);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, stat);
}

TEST_F(DBusMultipleConnectionTest, Broadcast) {
    uint32_t v1 = 5;
    uint32_t v3 = 0;
    std::string v2 = "Hai :)";

    std::promise<bool> promise;
    auto future = promise.get_future();

    auto subscription = proxy->getTestPredefinedTypeBroadcastEvent().subscribe([&](
                    const uint32_t intVal, const std::string& strVal) {
        v3 = intVal;
        promise.set_value(true);
    });

    stub->fireTestPredefinedTypeBroadcastEvent(v1, v2);

    ASSERT_TRUE(future.get());
    ASSERT_EQ(v1, v3);

    proxy->getTestPredefinedTypeBroadcastEvent().unsubscribe(subscription);
}

TEST_F(DBusMultipleConnectionTest, SetAttribute) {
    uint32_t v1 = 5;
    uint32_t v2;
    CommonAPI::CallStatus stat;
    proxy->getTestPredefinedTypeAttributeAttribute().setValue(v1, stat, v2);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, stat);
    ASSERT_EQ(v1, v2);
}

TEST_F(DBusMultipleConnectionTest, SetAttributeBroadcast) {
    uint32_t v1 = 6;
    uint32_t v2;
    uint32_t v3 = 0;

    std::promise<bool> promise;
    auto future = promise.get_future();

    auto subscription = proxy->getTestPredefinedTypeAttributeAttribute().getChangedEvent().subscribe([&](
                    const uint32_t intVal) {
        v3 = intVal;
        promise.set_value(true);
    });

    CommonAPI::CallStatus stat;
    proxy->getTestPredefinedTypeAttributeAttribute().setValue(v1, stat, v2);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, stat);
    ASSERT_EQ(v1, v2);

    ASSERT_TRUE(future.get());
    ASSERT_EQ(v1, v3);

    proxy->getTestPredefinedTypeAttributeAttribute().getChangedEvent().unsubscribe(subscription);
}


TEST_F(DBusMultipleConnectionTest, GetAttribute) {
    uint32_t v1;
    CommonAPI::CallStatus stat;
    proxy->getTestPredefinedTypeAttributeAttribute().getValue(stat, v1);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, stat);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

