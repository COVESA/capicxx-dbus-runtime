/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef COMMONAPI_DBUS_DBUS_SERVICE_PUBLISHER_H_
#define COMMONAPI_DBUS_DBUS_SERVICE_PUBLISHER_H_


#include <CommonAPI/ServicePublisher.h>
#include "DBusStubAdapter.h"


namespace CommonAPI {
namespace DBus {


class DBusServicePublisher: public ServicePublisher {
 public:
    DBusServicePublisher() {}

    static std::shared_ptr<DBusServicePublisher> getInstance();

    bool registerService(const std::string& serviceAddress, std::shared_ptr<DBusStubAdapter> adapter);

    bool unregisterService(const std::string& serviceAddress);

 private:
    std::unordered_map<std::string, std::shared_ptr<DBusStubAdapter>> registeredServices_;
};

} // namespace DBus
} // namespace CommonAPI

#endif /* COMMONAPI_DBUS_DBUS_SERVICE_PUBLISHER_H_ */
