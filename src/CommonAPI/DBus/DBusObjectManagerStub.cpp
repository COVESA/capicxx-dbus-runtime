// Copyright (C) 2013-2017 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <vector>
#include <algorithm>
#include <CommonAPI/DBus/DBusObjectManagerStub.hpp>
#include <CommonAPI/DBus/DBusOutputStream.hpp>
#include <CommonAPI/DBus/DBusStubAdapter.hpp>
#include <CommonAPI/DBus/DBusTypes.hpp>

namespace CommonAPI {
namespace DBus {

DBusObjectManagerStub::DBusObjectManagerStub(const std::string& dbusObjectPath,
                                             const std::shared_ptr<DBusProxyConnection>& dbusConnection) :
                        dbusObjectPath_(dbusObjectPath),
                        dbusConnection_(dbusConnection) {
    if (dbusObjectPath.empty()) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " empty _path");
    }
    if ('/' != dbusObjectPath[0]) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " invalid _path ", dbusObjectPath);
    }
    if (!dbusConnection) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " invalid _connection");
    }
}

DBusObjectManagerStub::~DBusObjectManagerStub() {
    for (auto& dbusObjectPathIterator : registeredDBusObjectPathsMap_) {
        auto& registeredDBusInterfacesMap = dbusObjectPathIterator.second;

        for (auto& dbusInterfaceIterator : registeredDBusInterfacesMap) {
            auto managedDBusStubAdapter = dbusInterfaceIterator.second;
#ifdef COMMONAPI_TODO
            for (auto& adapterIterator : dbusInterfaceIterator.second) {
                auto managedDBusStubAdapterServiceAddress = adapterIterator->getDBusAddress();

                const bool isServiceUnregistered = DBusServicePublisher::getInstance()->unregisterManagedService(
                                managedDBusStubAdapterServiceAddress);
                if (!isServiceUnregistered) {
                    COMMONAPI_ERROR(std::string(__FUNCTION__), " service still registered ", managedDBusStubAdapterServiceAddress.getService());
                }
#endif
        }
    }
}

bool DBusObjectManagerStub::exportManagedDBusStubAdapter(std::shared_ptr<DBusStubAdapter> managedDBusStubAdapter) {
    if (!managedDBusStubAdapter) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), "managedDBusStubAdapter == nullptr");
        return false;
    }

    // if already registered, return true.
    const bool alreadyExported = isDBusStubAdapterExported(managedDBusStubAdapter);
    if (alreadyExported) {
        return true;
    }

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
    if (!managedDBusStubAdapter) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), "managedDBusStubAdapter == nullptr");
        return false;
    }

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
    if (!dbusStubAdapter) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), "dbusStubAdapter == nullptr");
        return false;
    }

    const auto& dbusObjectPath = dbusStubAdapter->getDBusAddress().getObjectPath();
    const auto& dbusInterfaceName = dbusStubAdapter->getDBusAddress().getInterface();

    std::lock_guard<std::mutex> dbusObjectManagerStubLock(dbusObjectManagerStubLock_);

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

    const auto& registeredDBusStubAdapterList = registeredDBusInterfaceIterator->second;
    auto registeredDBusStubAdapter = find(registeredDBusStubAdapterList.begin(), registeredDBusStubAdapterList.end(), dbusStubAdapter);

    return registeredDBusStubAdapter != registeredDBusStubAdapterList.end();

}

