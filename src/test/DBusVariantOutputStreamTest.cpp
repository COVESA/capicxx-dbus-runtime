/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gtest/gtest.h>
#include <CommonAPI/DBus/DBusOutputStream.h>
#include <CommonAPI/SerializableVariant.h>

using namespace CommonAPI;

class VariantOutputStreamTest: public ::testing::Test {
  protected:
    size_t numOfElements;
    CommonAPI::DBus::DBusMessage message;
    const char* busName;
    const char* objectPath;
    const char* interfaceName;
    const char* methodName;

    void SetUp() {
      numOfElements = 10;
      busName = "no.bus.here";
      objectPath = "/no/object/here";
      interfaceName = "no.interface.here";
      methodName = "noMethodHere";
    }

    void TearDown() {
    }

};

TEST_F(VariantOutputStreamTest, CanBeCalled) {
    const char* signature = "yyyyyyyyyy";
    message = CommonAPI::DBus::DBusMessage::createMethodCall(busName, objectPath, interfaceName, methodName, signature);
    DBus::DBusOutputStream outputStream(message);


    int baseInt = 1;
    std::string baseString = "abc";
    Variant<int, bool> primitiveVariant(baseInt);
    Variant<int, std::string> stringVariant(baseString);
    Variant<int, Variant<int, std::string> > layeredVariant(stringVariant);

    outputStream << primitiveVariant;
    outputStream << stringVariant;
    outputStream << layeredVariant;

}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
