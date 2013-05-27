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
        usleep(30000);
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<CommonAPI::Factory> proxyFactory_;
    std::shared_ptr<CommonAPI::Factory> stubFactory_;

    static const std::string serviceAddress_;
    static const std::string nonstandardAddress_;
};

const std::string DBusCommunicationTest::serviceAddress_ = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService";
const std::string DBusCommunicationTest::nonstandardAddress_ = "local:non.standard.ServiceName:non.standard.participand.ID";



TEST_F(DBusCommunicationTest, RemoteMethodCallSucceeds) {
    auto defaultTestProxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(serviceAddress_);
    ASSERT_TRUE((bool)defaultTestProxy);

    auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

    bool serviceRegistered = stubFactory_->registerService(stub, serviceAddress_);
    for(unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = stubFactory_->registerService(stub, serviceAddress_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);

    for(unsigned int i = 0; !defaultTestProxy->isAvailable() && i < 100; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(defaultTestProxy->isAvailable());

    uint32_t v1 = 5;
    std::string v2 = "Ciao ;)";
    CommonAPI::CallStatus stat;
    defaultTestProxy->testVoidPredefinedTypeMethod(v1, v2, stat);

    EXPECT_EQ(stat, CommonAPI::CallStatus::SUCCESS);

    stubFactory_->unregisterService(serviceAddress_);
}


TEST_F(DBusCommunicationTest, RemoteMethodCallWithNonstandardAddressSucceeds) {
    auto defaultTestProxy = proxyFactory_->buildProxy<commonapi::tests::TestInterfaceProxy>(nonstandardAddress_);
    ASSERT_TRUE((bool)defaultTestProxy);

    auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

    bool serviceRegistered = stubFactory_->registerService(stub, nonstandardAddress_);
    for(unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
        serviceRegistered = stubFactory_->registerService(stub, nonstandardAddress_);
        usleep(10000);
    }
    ASSERT_TRUE(serviceRegistered);

    for(unsigned int i = 0; !defaultTestProxy->isAvailable() && i < 100; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(defaultTestProxy->isAvailable());

    uint32_t v1 = 5;
    std::string v2 = "Hai :)";
    CommonAPI::CallStatus stat;
    defaultTestProxy->testVoidPredefinedTypeMethod(v1, v2, stat);

    EXPECT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
    stubFactory_->unregisterService(nonstandardAddress_);
}


//XXX This test case requires CommonAPI::DBus::DBusConnection::suspendDispatching and ...::resumeDispatching to be public!

//static const std::string commonApiAddress = "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService";
//static const std::string interfaceName = "CommonAPI.DBus.tests.DBusProxyTestInterface";
//static const std::string busName = "CommonAPI.DBus.tests.DBusProxyTestService";
//static const std::string objectPath = "/CommonAPI/DBus/tests/DBusProxyTestService";

//TEST_F(DBusCommunicationTest, AsyncCallsAreQueuedCorrectly) {
//    auto proxyDBusConnection = CommonAPI::DBus::DBusConnection::getSessionBus();
//    ASSERT_TRUE(proxyDBusConnection->connect());
//
//    auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
//
//    bool serviceRegistered = stubFactory_->registerService(stub, serviceAddress_);
//    for(unsigned int i = 0; !serviceRegistered && i < 100; ++i) {
//        serviceRegistered = stubFactory_->registerService(stub, serviceAddress_);
//        usleep(10000);
//    }
//    ASSERT_TRUE(serviceRegistered);
//
//    auto defaultTestProxy = std::make_shared<commonapi::tests::TestInterfaceDBusProxy>(
//                            commonApiAddress,
//                            interfaceName,
//                            busName,
//                            objectPath,
//                            proxyDBusConnection);
//
//    for(unsigned int i = 0; !defaultTestProxy->isAvailable() && i < 100; ++i) {
//        usleep(10000);
//    }
//    ASSERT_TRUE(defaultTestProxy->isAvailable());
//
//    auto val1 = commonapi::tests::DerivedTypeCollection::TestEnumExtended2::E_OK;
//    commonapi::tests::DerivedTypeCollection::TestMap val2;
//    CommonAPI::CallStatus status;
//    unsigned int numCalled = 0;
//    const unsigned int maxNumCalled = 1000;
//    for(unsigned int i = 0; i < maxNumCalled/2; ++i) {
//        defaultTestProxy->testVoidDerivedTypeMethodAsync(val1, val2,
//                [&] (CommonAPI::CallStatus stat) {
//                    if(stat == CommonAPI::CallStatus::SUCCESS) {
//                        numCalled++;
//                    }
//                }
//        );
//    }
//
//    proxyDBusConnection->suspendDispatching();
//
//    for(unsigned int i = maxNumCalled/2; i < maxNumCalled; ++i) {
//        defaultTestProxy->testVoidDerivedTypeMethodAsync(val1, val2,
//                [&] (CommonAPI::CallStatus stat) {
//                    if(stat == CommonAPI::CallStatus::SUCCESS) {
//                        numCalled++;
//                    }
//                }
//        );
//    }
//    sleep(2);
//
//    proxyDBusConnection->resumeDispatching();
//
//    sleep(2);
//
//    ASSERT_EQ(maxNumCalled, numCalled);
//
//    numCalled = 0;
//
//    defaultTestProxy->getTestPredefinedTypeBroadcastEvent().subscribe(
//            [&] (uint32_t, std::string) {
//                numCalled++;
//            }
//    );
//
//    proxyDBusConnection->suspendDispatching();
//
//    for(unsigned int i = 0; i < maxNumCalled; ++i) {
//        stub->fireTestPredefinedTypeBroadcastEvent(0, "Nonething");
//    }
//
//    sleep(2);
//    proxyDBusConnection->resumeDispatching();
//    sleep(2);
//
//    ASSERT_EQ(maxNumCalled, numCalled);
//}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
