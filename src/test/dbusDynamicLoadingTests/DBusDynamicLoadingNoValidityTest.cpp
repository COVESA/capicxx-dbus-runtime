/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <gtest/gtest.h>

#include <fstream>

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

        configFileName_ = CommonAPI::getCurrentBinaryFileFQN();
        configFileName_ += COMMONAPI_CONFIG_SUFFIX;
        std::ofstream configFile(configFileName_);
        ASSERT_TRUE(configFile.is_open());
        configFile << noValidityValuesAndBindings;
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


class DBusDynamicLoadingFullyInvalidConfigTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};


TEST_F(DBusDynamicLoadingFullyInvalidConfigTest, ErrorOnLoadingWronglyConfiguredDefaultDynamicallyLinkedLibrary) {
    CommonAPI::Runtime::LoadState loadState;
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load(loadState);
    ASSERT_EQ(CommonAPI::Runtime::LoadState::CONFIGURATION_ERROR, loadState);
}

TEST_F(DBusDynamicLoadingFullyInvalidConfigTest, ErrorOnLoadingWronglyConfiguredGeneratedCodePath) {
    CommonAPI::Runtime::LoadState loadState;
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load("DBus", loadState);
    ASSERT_EQ(CommonAPI::Runtime::LoadState::BINDING_ERROR, loadState);
}

TEST_F(DBusDynamicLoadingFullyInvalidConfigTest, LoadsNoAliasedDynamicallyLinkedLibrary) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load("MyFirstAlias");
    //The first alias is claimed by another binding, which was defined earlier in the config
    EXPECT_FALSE((bool)runtime);
    std::shared_ptr<CommonAPI::Runtime> runtime2 = CommonAPI::Runtime::load("MySecondAlias");
    EXPECT_FALSE((bool)runtime2);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
