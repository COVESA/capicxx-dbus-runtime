/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusObjectManager.h"
#include "DBusDaemonProxy.h"
#include "DBusStubAdapter.h"
#include "DBusOutputStream.h"
#include "DBusUtils.h"

#include <CommonAPI/utils.h>

#include <dbus/dbus-protocol.h>

#include <cassert>
#include <sstream>

#include <unordered_set>

namespace CommonAPI {
namespace DBus {

DBusObjectManager::DBusObjectManager(const std::shared_ptr<DBusProxyConnection>& dbusConnection):
        dbusConnection_(dbusConnection),
        rootDBusObjectManagerStub_(new DBusObjectManagerStub("/", dbusConnection)) {

    if (!dbusConnection->isObjectPathMessageHandlerSet()) {
        dbusConnection->setObjectPathMessageHandler(
                        std::bind(&DBusObjectManager::handleMessage, this, std::placeholders::_1));
    }
    dbusConnection->registerObjectPath("/");

    dbusRegisteredObjectsTable_.insert({
                    DBusInterfaceHandlerPath("/", DBusObjectManagerStub::getInterfaceName()),
                    rootDBusObjectManagerStub_ });
}

DBusObjectManager::~DBusObjectManager() {
    std::shared_ptr<DBusProxyConnection> dbusConnection = dbusConnection_.lock();
    if (dbusConnection) {
        dbusConnection->unregisterObjectPath("/");
        dbusConnection->setObjectPathMessageHandler(DBusProxyConnection::DBusObjectPathMessageHandler());
    }
}

bool DBusObjectManager::registerDBusStubAdapter(std::shared_ptr<DBusStubAdapter> dbusStubAdapter) {
    const auto& dbusStubAdapterObjectPath = dbusStubAdapter->getObjectPath();
    const auto& dbusStubAdapterInterfaceName = dbusStubAdapter->getInterfaceName();
    DBusInterfaceHandlerPath dbusStubAdapterHandlerPath(dbusStubAdapterObjectPath, dbusStubAdapterInterfaceName);
    bool isRegistrationSuccessful = false;

    objectPathLock_.lock();
    isRegistrationSuccessful = addDBusInterfaceHandler(dbusStubAdapterHandlerPath, dbusStubAdapter);

    if (isRegistrationSuccessful && dbusStubAdapter->isManagingInterface()) {
        auto managerStubIterator = managerStubs_.find(dbusStubAdapterObjectPath);
        const bool managerStubExists = managerStubIterator != managerStubs_.end();

        if (!managerStubExists) {
            const std::shared_ptr<DBusObjectManagerStub> newManagerStub = std::make_shared<DBusObjectManagerStub>(dbusStubAdapterObjectPath, dbusStubAdapter->getDBusConnection());
            auto insertResult = managerStubs_.insert( {dbusStubAdapterObjectPath, {newManagerStub, 1} });
            assert(insertResult.second);
            managerStubIterator = insertResult.first;
        } else {
            uint32_t& countReferencesToManagerStub = std::get<1>(managerStubIterator->second);
            ++countReferencesToManagerStub;
        }

        std::shared_ptr<DBusObjectManagerStub> dbusObjectManagerStub = std::get<0>(managerStubIterator->second);
        assert(dbusObjectManagerStub);

        isRegistrationSuccessful = addDBusInterfaceHandler(
                        { dbusStubAdapterObjectPath, dbusObjectManagerStub->getInterfaceName() }, dbusObjectManagerStub);

        if (!isRegistrationSuccessful) {
            const bool isDBusStubAdapterRemoved = removeDBusInterfaceHandler(dbusStubAdapterHandlerPath, dbusStubAdapter);
            assert(isDBusStubAdapterRemoved);
        }
    }

    if (isRegistrationSuccessful) {
        std::shared_ptr<DBusProxyConnection> dbusConnection = dbusConnection_.lock();
        if (dbusConnection) {
            dbusConnection->registerObjectPath(dbusStubAdapterObjectPath);
        }
    }
    objectPathLock_.unlock();

    return isRegistrationSuccessful;
}


bool DBusObjectManager::unregisterDBusStubAdapter(std::shared_ptr<DBusStubAdapter> dbusStubAdapter) {
    const auto& dbusStubAdapterObjectPath = dbusStubAdapter->getObjectPath();
    const auto& dbusStubAdapterInterfaceName = dbusStubAdapter->getInterfaceName();
    DBusInterfaceHandlerPath dbusStubAdapterHandlerPath(dbusStubAdapterObjectPath, dbusStubAdapterInterfaceName);
    bool isDeregistrationSuccessful = false;

    objectPathLock_.lock();
    isDeregistrationSuccessful = removeDBusInterfaceHandler(dbusStubAdapterHandlerPath, dbusStubAdapter);

    if (isDeregistrationSuccessful && dbusStubAdapter->isManagingInterface()) {
        auto managerStubIterator = managerStubs_.find(dbusStubAdapterObjectPath);
        assert(managerStubIterator != managerStubs_.end());

        std::shared_ptr<DBusObjectManagerStub> dbusObjectManagerStub = std::get<0>(managerStubIterator->second);
        assert(dbusObjectManagerStub);

        uint32_t& countReferencesToManagerStub = std::get<1>(managerStubIterator->second);
        assert(countReferencesToManagerStub > 0);
        --countReferencesToManagerStub;

        if (countReferencesToManagerStub == 0) {
            isDeregistrationSuccessful = removeDBusInterfaceHandler(
                            { dbusStubAdapterObjectPath, dbusObjectManagerStub->getInterfaceName() }, dbusObjectManagerStub);
            managerStubs_.erase(managerStubIterator);
            assert(isDeregistrationSuccessful);
        }
    }

    if (isDeregistrationSuccessful) {
        std::shared_ptr<DBusProxyConnection> lockedConnection = dbusConnection_.lock();
        if (lockedConnection) {
            lockedConnection->unregisterObjectPath(dbusStubAdapterObjectPath);
        }
    }

    objectPathLock_.unlock();

    return isDeregistrationSuccessful;
}


bool DBusObjectManager::exportManagedDBusStubAdapter(const std::string& parentObjectPath, std::shared_ptr<DBusStubAdapter> dbusStubAdapter) {
    auto foundManagerStubIterator = managerStubs_.find(parentObjectPath);

    assert(foundManagerStubIterator != managerStubs_.end());

    if (std::get<0>(foundManagerStubIterator->second)->exportManagedDBusStubAdapter(dbusStubAdapter)) {
        //XXX Other handling necessary?
        return true;
    }
    return false;
}


bool DBusObjectManager::unexportManagedDBusStubAdapter(const std::string& parentObjectPath, std::shared_ptr<DBusStubAdapter> dbusStubAdapter) {
    auto foundManagerStubIterator = managerStubs_.find(parentObjectPath);

    assert(foundManagerStubIterator != managerStubs_.end());

    if (std::get<0>(foundManagerStubIterator->second)->unexportManagedDBusStubAdapter(dbusStubAdapter)) {
        //XXX Other handling necessary?
        return true;
    }
    return false;
}


bool DBusObjectManager::handleMessage(const DBusMessage& dbusMessage) {
    const char* objectPath = dbusMessage.getObjectPath();
    const char* interfaceName = dbusMessage.getInterfaceName();

    assert(objectPath);
    assert(interfaceName);

    DBusInterfaceHandlerPath handlerPath(objectPath, interfaceName);

    objectPathLock_.lock();
    auto handlerIterator = dbusRegisteredObjectsTable_.find(handlerPath);
    const bool foundDBusInterfaceHandler = handlerIterator != dbusRegisteredObjectsTable_.end();
    bool dbusMessageHandled = false;

    if (foundDBusInterfaceHandler) {
        std::shared_ptr<DBusInterfaceHandler> dbusStubAdapterBase = handlerIterator->second;
        dbusMessageHandled = dbusStubAdapterBase->onInterfaceDBusMessage(dbusMessage);
    } else if (dbusMessage.hasInterfaceName("org.freedesktop.DBus.Introspectable")) {
        dbusMessageHandled = onIntrospectableInterfaceDBusMessage(dbusMessage);
    }
    objectPathLock_.unlock();

    return dbusMessageHandled;
}

bool DBusObjectManager::addDBusInterfaceHandler(const DBusInterfaceHandlerPath& dbusInterfaceHandlerPath,
                                                std::shared_ptr<DBusInterfaceHandler> dbusInterfaceHandler) {
    const auto& dbusRegisteredObjectsTableIter = dbusRegisteredObjectsTable_.find(dbusInterfaceHandlerPath);
    const bool isDBusInterfaceHandlerAlreadyAdded = (dbusRegisteredObjectsTableIter != dbusRegisteredObjectsTable_.end());

    if (isDBusInterfaceHandlerAlreadyAdded) {
        //If another ObjectManager is to be registered, you can go on and just use the first one.
        if (dbusInterfaceHandlerPath.second == "org.freedesktop.DBus.ObjectManager") {
            return true;
        }
        return false;
    }

    auto insertResult = dbusRegisteredObjectsTable_.insert({ dbusInterfaceHandlerPath, dbusInterfaceHandler });
    const bool insertSuccess = insertResult.second;

    return insertSuccess;
}

bool DBusObjectManager::removeDBusInterfaceHandler(const DBusInterfaceHandlerPath& dbusInterfaceHandlerPath,
                                                   std::shared_ptr<DBusInterfaceHandler> dbusInterfaceHandler) {
    const auto& dbusRegisteredObjectsTableIter = dbusRegisteredObjectsTable_.find(dbusInterfaceHandlerPath);
    const bool isDBusInterfaceHandlerAdded = (dbusRegisteredObjectsTableIter != dbusRegisteredObjectsTable_.end());

    if (isDBusInterfaceHandlerAdded) {
        auto registeredDBusStubAdapter = dbusRegisteredObjectsTableIter->second;
        assert(registeredDBusStubAdapter == dbusInterfaceHandler);

        dbusRegisteredObjectsTable_.erase(dbusRegisteredObjectsTableIter);
    }

    return isDBusInterfaceHandlerAdded;
}

bool DBusObjectManager::onIntrospectableInterfaceDBusMessage(const DBusMessage& dbusMessage) {
    std::shared_ptr<DBusProxyConnection> dbusConnection = dbusConnection_.lock();

    if (!dbusConnection || !dbusMessage.isMethodCallType() || !dbusMessage.hasMemberName("Introspect")) {
        return false;
    }

    bool foundRegisteredObjects = false;
    std::stringstream xmlData(std::ios_base::out);

    xmlData << "<!DOCTYPE node PUBLIC \"" DBUS_INTROSPECT_1_0_XML_PUBLIC_IDENTIFIER "\"\n\""
                    DBUS_INTROSPECT_1_0_XML_SYSTEM_IDENTIFIER"\">\n"
                    "<node name=\"" << dbusMessage.getObjectPath() << "\">\n"
                        "<interface name=\"org.freedesktop.DBus.Introspectable\">\n"
                            "<method name=\"Introspect\">\n"
                                "<arg type=\"s\" name=\"xml_data\" direction=\"out\"/>\n"
                            "</method>\n"
                        "</interface>\n";

    std::unordered_set<std::string> nodeSet;
    for (auto& registeredObjectsIterator : dbusRegisteredObjectsTable_) {
        const DBusInterfaceHandlerPath& handlerPath = registeredObjectsIterator.first;
        const std::string& dbusObjectPath = handlerPath.first;
        const std::string& dbusInterfaceName = handlerPath.second;
        std::shared_ptr<DBusInterfaceHandler> dbusStubAdapterBase = registeredObjectsIterator.second;
        std::vector<std::string> elems = CommonAPI::split(dbusObjectPath, '/');

        if (dbusMessage.hasObjectPath(dbusObjectPath)) {
            foundRegisteredObjects = true;

            xmlData << "<interface name=\"" << dbusInterfaceName << "\">\n"
                            << dbusStubAdapterBase->getMethodsDBusIntrospectionXmlData() << "\n"
                            "</interface>\n";
            nodeSet.insert(elems.back());
            //break;
        } else {
            if (dbusMessage.hasObjectPath("/") && elems.size() > 1) {
                if (nodeSet.find(elems[1]) == nodeSet.end()) {
                    if (nodeSet.size() == 0) {
                        xmlData.str("");
                        xmlData << "<!DOCTYPE node PUBLIC \"" DBUS_INTROSPECT_1_0_XML_PUBLIC_IDENTIFIER "\"\n\""
                        DBUS_INTROSPECT_1_0_XML_SYSTEM_IDENTIFIER"\">\n"
                        "<node>\n";
                    }
                    xmlData << "    <node name=\"" << elems[1] << "\"/>\n";
                    nodeSet.insert(elems[1]);
                    foundRegisteredObjects = true;
                }
            } else {
                for (int i = 1; i < elems.size() - 1; i++) {
                    std::string build;
                    for (int j = 1; j <= i; j++) {
                        build = build + "/" + elems[j];
                        if (dbusMessage.hasObjectPath(build)) {
                            if (nodeSet.find(elems[j + 1]) == nodeSet.end()) {
                                if (nodeSet.size() == 0) {
                                    xmlData.str("");
                                    xmlData << "<!DOCTYPE node PUBLIC \"" DBUS_INTROSPECT_1_0_XML_PUBLIC_IDENTIFIER "\"\n\""
                                            DBUS_INTROSPECT_1_0_XML_SYSTEM_IDENTIFIER"\">\n"
                                            "<node>\n";
                                }
                                xmlData << "    <node name=\"" << elems[j + 1] << "\"/>\n";
                                nodeSet.insert(elems[j + 1]);
                                foundRegisteredObjects = true;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    if (foundRegisteredObjects) {
        DBusMessage dbusMessageReply = dbusMessage.createMethodReturn("s");
        DBusOutputStream dbusOutputStream(dbusMessageReply);

        xmlData << "</node>"
                        "";

        dbusOutputStream << xmlData.str();
        dbusOutputStream.flush();

        return dbusConnection->sendDBusMessage(dbusMessageReply);
    }

    return false;
}

} // namespace DBus
} // namespace CommonAPI
