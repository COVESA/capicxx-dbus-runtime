/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <unistd.h>

#include <CommonAPI/CommonAPI.h>

#define COMMONAPI_INTERNAL_COMPILATION

#include <CommonAPI/DBus/DBusAddressTranslator.h>
#include <CommonAPI/DBus/DBusUtils.h>
#include <CommonAPI/DBus/DBusConnection.h>
#include <CommonAPI/DBus/DBusProxy.h>
#include <CommonAPI/DBus/DBusRuntime.h>

#include "commonapi/tests/PredefinedTypeCollection.h"
#include "commonapi/tests/DerivedTypeCollection.h"
#include "commonapi/tests/TestInterfaceProxy.h"
#include "commonapi/tests/TestInterfaceStubDefault.h"
#include "commonapi/tests/TestInterfaceDBusStubAdapter.h"

#include <fake/legacy/service/LegacyInterfaceProxy.h>


static const std::vector<std::string> commonApiAddresses = {
    "local:no.nothing.service:no.nothing.instance",
    "local:service:instance",
    "local:no.interface.service:no.interface.instance",
    "local:no.connection.service:no.connection.instance",
    "local:no.object.service:no.object.instance",
    "local:only.interface.service:only.interface.instance",
    "local:only.connection.service:only.connection.instance",
    "local:only.object.service:only.object.instance",
    "local:fake.legacy.service.LegacyInterface:fake.legacy.service"
};

static const std::string fileString =
""
"[not#a$valid/address]\n"
"[]\n"
"   98t3hpgjvqpvnü0 t4b+qßk4 kv+üg4krgv+ß4krgv+ßkr\n"
"[too.short:address]\n"
"[incomplete:address:]\n"
"[:address:incomplete]\n"
"[]đwqervqerverver\n"
"[too:long:address:here]\n"
"jfgv2nqp3 riqpnvi39r[]"
"\n"
"[local:no.nothing.service:no.nothing.instance]\n"
"\n"
"[local:service:instance]\n"
"dbus_connection=connection.name\n"
"dbus_object=/path/to/object\n"
"dbus_interface=service.name\n"
"\n"
"[local:no.interface.service:no.interface.instance]\n"
"dbus_connection=no.interface.connection\n"
"dbus_object=/no/interface/path\n"
"\n"
"[local:no.connection.service:no.connection.instance]\n"
"dbus_object=/no/connection/path\n"
"dbus_interface=no.connection.interface\n"
"\n"
"[local:no.object.service:no.object.instance]\n"
"dbus_connection=no.object.connection\n"
"dbus_interface=no.object.interface\n"
"\n"
"[local:only.interface.service:only.interface.instance]\n"
"dbus_interface=only.interface.interface\n"
"\n"
"[local:only.connection.service:only.connection.instance]\n"
"dbus_connection=only.connection.connection\n"
"\n"
"[local:only.object.service:only.object.instance]\n"
"dbus_object=/only/object/path\n"
"\n"
"[local:fake.legacy.service.LegacyInterface:fake.legacy.service]\n"
"dbus_connection=fake.legacy.service.connection\n"
"dbus_object=/some/legacy/path/6259504\n"
"dbus_interface=fake.legacy.service.LegacyInterface\n"
"dbus_predefined=true\n";

typedef std::vector<CommonAPI::DBus::DBusServiceAddress>::value_type vt;
static const std::vector<CommonAPI::DBus::DBusServiceAddress> dbusAddresses = {
                vt("no.nothing.instance", "/no/nothing/instance", "no.nothing.service"),
                vt("connection.name", "/path/to/object", "service.name"),
                vt("no.interface.connection", "/no/interface/path", "no.interface.service"),
                vt("no.connection.instance", "/no/connection/path", "no.connection.interface"),
                vt("no.object.connection", "/no/object/instance", "no.object.interface"),
                vt("only.interface.instance", "/only/interface/instance", "only.interface.interface"),
                vt("only.connection.connection", "/only/connection/instance", "only.connection.service"),
                vt("only.object.instance", "/only/object/path", "only.object.service"),
                vt("fake.legacy.service.connection", "/some/legacy/path/6259504", "fake.legacy.service.LegacyInterface")
};