bool DBusObjectManagerStub::registerDBusStubAdapter(std::shared_ptr<DBusStubAdapter> dbusStubAdapter) {
    const auto& dbusObjectPath = dbusStubAdapter->getDBusAddress().getObjectPath();
    const auto& dbusInterfaceName = dbusStubAdapter->getDBusAddress().getInterface();
    const auto& registeredDBusObjectPathIterator = registeredDBusObjectPathsMap_.find(dbusObjectPath);
    const bool isKnownDBusObjectPath = (registeredDBusObjectPathIterator != registeredDBusObjectPathsMap_.end());
    bool isRegisterationSuccessful = false;

    if (isKnownDBusObjectPath) {
        auto& registeredDBusInterfacesMap = registeredDBusObjectPathIterator->second;
        const auto& registeredDBusInterfaceIterator = registeredDBusInterfacesMap.find(dbusInterfaceName);
        const bool isDBusInterfaceAlreadyRegistered = (registeredDBusInterfaceIterator != registeredDBusInterfacesMap.end());

        if (!isDBusInterfaceAlreadyRegistered) {
            const auto& insertResult = registeredDBusInterfacesMap.insert(
                { dbusInterfaceName,
                  std::vector<std::shared_ptr<DBusStubAdapter>>({dbusStubAdapter})});
            isRegisterationSuccessful = insertResult.second;
        }
        else {
            // add to existing interface
            auto adapterList = registeredDBusInterfaceIterator->second;
            if (find(adapterList.begin(), adapterList.end(), dbusStubAdapter) == adapterList.end()) {
                adapterList.push_back(dbusStubAdapter);
                registeredDBusInterfaceIterator->second = adapterList;
                isRegisterationSuccessful = true;
            }
            else {
                // already registered
                isRegisterationSuccessful = false;
            }
        }
    } else {
        const auto& insertResult = registeredDBusObjectPathsMap_.insert({
            dbusObjectPath, DBusInterfacesMap({{ dbusInterfaceName, std::vector<std::shared_ptr<DBusStubAdapter>>({dbusStubAdapter}) }})
        });
        isRegisterationSuccessful = insertResult.second;
    }

    return isRegisterationSuccessful;
}

bool DBusObjectManagerStub::unregisterDBusStubAdapter(std::shared_ptr<DBusStubAdapter> dbusStubAdapter) {
    const auto& dbusObjectPath = dbusStubAdapter->getDBusAddress().getObjectPath();
    const auto& dbusInterfaceName = dbusStubAdapter->getDBusAddress().getInterface();
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

    auto& registeredAdapterList = registeredDBusInterfaceIterator->second;
    auto adapter = find (registeredAdapterList.begin(), registeredAdapterList.end(), dbusStubAdapter);

    if (registeredAdapterList.end() == adapter) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " stub adapter not registered ", dbusObjectPath, " interface: ", dbusInterfaceName);
        return false;
    }

    registeredAdapterList.erase(adapter);

    if (registeredAdapterList.empty()) {
        registeredDBusInterfacesMap.erase(registeredDBusInterfaceIterator);

        if (registeredDBusInterfacesMap.empty()) {
            registeredDBusObjectPathsMap_.erase(registeredDBusObjectPathIterator);
        }
    }
    return true;
}


bool DBusObjectManagerStub::emitInterfacesAddedSignal(std::shared_ptr<DBusStubAdapter> dbusStubAdapter,
                                                      const std::shared_ptr<DBusProxyConnection>& dbusConnection) const {
    if (!dbusConnection) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " dbusConnection == nullptr");
        return false;
    }
    if (!dbusConnection->isConnected()) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " not connected");
        return false;
    }

    const auto& dbusStubObjectPath = dbusStubAdapter->getDBusAddress().getObjectPath();
    const auto& dbusStubInterfaceName = dbusStubAdapter->getDBusAddress().getInterface();
    DBusMessage dbusSignal = DBusMessage::createSignal(dbusObjectPath_, getInterfaceName(), "InterfacesAdded", "oa{sa{sv}}");
    DBusOutputStream dbusOutputStream(dbusSignal);
    DBusInterfacesAndPropertiesDict dbusInterfacesAndPropertiesDict({
        { dbusStubInterfaceName, DBusPropertiesChangedDict() }
    });

    if (dbusStubAdapter->isManaging()) {
        dbusInterfacesAndPropertiesDict.insert({ getInterfaceName(), DBusPropertiesChangedDict() });
    }

    if (dbusStubAdapter->hasFreedesktopProperties()) {
        dbusInterfacesAndPropertiesDict.insert({ "org.freedesktop.DBus.Properties", DBusPropertiesChangedDict() });
    }

    dbusOutputStream << dbusStubObjectPath;
    dbusOutputStream << dbusInterfacesAndPropertiesDict;
    dbusOutputStream.flush();

    const bool dbusSignalEmitted = dbusConnection->sendDBusMessage(dbusSignal);
    return dbusSignalEmitted;
}

