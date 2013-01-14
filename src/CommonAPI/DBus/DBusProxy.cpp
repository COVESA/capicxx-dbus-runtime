/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusProxy.h"
#include "DBusServiceRegistry.h"
#include "DBusUtils.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <dbus/dbus.h>
#include <functional>
#include <CommonAPI/Event.h>

namespace CommonAPI {
namespace DBus {

DBusProxyStatusEvent::DBusProxyStatusEvent(DBusProxy* dbusProxy) :
                dbusProxy_(dbusProxy) {
}

void DBusProxyStatusEvent::onFirstListenerAdded(const Listener& listener) {
    auto serviceStatusListener = std::bind(
                    &DBusProxyStatusEvent::onServiceAvailableSignalHandler,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2);

    subscription_ = dbusProxy_->getDBusConnection()->getDBusServiceRegistry()->getServiceStatusEvent().subscribe(
                    dbusProxy_->dbusBusName_ + ":" + dbusProxy_->dbusObjectPath_ + ":" + dbusProxy_->interfaceName_,
                    serviceStatusListener);
}

void DBusProxyStatusEvent::onLastListenerRemoved(const Listener& listener) {
    dbusProxy_->getDBusConnection()->getDBusServiceRegistry()->getServiceStatusEvent().unsubscribe(subscription_);
}

SubscriptionStatus DBusProxyStatusEvent::onServiceAvailableSignalHandler(const std::string& name,
                                                                         const AvailabilityStatus& availabilityStatus) {
    AvailabilityStatus availability = availabilityStatus;

    return notifyListeners(availability);
}

const std::string DBusProxy::domain_ = "local";

DBusProxy::DBusProxy(const std::string& dbusBusName,
                     const std::string& dbusObjectPath,
                     const std::string& interfaceName,
                     const std::shared_ptr<DBusProxyConnection>& dbusProxyConnection) :
						 dbusBusName_(dbusBusName),
		                 dbusObjectPath_(dbusObjectPath),
		                 interfaceName_(interfaceName),
		                 statusEvent_(this),
		                 interfaceVersionAttribute_(*this, "getInterfaceVersion"),
		                 available_(false),
		                 availableSet_(false),
		                 connection_(dbusProxyConnection) {
}

DBusProxy::DBusProxy(const std::string& dbusBusName,
                     const std::string& dbusObjectPath,
                     const std::string& interfaceName,
                     const std::shared_ptr<DBusProxyConnection>& connection,
                     const bool isAlwaysAvailable) :
                 dbusBusName_(dbusBusName),
                 dbusObjectPath_(dbusObjectPath),
                 interfaceName_(interfaceName),
                 statusEvent_(this),
                 interfaceVersionAttribute_(*this, "getInterfaceVersion"),
                 available_(isAlwaysAvailable),
                 availableSet_(isAlwaysAvailable),
                 connection_(connection) {
}

std::string DBusProxy::getAddress() const {
    return domain_ + ":" + interfaceName_ + ":" + dbusBusName_;
}

const std::string& DBusProxy::getDomain() const {
    return domain_;
}

const std::string& DBusProxy::getServiceId() const {
    return getInterfaceName();
}

const std::string& DBusProxy::getInstanceId() const {
    return dbusBusName_;
}


bool DBusProxy::isAvailable() const {
    if (!availableSet_) {
        auto status = getDBusConnection()->getDBusServiceRegistry()->getReadyFuture().wait_for(std::chrono::milliseconds(1));
        if (checkReady(status)) {
            available_ = getDBusConnection()->getDBusServiceRegistry()->isServiceInstanceAlive(getAddress());
            availableSet_ = true;
        }
    }
    return available_;
}

bool DBusProxy::isAvailableBlocking() const {

    if (!availableSet_) {
        getDBusConnection()->getDBusServiceRegistry()->getReadyFuture().wait();
        available_ = getDBusConnection()->getDBusServiceRegistry()->isServiceInstanceAlive(getAddress());
        availableSet_ = true;
    }
    return available_;
}

ProxyStatusEvent& DBusProxy::getProxyStatusEvent() {
    return statusEvent_;
}

InterfaceVersionAttribute& DBusProxy::getInterfaceVersionAttribute() {
    return interfaceVersionAttribute_;
}

DBusMessage DBusProxy::createMethodCall(const char* methodName,
                                        const char* methodSignature) const {
    return DBusMessage::createMethodCall(
                    dbusBusName_.c_str(),
                    dbusObjectPath_.c_str(),
                    getInterfaceName().c_str(),
                    methodName,
                    methodSignature);
}

} // namespace DBus
} // namespace CommonAPI
