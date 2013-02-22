/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusServiceStatusEvent.h"
#include "DBusServiceRegistry.h"
#include <cassert>

namespace CommonAPI {
namespace DBus {

DBusServiceStatusEvent::DBusServiceStatusEvent(std::shared_ptr<DBusServiceRegistry> registry) :
                registry_(registry) {
}

void DBusServiceStatusEvent::onFirstListenerAdded(const std::string& commonApiServiceName, const Listener& listener) {

}

void DBusServiceStatusEvent::availabilityEvent(const std::string& commonApiServiceName, const bool& available) {

    const AvailabilityStatus availabilityStatus = !available ? AvailabilityStatus::NOT_AVAILABLE :
                                                               AvailabilityStatus::AVAILABLE;
    notifyListeners(commonApiServiceName, availabilityStatus);
}

void DBusServiceStatusEvent::onListenerAdded(const std::string& commonApiServiceName, const Listener& listener) {
    if (registry_) {

        registry_->registerAvailabilityListener(commonApiServiceName, std::bind(
                        &DBusServiceStatusEvent::availabilityEvent,
                        this,
                        commonApiServiceName,
                        std::placeholders::_1));

        std::string dbusInterfaceName;
        std::string dbusConnectionName;
        std::string dbusObjectPath;
        DBusAddressTranslator::getInstance().searchForDBusAddress(
                        commonApiServiceName,
                        dbusInterfaceName,
                        dbusConnectionName,
                        dbusObjectPath);

        const AvailabilityStatus availabilityStatus =
                        !registry_->isServiceInstanceAlive(dbusInterfaceName, dbusConnectionName, dbusObjectPath) ? AvailabilityStatus::NOT_AVAILABLE :
                                        AvailabilityStatus::AVAILABLE;

        notifyListeners(commonApiServiceName, availabilityStatus);
    }
}

} // namespace DBus
} // namespace CommonAPI
