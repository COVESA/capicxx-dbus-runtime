/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_PROXY_H_
#define COMMONAPI_DBUS_DBUS_PROXY_H_

#include "DBusProxyBase.h"
#include "DBusAttribute.h"
#include "DBusServiceRegistry.h"

#include <functional>
#include <memory>
#include <string>


namespace CommonAPI {
namespace DBus {

class DBusProxyStatusEvent: public ProxyStatusEvent {
    friend class DBusProxy;

 public:
    DBusProxyStatusEvent(DBusProxy* dbusProxy);

 protected:
    virtual void onListenerAdded(const CancellableListener& listener);

    DBusProxy* dbusProxy_;
};


class DBusProxy: public DBusProxyBase {
 public:
    DBusProxy(const std::string& commonApiAddress,
              const std::string& dbusInterfaceName,
              const std::string& dbusBusName,
              const std::string& dbusObjectPath,
              const std::shared_ptr<DBusProxyConnection>& dbusConnection);

    virtual ~DBusProxy();

    virtual bool isAvailable() const;
    virtual ProxyStatusEvent& getProxyStatusEvent();
    virtual InterfaceVersionAttribute& getInterfaceVersionAttribute();

    virtual bool isAvailableBlocking() const;

 private:
    DBusProxy(const DBusProxy&) = delete;

    void onDBusServiceInstanceStatus(const AvailabilityStatus& availabilityStatus);

    DBusProxyStatusEvent dbusProxyStatusEvent_;
    DBusServiceRegistry::Subscription dbusServiceRegistrySubscription_;
    DBusServiceStatusEvent::Subscription dbusServiceStatusEventSubscription_;
    AvailabilityStatus availabilityStatus_;

    DBusReadonlyAttribute<InterfaceVersionAttribute> interfaceVersionAttribute_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_PROXY_H_

