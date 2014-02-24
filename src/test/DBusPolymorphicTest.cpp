/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <gtest/gtest.h>
#include <commonapi/tests/DerivedTypeCollection.h>
#include <commonapi/tests/TestInterfaceDBusProxy.h>
#include <commonapi/tests/TestInterfaceDBusStubAdapter.h>
#include <commonapi/tests/TestInterfaceStubDefault.h>

#ifndef COMMONAPI_INTERNAL_COMPILATION
#define COMMONAPI_INTERNAL_COMPILATION
#endif
#include <CommonAPI/DBus/DBusRuntime.h>

#ifndef COMMONAPI_INTERNAL_COMPILATION
#define COMMONAPI_INTERNAL_COMPILATION
#endif

static const std::string commonApiAddress =
                "local:CommonAPI.DBus.tests.DBusProxyTestInterface:CommonAPI.DBus.tests.DBusProxyTestService";
static const std::string interfaceName = "CommonAPI.DBus.tests.DBusProxyTestInterface";
static const std::string busName = "CommonAPI.DBus.tests.DBusProxyTestService";
static const std::string objectPath = "/CommonAPI/DBus/tests/DBusProxyTestService";

class PolymorphicTestStub: public commonapi::tests::TestInterfaceStubDefault {
public:
    void TestArrayOfPolymorphicStructMethod(
                    const std::shared_ptr<CommonAPI::ClientId> clientId,
                    std::vector<std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestPolymorphicStruct>> inArray) {
        numberOfContainedElements_ = inArray.size();

        if (numberOfContainedElements_ > 0) {
            std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestExtendedPolymorphicStruct> extended =
                            std::dynamic_pointer_cast<
                                            commonapi::tests::DerivedTypeCollection::TestExtendedPolymorphicStruct>(
                                            inArray[0]);
            firstElementIsExtended_ = (extended != NULL);
        }

        if (numberOfContainedElements_ > 1) {
            std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestExtendedPolymorphicStruct> extended =
                            std::dynamic_pointer_cast<
                                            commonapi::tests::DerivedTypeCollection::TestExtendedPolymorphicStruct>(
                                            inArray[1]);
            secondElementIsExtended_ = (extended != NULL);
        }
    }

    void TestMapOfPolymorphicStructMethod(
                    const std::shared_ptr<CommonAPI::ClientId> clientId,
                    std::unordered_map<uint8_t, std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestPolymorphicStruct>> inMap) {
        numberOfContainedElements_ = inMap.size();

        auto iteratorFirst = inMap.find(5);
        auto iteratorSecond = inMap.find(10);

        if (iteratorFirst != inMap.end()) {
            std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestExtendedPolymorphicStruct> extended =
                            std::dynamic_pointer_cast<
                                            commonapi::tests::DerivedTypeCollection::TestExtendedPolymorphicStruct>(
                                            iteratorFirst->second);
            firstElementIsExtended_ = (extended != NULL);
        }

        if (iteratorSecond != inMap.end()) {
            std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestExtendedPolymorphicStruct> extended =
                            std::dynamic_pointer_cast<
                                            commonapi::tests::DerivedTypeCollection::TestExtendedPolymorphicStruct>(
                                            iteratorSecond->second);
            secondElementIsExtended_ = (extended != NULL);
        }
    }

    void TestStructWithPolymorphicMemberMethod(const std::shared_ptr<CommonAPI::ClientId> clientId,
                                               commonapi::tests::DerivedTypeCollection::StructWithPolymorphicMember inStruct) {
        if (inStruct.polymorphicMember != NULL) {
            numberOfContainedElements_ = 1;

            std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestExtendedPolymorphicStruct> extended =
                            std::dynamic_pointer_cast<
                                            commonapi::tests::DerivedTypeCollection::TestExtendedPolymorphicStruct>(
                                            inStruct.polymorphicMember);
            firstElementIsExtended_ = (extended != NULL);
        }
    }

    int numberOfContainedElements_;
    bool firstElementIsExtended_;
    bool secondElementIsExtended_;
};

class PolymorphicTest: public ::testing::Test {
protected:
    void SetUp() {
        auto runtime = std::dynamic_pointer_cast<CommonAPI::DBus::DBusRuntime>(CommonAPI::Runtime::load());

        serviceFactory = std::dynamic_pointer_cast<CommonAPI::DBus::DBusFactory>(runtime->createFactory());

        proxyDBusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(proxyDBusConnection_->connect());

        proxy_ = std::make_shared<commonapi::tests::TestInterfaceDBusProxy>(
                        serviceFactory,
                        commonApiAddress,
                        interfaceName,
                        busName,
                        objectPath,
                        proxyDBusConnection_);
        proxy_->init();

        registerTestStub();

        for (unsigned int i = 0; !proxy_->isAvailable() && i < 100; ++i) {
            usleep(10000);
        }
        ASSERT_TRUE(proxy_->isAvailable());

        baseInstance1_ = std::make_shared<commonapi::tests::DerivedTypeCollection::TestPolymorphicStruct>();
        baseInstance1_->testString = "abc";

        extendedInstance1_ = std::make_shared<commonapi::tests::DerivedTypeCollection::TestExtendedPolymorphicStruct>();
        extendedInstance1_->additionalValue = 7;
    }

