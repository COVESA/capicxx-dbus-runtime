/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_FREEDESKTOP_PROPERTIES_STUB_H_
#define COMMONAPI_DBUS_DBUS_FREEDESKTOP_PROPERTIES_STUB_H_

#include "DBusInterfaceHandler.h"

#include <memory>
#include <mutex>
#include <string>

namespace CommonAPI {
namespace DBus {

class DBusStubAdapter;

/**
 * Stub for standard <a href="http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties">org.freedesktop.dbus.Properties</a> interface.
 *
 * DBusFreedesktopPropertiesStub gets the DBusStubAdapter for handling the actual properties with instantiation.
 */
class DBusFreedesktopPropertiesStub: public DBusInterfaceHandler {
public:
    DBusFreedesktopPropertiesStub(const std::string& dbusObjectPath,
                                  const std::string& dbusInterfaceName,
                                  const std::shared_ptr<DBusProxyConnection>&,
                                  const std::shared_ptr<DBusStubAdapter>& dbusStubAdapter);

    virtual ~DBusFreedesktopPropertiesStub();

    const std::string& getDBusObjectPath() const;
    static const char* getInterfaceName();

    virtual const char* getMethodsDBusIntrospectionXmlData() const;
    virtual bool onInterfaceDBusMessage(const DBusMessage& dbusMessage);
    virtual const bool hasFreedesktopProperties();
private:
    std::string dbusObjectPath_;
    std::weak_ptr<DBusProxyConnection> dbusConnection_;
    std::shared_ptr<DBusStubAdapter> dbusStubAdapter_;

    typedef std::unordered_map<std::string, std::shared_ptr<DBusStubAdapter>> DBusInterfacesMap;
    DBusInterfacesMap managedInterfaces_;

    std::mutex dbusInterfacesLock_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_FREEDESKTOP_PROPERTIES_STUB_H_