bool DBusObjectManagerStub::emitInterfacesRemovedSignal(std::shared_ptr<DBusStubAdapter> dbusStubAdapter,
                                                        const std::shared_ptr<DBusProxyConnection>& dbusConnection) const {
    if (!dbusConnection) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " dbusConnection == nullptr");
        return false;
    }
    if (!dbusConnection->isConnected()) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " not connected");
        return false;
    }

    const auto& dbusStubObjectPath = dbusStubAdapter->getDBusAddress().getObjectPath();
    const auto& dbusStubInterfaceName = dbusStubAdapter->getDBusAddress().getInterface();
    DBusMessage dbusSignal = DBusMessage::createSignal(dbusObjectPath_, getInterfaceName(), "InterfacesRemoved", "oas");
    DBusOutputStream dbusOutputStream(dbusSignal);
    std::vector<std::string> removedInterfaces({ { dbusStubInterfaceName } });

    if (dbusStubAdapter->isManaging()) {
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

    DBusMessage dbusMessageReply = dbusMessage.createMethodReturn("a{oa{sa{sv}}}");
    DBusOutputStream dbusOutputStream(dbusMessageReply);
	dbusOutputStream.beginWriteMap();

    for (const auto& registeredDBusObjectPathIterator : registeredDBusObjectPathsMap_)
    {
        const std::string& registeredDBusObjectPath = registeredDBusObjectPathIterator.first;
        const auto& registeredDBusInterfacesMap = registeredDBusObjectPathIterator.second;

        if (0 == registeredDBusObjectPath.length()) {
            COMMONAPI_ERROR(std::string(__FUNCTION__), " empty object path");
        } else {

            if (0 == registeredDBusInterfacesMap.size()) {
                COMMONAPI_ERROR(std::string(__FUNCTION__), " empty interfaces map for ", registeredDBusObjectPath);
            }

            dbusOutputStream.align(8);
            dbusOutputStream << registeredDBusObjectPath;
            dbusOutputStream.beginWriteMap();

            for (const auto& registeredDBusInterfaceIterator : registeredDBusInterfacesMap)
            {
                const std::string& registeredDBusInterfaceName = registeredDBusInterfaceIterator.first;
                const auto& registeredDBusStubAdapter = registeredDBusInterfaceIterator.second.begin();

                if (0 == registeredDBusInterfaceName.length()) {
                    COMMONAPI_ERROR(std::string(__FUNCTION__), " empty interface name for ", registeredDBusObjectPath);
                } else {
                    dbusOutputStream.align(8);
                    dbusOutputStream << registeredDBusInterfaceName;
                    dbusOutputStream.beginWriteMap();

					(*registeredDBusStubAdapter)->appendGetAllReply(dbusMessage, dbusOutputStream);

                    dbusOutputStream.endWriteMap();
                }

                if ((*registeredDBusStubAdapter)->isManaging())
                {
//                	dbusOutputStream.align(8);
//                    dbusOutputStream << std::string(getInterfaceName());
//                    dbusOutputStream << DBusPropertiesChangedDict();

                }
            }

            dbusOutputStream.endWriteMap();
        }
    }

    dbusOutputStream.endWriteMap();
    dbusOutputStream.flush();

    const bool dbusMessageReplySent = dbusConnection->sendDBusMessage(dbusMessageReply);
    return dbusMessageReplySent;
}


bool DBusObjectManagerStub::hasFreedesktopProperties() {
    return false;
}

const std::string& DBusObjectManagerStub::getDBusObjectPath() const {
    return dbusObjectPath_;
}

const char* DBusObjectManagerStub::getInterfaceName() {
    return "org.freedesktop.DBus.ObjectManager";
}

} // namespace DBus
} // namespace CommonAPI
