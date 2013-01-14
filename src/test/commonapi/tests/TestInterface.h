/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_TESTS_TEST_INTERFACE_H_
#define COMMONAPI_TESTS_TEST_INTERFACE_H_

#include <CommonAPI/types.h>

namespace commonapi {
namespace tests {

class TestInterface {
 public:
    virtual ~TestInterface() { }

    static inline const char* getInterfaceName();
    static inline CommonAPI::Version getInterfaceVersion();
};

const char* TestInterface::getInterfaceName() {
    return "commonapi.tests.TestInterface";
}

CommonAPI::Version TestInterface::getInterfaceVersion() {
    return CommonAPI::Version(1, 0);
}


} // namespace tests
} // namespace commonapi

#endif // COMMONAPI_TESTS_TEST_INTERFACE_H_
