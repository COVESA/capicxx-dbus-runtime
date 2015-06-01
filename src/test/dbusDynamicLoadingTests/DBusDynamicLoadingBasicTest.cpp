/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <gtest/gtest.h>

#include <fstream>

#include "DBusDynamicLoadingDefinitions.h"

#define VERSION v1_0

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
        environmentIdentifier_ = COMMONAPI_ENVIRONMENT_BINDING_PATH + "=" ;
        environmentString_ = environmentIdentifier_ + currentWorkingDirectory;
        char* environment = (char*) (environmentString_.c_str());
        putenv(environment);

#ifdef WIN32
		configFileName_ = _pgmptr;
#else
		char cCurrentPath[FILENAME_MAX];
		getcwd(cCurrentPath, sizeof(cCurrentPath);
		configFileName_ = cCurrentPath;
#endif

        configFileName_ += COMMONAPI_CONFIG_SUFFIX;
        std::ofstream configFile(configFileName_);
        ASSERT_TRUE(configFile.is_open());
        configFile << validForLocalDBusBinding;
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


class DBusDynamicLoadingBasicTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

TEST_F(DBusDynamicLoadingBasicTest, LoadsUnconfiguredDefaultDynamicallyLinkedLibrary) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    EXPECT_TRUE((bool)runtime);
}

TEST_F(DBusDynamicLoadingBasicTest, LoadsSpecificDynamicallyLinkedDBusLibrary) {
    //DBus is defined as default binding in the configuration file
    std::shared_ptr<CommonAPI::Runtime> defaultRuntime = CommonAPI::Runtime::get();

    //The Fake binding has "DBus" defined as its alias, so this call will access the Fake binding!
    std::shared_ptr<CommonAPI::Runtime> dbusRuntime = CommonAPI::Runtime::get();

    EXPECT_TRUE((bool)defaultRuntime);
    EXPECT_TRUE((bool)dbusRuntime);

    ASSERT_NE(dbusRuntime, defaultRuntime);
}

TEST_F(DBusDynamicLoadingBasicTest, LoadsAliasedDynamicallyLinkedDBusLibrary) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    EXPECT_TRUE((bool)runtime);
    std::shared_ptr<CommonAPI::Runtime> runtime2 = CommonAPI::Runtime::get();
    EXPECT_TRUE((bool)runtime2);
}

TEST_F(DBusDynamicLoadingBasicTest, ReturnsEmptyPointerOnRequestForUnknownMiddleware) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    EXPECT_FALSE((bool)runtime);
}

TEST_F(DBusDynamicLoadingBasicTest, LoadsDBusLibraryAsSingleton) {
    //"DBus" is set as default. Due to the alias definition in the Fake binding, it would not
    //be accessible when passing this well known name as parameter, only the aliases still
    //point to the DBus binding.
    std::shared_ptr<CommonAPI::Runtime> runtime1 = CommonAPI::Runtime::get();
    std::shared_ptr<CommonAPI::Runtime> runtime2 = CommonAPI::Runtime::get();
    std::shared_ptr<CommonAPI::Runtime> runtime3 = CommonAPI::Runtime::get();
    std::shared_ptr<CommonAPI::Runtime> runtime4 = CommonAPI::Runtime::get();

    EXPECT_TRUE((bool)runtime1);
    EXPECT_TRUE((bool)runtime2);
    EXPECT_TRUE((bool)runtime3);
    EXPECT_TRUE((bool)runtime4);

    EXPECT_EQ(runtime1, runtime2);
    EXPECT_EQ(runtime1, runtime3);
    EXPECT_EQ(runtime2, runtime3);
    EXPECT_EQ(runtime1, runtime4);
    EXPECT_EQ(runtime2, runtime4);
    EXPECT_EQ(runtime3, runtime4);
}

TEST_F(DBusDynamicLoadingBasicTest, RuntimeLoadsFactory) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);
}

TEST_F(DBusDynamicLoadingBasicTest, RuntimeLoadsServicePublisher) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);
}

TEST_F(DBusDynamicLoadingBasicTest, FactoryCanCreateProxies) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);

	auto defaultTestProxy = runtime->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(testServiceAddressDomain, testServiceAddressInstance);
    ASSERT_TRUE((bool)defaultTestProxy);
}

TEST_F(DBusDynamicLoadingBasicTest, FakeFactoryCannotCreateProxies) {
    //Fake has the alias "DBus". Therefore, the actual DBus-binding is NOT accessible via
    //its well known name!
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);

	auto defaultTestProxy = runtime->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(testServiceAddressDomain, testServiceAddressInstance);
    ASSERT_FALSE((bool)defaultTestProxy);
}

TEST_F(DBusDynamicLoadingBasicTest, ServicePublisherCanRegisterStubs) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);

	auto myStub = std::make_shared<VERSION::commonapi::tests::TestInterfaceStubDefault>();

	EXPECT_TRUE(runtime->registerService(testServiceAddressDomain, testServiceAddressInstance, myStub));
	CommonAPI::Address address = myStub->getStubAdapter()->getAddress();
	EXPECT_TRUE(runtime->unregisterService(address.getDomain(), address.getInterface(), address.getInstance()));
}

TEST_F(DBusDynamicLoadingBasicTest, FakeServicePublisherTellsUsItWontRegisterStubs) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);

	auto myStub = std::make_shared<VERSION::commonapi::tests::TestInterfaceStubDefault>();

	EXPECT_TRUE(runtime->registerService(testServiceAddressDomain, testServiceAddressInstance, myStub));
	CommonAPI::Address address = myStub->getStubAdapter()->getAddress();
	EXPECT_FALSE(runtime->unregisterService(address.getDomain(), address.getInterface(), address.getInstance()));
}

TEST_F(DBusDynamicLoadingBasicTest, CreatedProxiesAndServicesCanCommunicate) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);

	auto defaultTestProxy = runtime->buildProxy<VERSION::commonapi::tests::TestInterfaceProxy>(testServiceAddressDomain, testServiceAddressInstance);
    ASSERT_TRUE((bool)defaultTestProxy);

    auto myStub = std::make_shared<VERSION::commonapi::tests::TestInterfaceStubDefault>();

	runtime->registerService(testServiceAddressDomain, testServiceAddressInstance, myStub);

    for (uint32_t i = 0; i < 300 && !defaultTestProxy->isAvailable(); ++i) {
        usleep(1000);
    }
    EXPECT_TRUE(defaultTestProxy->isAvailable());

    CommonAPI::CallStatus status;
    defaultTestProxy->testEmptyMethod(status);
	ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, status);

	CommonAPI::Address address = myStub->getStubAdapter()->getAddress();
	runtime->unregisterService(address.getDomain(), address.getInterface(), address.getInstance());
}

#ifndef __NO_MAIN__
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
#endif