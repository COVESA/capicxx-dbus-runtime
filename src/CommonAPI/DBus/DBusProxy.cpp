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
#include "DBusServiceRegistry.h"
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
                DBusProxyBase(split(commonApiAddress, ':')[1],
                              split(commonApiAddress, ':')[2],
                              dbusInterfaceName,
                              dbusBusName,
                              dbusObjectPath,
                              dbusConnection),
                dbusProxyStatusEvent_(this),
                availabilityStatus_(AvailabilityStatus::UNKNOWN),
                interfaceVersionAttribute_(*this, "getInterfaceVersion") {
    const std::string commonApiDomain = split(commonApiAddress, ':')[0];
    assert(commonApiDomain == "local");

    dbusConnection->getDBusServiceRegistry()->registerAvailabilityListener(
                    commonApiAddress,
                    std::bind(&DBusProxy::onDBusServiceInstanceStatus, this, std::placeholders::_1));
}

DBusProxy::~DBusProxy() {
}

bool DBusProxy::isAvailable() const {
    return (availabilityStatus_ == AvailabilityStatus::AVAILABLE);
}

bool DBusProxy::isAvailableBlocking() const {
    if (availabilityStatus_ == AvailabilityStatus::UNKNOWN) {
        std::chrono::milliseconds singleWaitDuration(100);

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

} // namespace DBus
} // namespace CommonAPI
