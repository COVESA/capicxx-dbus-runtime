/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_FREEDESKTOP_OBJECT_MANAGER_STUB_H_
#define COMMONAPI_DBUS_DBUS_FREEDESKTOP_OBJECT_MANAGER_STUB_H_


#include "DBusInterfaceHandler.h"

#include <memory>
#include <mutex>
#include <string>

namespace CommonAPI {
namespace DBus {

class DBusStubAdapter;

/**
 * Stub for standard <a href="http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager">org.freedesktop.dbus.ObjectManager</a> interface.
 *
 * Instantiated within a manager stub and it must hold reference to all registered objects.
 * Whenever the manager gets destroyed all references to registered objects are lost too.
 * This duplicates the semantic of the CommonAPI::ServicePublisher class.
 *
 * Only one DBusStubAdapter instance could be registered per DBusObjectManagerStub instance.
 *
 * The owner of the DBusObjectManagerStub instance must take care of registering and unregistering it.
 *
 * Example stub life cycle:
 *  - create CommonAPI::ServicePublisher
 *  - create stub A
 *  - register stub A to CommonAPI::ServicePublisher
 *  - create stub B
 *  - register stub B with stub A as object manager
 *  - drop all references to stub B, stub A keeps a reference to stub B
 *  - drop all references to stub A, CommonAPI::ServicePublisher keeps a reference to stub A
 *  - reference overview: Application > CommonAPI::ServicePublisher > Stub A > Stub B
 *  - drop all references to CommonAPI::ServicePublisher causes all object references to be dropped
 */
class DBusObjectManagerStub: public DBusInterfaceHandler {
 public:
    // serialization trick: use bool instead of variant since we never serialize it
    typedef std::unordered_map<std::string, bool> DBusPropertiesChangedDict;
    typedef std::unordered_map<std::string, DBusPropertiesChangedDict> DBusInterfacesAndPropertiesDict;
    typedef std::unordered_map<std::string, DBusInterfacesAndPropertiesDict> DBusObjectPathAndInterfacesDict;

 public:
	DBusObjectManagerStub(const std::string& dbusObjectPath, const std::shared_ptr<DBusProxyConnection>&);

	/**
	 * Unregisters all currently registered DBusStubAdapter instances from the DBusServicePublisher
	 */
	virtual ~DBusObjectManagerStub();

    /**
     * Export DBusStubAdapter instance with the current DBusObjectManagerStub instance.
     *
     * The DBusStubAdapter must be registered with the DBusServicePublisher!
     *
     * On registering a
     * <a href="http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager">InsterfaceAdded</a>
     * signal will be emitted with the DBusObjectManagerStub instance's current D-Bus object path.
     *
     * @param dbusStubAdapter a refernce to DBusStubAdapter instance
     *
     * @return false if the @a dbusStubAdapter instance was already registered
     * @return false if sending the InterfaceAdded signal fails
     *
     * @see ~DBusObjectManagerStub()
     * @see CommonAPI::ServicePublisher
     * @see DBusObjectManager
     */
    bool exportManagedDBusStubAdapter(std::shared_ptr<DBusStubAdapter> dbusStubAdapter);

    /**
     * Unexport DBusStubAdapter instance from this DBusObjectManagerStub instance.
     *
     * On unregistering a
     * <a href="http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager">InsterfaceRemoved</a>
     * signal will be emitted with the DBusObjectManagerStub instance's current D-Bus object path.
     *
     * @param dbusStubAdapter
     *
     * @return false if @a dbusStubAdapter wasn't registered
     * @return true even if sending the InterfaceRemoved signal fails
     *
     * @see exportDBusStubAdapter()
     */
    bool unexportManagedDBusStubAdapter(std::shared_ptr<DBusStubAdapter> dbusStubAdapter);

    bool isDBusStubAdapterExported(std::shared_ptr<DBusStubAdapter> dbusStubAdapter);

    inline const std::string& getDBusObjectPath() const;
    inline static const char* getInterfaceName();

    virtual const char* getMethodsDBusIntrospectionXmlData() const;
    virtual bool onInterfaceDBusMessage(const DBusMessage& dbusMessage);

 private:
    bool registerDBusStubAdapter(std::shared_ptr<DBusStubAdapter> dbusStubAdapter);
    bool unregisterDBusStubAdapter(std::shared_ptr<DBusStubAdapter> dbusStubAdapter);

    bool emitInterfacesAddedSignal(std::shared_ptr<DBusStubAdapter> dbusStubAdapter,
                                   const std::shared_ptr<DBusProxyConnection>& dbusConnection) const;

    bool emitInterfacesRemovedSignal(std::shared_ptr<DBusStubAdapter> dbusStubAdapter,
                                     const std::shared_ptr<DBusProxyConnection>& dbusConnection) const;

    std::string dbusObjectPath_;
    std::weak_ptr<DBusProxyConnection> dbusConnection_;

    typedef std::unordered_map<std::string, std::shared_ptr<DBusStubAdapter>> DBusInterfacesMap;
    typedef std::unordered_map<std::string, DBusInterfacesMap> DBusObjectPathsMap;
    DBusObjectPathsMap registeredDBusObjectPathsMap_;

    std::mutex dbusObjectManagerStubLock_;
};


inline const std::string& DBusObjectManagerStub::getDBusObjectPath() const {
    return dbusObjectPath_;
}

const char* DBusObjectManagerStub::getInterfaceName() {
    return "org.freedesktop.DBus.ObjectManager";
}

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_FREEDESKTOP_OBJECT_MANAGER_STUB_H_
