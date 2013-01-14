/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusError.h"

#include <cassert>
#include <cstring>

namespace CommonAPI {
namespace DBus {


DBusError::DBusError() {
	dbus_error_init(&libdbusError_);
}

DBusError::~DBusError() {
	dbus_error_free(&libdbusError_);
}

DBusError::operator bool() const {
	return dbus_error_is_set(&libdbusError_);
}

void DBusError::clear() {
	dbus_error_free(&libdbusError_);
}

std::string DBusError::getName() const {
	assert(*this);

	return std::string(libdbusError_.name);
}

std::string DBusError::getMessage() const {
	assert(*this);

	return std::string(libdbusError_.message);
}

} // namespace DBus
} // namespace CommonAPI
