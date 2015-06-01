// Copyright (C) 2014-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUSPROXY_HPP_
#define COMMONAPI_DBUS_DBUSPROXY_HPP_

#include <functional>
#include <memory>
#include <string>

#include <CommonAPI/Export.hpp>
#include <CommonAPI/DBus/DBusAttribute.hpp>
#include <CommonAPI/DBus/DBusServiceRegistry.hpp>

namespace CommonAPI {
namespace DBus {

class DBusProxyStatusEvent
		: public ProxyStatusEvent {
    friend class DBusProxy;

 public:
    DBusProxyStatusEvent(DBusProxy* dbusProxy);
    virtual ~DBusProxyStatusEvent() {}

 protected:
    virtual void onListenerAdded(const Listener& listener);

    DBusProxy* dbusProxy_;
};


class DBusProxy
		: public DBusProxyBase {
public:
	COMMONAPI_EXPORT DBusProxy(const DBusAddress &_address,
              const std::shared_ptr<DBusProxyConnection> &_connection);
	COMMONAPI_EXPORT virtual ~DBusProxy();

	COMMONAPI_EXPORT virtual ProxyStatusEvent& getProxyStatusEvent();
	COMMONAPI_EXPORT virtual InterfaceVersionAttribute& getInterfaceVersionAttribute();

	COMMONAPI_EXPORT virtual bool isAvailable() const;
	COMMONAPI_EXPORT virtual bool isAvailableBlocking() const;

	COMMONAPI_EXPORT DBusProxyConnection::DBusSignalHandlerToken subscribeForSelectiveBroadcastOnConnection(
              bool& subscriptionAccepted,
              const std::string& objectPath,
              const std::string& interfaceName,
              const std::string& interfaceMemberName,
              const std::string& interfaceMemberSignature,
              DBusProxyConnection::DBusSignalHandler* dbusSignalHandler);
	COMMONAPI_EXPORT void unsubscribeFromSelectiveBroadcast(const std::string& eventName,
                                           DBusProxyConnection::DBusSignalHandlerToken subscription,
                                           const DBusProxyConnection::DBusSignalHandler* dbusSignalHandler);

	COMMONAPI_EXPORT void init();

private:
	COMMONAPI_EXPORT DBusProxy(const DBusProxy &) = delete;

	COMMONAPI_EXPORT void onDBusServiceInstanceStatus(const AvailabilityStatus& availabilityStatus);

    DBusProxyStatusEvent dbusProxyStatusEvent_;
    DBusServiceRegistry::DBusServiceSubscription dbusServiceRegistrySubscription_;
    AvailabilityStatus availabilityStatus_;

    DBusReadonlyAttribute<InterfaceVersionAttribute> interfaceVersionAttribute_;

    std::shared_ptr<DBusServiceRegistry> dbusServiceRegistry_;
};


} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUSPROXY_HPP_

