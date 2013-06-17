/*
 * DBusConnectionBusType.h
 *
 *  Created on: May 27, 2013
 *      Author: hmi
 */

#ifndef DBUSCONNECTIONBUSTYPE_H_
#define DBUSCONNECTIONBUSTYPE_H_

#include <dbus/dbus-shared.h>

namespace CommonAPI
{
namespace DBus
{
enum BusType {
    SESSION = DBUS_BUS_SESSION,
    SYSTEM = DBUS_BUS_SYSTEM,
    STARTER = DBUS_BUS_STARTER,
    WRAPPED
};
}
}

#endif /* DBUSCONNECTIONBUSTYPE_H_ */
