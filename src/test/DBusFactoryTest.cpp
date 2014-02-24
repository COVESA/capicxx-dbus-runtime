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
#include <fstream>
#include <functional>
#include <memory>
#include <stdint.h>
#include <string>
#include <utility>
#include <tuple>
#include <type_traits>

#include <CommonAPI/CommonAPI.h>

#define COMMONAPI_INTERNAL_COMPILATION

#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusProxy.h>
#include <CommonAPI/DBus/DBusRuntime.h>
#include <CommonAPI/DBus/DBusUtils.h>

#include "commonapi/tests/PredefinedTypeCollection.h"
#include "commonapi/tests/DerivedTypeCollection.h"
#include "commonapi/tests/TestInterfaceProxy.h"
#include "commonapi/tests/TestInterfaceStubDefault.h"
#include "commonapi/tests/TestInterfaceDBusStubAdapter.h"

#include "commonapi/tests/TestInterfaceDBusProxy.h"

static const std::string fileString =
""
"[factory$session]\n"
"dbus_bustype=session\n"
"[factory$system]\n"
"dbus_bustype=system\n"
"";


class DBusProxyFactoryTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
        runtime_ = CommonAPI::Runtime::load();
        ASSERT_TRUE((bool)runtime_);
        CommonAPI::DBus::DBusRuntime* dbusRuntime = dynamic_cast<CommonAPI::DBus::DBusRuntime*>(&(*runtime_));
        ASSERT_TRUE(dbusRuntime != NULL);

        configFileName_ = CommonAPI::getCurrentBinaryFileFQN();
        configFileName_ += CommonAPI::DBus::DBUS_CONFIG_SUFFIX;
        std::ofstream configFile(configFileName_);
        ASSERT_TRUE(configFile.is_open());
        configFile << fileString;
        configFile.close();
    }

    virtual void TearDown() {
        usleep(30000);
        std::remove(configFileName_.c_str());
    }

    std::string configFileName_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;
};



namespace myExtensions {

template<typename _AttributeType>
class AttributeTestExtension: public CommonAPI::AttributeExtension<_AttributeType> {
    typedef CommonAPI::AttributeExtension<_AttributeType> __baseClass_t;

public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

    AttributeTestExtension(_AttributeType& baseAttribute) :
                    CommonAPI::AttributeExtension<_AttributeType>(baseAttribute) {}

   ~AttributeTestExtension() {}

   bool testExtensionMethod() const {
       return true;
   }
};

} // namespace myExtensions

//####################################################################################################################

TEST_F(DBusProxyFactoryTest, DBusFactoryCanBeCreated) {
    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime_->createFactory();
    ASSERT_TRUE((bool)proxyFactory);
    CommonAPI::DBus::DBusFactory* dbusProxyFactory = dynamic_cast<CommonAPI::DBus::DBusFactory*>(&(*proxyFactory));
    ASSERT_TRUE(dbusProxyFactory != NULL);
}

TEST_F(DBusProxyFactoryTest, CreatesDefaultTestProxy) {
    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime_->createFactory();
    ASSERT_TRUE((bool)proxyFactory);
    auto defaultTestProxy = proxyFactory->buildProxy<commonapi::tests::TestInterfaceProxy>("local:commonapi.tests.TestInterface:commonapi.tests.TestInterface");
    ASSERT_TRUE((bool)defaultTestProxy);
}

TEST_F(DBusProxyFactoryTest, CreatesDefaultExtendedTestProxy) {
    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime_->createFactory();
    ASSERT_TRUE((bool)proxyFactory);
    auto defaultTestProxy = proxyFactory->buildProxyWithDefaultAttributeExtension<
                    commonapi::tests::TestInterfaceProxy,
                    myExtensions::AttributeTestExtension>("local:commonapi.tests.TestInterface:commonapi.tests.TestInterface");
    ASSERT_TRUE((bool)defaultTestProxy);

    auto attributeExtension = defaultTestProxy->getTestDerivedArrayAttributeAttributeExtension();
    ASSERT_TRUE(attributeExtension.testExtensionMethod());
}

TEST_F(DBusProxyFactoryTest, CreatesIndividuallyExtendedTestProxy) {
    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime_->createFactory();
    ASSERT_TRUE((bool)proxyFactory);
    auto specificAttributeExtendedTestProxy = proxyFactory->buildProxy<
                    commonapi::tests::TestInterfaceProxy,
                    commonapi::tests::TestInterfaceExtensions::TestDerivedArrayAttributeAttributeExtension<myExtensions::AttributeTestExtension> >
            ("local:commonapi.tests.TestInterface:commonapi.tests.TestInterface");

    ASSERT_TRUE((bool)specificAttributeExtendedTestProxy);

    auto attributeExtension = specificAttributeExtendedTestProxy->getTestDerivedArrayAttributeAttributeExtension();
    ASSERT_TRUE(attributeExtension.testExtensionMethod());
}

TEST_F(DBusProxyFactoryTest, CreateNamedFactory) {
    std::shared_ptr<CommonAPI::Factory> defaultFactory = runtime_->createFactory(
                    std::shared_ptr<CommonAPI::MainLoopContext>(NULL),
                    "nonexistingFactoryName",
                    false);
    ASSERT_TRUE((bool)defaultFactory);
    std::shared_ptr<CommonAPI::Factory> configuredFactory = runtime_->createFactory(
                    std::shared_ptr<CommonAPI::MainLoopContext>(NULL),
                    "configuredFactory",
                    true);
    ASSERT_FALSE((bool)configuredFactory);
    std::shared_ptr<CommonAPI::Factory> configuredFactory2 = runtime_->createFactory(
                    std::shared_ptr<CommonAPI::MainLoopContext>(NULL),
                    "session",
                    true);
    ASSERT_TRUE((bool)configuredFactory2);
    std::shared_ptr<CommonAPI::Factory> configuredFactory3 = runtime_->createFactory(
                    std::shared_ptr<CommonAPI::MainLoopContext>(NULL),
                    "system",
                    true);
    ASSERT_TRUE((bool)configuredFactory3);
    std::shared_ptr<CommonAPI::Factory> nullFactory = runtime_->createFactory(
                    std::shared_ptr<CommonAPI::MainLoopContext>(NULL),
                    "nonexistingFactoryName",
                    true);
    ASSERT_TRUE(nullFactory == NULL);
}

#ifndef WIN32
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif