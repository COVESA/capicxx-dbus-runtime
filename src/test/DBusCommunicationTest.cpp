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


class DBusCommunicationTest: public ::testing::Test {
 protected:
    virtual void SetUp() {
        runtime_ = CommonAPI::Runtime::load();
        ASSERT_TRUE((bool)runtime_);
        CommonAPI::DBus::DBusRuntime* dbusRuntime = dynamic_cast<CommonAPI::DBus::DBusRuntime*>(&(*runtime_));
        ASSERT_TRUE(dbusRuntime != NULL);

        proxyFactory_ = runtime_->createFactory();
        ASSERT_TRUE((bool)proxyFactory_);
        stubFactory_ = runtime_->createFactory();
        ASSERT_TRUE((bool)stubFactory_);
    }

    virtual void TearDown() {
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<CommonAPI::Factory> proxyFactory_;
    std::shared_ptr<CommonAPI::Factory> stubFactory_;

    static const std::string serviceAddress_;
    static const std::string nonstandardAddress_;
};

const std::string DBusCommunicationTest::serviceAddress_ = "local:commonapi.tests.TestInterface:commonapi.tests.TestInterface";
const std::string DBusCommunicationTest::nonstandardAddress_ = "local:non.standard.ServiceName:non.standard.participand.ID";



TEST_F(DBusCommunicationTest, RemoteMethodCallSucceeds) {
    auto defaultTestProxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);
    ASSERT_TRUE((bool)defaultTestProxy);

    auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
    bool success = stubFactory_->registerService(stub, serviceAddress_);
    ASSERT_TRUE(success);

    uint32_t v1 = 5;
    std::string v2 = "Hai :)";
    CommonAPI::CallStatus stat;
    defaultTestProxy->testVoidPredefinedTypeMethod(v1, v2, stat);

    ASSERT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
}


TEST_F(DBusCommunicationTest, RemoteMethodCallWithNonstandardAddressSucceeds) {
    auto defaultTestProxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(nonstandardAddress_);
    ASSERT_TRUE((bool)defaultTestProxy);

    auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
    bool success = stubFactory_->registerService(stub, nonstandardAddress_);
    ASSERT_TRUE(success);

    uint32_t v1 = 5;
    std::string v2 = "Hai :)";
    CommonAPI::CallStatus stat;
    defaultTestProxy->testVoidPredefinedTypeMethod(v1, v2, stat);

    ASSERT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