class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
        configFileName_ = CommonAPI::getCurrentBinaryFileFQN();
        configFileName_ += CommonAPI::DBus::DBUS_CONFIG_SUFFIX;
        std::ofstream configFile(configFileName_);
        ASSERT_TRUE(configFile.is_open());
        configFile << fileString;
        configFile.close();
    }

    virtual void TearDown() {
        std::remove(configFileName_.c_str());
    }

    std::string configFileName_;
};


class AddressTranslatorTest: public ::testing::Test {
protected:
    void SetUp() {
    }

    virtual void TearDown() {
        usleep(30000);
    }
};


TEST_F(AddressTranslatorTest, InstanceCanBeRetrieved) {
    CommonAPI::DBus::DBusAddressTranslator& translator = CommonAPI::DBus::DBusAddressTranslator::getInstance();
}


TEST_F(AddressTranslatorTest, ParsesDBusAddresses) {
    CommonAPI::DBus::DBusAddressTranslator& translator = CommonAPI::DBus::DBusAddressTranslator::getInstance();

    for(unsigned int i = 0; i < commonApiAddresses.size(); i++) {
        std::string interfaceName, connectionName, objectPath;
        translator.searchForDBusAddress(commonApiAddresses[i], interfaceName, connectionName, objectPath);
        ASSERT_EQ(std::get<0>(dbusAddresses[i]), connectionName);
        ASSERT_EQ(std::get<1>(dbusAddresses[i]), objectPath);
        ASSERT_EQ(std::get<2>(dbusAddresses[i]), interfaceName);
    }
}


TEST_F(AddressTranslatorTest, ParsesCommonAPIAddresses) {
    CommonAPI::DBus::DBusAddressTranslator& translator = CommonAPI::DBus::DBusAddressTranslator::getInstance();

    for(unsigned int i = 0; i < commonApiAddresses.size(); i++) {
        std::string commonApiAddress;
        translator.searchForCommonAddress(
                        std::get<2>(dbusAddresses[i]),
                        std::get<0>(dbusAddresses[i]),
                        std::get<1>(dbusAddresses[i]),
                        commonApiAddress);
        ASSERT_EQ(commonApiAddresses[i], commonApiAddress);
    }
}


TEST_F(AddressTranslatorTest, ServicesUsingPredefinedAddressesCanCommunicate) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
    ASSERT_TRUE((bool)runtime);
    CommonAPI::DBus::DBusRuntime* dbusRuntime = dynamic_cast<CommonAPI::DBus::DBusRuntime*>(&(*runtime));
    ASSERT_TRUE(dbusRuntime != NULL);

    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime->createFactory();
    ASSERT_TRUE((bool)proxyFactory);
    std::shared_ptr<CommonAPI::Factory> stubFactory = runtime->createFactory();
    ASSERT_TRUE((bool)stubFactory);

    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher = runtime->getServicePublisher();

    auto defaultTestProxy = proxyFactory->buildProxy<commonapi::tests::TestInterfaceProxy>(commonApiAddresses[0]);
    ASSERT_TRUE((bool)defaultTestProxy);

    auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();

    bool serviceNameAcquired = servicePublisher->registerService(stub, commonApiAddresses[0], stubFactory);
    for(unsigned int i = 0; !serviceNameAcquired && i < 100; i++) {
        serviceNameAcquired = servicePublisher->registerService(stub, commonApiAddresses[0], stubFactory);
        usleep(10000);
    }
    ASSERT_TRUE(serviceNameAcquired);

    for(unsigned int i = 0; !defaultTestProxy->isAvailable() && i < 100; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(defaultTestProxy->isAvailable());

    uint32_t v1 = 5;
    std::string v2 = "Hai :)";
    CommonAPI::CallStatus stat;
    defaultTestProxy->testVoidPredefinedTypeMethod(v1, v2, stat);

    ASSERT_EQ(stat, CommonAPI::CallStatus::SUCCESS);

    servicePublisher->unregisterService(commonApiAddresses[0]);
}


