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

void DBusServiceStatusEvent::onFirstListenerAdded(const std::string& serviceName, const Listener& listener) {
    if (registry_) {
        registry_->registerAvailabilityListener(serviceName, std::bind(
                        &DBusServiceStatusEvent::availabilityEvent,
                        this,
                        serviceName,
                        std::placeholders::_1));
    }
}

void DBusServiceStatusEvent::availabilityEvent(const std::string& name, const bool& available) {

    const AvailabilityStatus availabilityStatus = !available ? AvailabilityStatus::NOT_AVAILABLE :
                                                               AvailabilityStatus::AVAILABLE;
    notifyListeners(name, availabilityStatus);
}

void DBusServiceStatusEvent::onListenerAdded(const std::string& name, const Listener& listener) {
    if (registry_) {
        const AvailabilityStatus availabilityStatus =
                        !registry_->isServiceInstanceAlive(name) ? AvailabilityStatus::NOT_AVAILABLE :
                                        AvailabilityStatus::AVAILABLE;

        notifyListeners(name, availabilityStatus);
    }

}

} // namespace DBus
} // namespace CommonAPI