    std::shared_ptr<CommonAPI::DBus::DBusFactory> serviceFactory;

    virtual void TearDown() {
        deregisterTestStub();
        usleep(30000);
    }

    void registerTestStub() {
        stubDBusConnection_ = CommonAPI::DBus::DBusConnection::getSessionBus();
        ASSERT_TRUE(stubDBusConnection_->connect());

        testStub = std::make_shared<PolymorphicTestStub>();
        stubAdapter_ = std::make_shared<commonapi::tests::TestInterfaceDBusStubAdapter>(
                        serviceFactory,
                        commonApiAddress,
                        interfaceName,
                        busName,
                        objectPath,
                        stubDBusConnection_,
                        testStub);
        stubAdapter_->init(stubAdapter_);

        const bool isStubAdapterRegistered = CommonAPI::DBus::DBusServicePublisher::getInstance()->registerService(
                        stubAdapter_);
        ASSERT_TRUE(isStubAdapterRegistered);
    }

    void deregisterTestStub() {
        const bool isStubAdapterUnregistered = CommonAPI::DBus::DBusServicePublisher::getInstance()->unregisterService(
                        stubAdapter_->getAddress());
        ASSERT_TRUE(isStubAdapterUnregistered);
        stubAdapter_.reset();

        if (stubDBusConnection_->isConnected()) {
            stubDBusConnection_->disconnect();
        }
        stubDBusConnection_.reset();
    }

    std::shared_ptr<CommonAPI::DBus::DBusConnection> proxyDBusConnection_;
    std::shared_ptr<commonapi::tests::TestInterfaceDBusProxy> proxy_;

    std::shared_ptr<CommonAPI::DBus::DBusConnection> stubDBusConnection_;
    std::shared_ptr<commonapi::tests::TestInterfaceDBusStubAdapter> stubAdapter_;

    std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestPolymorphicStruct> baseInstance1_;
    std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestExtendedPolymorphicStruct> extendedInstance1_;

    std::shared_ptr<PolymorphicTestStub> testStub;
};

TEST_F(PolymorphicTest, SendVectorOfBaseType) {
    CommonAPI::CallStatus stat;
    std::vector<std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestPolymorphicStruct> > inputArray;
    inputArray.push_back(baseInstance1_);

    proxy_->TestArrayOfPolymorphicStructMethod(inputArray, stat);

    ASSERT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(testStub->numberOfContainedElements_, 1);
    ASSERT_FALSE(testStub->firstElementIsExtended_);
}

TEST_F(PolymorphicTest, SendVectorOfExtendedType) {
    CommonAPI::CallStatus stat;
    std::vector<std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestPolymorphicStruct> > inputArray;
    inputArray.push_back(extendedInstance1_);

    proxy_->TestArrayOfPolymorphicStructMethod(inputArray, stat);

    ASSERT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(testStub->numberOfContainedElements_, 1);
    ASSERT_TRUE(testStub->firstElementIsExtended_);
}

TEST_F(PolymorphicTest, SendVectorOfBaseAndExtendedType) {
    CommonAPI::CallStatus stat;
    std::vector<std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestPolymorphicStruct> > inputArray;
    inputArray.push_back(baseInstance1_);
    inputArray.push_back(extendedInstance1_);

    proxy_->TestArrayOfPolymorphicStructMethod(inputArray, stat);

    ASSERT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(testStub->numberOfContainedElements_, 2);
    ASSERT_FALSE(testStub->firstElementIsExtended_);
    ASSERT_TRUE(testStub->secondElementIsExtended_);
}

TEST_F(PolymorphicTest, SendMapOfBaseAndExtendedType) {
    CommonAPI::CallStatus stat;
    std::unordered_map<uint8_t, std::shared_ptr<commonapi::tests::DerivedTypeCollection::TestPolymorphicStruct> > inputMap;
    inputMap.insert( {5, baseInstance1_});
    inputMap.insert( {10, extendedInstance1_});

    proxy_->TestMapOfPolymorphicStructMethod(inputMap, stat);

    ASSERT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(testStub->numberOfContainedElements_, 2);
    ASSERT_FALSE(testStub->firstElementIsExtended_);
    ASSERT_TRUE(testStub->secondElementIsExtended_);
}

TEST_F(PolymorphicTest, SendStructWithPolymorphicMember) {
    CommonAPI::CallStatus stat;
    commonapi::tests::DerivedTypeCollection::StructWithPolymorphicMember inputStruct;
    inputStruct.polymorphicMember = extendedInstance1_;

    proxy_->TestStructWithPolymorphicMemberMethod(inputStruct, stat);

    ASSERT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(testStub->numberOfContainedElements_, 1);
    ASSERT_TRUE(testStub->firstElementIsExtended_);
}

TEST_F(PolymorphicTest, SendStructWithMapWithEnumKeyMember) {
    CommonAPI::CallStatus stat;
    commonapi::tests::DerivedTypeCollection::StructWithEnumKeyMap inputStruct;
    inputStruct.testMap.insert( { commonapi::tests::DerivedTypeCollection::TestEnum::E_OK, "test" } );

    proxy_->TestStructWithEnumKeyMapMember(inputStruct, stat);

    ASSERT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
}

#ifndef WIN32
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif