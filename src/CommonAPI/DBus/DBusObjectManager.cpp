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

#include <dbus/dbus-protocol.h>

#include <cassert>
#include <sstream>

#include <unordered_set>

namespace CommonAPI {
namespace DBus {

DBusObjectManager::DBusObjectManager(const std::shared_ptr<DBusProxyConnection>& dbusConnection):
        dbusConnection_(dbusConnection) {

    if (!dbusConnection->isObjectPathMessageHandlerSet()) {
        dbusConnection->setObjectPathMessageHandler(
                        std::bind(&DBusObjectManager::handleMessage, this, std::placeholders::_1));
    }
    dbusConnection->registerObjectPath("/");
}

DBusObjectManager::~DBusObjectManager() {
    std::shared_ptr<DBusProxyConnection> dbusConnection = dbusConnection_.lock();
    if (dbusConnection) {
        dbusConnection->unregisterObjectPath("/");
        dbusConnection->setObjectPathMessageHandler(DBusProxyConnection::DBusObjectPathMessageHandler());
    }
}

DBusInterfaceHandlerToken DBusObjectManager::registerDBusStubAdapter(const std::string& objectPath,
                                                                     const std::string& interfaceName,
                                                                     DBusStubAdapter* dbusStubAdapter) {
    DBusInterfaceHandlerPath handlerPath(objectPath, interfaceName);

    objectPathLock_.lock();
    bool noSuchHandlerRegistered = dbusRegisteredObjectsTable_.find(handlerPath) == dbusRegisteredObjectsTable_.end();

    assert(noSuchHandlerRegistered);

    dbusRegisteredObjectsTable_.insert({handlerPath, dbusStubAdapter});
    objectPathLock_.unlock();

    std::shared_ptr<DBusProxyConnection> dbusConnection = dbusConnection_.lock();
    if (dbusConnection) {
        dbusConnection->registerObjectPath(objectPath);
    }

    return handlerPath;
}

void DBusObjectManager::unregisterDBusStubAdapter(const DBusInterfaceHandlerToken& dbusInterfaceHandlerToken) {
    objectPathLock_.lock();
    const std::string& objectPath = dbusInterfaceHandlerToken.first;

    std::shared_ptr<DBusProxyConnection> lockedConnection = dbusConnection_.lock();
    if (lockedConnection) {
        lockedConnection->unregisterObjectPath(objectPath);
    }

    dbusRegisteredObjectsTable_.erase(dbusInterfaceHandlerToken);
    objectPathLock_.unlock();
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
        DBusStubAdapter* dbusStubAdapter = handlerIterator->second;
        dbusMessageHandled = dbusStubAdapter->onInterfaceDBusMessage(dbusMessage);
    } else if (dbusMessage.hasInterfaceName("org.freedesktop.DBus.Introspectable")) {
        dbusMessageHandled = onIntrospectableInterfaceDBusMessage(dbusMessage);
    } else if (dbusMessage.hasInterfaceName("org.freedesktop.DBus.ObjectManager")) {
        dbusMessageHandled = onObjectManagerInterfaceDBusMessage(dbusMessage);
    }
    objectPathLock_.unlock();

    return dbusMessageHandled;
}

bool DBusObjectManager::onObjectManagerInterfaceDBusMessage(const DBusMessage& dbusMessage) {
    std::shared_ptr<DBusProxyConnection> dbusConnection = dbusConnection_.lock();

    if (!dbusConnection || !dbusMessage.isMethodCallType() || !dbusMessage.hasMemberName("GetManagedObjects")) {
        return false;
    }

    DBusDaemonProxy::DBusObjectToInterfaceDict ObjectPathsInterfacesAndPropertiesDict;

    objectPathLock_.lock();
    auto registeredObjectsIterator = dbusRegisteredObjectsTable_.begin();

    while(registeredObjectsIterator != dbusRegisteredObjectsTable_.end()) {
        DBusInterfaceHandlerPath handlerPath = registeredObjectsIterator->first;
        auto foundDictEntry = ObjectPathsInterfacesAndPropertiesDict.find(handlerPath.first);

        if (foundDictEntry == ObjectPathsInterfacesAndPropertiesDict.end()) {
            ObjectPathsInterfacesAndPropertiesDict.insert( { handlerPath.first, { { handlerPath.second, {} } } } );
        } else {
            foundDictEntry->second.insert( {handlerPath.second, {} } );
        }

        ++registeredObjectsIterator;
    }
    objectPathLock_.unlock();

    const char* getManagedObjectsDBusSignature = "a{oa{sa{sv}}}";
    DBusMessage dbusMessageReply = dbusMessage.createMethodReturn(getManagedObjectsDBusSignature);
    DBusOutputStream outStream(dbusMessageReply);

    outStream << ObjectPathsInterfacesAndPropertiesDict;
    outStream.flush();

    return dbusConnection->sendDBusMessage(dbusMessageReply);
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
        DBusStubAdapter* dbusStubAdapter = registeredObjectsIterator.second;

        if (dbusMessage.hasObjectPath(dbusObjectPath)) {
            foundRegisteredObjects = true;

            xmlData << "<interface name=\"" << dbusInterfaceName << "\">\n"
                            << dbusStubAdapter->getMethodsDBusIntrospectionXmlData() << "\n"
                            "</interface>\n";
        } else {
            std::vector<std::string> elems = CommonAPI::DBus::split(dbusObjectPath, '/');
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
                        if (dbusMessage.hasObjectPath(dbusObjectPath)) {
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
