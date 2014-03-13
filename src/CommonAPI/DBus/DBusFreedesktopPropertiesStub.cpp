/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusFreedesktopPropertiesStub.h"
#include "DBusStubAdapter.h"
#include "DBusServicePublisher.h"
#include "DBusOutputStream.h"
#include "DBusInputStream.h"

#include <cassert>
#include <vector>

namespace CommonAPI {
namespace DBus {

DBusFreedesktopPropertiesStub::DBusFreedesktopPropertiesStub(const std::string& dbusObjectPath,
                                                             const std::string& dbusInterfaceName,
                                                             const std::shared_ptr<DBusProxyConnection>& dbusConnection,
                                                             const std::shared_ptr<DBusStubAdapter>& dbusStubAdapter) :
                        dbusObjectPath_(dbusObjectPath),
                        dbusConnection_(dbusConnection),
                        dbusStubAdapter_(dbusStubAdapter) {
    assert(!dbusObjectPath.empty());
    assert(dbusObjectPath[0] == '/');
    assert(dbusConnection);

    dbusInterfacesLock_.lock();
    if(managedInterfaces_.find(dbusInterfaceName) == managedInterfaces_.end()) {
        managedInterfaces_.insert({dbusInterfaceName, dbusStubAdapter});
    }
    dbusInterfacesLock_.unlock();

}

DBusFreedesktopPropertiesStub::~DBusFreedesktopPropertiesStub() {
    // TODO: maybee some deregistration etc.
}

const char* DBusFreedesktopPropertiesStub::getMethodsDBusIntrospectionXmlData() const {
    return "<interface name=\"org.freedesktop.DBus.Properties\">\n"
             "<method name=\"Get\">\n"
               "<arg type=\"s\" name=\"interface_name\" direction=\"in\"/>\n"
               "<arg type=\"s\" name=\"property_name\" direction=\"in\"/>\n"
               "<arg type=\"v\" name=\"value\" direction=\"out\"/>\n"
             "</method>\n"
             "<method name=\"GetAll\">\n"
               "<arg type=\"s\" name=\"interface_name\" direction=\"in\"/>\n"
               "<arg type=\"a{sv}\" name=\"properties\" direction=\"out\"/>\n"
             "</method>\n"
             "<method name=\"Set\">\n"
               "<arg type=\"s\" name=\"interface_name\" direction=\"in\"/>\n"
               "<arg type=\"s\" name=\"property_name\" direction=\"in\"/>\n"
               "<arg type=\"v\" name=\"value\" direction=\"in\"/>\n"
             "</method>\n"
             "<signal name=\"PropertiesChanged\">\n"
               "<arg type=\"s\" name=\"interface_name\"/>\n"
               "<arg type=\"a{sv}\" name=\"changed_properties\"/>\n"
               "<arg type=\"as\" name=\"invalidated_properties\"/>\n"
             "</signal>\n"
           "</interface>\n";
}

bool DBusFreedesktopPropertiesStub::onInterfaceDBusMessage(const DBusMessage& dbusMessage) {
    auto dbusConnection = dbusConnection_.lock();

    if (!dbusConnection || !dbusConnection->isConnected()) {
        return false;
    }

    if (!dbusMessage.isMethodCallType() || !(dbusMessage.hasMemberName("Get") || dbusMessage.hasMemberName("GetAll") || dbusMessage.hasMemberName("Set"))) {
        return false;
    }

    DBusInputStream dbusInputStream(dbusMessage);
    std::string interfaceName;

    dbusInputStream >> interfaceName;

    if(dbusInputStream.hasError()) {
        return false;
    }

    std::lock_guard<std::mutex> dbusInterfacesLock(dbusInterfacesLock_);

    auto managedInterfacesIterator = managedInterfaces_.find(interfaceName);

    if(managedInterfacesIterator == managedInterfaces_.end()) {
        return false;
    }

    return managedInterfacesIterator->second->onInterfaceDBusFreedesktopPropertiesMessage(dbusMessage);
}

const bool DBusFreedesktopPropertiesStub::hasFreedesktopProperties() {
    return false;
}

const std::string& DBusFreedesktopPropertiesStub::getDBusObjectPath() const {
    return dbusObjectPath_;
}

const char* DBusFreedesktopPropertiesStub::getInterfaceName() {
    return "org.freedesktop.DBus.Properties";
}

} // namespace DBus
} // namespace CommonAPI
