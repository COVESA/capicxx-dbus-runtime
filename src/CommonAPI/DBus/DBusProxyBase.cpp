/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusProxyBase.h"
#include "DBusMessage.h"

namespace CommonAPI {
namespace DBus {

DBusProxyBase::DBusProxyBase(const std::shared_ptr<DBusProxyConnection>& dbusConnection) :
                        commonApiDomain_("local"),
                        dbusConnection_(dbusConnection){
}

DBusMessage DBusProxyBase::createMethodCall(const char* methodName,
                                        const char* methodSignature) const {
    return DBusMessage::createMethodCall(
                    getDBusBusName().c_str(),
                    getDBusObjectPath().c_str(),
                    getInterfaceName().c_str(),
                    methodName,
                    methodSignature);
}

const std::shared_ptr<DBusProxyConnection>& DBusProxyBase::getDBusConnection() const {
    return dbusConnection_;
}

DBusProxyConnection::DBusSignalHandlerToken DBusProxyBase::addSignalMemberHandler(
        const std::string& signalName,
        const std::string& signalSignature,
        DBusProxyConnection::DBusSignalHandler* dbusSignalHandler,
        const bool justAddFilter) {
    return addSignalMemberHandler(
            getDBusObjectPath(),
            getInterfaceName(),
            signalName,
            signalSignature,
            dbusSignalHandler,
            justAddFilter);
}

DBusProxyConnection::DBusSignalHandlerToken DBusProxyBase::addSignalMemberHandler(
                const std::string& objectPath,
                const std::string& interfaceName,
                const std::string& signalName,
                const std::string& signalSignature,
                DBusProxyConnection::DBusSignalHandler* dbusSignalHandler,
                const bool justAddFilter) {
    return dbusConnection_->addSignalMemberHandler(
                objectPath,
                interfaceName,
                signalName,
                signalSignature,
                dbusSignalHandler,
                justAddFilter);
}

bool DBusProxyBase::removeSignalMemberHandler(const DBusProxyConnection::DBusSignalHandlerToken& dbusSignalHandlerToken, const DBusProxyConnection::DBusSignalHandler* dbusSignalHandler) {
    return dbusConnection_->removeSignalMemberHandler(dbusSignalHandlerToken, dbusSignalHandler);
}

} // namespace DBus
} // namespace CommonAPI
