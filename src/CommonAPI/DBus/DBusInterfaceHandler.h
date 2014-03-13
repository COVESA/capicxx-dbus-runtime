/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_INTERFACE_HANDLER_H_
#define COMMONAPI_DBUS_INTERFACE_HANDLER_H_

#include "DBusProxyConnection.h"
#include "DBusMessage.h"

#include <memory>

namespace CommonAPI {
namespace DBus {

class DBusInterfaceHandler {
 public:
    virtual ~DBusInterfaceHandler() { }

    virtual const char* getMethodsDBusIntrospectionXmlData() const = 0;

    virtual bool onInterfaceDBusMessage(const DBusMessage& dbusMessage) = 0;

    virtual const bool hasFreedesktopProperties() = 0;
};

} // namespace dbus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_INTERFACE_HANDLER_H_
