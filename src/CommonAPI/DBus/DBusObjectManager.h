/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_OBJECT_MANAGER_H_
#define COMMONAPI_DBUS_DBUS_OBJECT_MANAGER_H_

#include "DBusProxyConnection.h"
#include "DBusMessage.h"

namespace CommonAPI {
namespace DBus {

// objectPath, interfaceName
typedef std::pair<std::string, std::string> DBusInterfaceHandlerPath;
typedef DBusInterfaceHandlerPath DBusInterfaceHandlerToken;

class DBusStubAdapter;

class DBusObjectManager {
 public:
    DBusObjectManager(const std::shared_ptr<DBusProxyConnection>&);
    ~DBusObjectManager();

    void init();

    DBusInterfaceHandlerToken registerDBusStubAdapter(const std::string& objectPath,
                                                      const std::string& interfaceName,
                                                      DBusStubAdapter* dbusStubAdapter);

    void unregisterDBusStubAdapter(const DBusInterfaceHandlerToken& dbusInterfaceHandlerToken);

    bool handleMessage(const DBusMessage&);


 private:
    void addLibdbusObjectPathHandler(const std::string& objectPath);
    void removeLibdbusObjectPathHandler(const std::string& objectPath);

    bool onObjectManagerInterfaceDBusMessage(const DBusMessage& callMessage);
    bool onIntrospectableInterfaceDBusMessage(const DBusMessage& callMessage);

    typedef std::unordered_map<DBusInterfaceHandlerPath, DBusStubAdapter*> DBusRegisteredObjectsTable;
    DBusRegisteredObjectsTable dbusRegisteredObjectsTable_;

    std::weak_ptr<DBusProxyConnection> dbusConnection_;
    std::recursive_mutex objectPathLock_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_OBJECT_MANAGER_H_
