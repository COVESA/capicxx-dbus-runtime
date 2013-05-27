/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_PROXY_BASE_H_
#define COMMONAPI_DBUS_DBUS_PROXY_BASE_H_

#include "DBusProxyConnection.h"

#include <CommonAPI/Proxy.h>
#include <CommonAPI/types.h>

#include <functional>
#include <memory>
#include <string>

namespace CommonAPI {
namespace DBus {

class DBusProxyBase: public virtual CommonAPI::Proxy {
 public:
    DBusProxyBase(const std::shared_ptr<DBusProxyConnection>& dbusProxyConnection);

    virtual std::string getAddress() const = 0;
    virtual const std::string& getDomain() const = 0;
    virtual const std::string& getServiceId() const = 0;
    virtual const std::string& getInstanceId() const = 0;

    virtual const std::string& getDBusBusName() const = 0;
    virtual const std::string& getDBusObjectPath() const = 0;
    virtual const std::string& getInterfaceName() const = 0;
    inline const std::shared_ptr<DBusProxyConnection>& getDBusConnection() const;

    DBusMessage createMethodCall(const char* methodName,
                                 const char* methodSignature = NULL) const;

    inline DBusProxyConnection::DBusSignalHandlerToken addSignalMemberHandler(
    		const std::string& signalName,
    		const std::string& signalSignature,
    		DBusProxyConnection::DBusSignalHandler* dbusSignalHandler);

    inline void removeSignalMemberHandler(const DBusProxyConnection::DBusSignalHandlerToken& dbusSignalHandlerToken);

 protected:
    static const std::string commonApiDomain_;

 private:
    DBusProxyBase(const DBusProxyBase&) = delete;

    std::shared_ptr<DBusProxyConnection> dbusConnection_;
};

const std::shared_ptr<DBusProxyConnection>& DBusProxyBase::getDBusConnection() const {
    return dbusConnection_;
}

DBusProxyConnection::DBusSignalHandlerToken DBusProxyBase::addSignalMemberHandler(
        const std::string& signalName,
        const std::string& signalSignature,
        DBusProxyConnection::DBusSignalHandler* dbusSignalHandler) {
    return dbusConnection_->addSignalMemberHandler(
            getDBusObjectPath(),
            getInterfaceName(),
            signalName,
            signalSignature,
            dbusSignalHandler);
}

void DBusProxyBase::removeSignalMemberHandler(const DBusProxyConnection::DBusSignalHandlerToken& dbusSignalHandlerToken) {
    return dbusConnection_->removeSignalMemberHandler(dbusSignalHandlerToken);
}

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_PROXY_BASE_H_

