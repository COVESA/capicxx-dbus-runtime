/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include <gtest/gtest.h>

#include <CommonAPI/DBus/DBusAddressTranslator.h>


static const std::string fileString = "\n"
"\n"
"\n"
"[domain:service:instance]\n"
"dbus_connection=connection.name\n"
"dbus_object=/path/to/object\n"
"dbus_interface=service.name\n"
"\n"
"[noNothingDomain:noNothingService:noNothingInterface]\n"
"\n"
"[noInterfaceDomain:noInterfaceService:noInterfaceInstance]\n"
"dbus_connection=no.interface.connection\n"
"dbus_object=/no/interface/path\n"
"\n"
"[noConnectionDomain:noConnectionService:noConnectionInstance]\n"
"dbus_object=/no/connection/path\n"
"dbus_interface=no.connection.interface\n"
"\n"
"[noObjectDomain:noObjectService:noObjectInstance]\n"
"dbus_connection=no.object.connection\n"
"dbus_interface=no.object.interface\n"
"\n"
"[onlyInterfaceDomain:onlyInterfaceService:onlyInterfaceInstance]\n"
"dbus_interface=only.interface.interface\n"
"\n"
"[onlyConnectionDomain:onlyConnectionService:onlyConnectionInstance]\n"
"dbus_connection=only.connection.connection\n"
"\n"
"[onlyObjectDomain:onlyObjectService:onlyObjectInstance]\n"
"dbus_object=/only/object/path\n";


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


TEST_F(AddressTranslatorTest, ParsesContainedDBusAddresses) {
    CommonAPI::DBus::DBusAddressTranslator& translator = CommonAPI::DBus::DBusAddressTranslator::getInstance();
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
