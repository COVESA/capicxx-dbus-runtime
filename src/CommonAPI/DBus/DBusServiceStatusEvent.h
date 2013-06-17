/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_SERVICE_STATUS_EVENT_H_
#define COMMONAPI_DBUS_DBUS_SERVICE_STATUS_EVENT_H_

#include "DBusMultiEvent.h"

#include <CommonAPI/Event.h>
#include <CommonAPI/types.h>

#include <string>
#include <memory>

namespace CommonAPI {
namespace DBus {


class DBusServiceRegistry;

class DBusServiceStatusEvent: public DBusMultiEvent<AvailabilityStatus> {
 public:
	DBusServiceStatusEvent(std::shared_ptr<DBusServiceRegistry> registry);

 protected:
	void onFirstListenerAdded(const std::string& commonApiServiceName, const Listener& listener);
	void onListenerAdded(const std::string& commonApiServiceName, const Listener& listener);

 private:
	SubscriptionStatus availabilityEvent(const std::string& commonApiServiceName, const AvailabilityStatus& availabilityStatus);

	std::shared_ptr<DBusServiceRegistry> registry_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_SERVICE_STATUS_EVENT_H_

