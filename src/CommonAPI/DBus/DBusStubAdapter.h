/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_STUB_ADAPTER_H_
#define COMMONAPI_DBUS_DBUS_STUB_ADAPTER_H_

#include "DBusProxyConnection.h"
#include "DBusInterfaceHandler.h"
#include "DBusMessage.h"

#include <CommonAPI/Stub.h>

#include <string>
#include <memory>

namespace CommonAPI {
namespace DBus {

class DBusObjectManagerStub;
class DBusFactory;

class DBusStubAdapter: virtual public CommonAPI::StubAdapter, public DBusInterfaceHandler {
 public:
    DBusStubAdapter(const std::shared_ptr<DBusFactory>& factory,
                    const std::string& commonApiAddress,
                    const std::string& dbusInterfaceName,
                    const std::string& dbusBusName,
                    const std::string& dbusObjectPath,
                    const std::shared_ptr<DBusProxyConnection>& dbusConnection,
                    const bool isManagingInterface);

    virtual ~DBusStubAdapter();

    virtual void init(std::shared_ptr<DBusStubAdapter> instance);
    virtual void deinit();

    virtual const std::string getAddress() const;
    virtual const std::string& getDomain() const;
    virtual const std::string& getServiceId() const;
    virtual const std::string& getInstanceId() const;

    const std::string& getDBusName() const;
    const std::string& getObjectPath() const;
    const std::string& getInterfaceName() const;

    const std::shared_ptr<DBusProxyConnection>& getDBusConnection() const;

    const bool isManagingInterface();

    virtual const char* getMethodsDBusIntrospectionXmlData() const = 0;
    virtual bool onInterfaceDBusMessage(const DBusMessage& dbusMessage) = 0;

    virtual void deactivateManagedInstances() = 0;
    virtual const bool hasFreedesktopProperties();
    virtual bool onInterfaceDBusFreedesktopPropertiesMessage(const DBusMessage& dbusMessage) = 0;
 protected:

    const std::string commonApiDomain_;
    const std::string commonApiServiceId_;
    const std::string commonApiParticipantId_;

    const std::string dbusBusName_;
    const std::string dbusObjectPath_;
    const std::string dbusInterfaceName_;
    const std::shared_ptr<DBusProxyConnection> dbusConnection_;

    static const std::string domain_;

    const std::shared_ptr<DBusFactory> factory_;

    const bool isManagingInterface_;
};


} // namespace dbus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_STUB_ADAPTER_H_
