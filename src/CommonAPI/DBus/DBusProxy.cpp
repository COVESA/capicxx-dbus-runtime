/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
// Workaround for libstdc++ bug
#ifndef _GLIBCXX_USE_NANOSLEEP
#define _GLIBCXX_USE_NANOSLEEP
#endif

#include "DBusProxy.h"
#include "DBusUtils.h"

#include <cassert>


namespace CommonAPI {
namespace DBus {

DBusProxyStatusEvent::DBusProxyStatusEvent(DBusProxy* dbusProxy) :
                dbusProxy_(dbusProxy) {
}

void DBusProxyStatusEvent::onListenerAdded(const CancellableListener& listener) {
    if (dbusProxy_->isAvailable())
        listener(AvailabilityStatus::AVAILABLE);
}


DBusProxy::DBusProxy(const std::string& commonApiAddress,
                     const std::string& dbusInterfaceName,
                     const std::string& dbusBusName,
                     const std::string& dbusObjectPath,
                     const std::shared_ptr<DBusProxyConnection>& dbusConnection):
                DBusProxyBase(dbusConnection),
                commonApiServiceId_(split(commonApiAddress, ':')[1]),
                commonApiParticipantId_(split(commonApiAddress, ':')[2]),
                dbusBusName_(dbusBusName),
                dbusObjectPath_(dbusObjectPath),
                dbusInterfaceName_(dbusInterfaceName),
                dbusProxyStatusEvent_(this),
                availabilityStatus_(AvailabilityStatus::UNKNOWN),
                interfaceVersionAttribute_(*this, "getInterfaceVersion"),
                dbusServiceRegistry_(dbusConnection->getDBusServiceRegistry()) {

    const std::string commonApiDomain = split(commonApiAddress, ':')[0];
    assert(commonApiDomain == "local");

    dbusServiceRegistrySubscription_ = dbusServiceRegistry_->subscribeAvailabilityListener(
                    commonApiAddress,
                    std::bind(&DBusProxy::onDBusServiceInstanceStatus, this, std::placeholders::_1));
}

DBusProxy::~DBusProxy() {
    dbusServiceRegistry_->unsubscribeAvailabilityListener(
                    getAddress(),
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

const std::string& DBusProxy::getDBusBusName() const {
    return dbusBusName_;
}

const std::string& DBusProxy::getDBusObjectPath() const {
    return dbusObjectPath_;
}

const std::string& DBusProxy::getInterfaceName() const {
    return dbusInterfaceName_;
}

const std::string& DBusProxy::getDomain() const {
    return commonApiDomain_;
}

const std::string& DBusProxy::getServiceId() const {
    return commonApiServiceId_;
}

const std::string& DBusProxy::getInstanceId() const {
    return commonApiParticipantId_;
}

std::string DBusProxy::getAddress() const {
    return commonApiDomain_ + ":" + commonApiServiceId_ + ":" + commonApiParticipantId_;
}

} // namespace DBus
} // namespace CommonAPI
