/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_ERROR_H_
#define COMMONAPI_DBUS_DBUS_ERROR_H_

#include <string>
#include <dbus/dbus.h>

namespace CommonAPI {
namespace DBus {

class DBusConnection;


class DBusError {
 public:
	DBusError();
	~DBusError();

	operator bool() const;

	void clear();

	std::string getName() const;
	std::string getMessage() const;

 private:
	::DBusError libdbusError_;

	friend class DBusConnection;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_ERROR_H_
