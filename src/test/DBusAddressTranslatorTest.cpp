/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include <gtest/gtest.h>

#include <CommonAPI/DBus/DBusAddressTranslator.h>


class AddressTranslatorTest: public ::testing::Test {
protected:

    virtual void TearDown() {
    }

    void SetUp() {
    }
};


TEST_F(AddressTranslatorTest, InstanceCanBeRetrieved) {
    CommonAPI::DBus::DBusAddressTranslator& translator = CommonAPI::DBus::DBusAddressTranslator::getInstance();
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
