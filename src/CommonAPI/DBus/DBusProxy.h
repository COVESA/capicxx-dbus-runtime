/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_PROXY_H_
#define COMMONAPI_DBUS_DBUS_PROXY_H_

#include "DBusProxyBase.h"
#include "DBusAttribute.h"
#include "DBusServiceRegistry.h"
#include "DBusFactory.h"

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
    DBusProxy(const std::shared_ptr<DBusFactory>& factory,
              const std::string& commonApiAddress,
              const std::string& dbusInterfaceName,
              const std::string& dbusBusName,
              const std::string& dbusObjectPath,
              const std::shared_ptr<DBusProxyConnection>& dbusConnection);

    virtual ~DBusProxy();

    virtual bool isAvailable() const;
    virtual ProxyStatusEvent& getProxyStatusEvent();
    virtual InterfaceVersionAttribute& getInterfaceVersionAttribute();

    virtual bool isAvailableBlocking() const;

    virtual std::string getAddress() const;
    virtual const std::string& getDomain() const;
    virtual const std::string& getServiceId() const;
    virtual const std::string& getInstanceId() const;

    virtual const std::string& getDBusBusName() const;
    virtual const std::string& getDBusObjectPath() const;
    virtual const std::string& getInterfaceName() const;
    DBusProxyConnection::DBusSignalHandlerToken subscribeForSelectiveBroadcastOnConnection(
              bool& subscriptionAccepted,
              const std::string& objectPath,
              const std::string& interfaceName,
              const std::string& interfaceMemberName,
              const std::string& interfaceMemberSignature,
              DBusProxyConnection::DBusSignalHandler* dbusSignalHandler);
    void unsubscribeFromSelectiveBroadcast(const std::string& eventName,
                                           DBusProxyConnection::DBusSignalHandlerToken subscription,
                                           const DBusProxyConnection::DBusSignalHandler* dbusSignalHandler);

    void init();
 private:
    DBusProxy(const DBusProxy&) = delete;

    SubscriptionStatus onDBusServiceInstanceStatus(const AvailabilityStatus& availabilityStatus);

    DBusProxyStatusEvent dbusProxyStatusEvent_;
    DBusServiceRegistry::DBusServiceSubscription dbusServiceRegistrySubscription_;
    AvailabilityStatus availabilityStatus_;

    DBusReadonlyAttribute<InterfaceVersionAttribute> interfaceVersionAttribute_;

    std::shared_ptr<DBusServiceRegistry> dbusServiceRegistry_;

    const std::string commonApiServiceId_;
    const std::string commonApiParticipantId_;

    const std::string dbusBusName_;
    const std::string dbusObjectPath_;
    const std::string dbusInterfaceName_;

    const std::shared_ptr<DBusFactory>& factory_;
};


} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_PROXY_H_

