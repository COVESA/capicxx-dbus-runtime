/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef FAKE_LEGACY_SERVICE_LEGACY_INTERFACE_H_
#define FAKE_LEGACY_SERVICE_LEGACY_INTERFACE_H_

#include <CommonAPI/types.h>

namespace fake {
namespace legacy {
namespace service {

class LegacyInterface {
 public:
    virtual ~LegacyInterface() { }

    static inline const char* getInterfaceId();
    static inline CommonAPI::Version getInterfaceVersion();
};

const char* LegacyInterface::getInterfaceId() {
    return "fake.legacy.service.LegacyInterface";
}

CommonAPI::Version LegacyInterface::getInterfaceVersion() {
    return CommonAPI::Version(1, 0);
}


} // namespace service
} // namespace legacy
} // namespace fake

namespace CommonAPI {

}

#endif // FAKE_LEGACY_SERVICE_LEGACY_INTERFACE_H_
