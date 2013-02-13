/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_OBJECT_MANAGER_H_
#define COMMONAPI_DBUS_DBUS_OBJECT_MANAGER_H_

#include "DBusMessage.h"
#include "DBusConnection.h"

namespace CommonAPI {
namespace DBus {

// objectPath, interfaceName
typedef std::function<bool(const DBusMessage&)> DBusMessageInterfaceHandler;
typedef std::pair<std::string, std::string> DBusInterfaceHandlerPath;
typedef DBusInterfaceHandlerPath DBusInterfaceHandlerToken;

class DBusConnection;

class DBusObjectManager {
 public:
    DBusObjectManager(const std::shared_ptr<DBusConnection>&);

    void init();

    const DBusInterfaceHandlerToken registerInterfaceHandlerForDBusObject(const std::string& objectPath,
                                                                          const std::string& interfaceName,
                                                                          const DBusMessageInterfaceHandler& dbusMessageInterfaceHandler);

    DBusInterfaceHandlerToken registerInterfaceHandler(const std::string& objectPath,
                                                       const std::string& interfaceName,
                                                       const DBusMessageInterfaceHandler& dbusMessageInterfaceHandler);

    void unregisterInterfaceHandler(const DBusInterfaceHandlerToken& dbusInterfaceHandlerToken);

    bool handleMessage(const DBusMessage&) const;


 private:
    void addLibdbusObjectPathHandler(const std::string& objectPath);
    void removeLibdbusObjectPathHandler(const std::string& objectPath);

    bool onGetDBusObjectManagerData(const DBusMessage& callMessage);

    typedef std::unordered_map<DBusInterfaceHandlerPath, DBusMessageInterfaceHandler> DBusRegisteredObjectsTable;
    DBusRegisteredObjectsTable dbusRegisteredObjectsTable_;

    std::shared_ptr<DBusConnection> dbusConnection_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_OBJECT_MANAGER_H_
