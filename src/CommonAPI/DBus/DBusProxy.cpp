// Copyright (C) 2013-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cassert>
#include <sstream>

#include <CommonAPI/Utils.hpp>
#include <CommonAPI/DBus/DBusProxy.hpp>
#include <CommonAPI/DBus/DBusUtils.hpp>

namespace CommonAPI {
namespace DBus {

DBusProxyStatusEvent::DBusProxyStatusEvent(DBusProxy *_dbusProxy)
	: dbusProxy_(_dbusProxy) {
}

void DBusProxyStatusEvent::onListenerAdded(const Listener& listener) {
    if (dbusProxy_->isAvailable())
        listener(AvailabilityStatus::AVAILABLE);
}

DBusProxy::DBusProxy(const DBusAddress &_dbusAddress,
                     const std::shared_ptr<DBusProxyConnection> &_connection):
                DBusProxyBase(_dbusAddress, _connection),
                dbusProxyStatusEvent_(this),
                availabilityStatus_(AvailabilityStatus::UNKNOWN),
                interfaceVersionAttribute_(*this, "uu", "getInterfaceVersion"),
                dbusServiceRegistry_(DBusServiceRegistry::get(_connection))
{
}

void DBusProxy::init() {
    dbusServiceRegistrySubscription_ = dbusServiceRegistry_->subscribeAvailabilityListener(
                    getAddress().getAddress(),
                    std::bind(&DBusProxy::onDBusServiceInstanceStatus, this, std::placeholders::_1));
}

DBusProxy::~DBusProxy() {
    dbusServiceRegistry_->unsubscribeAvailabilityListener(
                    getAddress().getAddress(),
                    dbusServiceRegistrySubscription_);
}

bool DBusProxy::isAvailable() const {
    return (availabilityStatus_ == AvailabilityStatus::AVAILABLE);
}

bool DBusProxy::isAvailableBlocking() const {
    if (availabilityStatus_ == AvailabilityStatus::UNKNOWN) {
        std::chrono::milliseconds singleWaitDuration(2);

        // Wait for the service registry
        while (availabilityStatus_ == AvailabilityStatus::UNKNOWN) {
            std::this_thread::sleep_for(singleWaitDuration);
        }
    }

    return isAvailable();
}

ProxyStatusEvent& DBusProxy::getProxyStatusEvent() {
    return dbusProxyStatusEvent_;
}

InterfaceVersionAttribute& DBusProxy::getInterfaceVersionAttribute() {
    return interfaceVersionAttribute_;
}

void DBusProxy::onDBusServiceInstanceStatus(const AvailabilityStatus& availabilityStatus) {
    availabilityStatus_ = availabilityStatus;
    dbusProxyStatusEvent_.notifyListeners(availabilityStatus);
}

DBusProxyConnection::DBusSignalHandlerToken DBusProxy::subscribeForSelectiveBroadcastOnConnection(
                                                      bool& subscriptionAccepted,
                                                      const std::string& objectPath,
                                                      const std::string& interfaceName,
                                                      const std::string& interfaceMemberName,
                                                      const std::string& interfaceMemberSignature,
                                                      DBusProxyConnection::DBusSignalHandler* dbusSignalHandler) {

    return getDBusConnection()->subscribeForSelectiveBroadcast(
                    subscriptionAccepted,
                    objectPath,
                    interfaceName,
                    interfaceMemberName,
                    interfaceMemberSignature,
                    dbusSignalHandler,
                    this);
}

void DBusProxy::unsubscribeFromSelectiveBroadcast(const std::string& eventName,
                                                 DBusProxyConnection::DBusSignalHandlerToken subscription,
                                                 const DBusProxyConnection::DBusSignalHandler* dbusSignalHandler) {
    getDBusConnection()->unsubscribeFromSelectiveBroadcast(eventName, subscription, this, dbusSignalHandler);
}

} // namespace DBus
} // namespace CommonAPI
