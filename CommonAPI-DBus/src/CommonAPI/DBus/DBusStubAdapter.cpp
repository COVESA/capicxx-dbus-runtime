/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusStubAdapter.h"

#include <dbus/dbus-protocol.h>

#include <cassert>
#include <functional>
#include <sstream>

namespace CommonAPI {
namespace DBus {

const std::string DBusStubAdapter::domain_ = "local";

DBusStubAdapter::DBusStubAdapter(const std::string& dbusBusName,
                                 const std::string& dbusObjectPath,
                                 const std::string& interfaceName,
                                 const std::shared_ptr<DBusProxyConnection>& dbusConnection) :
                dbusBusName_(dbusBusName),
                dbusObjectPath_(dbusObjectPath),
                interfaceName_(interfaceName),
                dbusConnection_(dbusConnection),
                isInitialized_(false) {

    assert(!dbusBusName_.empty());
    assert(!interfaceName_.empty());
    assert(!dbusObjectPath_.empty());
    assert(dbusObjectPath_[0] == '/');
    assert(!interfaceName.empty());
    assert(dbusConnection_);
}

DBusStubAdapter::~DBusStubAdapter() {
	assert(dbusConnection_);
	assert(isInitialized_);

    dbusConnection_->getDBusObjectManager()->unregisterInterfaceHandler(dbusIntrospectionInterfaceHandlerToken_);
	dbusConnection_->getDBusObjectManager()->unregisterInterfaceHandler(dbusInterfaceHandlerToken_);
}

void DBusStubAdapter::init() {
    dbusIntrospectionInterfaceHandlerToken_ = dbusConnection_->getDBusObjectManager()->registerInterfaceHandler(
                    dbusObjectPath_,
                    "org.freedesktop.DBus.Introspectable",
                    std::bind(&DBusStubAdapter::onIntrospectionInterfaceDBusMessage, this, std::placeholders::_1));

    dbusInterfaceHandlerToken_ = dbusConnection_->getDBusObjectManager()->registerInterfaceHandler(
                    dbusObjectPath_,
                    interfaceName_,
                    std::bind(&DBusStubAdapter::onInterfaceDBusMessage, this, std::placeholders::_1));

    isInitialized_ = true;
}

const std::string DBusStubAdapter::getAddress() const {
	return domain_ + ":" + interfaceName_ + ":" + dbusBusName_;
}

const std::string& DBusStubAdapter::getDomain() const {
    return domain_;
}

const std::string& DBusStubAdapter::getServiceId() const {
    return interfaceName_;
}

const std::string& DBusStubAdapter::getInstanceId() const {
    return dbusObjectPath_;
}

bool DBusStubAdapter::onIntrospectionInterfaceDBusMessage(const DBusMessage& dbusMessage) {
    bool dbusMessageHandled = false;

    if (dbusMessage.isMethodCallType() && dbusMessage.hasMemberName("Introspect")) {
        std::stringstream xmlData(std::ios_base::out);
        xmlData << "<!DOCTYPE node PUBLIC \"" DBUS_INTROSPECT_1_0_XML_PUBLIC_IDENTIFIER "\"\n\""
                        DBUS_INTROSPECT_1_0_XML_SYSTEM_IDENTIFIER"\">\n"
                   "<node name=\"" << dbusObjectPath_ << "\">\n"
                       "<interface name=\"org.freedesktop.DBus.Introspectable\">\n"
                           "<method name=\"Introspect\">\n"
                               "<arg type=\"s\" name=\"xml_data\" direction=\"out\"/>\n"
                           "</method>\n"
                       "</interface>\n"
                       "<interface name=\"" << interfaceName_ << "\">\n"
                           << getMethodsDBusIntrospectionXmlData() << "\n"
                       "</interface>\n"
                   "</node>";

        DBusMessage dbusMessageReply = dbusMessage.createMethodReturn("s");
        DBusOutputStream dbusOutputStream(dbusMessageReply);
        dbusOutputStream << xmlData.str();
        dbusOutputStream.flush();

        dbusMessageHandled = dbusConnection_->sendDBusMessage(dbusMessageReply);
    }

    return dbusMessageHandled;
}

} // namespace dbus
} // namespace CommonAPI
