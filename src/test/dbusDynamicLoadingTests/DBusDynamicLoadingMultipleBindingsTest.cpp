/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <gtest/gtest.h>

#include "DBusDynamicLoadingDefinitions.h"


class Environment: public ::testing::Environment {
 public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
        environmentIdentifier_ = COMMONAPI_ENVIRONMENT_BINDING_PATH + "=" ;
        environmentString_ = environmentIdentifier_ + currentWorkingDirectory;
        char* environment = (char*) (environmentString_.c_str());
        putenv(environment);
    }

    virtual void TearDown() {
        char* environment = (char*) (environmentIdentifier_.c_str());
        putenv(environment);
    }

 private:
    std::string environmentIdentifier_;
    std::string environmentString_;
};


class DBusDynamicLoadingMultipleBindingsTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};


TEST_F(DBusDynamicLoadingMultipleBindingsTest, LoadsUnconfiguredDefaultDynamicallyLinkedLibrary) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
    EXPECT_TRUE((bool)runtime);
}

TEST_F(DBusDynamicLoadingMultipleBindingsTest, LoadsSpecificDynamicallyLinkedDBusLibrary) {
    std::shared_ptr<CommonAPI::Runtime> defaultRuntime = CommonAPI::Runtime::load();
    std::shared_ptr<CommonAPI::Runtime> fakeRuntime = CommonAPI::Runtime::load("Fake");
    EXPECT_TRUE((bool)defaultRuntime);
    EXPECT_TRUE((bool)fakeRuntime);
    //The DBus binding is alphabetically before the Fake binding, so the DBusRuntime will be loaded as default
    ASSERT_NE(fakeRuntime, defaultRuntime);
}

TEST_F(DBusDynamicLoadingMultipleBindingsTest, LoadsSpecificDynamicallyLinkedFakeLibrary) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load("Fake");
    EXPECT_TRUE((bool)runtime);
}

TEST_F(DBusDynamicLoadingMultipleBindingsTest, LoadsBothDynamicallyLinkedLibrariesAtTheSameTime) {
    std::shared_ptr<CommonAPI::Runtime> dbusRuntime = CommonAPI::Runtime::load("DBus");
    EXPECT_TRUE((bool)dbusRuntime);
    std::shared_ptr<CommonAPI::Runtime> fakeRuntime = CommonAPI::Runtime::load("Fake");
    EXPECT_TRUE((bool)fakeRuntime);

    ASSERT_NE(dbusRuntime, fakeRuntime);
}

TEST_F(DBusDynamicLoadingMultipleBindingsTest, LoadsBothLibrariesAsSingletons) {
    std::shared_ptr<CommonAPI::Runtime> dbusRuntime1 = CommonAPI::Runtime::load("DBus");
    std::shared_ptr<CommonAPI::Runtime> dbusRuntime2 = CommonAPI::Runtime::load("DBus");
    std::shared_ptr<CommonAPI::Runtime> dbusRuntime3 = CommonAPI::Runtime::load("DBus");
    EXPECT_TRUE((bool)dbusRuntime1);
    EXPECT_TRUE((bool)dbusRuntime2);
    EXPECT_TRUE((bool)dbusRuntime3);

    EXPECT_EQ(dbusRuntime1, dbusRuntime2);
    EXPECT_EQ(dbusRuntime1, dbusRuntime3);
    EXPECT_EQ(dbusRuntime2, dbusRuntime3);

    std::shared_ptr<CommonAPI::Runtime> fakeRuntime1 = CommonAPI::Runtime::load("Fake");
    std::shared_ptr<CommonAPI::Runtime> fakeRuntime2 = CommonAPI::Runtime::load("Fake");
    std::shared_ptr<CommonAPI::Runtime> fakeRuntime3 = CommonAPI::Runtime::load("Fake");
    EXPECT_TRUE((bool)fakeRuntime1);
    EXPECT_TRUE((bool)fakeRuntime2);
    EXPECT_TRUE((bool)fakeRuntime3);

    EXPECT_EQ(fakeRuntime1, fakeRuntime2);
    EXPECT_EQ(fakeRuntime1, fakeRuntime3);
    EXPECT_EQ(fakeRuntime2, fakeRuntime3);

    EXPECT_NE(fakeRuntime1, dbusRuntime1);
    EXPECT_NE(fakeRuntime1, dbusRuntime2);
    EXPECT_NE(fakeRuntime1, dbusRuntime3);

    EXPECT_NE(fakeRuntime2, dbusRuntime1);
    EXPECT_NE(fakeRuntime2, dbusRuntime2);
    EXPECT_NE(fakeRuntime2, dbusRuntime3);

    EXPECT_NE(fakeRuntime3, dbusRuntime1);
    EXPECT_NE(fakeRuntime3, dbusRuntime2);
    EXPECT_NE(fakeRuntime3, dbusRuntime3);
}

TEST_F(DBusDynamicLoadingMultipleBindingsTest, RuntimesLoadBothFactories) {
    std::shared_ptr<CommonAPI::Runtime> dbusRuntime = CommonAPI::Runtime::load("DBus");
    ASSERT_TRUE((bool)dbusRuntime);
    std::shared_ptr<CommonAPI::Factory> dbusProxyFactory = dbusRuntime->createFactory();
    EXPECT_TRUE((bool)dbusProxyFactory);

    std::shared_ptr<CommonAPI::Runtime> fakeRuntime = CommonAPI::Runtime::load("Fake");
    ASSERT_TRUE((bool)fakeRuntime);
    std::shared_ptr<CommonAPI::Factory> fakeProxyFactory = dbusRuntime->createFactory();
    EXPECT_TRUE((bool)fakeProxyFactory);

    ASSERT_NE(dbusRuntime, fakeRuntime);
    EXPECT_NE(dbusProxyFactory, fakeProxyFactory);
}

TEST_F(DBusDynamicLoadingMultipleBindingsTest, RuntimesLoadBothServicePublishers) {
    std::shared_ptr<CommonAPI::Runtime> dbusRuntime = CommonAPI::Runtime::load("DBus");
    ASSERT_TRUE((bool)dbusRuntime);
    std::shared_ptr<CommonAPI::ServicePublisher> dbusServicePublisher = dbusRuntime->getServicePublisher();
    EXPECT_TRUE((bool)dbusServicePublisher);

    std::shared_ptr<CommonAPI::Runtime> fakeRuntime = CommonAPI::Runtime::load("Fake");
    ASSERT_TRUE((bool)fakeRuntime);
    std::shared_ptr<CommonAPI::ServicePublisher> fakeServicePublisher = fakeRuntime->getServicePublisher();
    EXPECT_TRUE((bool)fakeServicePublisher);

    ASSERT_NE(dbusRuntime, fakeRuntime);
    EXPECT_NE(dbusServicePublisher, fakeServicePublisher);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
