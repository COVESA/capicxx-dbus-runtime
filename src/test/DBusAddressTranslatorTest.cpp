/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include <gtest/gtest.h>

#include <fstream>

#include <CommonAPI/DBus/DBusAddressTranslator.h>
#include <CommonAPI/DBus/DBusUtils.h>

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


static const std::string fileString =
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
"dbus_object=/only/object/path";

static const std::vector<std::string> commonApiAddresses = {
    "local:no.nothing.service:no.nothing.instance",
    "local:service:instance",
    "local:no.interface.service:no.interface.instance",
    "local:no.connection.service:no.connection.instance",
    "local:no.object.service:no.object.instance",
    "local:only.interface.service:only.interface.instance",
    "local:only.connection.service:only.connection.instance",
    "local:only.object.service:only.object.instance"
};

typedef std::vector<CommonAPI::DBus::DBusServiceAddress>::value_type vt;
static const std::vector<CommonAPI::DBus::DBusServiceAddress> dbusAddresses = {
                vt("no.nothing.instance", "/no/nothing/instance", "no.nothing.service"),
                vt("connection.name", "/path/to/object", "service.name"),
                vt("no.interface.connection", "/no/interface/path", "no.interface.service"),
                vt("no.connection.instance", "/no/connection/path", "no.connection.interface"),
                vt("no.object.connection", "/no/object/instance", "no.object.interface"),
                vt("only.interface.instance", "/only/interface/instance", "only.interface.interface"),
                vt("only.connection.connection", "/only/connection/instance", "only.connection.service"),
                vt("only.object.instance", "/only/object/path", "only.object.service")
};

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
        configFileName_ = CommonAPI::DBus::getCurrentBinaryFileName();
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
    std::shared_ptr<CommonAPI::Runtime> runtime;
    std::shared_ptr<CommonAPI::Factory> proxyFactory;
    std::shared_ptr<CommonAPI::Factory> stubFactory;

    runtime = CommonAPI::Runtime::load();
    ASSERT_TRUE((bool)runtime);
    CommonAPI::DBus::DBusRuntime* dbusRuntime = dynamic_cast<CommonAPI::DBus::DBusRuntime*>(&(*runtime));
    ASSERT_TRUE(dbusRuntime != NULL);

    proxyFactory = runtime->createFactory();
    ASSERT_TRUE((bool)proxyFactory);
    stubFactory = runtime->createFactory();
    ASSERT_TRUE((bool)stubFactory);

    auto defaultTestProxy = proxyFactory->buildProxy<commonapi::tests::TestInterfaceProxy>(commonApiAddresses[0]);
    ASSERT_TRUE((bool)defaultTestProxy);

    auto stub = std::make_shared<commonapi::tests::TestInterfaceStubDefault>();
    bool success = stubFactory->registerService(stub, commonApiAddresses[0]);
    ASSERT_TRUE(success);

    uint32_t v1 = 5;
    std::string v2 = "Hai :)";
    CommonAPI::CallStatus stat;
    defaultTestProxy->testVoidPredefinedTypeMethod(v1, v2, stat);

    ASSERT_EQ(stat, CommonAPI::CallStatus::SUCCESS);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
