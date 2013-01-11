/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <common-api-dbus/dbus-connection.h>
#include <common-api-dbus/dbus-name-cache.h>



int main(void) {
	auto dbusConnection = common::api::dbus::DBusConnection::getSessionBus();

	dbusConnection->connect();

	common::api::dbus::DBusNameCache dbusNameCache(dbusConnection);

	dbusConnection->requestServiceNameAndBlock("common.api.dbus.test.DBusNameCache");

	for (int i = 0; i < 5; i++)
		dbusConnection->readWriteDispatch(100);

	dbusConnection->releaseServiceName("common.api.dbus.test.DBusNameCache");

	for (int i = 0; i < 5; i++)
		dbusConnection->readWriteDispatch(100);

	return 0;
}
