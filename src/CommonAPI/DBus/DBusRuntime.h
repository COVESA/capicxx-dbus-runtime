/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_RUNTIME_H_
#define COMMONAPI_DBUS_DBUS_RUNTIME_H_

#include "CommonAPI/Runtime.h"

#include "DBusFactory.h"
#include "DBusServicePublisher.h"

namespace CommonAPI {
namespace DBus {

class DBusRuntime: public Runtime, public std::enable_shared_from_this<DBusRuntime> {
public:
    static std::shared_ptr<Runtime> getInstance();
    std::shared_ptr<ServicePublisher> getServicePublisher();
    static const MiddlewareInfo middlewareInfo_;
protected:
    std::shared_ptr<Factory> doCreateFactory(std::shared_ptr<MainLoopContext> mainLoopContext,
                                             const std::string& factoryName,
                                             const bool nullOnInvalidName);
private:
    static std::unordered_map<std::string, DBusRuntime> registeredRuntimes;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_RUNTIME_H_
