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

#include <CommonAPI/CommonAPI.h>

#define COMMONAPI_INTERNAL_COMPILATION

#include <CommonAPI/AttributeExtension.h>
#include <CommonAPI/types.h>

#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusProxy.h>
#include <CommonAPI/DBus/DBusRuntime.h>

#include "commonapi/tests/PredefinedTypeCollection.h"
#include "commonapi/tests/DerivedTypeCollection.h"
#include "commonapi/tests/TestInterfaceProxy.h"
#include "commonapi/tests/TestInterfaceStubDefault.h"
#include "commonapi/tests/TestInterfaceDBusStubAdapter.h"

#include "commonapi/tests/TestInterfaceDBusProxy.h"


class DBusServicePublisherTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
        runtime_ = CommonAPI::Runtime::load();
        ASSERT_TRUE((bool)runtime_);
    }

    virtual void TearDown() {
        usleep(30000);
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
};


TEST_F(DBusServicePublisherTest, HandlesManagementOfServices) {
    std::shared_ptr<CommonAPI::Factory> serviceFactory = runtime_->createFactory();
    ASSERT_TRUE((bool)serviceFactory);
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher = runtime_->getServicePublisher();
    ASSERT_TRUE((bool)servicePublisher);

    const std::string serviceAddress = "local:commonapi.tests.TestInterface:commonapi.tests.TestInterface";
    const std::string serviceAddress2 = "local:commonapi.tests.TestInterfaceTwo:commonapi.tests.TestInterface";

    auto myStub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
    bool success = servicePublisher->registerService(myStub, serviceAddress, serviceFactory);
    EXPECT_TRUE(success);

    auto myStub2 = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
    success = servicePublisher->registerService(myStub2, serviceAddress2, serviceFactory);
    EXPECT_TRUE(success);

    std::shared_ptr<CommonAPI::Factory> clientFactory = runtime_->createFactory();

    usleep(10000);

    bool instanceIsAvailable = clientFactory->isServiceInstanceAlive(serviceAddress);
    EXPECT_TRUE(instanceIsAvailable);
    instanceIsAvailable = clientFactory->isServiceInstanceAlive(serviceAddress2);
    EXPECT_TRUE(instanceIsAvailable);

    success = servicePublisher->unregisterService("SomeOther:Unknown:Service");
    EXPECT_FALSE(success);

    success = servicePublisher->unregisterService(serviceAddress);
    EXPECT_TRUE(success);

    success = servicePublisher->unregisterService(serviceAddress2);
    EXPECT_TRUE(success);

    usleep(10000);

    instanceIsAvailable = clientFactory->isServiceInstanceAlive(serviceAddress);
    EXPECT_FALSE(instanceIsAvailable);
    instanceIsAvailable = clientFactory->isServiceInstanceAlive(serviceAddress2);
    EXPECT_FALSE(instanceIsAvailable);
}


TEST_F(DBusServicePublisherTest, GracefullyHandlesWrongAddresses) {
    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime_->createFactory();
    ASSERT_TRUE((bool)proxyFactory);
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher = runtime_->getServicePublisher();
    ASSERT_TRUE((bool)servicePublisher);

    auto myStub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

    EXPECT_FALSE(servicePublisher->registerService(myStub, "", proxyFactory));
    EXPECT_FALSE(servicePublisher->registerService(myStub, "too:much:stuff:here", proxyFactory));
}

#ifndef WIN32
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif