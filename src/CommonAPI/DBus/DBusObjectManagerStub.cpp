/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusObjectManagerStub.h"
#include "DBusStubAdapter.h"
#include "DBusServicePublisher.h"
#include "DBusOutputStream.h"

#include <cassert>
#include <vector>

namespace CommonAPI {
namespace DBus {

DBusObjectManagerStub::DBusObjectManagerStub(const std::string& dbusObjectPath,
                                             const std::shared_ptr<DBusProxyConnection>& dbusConnection) :
                dbusConnection_(dbusConnection),
                dbusObjectPath_(dbusObjectPath) {
    assert(!dbusObjectPath.empty());
    assert(dbusObjectPath[0] == '/');
    assert(dbusConnection);
}

DBusObjectManagerStub::~DBusObjectManagerStub() {
    for (auto& dbusObjectPathIterator : registeredDBusObjectPathsMap_) {
        auto& registeredDBusInterfacesMap = dbusObjectPathIterator.second;

        for (auto& dbusInterfaceIterator : registeredDBusInterfacesMap) {
            auto managedDBusStubAdapter = dbusInterfaceIterator.second;
            auto managedDBusStubAdapterServiceAddress = managedDBusStubAdapter->getAddress();

            const bool isServiceUnregistered = DBusServicePublisher::getInstance()->unregisterManagedService(
                            managedDBusStubAdapterServiceAddress);
            assert(isServiceUnregistered);
        }
    }
}

bool DBusObjectManagerStub::exportManagedDBusStubAdapter(std::shared_ptr<DBusStubAdapter> managedDBusStubAdapter) {
    assert(managedDBusStubAdapter);

    std::lock_guard<std::mutex> dbusObjectManagerStubLock(dbusObjectManagerStubLock_);

    const bool isRegistrationSuccessful = registerDBusStubAdapter(managedDBusStubAdapter);
    if (!isRegistrationSuccessful) {
        return false;
    }

    auto dbusConnection = dbusConnection_.lock();
    if (dbusConnection && dbusConnection->isConnected()) {
        const bool isDBusSignalEmitted = emitInterfacesAddedSignal(managedDBusStubAdapter, dbusConnection);

        if (!isDBusSignalEmitted) {
            unregisterDBusStubAdapter(managedDBusStubAdapter);
            return false;
        }
    }

    return true;
}

bool DBusObjectManagerStub::unexportManagedDBusStubAdapter(std::shared_ptr<DBusStubAdapter> managedDBusStubAdapter) {
    assert(managedDBusStubAdapter);

    std::lock_guard<std::mutex> dbusObjectManagerStubLock(dbusObjectManagerStubLock_);

    const bool isRegistrationSuccessful = unregisterDBusStubAdapter(managedDBusStubAdapter);
    if (!isRegistrationSuccessful) {
        return false;
    }

    auto dbusConnection = dbusConnection_.lock();
    if (dbusConnection && dbusConnection->isConnected()) {
        const bool isDBusSignalEmitted = emitInterfacesRemovedSignal(managedDBusStubAdapter, dbusConnection);

        if (!isDBusSignalEmitted) {
            registerDBusStubAdapter(managedDBusStubAdapter);
            return false;
        }
    }

    return true;
}

bool DBusObjectManagerStub::isDBusStubAdapterExported(std::shared_ptr<DBusStubAdapter> dbusStubAdapter) {
    assert(dbusStubAdapter);

    const auto& dbusObjectPath = dbusStubAdapter->getObjectPath();
    const auto& dbusInterfaceName = dbusStubAdapter->getInterfaceName();

    std::lock_guard<std::mutex> dbusObjectManagerStubLock(dbusObjectManagerStubLock_);

    const auto& registeredDBusObjectPathIterator = registeredDBusObjectPathsMap_.find(dbusObjectPath);
    const bool isKnownDBusObjectPath = (registeredDBusObjectPathIterator != registeredDBusObjectPathsMap_.end());

    if (!isKnownDBusObjectPath) {
        return false;
    }

    auto& registeredDBusInterfacesMap = registeredDBusObjectPathIterator->second;
    const auto& registeredDBusInterfaceIterator = registeredDBusInterfacesMap.find(dbusInterfaceName);
    const bool isRegisteredDBusInterfaceName = (registeredDBusInterfaceIterator != registeredDBusInterfacesMap.end());

    if (isRegisteredDBusInterfaceName) {
        auto registeredDBusStubAdapter = registeredDBusInterfaceIterator->second;
        assert(registeredDBusStubAdapter == dbusStubAdapter);
    }

    return isRegisteredDBusInterfaceName;
}

bool DBusObjectManagerStub::registerDBusStubAdapter(std::shared_ptr<DBusStubAdapter> dbusStubAdapter) {
    const auto& dbusObjectPath = dbusStubAdapter->getObjectPath();
    const auto& dbusInterfaceName = dbusStubAdapter->getInterfaceName();
    const auto& registeredDBusObjectPathIterator = registeredDBusObjectPathsMap_.find(dbusObjectPath);
    const bool isKnownDBusObjectPath = (registeredDBusObjectPathIterator != registeredDBusObjectPathsMap_.end());
    bool isRegisterationSuccessful = false;

    if (isKnownDBusObjectPath) {
        auto& registeredDBusInterfacesMap = registeredDBusObjectPathIterator->second;
        const auto& registeredDBusInterfaceIterator = registeredDBusInterfacesMap.find(dbusInterfaceName);
        const bool isDBusInterfaceAlreadyRegistered = (registeredDBusInterfaceIterator != registeredDBusInterfacesMap.end());

        if (!isDBusInterfaceAlreadyRegistered) {
            const auto& insertResult = registeredDBusInterfacesMap.insert({ dbusInterfaceName, dbusStubAdapter });
            isRegisterationSuccessful = insertResult.second;
        }
    } else {
        const auto& insertResult = registeredDBusObjectPathsMap_.insert({
            dbusObjectPath, DBusInterfacesMap({{ dbusInterfaceName, dbusStubAdapter }})
        });
        isRegisterationSuccessful = insertResult.second;
    }

    return isRegisterationSuccessful;
}

bool DBusObjectManagerStub::unregisterDBusStubAdapter(std::shared_ptr<DBusStubAdapter> dbusStubAdapter) {
    const auto& dbusObjectPath = dbusStubAdapter->getObjectPath();
    const auto& dbusInterfaceName = dbusStubAdapter->getInterfaceName();
    const auto& registeredDBusObjectPathIterator = registeredDBusObjectPathsMap_.find(dbusObjectPath);
    const bool isKnownDBusObjectPath = (registeredDBusObjectPathIterator != registeredDBusObjectPathsMap_.end());

    if (!isKnownDBusObjectPath) {
        return false;
    }

    auto& registeredDBusInterfacesMap = registeredDBusObjectPathIterator->second;
    const auto& registeredDBusInterfaceIterator = registeredDBusInterfacesMap.find(dbusInterfaceName);
    const bool isRegisteredDBusInterfaceName = (registeredDBusInterfaceIterator != registeredDBusInterfacesMap.end());

    if (!isRegisteredDBusInterfaceName) {
        return false;
    }

    auto registeredDBusStubAdapter = registeredDBusInterfaceIterator->second;
    assert(registeredDBusStubAdapter == dbusStubAdapter);

    registeredDBusInterfacesMap.erase(registeredDBusInterfaceIterator);

    if (registeredDBusInterfacesMap.empty()) {
        registeredDBusObjectPathsMap_.erase(registeredDBusObjectPathIterator);
    }

    return true;
}


bool DBusObjectManagerStub::emitInterfacesAddedSignal(std::shared_ptr<DBusStubAdapter> dbusStubAdapter,
                                                      const std::shared_ptr<DBusProxyConnection>& dbusConnection) const {
    assert(dbusConnection);
    assert(dbusConnection->isConnected());

    const auto& dbusStubObjectPath = dbusStubAdapter->getObjectPath();
    const auto& dbusStubInterfaceName = dbusStubAdapter->getInterfaceName();
    DBusMessage dbusSignal = DBusMessage::createSignal(dbusObjectPath_, getInterfaceName(), "InterfacesAdded", "oa{sa{sv}}");
    DBusOutputStream dbusOutputStream(dbusSignal);
    DBusInterfacesAndPropertiesDict dbusInterfacesAndPropertiesDict({
        { dbusStubInterfaceName, DBusPropertiesChangedDict() }
    });

    if (dbusStubAdapter->isManagingInterface()) {
        dbusInterfacesAndPropertiesDict.insert({ getInterfaceName(), DBusPropertiesChangedDict() });
    }

    dbusOutputStream << dbusStubObjectPath;
    dbusOutputStream << dbusInterfacesAndPropertiesDict;
    dbusOutputStream.flush();

    const bool dbusSignalEmitted = dbusConnection->sendDBusMessage(dbusSignal);
    return dbusSignalEmitted;
}

bool DBusObjectManagerStub::emitInterfacesRemovedSignal(std::shared_ptr<DBusStubAdapter> dbusStubAdapter,
                                                        const std::shared_ptr<DBusProxyConnection>& dbusConnection) const {
    assert(dbusConnection);
    assert(dbusConnection->isConnected());

    const auto& dbusStubObjectPath = dbusStubAdapter->getObjectPath();
    const auto& dbusStubInterfaceName = dbusStubAdapter->getInterfaceName();
    DBusMessage dbusSignal = DBusMessage::createSignal(dbusObjectPath_, getInterfaceName(), "InterfacesRemoved", "oas");
    DBusOutputStream dbusOutputStream(dbusSignal);
    std::vector<std::string> removedInterfaces({ { dbusStubInterfaceName } });

    if (dbusStubAdapter->isManagingInterface()) {
        removedInterfaces.push_back(getInterfaceName());
    }

    dbusOutputStream << dbusStubObjectPath;
    dbusOutputStream << removedInterfaces;
    dbusOutputStream.flush();

    const bool dbusSignalEmitted = dbusConnection->sendDBusMessage(dbusSignal);
    return dbusSignalEmitted;
}

const char* DBusObjectManagerStub::getMethodsDBusIntrospectionXmlData() const {
    return "<method name=\"GetManagedObjects\">\n"
               "<arg type=\"a{oa{sa{sv}}}\" name=\"object_paths_interfaces_and_properties\" direction=\"out\"/>\n"
           "</method>\n"
           "<signal name=\"InterfacesAdded\">\n"
               "<arg type=\"o\" name=\"object_path\"/>\n"
               "<arg type=\"a{sa{sv}}\" name=\"interfaces_and_properties\"/>\n"
           "</signal>\n"
           "<signal name=\"InterfacesRemoved\">\n"
               "<arg type=\"o\" name=\"object_path\"/>\n"
               "<arg type=\"as\" name=\"interfaces\"/>\n"
           "</signal>";
}

bool DBusObjectManagerStub::onInterfaceDBusMessage(const DBusMessage& dbusMessage) {
    auto dbusConnection = dbusConnection_.lock();

    if (!dbusConnection || !dbusConnection->isConnected()) {
        return false;
    }

    if (!dbusMessage.isMethodCallType() || !dbusMessage.hasMemberName("GetManagedObjects")) {
        return false;
    }

    std::lock_guard<std::mutex> dbusObjectManagerStubLock(dbusObjectManagerStubLock_);
    DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;

    for (const auto& registeredDBusObjectPathIterator : registeredDBusObjectPathsMap_) {
        const std::string& registeredDBusObjectPath = registeredDBusObjectPathIterator.first;
        const auto& registeredDBusInterfacesMap = registeredDBusObjectPathIterator.second;
        DBusInterfacesAndPropertiesDict dbusInterfacesAndPropertiesDict;

        assert(registeredDBusObjectPath.length() > 0);
        assert(registeredDBusInterfacesMap.size() > 0);

        for (const auto& registeredDBusInterfaceIterator : registeredDBusInterfacesMap) {
            const std::string& registeredDBusInterfaceName = registeredDBusInterfaceIterator.first;
            const auto& registeredDBusStubAdapter = registeredDBusInterfaceIterator.second;

            assert(registeredDBusInterfaceName.length() > 0);

            dbusInterfacesAndPropertiesDict.insert({ registeredDBusInterfaceName, DBusPropertiesChangedDict() });

            if (registeredDBusStubAdapter->isManagingInterface()) {
                dbusInterfacesAndPropertiesDict.insert({ getInterfaceName(), DBusPropertiesChangedDict() });
            }
        }

        dbusObjectPathAndInterfacesDict.insert({ registeredDBusObjectPath, std::move(dbusInterfacesAndPropertiesDict) });
    }

    DBusMessage dbusMessageReply = dbusMessage.createMethodReturn("a{oa{sa{sv}}}");
    DBusOutputStream dbusOutputStream(dbusMessageReply);

    dbusOutputStream << dbusObjectPathAndInterfacesDict;
    dbusOutputStream.flush();

    const bool dbusMessageReplySent = dbusConnection->sendDBusMessage(dbusMessageReply);
    return dbusMessageReplySent;
}

} // namespace DBus
} // namespace CommonAPI
