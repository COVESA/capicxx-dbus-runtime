/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_OBJECT_MANAGER_H_
#define COMMONAPI_DBUS_DBUS_OBJECT_MANAGER_H_

#include "DBusProxyConnection.h"
#include "DBusMessage.h"
#include "DBusObjectManagerStub.h"


namespace CommonAPI {
namespace DBus {

class DBusStubAdapter;
class DBusInterfaceHandler;

class DBusObjectManager {
 public:
    DBusObjectManager(const std::shared_ptr<DBusProxyConnection>&);
    ~DBusObjectManager();

    bool registerDBusStubAdapter(DBusStubAdapter* dbusStubAdapter);
    bool unregisterDBusStubAdapter(DBusStubAdapter* dbusStubAdapter);

    bool handleMessage(const DBusMessage&);

    inline DBusObjectManagerStub& getRootDBusObjectManagerStub();

 private:
    // objectPath, interfaceName
    typedef std::pair<std::string, std::string> DBusInterfaceHandlerPath;

    bool addDBusInterfaceHandler(const DBusInterfaceHandlerPath& dbusInterfaceHandlerPath,
                                 DBusInterfaceHandler* dbusInterfaceHandler);

    bool removeDBusInterfaceHandler(const DBusInterfaceHandlerPath& dbusInterfaceHandlerPath,
                                    DBusInterfaceHandler* dbusInterfaceHandler);

    bool onIntrospectableInterfaceDBusMessage(const DBusMessage& callMessage);

    typedef std::unordered_map<DBusInterfaceHandlerPath, DBusInterfaceHandler*> DBusRegisteredObjectsTable;
    DBusRegisteredObjectsTable dbusRegisteredObjectsTable_;

    DBusObjectManagerStub rootDBusObjectManagerStub_;

    std::weak_ptr<DBusProxyConnection> dbusConnection_;
    std::recursive_mutex objectPathLock_;
};


DBusObjectManagerStub& DBusObjectManager::getRootDBusObjectManagerStub() {
    return rootDBusObjectManagerStub_;
}

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_OBJECT_MANAGER_H_
