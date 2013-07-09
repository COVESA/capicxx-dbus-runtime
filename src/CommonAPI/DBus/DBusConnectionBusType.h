/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DBUSCONNECTIONBUSTYPE_H_
#define DBUSCONNECTIONBUSTYPE_H_

#include <dbus/dbus-shared.h>

namespace CommonAPI {
namespace DBus {

enum BusType {
    SESSION = DBUS_BUS_SESSION,
    SYSTEM = DBUS_BUS_SYSTEM,
    STARTER = DBUS_BUS_STARTER,
    WRAPPED
};

} // namespace DBus
} // namespace CommonAPI

#endif /* DBUSCONNECTIONBUSTYPE_H_ */
