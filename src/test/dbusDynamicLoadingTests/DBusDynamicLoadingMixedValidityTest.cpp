/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <gtest/gtest.h>

#include <stdexcept>
#include <fstream>

#include "DBusDynamicLoadingDefinitions.h"

#ifndef COMMONAPI_INTERNAL_COMPILATION
#define COMMONAPI_INTERNAL_COMPILATION
#endif
#include <CommonAPI/DBus/DBusRuntime.h>


class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
        environmentIdentifier_ = COMMONAPI_ENVIRONMENT_BINDING_PATH + "=" ;
        environmentString_ = environmentIdentifier_ + currentWorkingDirectory;
        char* environment = (char*) (environmentString_.c_str());
        putenv(environment);

        configFileName_ = CommonAPI::getCurrentBinaryFileFQN();
        configFileName_ += COMMONAPI_CONFIG_SUFFIX;
        std::ofstream configFile(configFileName_);
        ASSERT_TRUE(configFile.is_open());
        configFile << mixedValidityValuesAndBindings;
        configFile.close();
    }

    virtual void TearDown() {
        std::remove(configFileName_.c_str());

        char* environment = (char*) (environmentIdentifier_.c_str());
        putenv(environment);
    }

    std::string configFileName_;
    std::string environmentIdentifier_;
    std::string environmentString_;
};


class DBusDynamicLoadingPartiallyInvalidConfigTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};


TEST_F(DBusDynamicLoadingPartiallyInvalidConfigTest, LoadsUnconfiguredDefaultDynamicallyLinkedLibrary) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
    EXPECT_TRUE((bool)runtime);
}

TEST_F(DBusDynamicLoadingPartiallyInvalidConfigTest, LoadsSpecificDynamicallyLinkedDBusLibrary) {
    std::shared_ptr<CommonAPI::Runtime> defaultRuntime = CommonAPI::Runtime::load();
    std::shared_ptr<CommonAPI::Runtime> fakeRuntime = CommonAPI::Runtime::load("Fake");
    EXPECT_TRUE((bool)defaultRuntime);
    EXPECT_TRUE((bool)fakeRuntime);
    //The DBus binding is alphabetically before the Fake binding, so the DBusRuntime will be loaded as default
    ASSERT_NE(fakeRuntime, defaultRuntime);
}

TEST_F(DBusDynamicLoadingPartiallyInvalidConfigTest, LoadsAliasedDynamicallyLinkedDBusLibrary) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load("MyFirstAlias");
    //The first alias is claimed by another binding, which was defined earlier in the config
    EXPECT_FALSE((bool)runtime);
    std::shared_ptr<CommonAPI::Runtime> runtime2 = CommonAPI::Runtime::load("MySecondAlias");
    EXPECT_TRUE((bool)runtime2);
}

TEST_F(DBusDynamicLoadingPartiallyInvalidConfigTest, ReturnsEmptyPointerOnRequestForUnknownMiddleware) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load("NonExisting");
    EXPECT_FALSE((bool)runtime);
}

TEST_F(DBusDynamicLoadingPartiallyInvalidConfigTest, LoadsDBusLibraryAsSingleton) {
    std::shared_ptr<CommonAPI::Runtime> runtime1 = CommonAPI::Runtime::load("DBus");
    std::shared_ptr<CommonAPI::Runtime> runtime2 = CommonAPI::Runtime::load("MyFirstAlias");
    std::shared_ptr<CommonAPI::Runtime> runtime3 = CommonAPI::Runtime::load("MySecondAlias");
    std::shared_ptr<CommonAPI::Runtime> runtime4 = CommonAPI::Runtime::load("DBus");
    EXPECT_TRUE((bool)runtime1);
    //The first alias is claimed by another binding, which was defined earlier in the config
    EXPECT_FALSE((bool)runtime2);
    EXPECT_TRUE((bool)runtime3);
    EXPECT_TRUE((bool)runtime4);

    EXPECT_NE(runtime1, runtime2);
    EXPECT_EQ(runtime1, runtime3);
    EXPECT_NE(runtime2, runtime3);
    EXPECT_EQ(runtime1, runtime4);
    EXPECT_NE(runtime2, runtime4);
    EXPECT_EQ(runtime3, runtime4);
}

TEST_F(DBusDynamicLoadingPartiallyInvalidConfigTest, RuntimeLoadsFactory) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load("DBus");
    ASSERT_TRUE((bool)runtime);

    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime->createFactory();
    EXPECT_TRUE((bool)proxyFactory);
}

TEST_F(DBusDynamicLoadingPartiallyInvalidConfigTest, RuntimeLoadsServicePublisher) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load("DBus");
    ASSERT_TRUE((bool)runtime);

    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher = runtime->getServicePublisher();
    EXPECT_TRUE((bool)servicePublisher);
}

TEST_F(DBusDynamicLoadingPartiallyInvalidConfigTest, FactoryCanCreateProxies) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load("DBus");
    ASSERT_TRUE((bool)runtime);

    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime->createFactory();
    EXPECT_TRUE((bool)proxyFactory);

    auto defaultTestProxy = proxyFactory->buildProxy<commonapi::tests::TestInterfaceProxy>(testServiceAddress);
    ASSERT_TRUE((bool)defaultTestProxy);
}

TEST_F(DBusDynamicLoadingPartiallyInvalidConfigTest, ServicePublisherCanRegisterStubs) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load("DBus");
    ASSERT_TRUE((bool)runtime);

    std::shared_ptr<CommonAPI::Factory> serviceFactory = runtime->createFactory();
    EXPECT_TRUE((bool)serviceFactory);

    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher = runtime->getServicePublisher();
    EXPECT_TRUE((bool)servicePublisher);

    auto myStub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

    servicePublisher->registerService(myStub, testServiceAddress, serviceFactory);
    servicePublisher->unregisterService(testServiceAddress);
}

TEST_F(DBusDynamicLoadingPartiallyInvalidConfigTest, CreatedProxiesAndServicesCanCommunicate) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load("DBus");
    ASSERT_TRUE((bool)runtime);

    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime->createFactory();
    EXPECT_TRUE((bool)proxyFactory);

    auto defaultTestProxy = proxyFactory->buildProxy<commonapi::tests::TestInterfaceProxy>(testServiceAddress);
    ASSERT_TRUE((bool)defaultTestProxy);

    std::shared_ptr<CommonAPI::Factory> serviceFactory = runtime->createFactory();
    EXPECT_TRUE((bool)serviceFactory);

    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher = runtime->getServicePublisher();
    EXPECT_TRUE((bool)servicePublisher);

    auto myStub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

    servicePublisher->registerService(myStub, testServiceAddress, serviceFactory);

    for (uint32_t i = 0; i < 300 && !defaultTestProxy->isAvailable(); ++i) {
        usleep(1000);
    }
    EXPECT_TRUE(defaultTestProxy->isAvailable());

    CommonAPI::CallStatus status;
    defaultTestProxy->testEmptyMethod(status);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, status);

    servicePublisher->unregisterService(testServiceAddress);
}

TEST_F(DBusDynamicLoadingPartiallyInvalidConfigTest, ErrorOnLoadingRuntimeForBrokenPath) {
    CommonAPI::Runtime::LoadState loadState;
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load("BrokenPathBinding", loadState);
    ASSERT_EQ(CommonAPI::Runtime::LoadState::CONFIGURATION_ERROR, loadState);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