const std::string addressOfFakeLegacyService = commonApiAddresses[8];

const std::string domainOfFakeLegacyService = "local";
const std::string serviceIdOfFakeLegacyService = "fake.legacy.service.LegacyInterface";
const std::string participantIdOfFakeLegacyService = "fake.legacy.service";

TEST_F(AddressTranslatorTest, CreatedProxyHasCorrectCommonApiAddress) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
    ASSERT_TRUE((bool)runtime);
    CommonAPI::DBus::DBusRuntime* dbusRuntime = dynamic_cast<CommonAPI::DBus::DBusRuntime*>(&(*runtime));
    ASSERT_TRUE(dbusRuntime != NULL);

    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime->createFactory();
    ASSERT_TRUE((bool)proxyFactory);
    auto proxyForFakeLegacyService = proxyFactory->buildProxy<fake::legacy::service::LegacyInterfaceProxy>(addressOfFakeLegacyService);
    ASSERT_TRUE((bool)proxyForFakeLegacyService);

    ASSERT_EQ(addressOfFakeLegacyService, proxyForFakeLegacyService->getAddress());
    ASSERT_EQ(domainOfFakeLegacyService, proxyForFakeLegacyService->getDomain());
    ASSERT_EQ(serviceIdOfFakeLegacyService, proxyForFakeLegacyService->getServiceId());
    ASSERT_EQ(participantIdOfFakeLegacyService, proxyForFakeLegacyService->getInstanceId());
}


void fakeLegacyServiceThread() {
    int resultCode = system("python ./src/test/fakeLegacyService/fakeLegacyService.py");
    EXPECT_EQ(0, resultCode);
}

TEST_F(AddressTranslatorTest, FakeLegacyServiceCanBeAddressed) {
    std::thread fakeServiceThread = std::thread(fakeLegacyServiceThread);
    usleep(500000);

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
    ASSERT_TRUE((bool)runtime);
    CommonAPI::DBus::DBusRuntime* dbusRuntime = dynamic_cast<CommonAPI::DBus::DBusRuntime*>(&(*runtime));
    ASSERT_TRUE(dbusRuntime != NULL);

    std::shared_ptr<CommonAPI::Factory> proxyFactory = runtime->createFactory();
    ASSERT_TRUE((bool)proxyFactory);
    auto proxyForFakeLegacyService = proxyFactory->buildProxy<fake::legacy::service::LegacyInterfaceProxy>(addressOfFakeLegacyService);
    ASSERT_TRUE((bool)proxyForFakeLegacyService);

    ASSERT_EQ(addressOfFakeLegacyService, proxyForFakeLegacyService->getAddress());

    CommonAPI::CallStatus status;

    const int32_t input = 42;
    int32_t output1, output2;
    proxyForFakeLegacyService->TestMethod(input, status, output1, output2);
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, status);
    if(CommonAPI::CallStatus::SUCCESS == status) {
        EXPECT_EQ(input -5, output1);
        EXPECT_EQ(input +5, output2);
    }

    std::string greeting;
    int32_t identifier;
    proxyForFakeLegacyService->OtherTestMethod(status, greeting, identifier);
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, status);
    if(CommonAPI::CallStatus::SUCCESS == status) {
        EXPECT_EQ(std::string("Hello"), greeting);
        EXPECT_EQ(42, identifier);
    }

    //end the fake legacy service via dbus
    int resultCode = system("python ./src/test/fakeLegacyService/sendToFakeLegacyService.py finish");
    ASSERT_EQ(0, resultCode);

    fakeServiceThread.join();
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
