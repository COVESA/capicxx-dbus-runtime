/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_STUB_ADAPTER_H_
#define COMMONAPI_DBUS_DBUS_STUB_ADAPTER_H_

#include "DBusProxyConnection.h"
#include "DBusObjectManager.h"
#include "DBusMessage.h"

#include <CommonAPI/Stub.h>

#include <string>
#include <memory>

namespace CommonAPI {
namespace DBus {

class DBusStubAdapter: virtual public CommonAPI::StubAdapter {
 public:
    DBusStubAdapter(const std::string& commonApiAddress,
                    const std::string& dbusInterfaceName,
                    const std::string& dbusBusName,
                    const std::string& dbusObjectPath,
                    const std::shared_ptr<DBusProxyConnection>& dbusConnection);

    virtual ~DBusStubAdapter();

    virtual void init();
    virtual void deinit();

    virtual const std::string getAddress() const;
    virtual const std::string& getDomain() const;
    virtual const std::string& getServiceId() const;
    virtual const std::string& getInstanceId() const;

    inline const std::string& getObjectPath() const;
    inline const std::string& getInterfaceName() const;

    inline const std::shared_ptr<DBusProxyConnection>& getDBusConnection() const;

    virtual const char* getMethodsDBusIntrospectionXmlData() const = 0;
    virtual bool onInterfaceDBusMessage(const DBusMessage& dbusMessage) = 0;

 private:
    const std::string commonApiDomain_;
    const std::string commonApiServiceId_;
    const std::string commonApiParticipantId_;

    const std::string dbusBusName_;
    const std::string dbusObjectPath_;
    const std::string dbusInterfaceName_;
    const std::shared_ptr<DBusProxyConnection> dbusConnection_;

    bool isInitialized_;

    DBusInterfaceHandlerToken dbusInterfaceHandlerToken_;

    static const std::string domain_;
};

const std::string& DBusStubAdapter::getObjectPath() const {
    return dbusObjectPath_;
}

const std::string& DBusStubAdapter::getInterfaceName() const {
    return dbusInterfaceName_;
}

const std::shared_ptr<DBusProxyConnection>& DBusStubAdapter::getDBusConnection() const {
    return dbusConnection_;
}

} // namespace dbus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_STUB_ADAPTER_H_
