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
#include <CommonAPI/AttributeExtension.h>
#include <CommonAPI/Runtime.h>

#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusProxy.h>
#include <CommonAPI/DBus/DBusRuntime.h>

#include "commonapi/tests/PredefinedTypeCollection.h"
#include "commonapi/tests/DerivedTypeCollection.h"
#include "commonapi/tests/TestInterfaceProxy.h"
#include "commonapi/tests/TestInterfaceStubDefault.h"
#include "commonapi/tests/TestInterfaceDBusStubAdapter.h"

#include "commonapi/tests/TestInterfaceDBusProxy.h"


class DBusProxyFactoryTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
        runtime_ = CommonAPI::Runtime::load();
        ASSERT_TRUE((bool)runtime_);
        CommonAPI::DBus::DBusRuntime* dbusRuntime = dynamic_cast<CommonAPI::DBus::DBusRuntime*>(&(*runtime_));
        ASSERT_TRUE(dbusRuntime != NULL);
    }

    virtual void TearDown() {
        usleep(30000);
    }

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

TEST_F(DBusProxyFactoryTest, HandlesRegistrationOfStubAdapters) {
    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime_->createFactory();
    ASSERT_TRUE((bool)proxyFactory);

    const std::string serviceAddress = "local:commonapi.tests.TestInterface:commonapi.tests.TestInterface";

    auto myStub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
    bool success = proxyFactory->registerService(myStub, serviceAddress);
    ASSERT_TRUE(success);

    success = proxyFactory->unregisterService("SomeOther:Unknown:Service");
    ASSERT_FALSE(success);

    success = proxyFactory->unregisterService(serviceAddress);
    ASSERT_TRUE(success);
}

TEST_F(DBusProxyFactoryTest, GracefullyHandlesWrongAddresses) {
    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime_->createFactory();
    ASSERT_TRUE((bool)proxyFactory);
    auto myStub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

    ASSERT_FALSE(proxyFactory->registerService(myStub, ""));
    ASSERT_FALSE(proxyFactory->registerService(myStub, "too:much:stuff:here"));
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
