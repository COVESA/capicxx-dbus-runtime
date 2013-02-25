/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusObjectManager.h"
#include "DBusOutputStream.h"

#include <cassert>

namespace CommonAPI {
namespace DBus {

DBusObjectManager::DBusObjectManager(const std::shared_ptr<DBusConnection>& dbusConnection):
        dbusConnection_(dbusConnection) {

    registerInterfaceHandler("/",
                             "org.freedesktop.DBus.ObjectManager",
                             std::bind(&DBusObjectManager::onGetDBusObjectManagerData, this, std::placeholders::_1));
}

DBusInterfaceHandlerToken DBusObjectManager::registerInterfaceHandler(const std::string& objectPath,
                                                                      const std::string& interfaceName,
                                                                      const DBusMessageInterfaceHandler& dbusMessageInterfaceHandler) {
    DBusInterfaceHandlerPath handlerPath(objectPath, interfaceName);
    bool noSuchHandlerRegistered = dbusRegisteredObjectsTable_.find(handlerPath) == dbusRegisteredObjectsTable_.end();

    assert(noSuchHandlerRegistered);

    dbusRegisteredObjectsTable_.insert({handlerPath, dbusMessageInterfaceHandler});
    dbusConnection_->registerObjectPath(objectPath);

    return handlerPath;
}

void DBusObjectManager::unregisterInterfaceHandler(const DBusInterfaceHandlerToken& dbusInterfaceHandlerToken) {
    const std::string& objectPath = dbusInterfaceHandlerToken.first;

    dbusConnection_->unregisterObjectPath(objectPath);

    dbusRegisteredObjectsTable_.erase(dbusInterfaceHandlerToken);
}

bool DBusObjectManager::handleMessage(const DBusMessage& dbusMessage) const {
    const char* objectPath = dbusMessage.getObjectPath();
    const char* interfaceName = dbusMessage.getInterfaceName();

    assert(objectPath);
    assert(interfaceName);

    DBusInterfaceHandlerPath handlerPath(objectPath, interfaceName);
    auto handlerIterator = dbusRegisteredObjectsTable_.find(handlerPath);
    const bool foundDBusInterfaceHandler = handlerIterator != dbusRegisteredObjectsTable_.end();
    bool dbusMessageHandled = false;

    if (foundDBusInterfaceHandler) {
        const DBusMessageInterfaceHandler& interfaceHandlerDBusMessageHandler = handlerIterator->second;
        dbusMessageHandled = interfaceHandlerDBusMessageHandler(dbusMessage);
    }

    return dbusMessageHandled;
}

bool DBusObjectManager::onGetDBusObjectManagerData(const DBusMessage& callMessage) {

    DBusObjectToInterfaceDict dictToSend;

    const char* interfaceName = callMessage.getInterfaceName();
    const char* signature = callMessage.getSignatureString();

    assert(!strcmp(interfaceName, "org.freedesktop.DBus.ObjectManager"));
    assert(!strcmp(signature, ""));
    assert(callMessage.getType() == DBusMessage::Type::MethodCall);

    auto registeredObjectsIterator = dbusRegisteredObjectsTable_.begin();

    while(registeredObjectsIterator != dbusRegisteredObjectsTable_.end()) {
        DBusInterfaceHandlerPath handlerPath = registeredObjectsIterator->first;
        auto foundDictEntry = dictToSend.find(handlerPath.first);

        if(foundDictEntry == dictToSend.end()) {
            dictToSend.insert( { handlerPath.first, { { handlerPath.second, {} } } } );
        } else {
            foundDictEntry->second.insert( {handlerPath.second, {} } );
        }

        ++registeredObjectsIterator;
    }

    const char* getManagedObjectsDBusSignature = "a{oa{sa{sv}}}";
    DBusMessage replyMessage = callMessage.createMethodReturn(getManagedObjectsDBusSignature);

    DBusOutputStream outStream(replyMessage);
    outStream << dictToSend;
    outStream.flush();
    return dbusConnection_->sendDBusMessage(replyMessage);
}


} // namespace DBus
} // namespace CommonAPI
