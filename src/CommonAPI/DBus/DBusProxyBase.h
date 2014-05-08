/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

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
    const std::shared_ptr<DBusProxyConnection>& getDBusConnection() const;

    DBusMessage createMethodCall(const char* methodName,
                                 const char* methodSignature = NULL) const;

    DBusProxyConnection::DBusSignalHandlerToken addSignalMemberHandler(
            const std::string& signalName,
            const std::string& signalSignature,
            DBusProxyConnection::DBusSignalHandler* dbusSignalHandler,
            const bool justAddFilter = false);

    DBusProxyConnection::DBusSignalHandlerToken addSignalMemberHandler(
                const std::string& objectPath,
                const std::string& interfaceName,
                const std::string& signalName,
                const std::string& signalSignature,
                DBusProxyConnection::DBusSignalHandler* dbusSignalHandler,
                const bool justAddFilter = false);

    bool removeSignalMemberHandler(const DBusProxyConnection::DBusSignalHandlerToken& dbusSignalHandlerToken, const DBusProxyConnection::DBusSignalHandler* dbusSignalHandler = NULL);

    virtual void init() = 0;

 protected:
    const std::string commonApiDomain_;

 private:
    DBusProxyBase(const DBusProxyBase&) = delete;

    std::shared_ptr<DBusProxyConnection> dbusConnection_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_PROXY_BASE_H_

