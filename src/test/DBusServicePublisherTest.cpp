// Copyright (C) 2013-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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

#include <CommonAPI/CommonAPI.hpp>

#ifndef COMMONAPI_INTERNAL_COMPILATION
#define COMMONAPI_INTERNAL_COMPILATION
#endif

#include <CommonAPI/DBus/DBusConnection.hpp>
#include <CommonAPI/DBus/DBusProxy.hpp>

#include "commonapi/tests/PredefinedTypeCollection.hpp"
#include "commonapi/tests/DerivedTypeCollection.hpp"
#include "v1_0/commonapi/tests/TestInterfaceProxy.hpp"
#include "v1_0/commonapi/tests/TestInterfaceStubDefault.hpp"
#include "v1_0/commonapi/tests/TestInterfaceDBusStubAdapter.hpp"

#include "v1_0/commonapi/tests/TestInterfaceDBusProxy.hpp"

#define VERSION v1_0

class DBusServicePublisherTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);
    }

    virtual void TearDown() {
        usleep(30000);
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
};

/** TODO: CHECK IF STILL USEFUL **/

/*
TEST_F(DBusServicePublisherTest, HandlesManagementOfServices) {
	const std::string domain = "local";
    const std::string serviceAddress = "local:commonapi.tests.TestInterface:commonapi.tests.TestInterface";
    const std::string serviceAddress2 = "local:commonapi.tests.TestInterfaceTwo:commonapi.tests.TestInterface";

    auto myStub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
	bool success = runtime_->registerService(domain, serviceAddress, myStub);
    EXPECT_TRUE(success);

    auto myStub2 = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
	success = runtime_->registerService(domain, serviceAddress2, myStub2);
    EXPECT_TRUE(success);

    usleep(10000);

	bool instanceIsAvailable = runtime_->isServiceInstanceAlive(serviceAddress);
    EXPECT_TRUE(instanceIsAvailable);
	instanceIsAvailable = runtime_->isServiceInstanceAlive(serviceAddress2);
    EXPECT_TRUE(instanceIsAvailable);

	success = runtime_->unregisterService("SomeOther:Unknown:Service");
    EXPECT_FALSE(success);

	success = runtime_->unregisterService(serviceAddress);
    EXPECT_TRUE(success);

	success = runtime_->unregisterService(serviceAddress2);
    EXPECT_TRUE(success);

    usleep(10000);

	instanceIsAvailable = runtime_->isServiceInstanceAlive(serviceAddress);
    EXPECT_FALSE(instanceIsAvailable);
	instanceIsAvailable = runtime_->isServiceInstanceAlive(serviceAddress2);
    EXPECT_FALSE(instanceIsAvailable);
}
*/

TEST_F(DBusServicePublisherTest, GracefullyHandlesWrongAddresses) {
	auto myStub = std::make_shared<VERSION::commonapi::tests::TestInterfaceStubDefault>();

	EXPECT_FALSE(runtime_->registerService("local", "", myStub, "connection"));
	EXPECT_FALSE(runtime_->registerService("local", "too:much:stuff:here", myStub, "connection"));
}

#ifndef __NO_MAIN__
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
